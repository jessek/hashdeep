
/* MD5DEEP - helpers.c
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

void make_newline(uint64_t mode)
{
  if (M_ZERO(mode))
    printf("%c", 0);
  else
    printf("%s", NEWLINE);
  fflush(stdout);
}


/* Shift the contents of a string so that the values after 'new_start'
   will now begin at location 'start' */
void shift_string(char *fn, size_t start, size_t new_start)
{
  if (start > strlen(fn) || new_start < start)
    return;

  while (new_start < strlen(fn))
    {
      fn[start] = fn[new_start];
      new_start++;
      start++;
    }

  fn[start] = 0;
}


/* Find the index of the next comma in the string s starting at index start.
   If there is no next comma, returns -1. */
int find_next_comma(char *s, unsigned int start)
{
  size_t size=strlen(s);
  unsigned int pos = start; 
  int in_quote = FALSE;
  
  while (pos < size)
  {
    switch (s[pos]) {
    case '"':
      in_quote = !in_quote;
      break;
    case ',':
      if (in_quote)
	break;

      /* Although it's potentially unwise to cast an unsigned int back
	 to an int, problems will only occur when the value is beyond 
	 the range of int. Because we're working with the index of a 
	 string that is probably less than 32,000 characters, we should
	 be okay. */
      return (int)pos;
    }
    ++pos;
  }
  return -1;
}

 
/* Returns the string after the nth comma in the string s. If that
   string is quoted, the quotes are removed. If there is no valid 
   string to be found, returns TRUE. Otherwise, returns FALSE */
int find_comma_separated_string(char *s, unsigned int n)
{
  int start = 0, end;
  unsigned int count = 0; 
  while (count < n)
  {
    if ((start = find_next_comma(s,start)) == -1)
      return TRUE;
    ++count;
    // Advance the pointer past the current comma
    ++start;
  }

  /* It's okay if there is no next comma, it just means that this is
     the last comma separated value in the string */
  if ((end = find_next_comma(s,start)) == -1)
    end = strlen(s);

  /* Strip off the quotation marks, if necessary. We don't have to worry
     about uneven quotation marks (i.e quotes at the start but not the end
     as they are handled by the the find_next_comma function. */
  if (s[start] == '"')
    ++start;
  if (s[end - 1] == '"')
    end--;

  s[end] = 0;
  shift_string(s,0,start);
  
  return FALSE;
}


void print_error(uint64_t mode, char *fn, char *msg) 
{
  if (!M_SILENT(mode))
    fprintf (stderr,"%s: %s: %s%s", __progname,fn,msg,NEWLINE);
}   


void internal_error(char *fn, char *msg)
{
  fprintf (stderr,"%s: %s: Internal error: %s CONTACT DEVELOPER!%s", 
	   __progname,fn,msg,NEWLINE);
  exit(STATUS_INTERNAL_ERROR);
}


void make_magic(void){printf("%s%s","\x53\x41\x4E\x20\x44\x49\x4D\x41\x53\x20\x48\x49\x47\x48\x20\x53\x43\x48\x4F\x4F\x4C\x20\x46\x4F\x4F\x54\x42\x41\x4C\x4C\x20\x52\x55\x4C\x45\x53\x21",NEWLINE);}


#ifndef _WIN32

/* Return the size, in bytes of an open file stream. On error, return 0 */
#if defined (__LINUX)


off_t find_file_size(FILE *f) 
{
  off_t num_sectors = 0;
  int fd = fileno(f);
  struct stat sb;

  if (fstat(fd,&sb))
    return 0;

  if (S_ISREG(sb.st_mode) || S_ISDIR(sb.st_mode))
    return sb.st_size;
  else if (S_ISCHR(sb.st_mode) || S_ISBLK(sb.st_mode))
  {
    if (ioctl(fd, BLKGETSIZE, &num_sectors))
    {
#if defined(__DEBUG)
      fprintf(stderr,"%s: ioctl call to BLKGETSIZE failed.%s", 
	      __progname,NEWLINE);
#endif
    }
    else 
      return (num_sectors * 512);
  }

  return 0;
}  

