
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

void make_newline(off_t mode)
{
  if (M_ZERO(mode))
    printf("%c", 0);
  else
    printf("%s", NEWLINE);
}


/* Shift the contents of a string so that the values after 'new_start'
   will now begin at location 'start' */
void shift_string(char *fn, int start, int new_start)
{
  if (start < 0 || start > strlen(fn) || new_start < 0 || new_start < start)
    return;

  while (new_start < strlen(fn))
    {
      fn[start] = fn[new_start];
      new_start++;
      start++;
    }

  fn[start] = 0;
}

void print_error(off_t mode, char *fn, char *msg) 
{
  if (!(M_SILENT(mode)))
    fprintf (stderr,"%s: %s: %s%s", __progname,fn,msg,NEWLINE);
}   


#if defined (__UNIX)

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

#elif defined (__MACOSX)

#include <stdint.h>
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

#endif /* UNIX Flavors */
#endif  /* ifdef __UNIX */


#if defined(__WIN32)
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
#endif /* ifdef __WIN32 */



