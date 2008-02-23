
/* MD5DEEP - hash.c
 *
 * By Jesse Kornblum
 *
 * This is a work of the US Government. In accordance with 17 USC 105,
 * copyright protection is not available for any work of the US Government.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#include "main.h"

#ifndef HASH_ALGORITHM
#error No hash algorithm defined! See algorithms.h
#endif



static void update_display(state *s, time_t elapsed)
{
  uint8_t hour,min;
  uint64_t seconds, mb_read;

  memset(s->msg,0,PATH_MAX);
  
  /* If we've read less than one MB, then the computed value for mb_read 
     will be zero. Later on we may need to divide the total file size, 
     total_megs, by mb_read. Dividing by zero can create... problems */
  if (s->bytes_read < ONE_MEGABYTE)
    mb_read = 1;
  else
    mb_read = s->bytes_read / ONE_MEGABYTE;

  if (s->total_megs == 0) 
  {
    snprintf(s->msg,LINE_LENGTH-1,
	     "%s: %"PRIu64"MB done. Unable to estimate remaining time.%s",
	     s->short_name,mb_read,BLANK_LINE);
  }
  else 
  {
    // Our estimate of the number of seconds remaining
    seconds = (off_t)floor(((double)s->total_megs/mb_read - 1) * elapsed);

    /* We don't care if the remaining time is more than one day.
       If you're hashing something that big, to quote the movie Jaws:
       
              "We're gonna need a bigger boat."                   */
    
    hour = floor((double)seconds/3600);
    seconds -= (hour * 3600);
    
    min = floor((double)seconds/60);
    seconds -= min * 60;

    snprintf(s->msg,LINE_LENGTH-1,
	     "%s: %"PRIu64"MB of %"PRIu64"MB done, %02"PRIu8":%02"PRIu8":%02"PRIu64" left%s",
	     s->short_name,mb_read,s->total_megs,hour,min,seconds,BLANK_LINE);
  }    

  fprintf(stderr,"\r%s", s->msg);
}


static void shorten_filename(char *dest, char *src)
{
  char *chopped, *basen;

  if (strlen(src) < MAX_FILENAME_LENGTH)
  {
    strncpy(dest,src, MAX_FILENAME_LENGTH);
    return;
  }

  basen = strdup(src);
  chopped = basename(basen);  
  free(basen);

  if (strlen(chopped) < MAX_FILENAME_LENGTH)
  {
    strncpy(dest,chopped,MAX_FILENAME_LENGTH);
    return;
  }
  
  chopped[MAX_FILENAME_LENGTH - 3] = 0;
  snprintf(dest,MAX_FILENAME_LENGTH,"%s...",chopped);
}


/* Returns TRUE if errno is currently set to a fatal error. That is,
   an error that can't possibly be fixed while trying to read this file */
static int file_fatal_error(void)
{
  switch(errno) 
    {
    case EACCES:   // Permission denied
    case ENODEV:   // Operation not supported (e.g. trying to read 
                   //   a write only device such as a printer)
    case EBADF:    // Bad file descriptor
    case EFBIG:    // File too big
    case ETXTBSY:  // Text file busy
      /* The file is being written to by another process.
	 This happens with Windows system files */

      return TRUE;  
    }
  
  return FALSE;
}


