
/* MD5DEEP
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

#include "md5deep.h"

#ifndef HASH_ALGORITHM
#error No hash algorithm defined! See algorithms.h
#endif

typedef struct hash_info {
  char *full_name;
  char *short_name;
  char *match_name;

  /* We never use the total number of bytes in a file, 
     only the number of megabytes when we display a time estimate */
  off_t total_megs;
  off_t bytes_read;

#ifdef __WIN32
  /* Win32 is a 32-bit operating system and can't handle file sizes
     larger than 4GB. We use this to keep track of overflows */
  off_t last_read;
  off_t overflow_count;
#endif

  char *result;

  FILE *handle;
  int is_stdin;
} hash_info;


void update_display(time_t elapsed, hash_info *h)
{
  char msg[LINE_LENGTH];
  unsigned int hour,min;
  off_t seconds, mb_read = h->bytes_read / ONE_MEGABYTE;

#ifdef __WIN32
  mb_read += h->overflow_count * 4096;
#endif

  if (h->total_megs == 0) 
  {
    snprintf(msg,LINE_LENGTH,
	     "%s: %lluMB done. Unable to estimate remaining time.",
	     h->short_name,mb_read);
  }
  else 
  {
    /* Our estimate of the number of seconds remaining */
    seconds = (off_t)floor(((double)h->total_megs/mb_read - 1) * elapsed);

    /* We don't care if the remaining time is more than one day.
       If you're hashing something that big, to quote the movie Jaws:
       
              "We're gonna need a bigger boat."                   */
    
    hour = floor((double)seconds/3600);
    seconds -= (hour * 3600);
    
    min = floor((double)seconds/60);
    seconds -= min * 60;
    
    snprintf(msg,LINE_LENGTH,
	     "%s: %lluMB of %lluMB done, %02u:%02u:%02llu left%s",
	     h->short_name,mb_read,h->total_megs,hour,min,seconds,BLANK_LINE);
  }    

  fprintf(stderr,"\r%s",msg);
}


