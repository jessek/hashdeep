
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

void updateDisplay(time_t elapsed,   off_t bytesRead, 
		   off_t totalBytes, char *file_name) 
{
  char msg[LINE_LENGTH];
  unsigned int hour,min;
  off_t remaining, 
    mb_read    = bytesRead  / ONE_MEGABYTE,
    total_megs = totalBytes / ONE_MEGABYTE;

  /* If we couldn't compute the input file size, punt */
  if (total_megs == 0) 
  {
    /* Because the length of this style of line is guarenteed
       only to increase, we don't worry about erasing anything 
       already on the line. Thus, we don't need the trailing 
       SPACE that's used for true time estimates below. */
    snprintf(msg,LINE_LENGTH,
	     "%s: %lluMB done. Unable to estimate remaining time.",
	     file_name,mb_read);
  }
  else 
  {
  
    /* Our estimate of the number of seconds remaining */
    remaining = (off_t)floor(((double)total_megs/mb_read - 1) * elapsed);

    /* We don't care if the remaining time is more than one day.
       If you're hashing something that big, to quote the movie Jaws:
       
              "We're gonna need a bigger boat."                   */
    
    hour = floor((double)remaining/3600);
    remaining -= (hour * 3600);
    
    min = floor((double)remaining/60);
    remaining -= min * 60;
    
    snprintf(msg,LINE_LENGTH,
	     "%s: %lluMB of %lluMB done, %02u:%02u:%02llu left%s",
	     file_name,mb_read,total_megs,hour,min,remaining,BLANK_LINE);
  }    

  fprintf(stderr,"\r%s",msg);
}


void shorten_filename(char *dest, char *src)
{
  char *temp;

  if (strlen(src) < MAX_FILENAME_LENGTH)
    /* Technically we could use strcpy and not strncpy, but it's good
       practice to ALWAYS use strncpy! */
    strncpy(dest,src,MAX_FILENAME_LENGTH);
  else
  {
    temp = strdup(src);
    temp[MAX_FILENAME_LENGTH - 3] = 0;
    sprintf(dest,"%s...",temp);
    free(temp);
  }
}


int fatal_error()
{
  return (errno == EACCES);
}


/* Returns TRUE if the file reads successfully, FALSE on failure */
int compute_hash(off_t mode, FILE *fp, char *file_name, char *short_name,
		 off_t file_size, int estimate_time, HASH_CONTEXT *md)
{
  time_t start_time,current_time,last_time = 0;
  off_t position = 0, bytes_read;
  unsigned char buffer[BUFSIZ];

  if (estimate_time)
  {
    time(&start_time);
    last_time = start_time;
  }

  while (TRUE) 
  {    
    /* clear the buffer in case we hit an error and need to pad the hash */
    memset(buffer,0,BUFSIZ);
    
    bytes_read = fread(buffer, 1, BUFSIZ, fp);
    
    /* If an error occured, display a message but still add this block */
    if (ferror(fp))
    {
      if (!(M_SILENT(mode)))
	fprintf(stderr,"%s: %s: error at offset %llu: %s%s", 
		__progname,file_name,position,strerror(errno),NEWLINE);
      
      if (fatal_error())
	return FALSE;

      HASH_Update(md, buffer, BUFSIZ);
      position += BUFSIZ;
      
      clearerr(fp);
      
      /* The file pointer's position is now undefined. We have to manually
	 advance it to the start of the next buffer to read. */
      fseek(fp,SEEK_SET,position);      
    } 
    else
    {
      /* If we hit the end of the file, we read less than BUFSIZ bytes! */
      HASH_Update(md, buffer, bytes_read);
      position += bytes_read;
    }

    /* Check if we've hit the end of the file */
    if (feof(fp))
    {	
      /* If we've been printing, we now need to clear the line. */
      if (estimate_time)   
	fprintf(stderr,"\r%s\r",BLANK_LINE);

      return TRUE;
    }
    
    if (estimate_time) {
      time(&current_time);
      
      /* We only update the display only if a full second has elapsed */
      if (last_time != current_time) {
	last_time = current_time;
	updateDisplay((current_time-start_time),position,file_size,short_name);
      }
    }
  }      
}




char *hash_file(off_t mode, FILE *fp, char *file_name) 
{
  off_t file_size = 0;
  HASH_CONTEXT md;
  unsigned char sum[HASH_LENGTH];
  static char result[2 * HASH_LENGTH + 1];
  static char hex[] = "0123456789abcdef";
  int i, status;
  char *temp, *basen = strdup(file_name),
    *short_name = (char *)malloc(sizeof(char) * PATH_MAX);

  if (M_ESTIMATE(mode))
  {
    file_size = find_file_size(fp);
    temp = basename(basen);
    shorten_filename(short_name,temp);
  }    

  HASH_Init(&md);
  status = compute_hash(mode,fp,file_name,short_name,file_size,
			M_ESTIMATE(mode),&md);
  free(short_name);
  if (!status)
    return NULL;

  HASH_Final(sum, &md);
  
  for (i = 0; i < HASH_LENGTH; i++) {
    result[2 * i] = hex[(sum[i] >> 4) & 0xf];
    result[2 * i + 1] = hex[sum[i] & 0xf];
  }

  return result;
}


void display_size(off_t mode, FILE *handle)
{
  off_t size;
  if (M_DISPLAY_SIZE(mode))
  {
    size = ftello(handle);
    
    if (size > 9999999999)
      printf ("9999999999  ");
    else
      printf ("%10llu  ", size);
  }
}


void display_match_result(off_t mode, char *result, char *file_name, FILE *handle)
{  
  int status = is_known_hash(result);
  if ((status && M_MATCH(mode)) || (!status && M_MATCHNEG(mode)))
  {
    display_size(mode,handle);

    if (M_DISPLAY_HASH(mode))
    {
      printf ("%s  ", result);
    }
    printf ("%s", file_name);
    make_newline(mode);
  }
}




void do_hash(off_t mode, FILE *handle, char *file_name,unsigned int is_stdin)
{
  char *result;

  if (is_stdin)
    result = hash_file(mode,handle,"stdin");
  else
    result = hash_file(mode,handle,file_name);

  if (result != NULL)
  {
    if (M_MATCH(mode) || M_MATCHNEG(mode))
      display_match_result(mode,result,file_name,handle);
    else 
    {
      display_size(mode,handle);

      printf ("%s", result);
      if (!is_stdin)
	printf("  %s",file_name);

      make_newline(mode);
    }
  }
}  


void hash(off_t mode, char *file_name) 
{
  FILE *handle;

  if ((handle = fopen(file_name,"rb")) != NULL) 
  {
    do_hash(mode,handle,file_name,FALSE);
    fclose(handle);
  }
  else
    print_error(mode,file_name,strerror(errno));
}


void hash_stdin(off_t mode)
{
  /* The "filename" stdin_matches is used for the positive and
     negative matching. The TRUE value we submit will set the progress
     meter's name to stdin. */
  do_hash(mode,stdin,"stdin matches",TRUE);
}