int compute_hash(state *s, HASH_CONTEXT *md)
{
  time_t start_time,current_time,last_time = 0;
  uint64_t current_read;
  unsigned char buffer[MD5DEEP_IDEAL_BLOCK_SIZE];
  uint64_t mysize, remaining;

  /* Although we need to read MD5DEEP_BLOCK_SIZE bytes before
     we exit this
     function, we may not be able to do that in one fell swoop.
     Instead we read in blocks of 8192 bytes (or as needed) to 
     get how many bytes we need */

  if (s->block_size < MD5DEEP_IDEAL_BLOCK_SIZE)
    mysize = s->block_size;
  else
    mysize = MD5DEEP_IDEAL_BLOCK_SIZE;

  remaining = s->block_size;

  if (s->mode & mode_estimate)
  {
    time(&start_time);
    last_time = start_time;
  }

  while (TRUE) 
  {    
    /* clear the buffer in case we hit an error and need to pad the hash */
    memset(buffer,0,mysize);
    
    current_read = fread(buffer, 1, mysize, s->handle);

    /* If an error occured, display a message but still add this block */
    if (ferror(s->handle))
    {
      print_error(s,"%s: error at offset %"PRIu64": %s", 
		  s->full_name,s->bytes_read,strerror(errno));
      
      if (file_fatal_error())
	return FALSE;

      HASH_Update(md, buffer, mysize);
      s->bytes_read += mysize;

      clearerr(s->handle);
      
      /* The file pointer's position is now undefined. We have to manually
	 advance it to the start of the next buffer to read. */
      fseek(s->handle,SEEK_SET,s->bytes_read);
    } 
    else
    {
      /* If we hit the end of the file, we read less than MD5DEEP_BLOCK_SIZE
	 bytes and must reflect that in how we update the hash. */
      HASH_Update(md, buffer, current_read);
      s->bytes_read += current_read;
    }
    
    /* Check if we've hit the end of the file */
    if (feof(s->handle))
    {	
      /* If we've been printing, we now need to clear the line. */
      if (s->mode & mode_estimate)
	fprintf(stderr,"\r%s\r",BLANK_LINE);

      return TRUE;
    }

    // In piecewise mode we only hash one block at a time
    if (s->mode & mode_piecewise)
    {
      remaining -= mysize;
      if (remaining == 0)
	return TRUE;

      if (remaining < MD5DEEP_IDEAL_BLOCK_SIZE)
	mysize = remaining;
    }
    
    if (s->mode & mode_estimate)
    {
      time(&current_time);
      
      /* We only update the display only if a full second has elapsed */
      if (last_time != current_time) 
      {
	last_time = current_time;
	update_display(s,current_time - start_time);
      }
    }
  }      
}


void display_size(state *s)
{
  if (s->mode & mode_display_size)
  {
    // We reserve ten characters for digits followed by two spaces
    if (s->bytes_read > 9999999999LL)
      printf ("9999999999  ");
    else
      printf ("%10"PRIu64"  ", s->bytes_read);      
  }	
}


char display_asterisk(state *s)
{
  if (s->mode & mode_asterisk)
    return '*';
  return ' ';
}


int display_match_result(state *s)
{  
  int known_hash;
  char *known_fn;

  if ((known_fn = (char *)malloc(sizeof(char) * PATH_MAX)) == NULL)
  {
    print_error(s,"%s: Out of memory", s->full_name);
    return TRUE;;
  }

  known_hash = is_known_hash(s->result,known_fn);
  if ((known_hash && (s->mode & mode_match)) ||
      (!known_hash && (s->mode & mode_match_neg)))
  {
    display_size(s);

    if (s->mode & mode_display_hash)
      printf ("%s %c",s->result,display_asterisk(s));

    if (s->mode & mode_which)
    {
      if (known_hash && (s->mode & mode_match))
	printf ("%s matched %s", s->full_name,known_fn);
      else
	printf ("%s does NOT match", s->full_name);
    }
    else
      printf ("%s", s->full_name);

    make_newline(s);
  }
  
  free(known_fn);
  return FALSE;
}



int display_hash(state *s)
{
  /* We can't call display_size here because we don't know if we're
     going to display *anything* yet. If we're in matching mode, we
     have to evaluate if there was a match first. */
  if ((s->mode & mode_match) || (s->mode & mode_match_neg))
    return display_match_result(s);

  display_size(s);
  printf ("%s", s->result);

  if (s->mode & mode_quiet)
    printf ("  ");
  else
  {
    if ((s->mode & mode_piecewise) ||
	!(s->is_stdin))
      printf(" %c%s", display_asterisk(s),s->full_name);
  }

  make_newline(s);
  return FALSE;
}