void shorten_filename(char *dest, char *src)
{
  char *chopped, *basen;

  if (strlen(src) < MAX_FILENAME_LENGTH)
  {
    strncpy(dest,src,MAX_FILENAME_LENGTH);
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
int fatal_error(void)
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


int compute_hash(off_t mode, hash_info *h, HASH_CONTEXT *md)
{
  time_t start_time,current_time,last_time = 0;
  off_t current_read;
  unsigned char buffer[BUFSIZ];

  if (M_ESTIMATE(mode))
  {
    time(&start_time);
    last_time = start_time;
  }

  while (TRUE) 
  {    
    /* clear the buffer in case we hit an error and need to pad the hash */
    memset(buffer,0,BUFSIZ);
    
    current_read = fread(buffer, 1, BUFSIZ, h->handle);

    /* If an error occured, display a message but still add this block */
    if (ferror(h->handle))
    {
      if (!(M_SILENT(mode)))
	fprintf(stderr,"%s: %s: error at offset %llu: %s%s", 
		__progname,h->full_name,h->bytes_read,strerror(errno),NEWLINE);
      
      if (fatal_error())
	return FALSE;

      HASH_Update(md, buffer, BUFSIZ);
      h->bytes_read += BUFSIZ;

      clearerr(h->handle);
      
      /* The file pointer's position is now undefined. We have to manually
	 advance it to the start of the next buffer to read. */
      fseek(h->handle,SEEK_SET,h->bytes_read);
    } 
    else
    {
      /* If we hit the end of the file, we read less than BUFSIZ bytes
         and must reflect that in how we update the hash. */
      HASH_Update(md, buffer, current_read);
      h->bytes_read += current_read;
    }

#ifdef __WIN32
    /* We have to update the overflow counter here. If it's only in 
       update_display() then it's possible that we roll over but never
       see it. This causes an error when we try to display the final size. */
    if (h->last_read > h->bytes_read)
      h->overflow_count++;
    h->last_read = h->bytes_read;
#endif
    
    /* Check if we've hit the end of the file */
    if (feof(h->handle))
    {	
      /* If we've been printing, we now need to clear the line. */
      if (M_ESTIMATE(mode))
	fprintf(stderr,"\r%s\r",BLANK_LINE);

      return TRUE;
    }
    
    if (M_ESTIMATE(mode)) 
    {
      time(&current_time);
      
      /* We only update the display only if a full second has elapsed */
      if (last_time != current_time) 
      {
	last_time = current_time;
	update_display(current_time - start_time,h);
      }
    }
  }      
}


void display_size(off_t mode, hash_info *h)
{
#ifndef __WIN32
  off_t max_size = 999999999;
  max_size *= 10;
  max_size += 9;

  /* We don't have Windows code for a "max value" because Win32 can
     only handle 32 bit numbers, or 2^32. This is far less than the
     ten digits we can display. If we go over 2^32 we overflow and
     there's no way to compute the true number of bytes.

     Also, Mac and *BSD don't like using ten digit constants, so we
     compute the max_size of 9999999999 instead. */

  if (M_DISPLAY_SIZE(mode))
  {
    if (h->bytes_read > max_size)
      printf ("%10llu  ", max_size);
    else
      printf ("%10llu  ", h->bytes_read);
  }
#else
  if (M_DISPLAY_SIZE(mode))
  {
    if (h->overflow_count > 0)
      printf ("9999999999  ");
    else
      printf ("%10lu  ", h->bytes_read);
  }
#endif
}


void display_match_result(off_t mode, hash_info *h)
{  
  int status;
  char *known_fn;

  if ((known_fn = (char *)malloc(sizeof(char) * PATH_MAX)) == NULL)
  {
    print_error(mode,h->full_name,"Out of memory");
    return;
  }

  status = is_known_hash(h->result,known_fn);
  if ((status && M_MATCH(mode)) || (!status && M_MATCHNEG(mode)))
  {
    display_size(mode,h);

    if (M_DISPLAY_HASH(mode))
    {
      printf ("%s  ", h->result);
    }

    if (M_WHICH(mode))
    {
      if (status && M_MATCH(mode))
	printf ("%s matched %s", h->match_name,known_fn);
      else
	printf ("%s does NOT match", h->match_name);
    }
    else
      printf ("%s", h->match_name);

    make_newline(mode);
  }
  
  free(known_fn);
}


void display_hash(off_t mode, hash_info *h)
{
  /* We can't call display_size here because we don't know if we're
     going to display *anything* yet. If we're in matching mode, we
     have to evaluate if there was a match first! */
  if (M_MATCH(mode) || M_MATCHNEG(mode))
    display_match_result(mode,h);

  else 
  {
    display_size(mode,h);
      
    printf ("%s", h->result);
    if (!(h->is_stdin))
      printf("  %s", h->full_name);
    
    make_newline(mode);
  }
}


void hash(off_t mode, hash_info *h)
{
  HASH_CONTEXT md;
  unsigned char sum[HASH_LENGTH];
  static char result[2 * HASH_LENGTH + 1];
  static char hex[] = "0123456789abcdef";
  int i;

#ifdef __WIN32
  h->last_read = 0;
  h->overflow_count = 0;
#endif

  h->bytes_read = 0;
  h->result = NULL;

  HASH_Init(&md);

  if (!compute_hash(mode,h,&md))
    return;

  HASH_Final(sum, &md);  
  for (i = 0; i < HASH_LENGTH; ++i) {
    result[2 * i] = hex[(sum[i] >> 4) & 0xf];
    result[2 * i + 1] = hex[sum[i] & 0xf];
  }
  
  h->result = strdup(result);
  display_hash(mode,h);
  free(h->result);
}


/* The basename function kept misbehaving on OS X, so I rewrote it.
   This function isn't perfect, nor is it designed to be. Because
   we're guarenteed to be working with a filename here, there's no way
   that s will end with a DIR_SEPARATOR (e.g. /foo/bar/). This function
   will not work properly for a string that ends in a DIR_SEPARATOR */
int my_basename(char *s)
{
  unsigned long pos = strlen(s);
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


int setup_barename(off_t mode, hash_info *h, char *file_name)
{
  char *basen = strdup(file_name);
  if (basen == NULL)
  {
    print_error(mode,file_name,"Out of memory");
    return TRUE;
  }

  if (my_basename(basen))
  {
    free(basen);
    print_error(mode,file_name,"Illegal filename");
    return TRUE;
  }
  h->full_name = basen;
  return FALSE;
}


void hash_file(off_t mode, char *file_name)
{
  hash_info *h = (hash_info *)malloc(sizeof(hash_info));

  h->is_stdin = FALSE;

  if (M_BARENAME(mode))
  {
    if (setup_barename(mode,h,file_name))
      return;
  }
  else
    h->full_name = file_name;

  // We have a separate 'match name' because these values are
  // different when processing stdin. They are the same for normal files
  h->match_name = h->full_name;

  if ((h->handle = fopen(file_name,"rb")) != NULL)
  {
    if (M_ESTIMATE(mode))
    {
      h->total_megs = find_file_size(h->handle) / ONE_MEGABYTE;
      h->short_name = (char *)malloc(sizeof(char) * MAX_FILENAME_LENGTH);
      shorten_filename(h->short_name,h->full_name);    
    }    

    hash(mode,h);
    fclose(h->handle);

    if (M_ESTIMATE(mode))
      free(h->short_name);

    if (M_BARENAME(mode))
      free(h->full_name);
  }
  else
    print_error(mode,file_name,strerror(errno));
  
  free(h);
}

void hash_stdin(off_t mode)
{
  hash_info *h = (hash_info *)malloc(sizeof(hash_info));

  h->full_name = strdup("stdin");
  h->match_name = strdup("stdin matches");
  h->is_stdin = TRUE;
  h->handle = stdin;

  if (M_ESTIMATE(mode))
  {
    h->short_name = strdup("stdin");
    h->total_megs = 0;
  }

  hash(mode,h);

  free(h->full_name);
  free(h->match_name);

  if (M_ESTIMATE(mode))
    free(h->short_name);

  free(h);
}
