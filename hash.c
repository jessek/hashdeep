
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


int fatal_error()
{
  return (errno == EPERM ||    /* Operation not permitted */
	  errno == EINVAL ||   /* Invalid argument */
	  errno == EIO);       /* Input/Output error */
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
    temp = (char *)malloc(sizeof(char) * MAX_FILENAME_LENGTH);
    snprintf(temp,MAX_FILENAME_LENGTH-3,src);
    snprintf(dest,MAX_FILENAME_LENGTH,"%s...",temp);
    free(temp);
  }
}

/* Returns TRUE if the file reads successfully, FALSE on failure */
int compute_md5(off_t mode, FILE *fp, char *file_name, char *short_name,
		off_t file_size, int estimate_time, MD5_CTX *md)
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
	fprintf(stderr,"%s: %s: error at offset %llu: %s\n", 
		__progname,file_name,position,strerror(errno));
      
      MD5Update(md, buffer, BUFSIZ);
      position += BUFSIZ;
      
      if (fatal_error())
      {
	return FALSE;
      }

      clearerr(fp);
      
      /* The file pointer's position is now undefined. We have to manually
	 advance it to the start of the next buffer to read. */
      fseek(fp,SEEK_SET,position);      
    } 
    else
    {
      /* If we hit the end of the file, we read less than BUFSIZ bytes! */
      MD5Update(md, buffer, bytes_read);
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

char *md5_file(off_t mode, FILE *fp, char *file_name) 
{
  off_t file_size = 0;
  MD5_CTX md;
  unsigned char sum[MD5_HASH_LENGTH];
  static char result[2 * MD5_HASH_LENGTH + 1];
  static char hex[] = "0123456789abcdef";
  int i, status;
  char *temp, *basen = strdup(file_name),
    *short_name = (char *)malloc(sizeof(char) * PATH_MAX);

  if (M_ESTIMATE(mode))
  {
    file_size = find_file_size(fp);

    temp = basename(basen);
    shorten_filename(short_name,temp);
    free(basen);
  }    

  MD5Init(&md);
  status = compute_md5(mode,fp,file_name,short_name,file_size,
		       M_ESTIMATE(mode),&md);
  free(short_name);
  if (!status)
    return NULL;

  MD5Final(sum, &md);
  
  for (i = 0; i < MD5_HASH_LENGTH; i++) {
    result[2 * i] = hex[(sum[i] >> 4) & 0xf];
    result[2 * i + 1] = hex[sum[i] & 0xf];
  }
  return (result);
}


void md5(off_t mode, char *file_name) 
{
  int result;
  char *hash;
  FILE *handle;

  if ((handle = fopen(file_name,"rb")) == NULL) 
  {
    print_error(mode,file_name,strerror(errno));
    return;
  }

  if ((hash = md5_file(mode,handle,file_name)) != NULL)
  {
    if (M_MATCH(mode) || M_MATCHNEG(mode))
    {
      result = is_known_hash(hash);
      if ((result && M_MATCH(mode)) || (!result && M_MATCHNEG(mode)))
      {
	if (M_DISPLAY_HASH(mode))
	{
	  printf ("%s  ", hash);
	}
	printf ("%s\n", file_name);
      }
    }
    else 
    {
      printf("%s  %s\n",hash,file_name);
    }
  }

  fclose(handle);
}