int hash(state *s)
{
  int done = FALSE, status = FALSE;
  HASH_CONTEXT md;
  unsigned char sum[HASH_LENGTH];
  static char result[2 * HASH_LENGTH + 1];
  static char hex[] = "0123456789abcdef";
  int i;
  char *tmp_name = NULL;

  s->bytes_read = 0;
  s->block_size = MD5DEEP_IDEAL_BLOCK_SIZE;

  if (s->mode & mode_piecewise)
  {
    s->block_size = s->piecewise_block;

    // We copy out the original file name and saved it in tmp_name
    tmp_name = s->full_name;
    s->full_name = (char *)malloc(sizeof(char) * PATH_MAX);
    if (NULL == s->full_name)
      return TRUE;
  }

  while (!done)
  {
    memset(s->result,0,PATH_MAX);

    HASH_Init(&md);
    
    if (s->mode & mode_piecewise)
    {
      /* If the BLOCK_SIZE is larger than the file, this keeps the
	 offset values correct */
      if (s->bytes_read + s->block_size >  s->total_bytes)
	snprintf(s->full_name,PATH_MAX,"%s offset %"PRIu64"-%"PRIu64,
		 tmp_name, s->bytes_read, s->total_bytes);
      else
	snprintf(s->full_name,PATH_MAX,"%s offset %"PRIu64"-%"PRIu64,
		 tmp_name, s->bytes_read, s->bytes_read + s->block_size);
    }

    if (!compute_hash(s,&md))
    {
      if (s->mode & mode_piecewise)
	free(s->full_name);
      return TRUE;
    }

    HASH_Final(sum, &md);  
    for (i = 0; i < HASH_LENGTH; ++i) 
    {
      result[2 * i] = hex[(sum[i] >> 4) & 0xf];
      result[2 * i + 1] = hex[sum[i] & 0xf];
    }
  
    strncpy(s->result,result,PATH_MAX);
  
    /* Under not matched mode, we only display those known hashes that
       didn't match any input files. Thus, we don't display anything now.
       The lookup is to mark those known hashes that we do encounter */
    if (s->mode & mode_not_matched)
      is_known_hash(s->result,NULL);
    else
      status = display_hash(s);

    if (s->mode & mode_piecewise)
      done = feof(s->handle);
    else
      done = TRUE;
  }

  if (s->mode & mode_piecewise)
  {
    free(s->full_name);
    s->full_name = tmp_name;
  }

  return status;
}


/* The basename function kept misbehaving on OS X, so I rewrote it.
   This function isn't perfect, nor is it designed to be. Because
   we're guarenteed to be working with a file here, there's no way
   that s will end with a DIR_SEPARATOR (e.g. /foo/bar/). This function
   will not work properly for a string that ends in a DIR_SEPARATOR */
int my_basename(char *s)
{
  size_t pos = strlen(s);
  if (0 == pos || pos > PATH_MAX)
    return TRUE;
  
  while(s[pos] != DIR_SEPARATOR && pos > 0)
    --pos;
  
  // If there were no DIR_SEPARATORs in the string, we were still successful!
  if (0 == pos)
    return FALSE;

  // The current character is a DIR_SEPARATOR. We advance one to ignore it
  ++pos;
  shift_string(s,0,pos);
  return FALSE;
}


int setup_barename(state *s, char *file_name)
{
  char *basen = strdup(file_name);
  if (basen == NULL)
  {
    print_error(s,"%s: Out of memory", file_name);
    return TRUE;
  }

  if (my_basename(basen))
  {
    free(basen);
    print_error(s,"%s: Illegal filename", file_name);
    return TRUE;
  }
  s->full_name = basen;
  return FALSE;
}


int hash_file(state *s, char *file_name)
{
  int status = STATUS_OK;

  s->is_stdin = FALSE;

  if (s->mode & mode_barename)
  {
    if (setup_barename(s,file_name))
      return TRUE;
  }
  else
    s->full_name = file_name;

  if ((s->handle = fopen(file_name,"rb")) != NULL)
  {

    s->total_bytes = find_file_size(s->handle);

    if (s->mode & mode_estimate)
    {
      // The find file size returns a value of type off_t, so we must cast it
      s->total_megs = s->total_bytes / ONE_MEGABYTE;
      shorten_filename(s->short_name,s->full_name);    
    }    

    status = hash(s);
    fclose(s->handle);
  }
  else
  {
    print_error(s,"%s: %s", file_name,strerror(errno));
    status = TRUE;
  }
  
  return status;
}


int hash_stdin(state *s)
{
  strncpy(s->full_name,"stdin",PATH_MAX);
  s->is_stdin  = TRUE;
  s->handle    = stdin;

  if (s->mode & mode_estimate)
  {
    s->short_name = s->full_name;
    s->total_megs = 0LL;
  }

  return (hash(s));
}