#elif defined (__APPLE__)

//#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/disk.h>

off_t find_file_size(FILE *f) {
  struct stat info;
  off_t total = 0;
  off_t original = ftello(f);
  int ok = TRUE, fd = fileno(f);


  /* I'd prefer not to use fstat as it will follow symbolic links. We don't
     follow symbolic links. That being said, all symbolic links *should*
     have been caught before we got here. */

  fstat(fd, &info);

  /* Block devices, like /dev/hda, don't return a normal filesize.
     If we are working with a block device, we have to ask the operating
     system to tell us the true size of the device. 
     
     The following only works on Linux as far as I know. If you know
     how to port this code to another operating system, please contact
     the current maintainer of this program! */

  if (S_ISBLK(info.st_mode)) {
	daddr_t blocksize = 0;
	daddr_t blockcount = 0;


    /* Get the block size */
    if (ioctl(fd, DKIOCGETBLOCKSIZE,blocksize) < 0) {
      ok = FALSE;
#if defined(__DEBUG)
      perror("DKIOCGETBLOCKSIZE failed");
#endif
    } 
  
    /* Get the number of blocks */
    if (ok) {
      if (ioctl(fd, DKIOCGETBLOCKCOUNT, blockcount) < 0) {
#if defined(__DEBUG)
	perror("DKIOCGETBLOCKCOUNT failed");
#endif
      }
    }

    total = blocksize * blockcount;

  }

  else {

    /* I don't know why, but if you don't initialize this value you'll
       get wildly innacurate results when you try to run this function */

    if ((fseeko(f,0,SEEK_END)))
      return 0;
    total = ftello(f);
    if ((fseeko(f,original,SEEK_SET)))
      return 0;
  }

  return (total - original);
}


#else 

/* This is code for general UNIX systems 
   (e.g. NetBSD, FreeBSD, OpenBSD, etc) */

static off_t
midpoint (off_t a, off_t b, long blksize)
{
  off_t aprime = a / blksize;
  off_t bprime = b / blksize;
  off_t c, cprime;

  cprime = (bprime - aprime) / 2 + aprime;
  c = cprime * blksize;

  return c;
}



off_t find_dev_size(int fd, int blk_size)
{

  off_t curr = 0, amount = 0;
  void *buf;
  
  if (blk_size == 0)
    return 0;
  
  buf = malloc(blk_size);
  
  for (;;) {
    ssize_t nread;
    
    lseek(fd, curr, SEEK_SET);
    nread = read(fd, buf, blk_size);
    if (nread < blk_size) {
      if (nread <= 0) {
	if (curr == amount) {
	  free(buf);
	  lseek(fd, 0, SEEK_SET);
	  return amount;
	}
	curr = midpoint(amount, curr, blk_size);
      } else { /* 0 < nread < blk_size */
	free(buf);
	lseek(fd, 0, SEEK_SET);
	return amount + nread;
      }
    } else {
      amount = curr + blk_size;
      curr = amount * 2;
    }
  }
  free(buf);
  lseek(fd, 0, SEEK_SET);
  return amount;
}


off_t find_file_size(FILE *f) 
{
  int fd = fileno(f);
  struct stat sb;
  
  if (fstat(fd,&sb))
    return 0;

  if (S_ISREG(sb.st_mode) || S_ISDIR(sb.st_mode))
    return sb.st_size;
  else if (S_ISCHR(sb.st_mode) || S_ISBLK(sb.st_mode))
    return find_dev_size(fd,sb.st_blksize);

  return 0;
}  

#endif // ifdef __LINUX
#endif // ifndef _WIN32

#if defined(_WIN32)
off_t find_file_size(FILE *f) 
{
  off_t total = 0, original = ftello(f);

  if ((fseeko(f,0,SEEK_END)))
    return 0;

  total = ftello(f);
  if ((fseeko(f,original,SEEK_SET)))
    return 0;
  
  return total;
}
#endif /* ifdef _WIN32 */



