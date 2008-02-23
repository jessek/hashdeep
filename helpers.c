
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

/* $Id: helpers.c,v 1.18 2007/09/26 20:27:54 jessekornblum Exp $ */

#include "main.h"

/* The basename function kept misbehaving on OS X, so I rewrote it.
   This function isn't perfect, nor is it designed to be. Because
   we're guarenteed to be working with a file here, there's no way
   that s will end with a DIR_SEPARATOR (e.g. /foo/bar/). This function
   will not work properly for a string that ends in a DIR_SEPARATOR */
int my_basename(TCHAR *s)
{
  size_t len;
  TCHAR *tmp = _tcsrchr(s,DIR_SEPARATOR);

  if (NULL == tmp)
    return FALSE;

  len = _tcslen(tmp);

  // We advance tmp one character to move us past the DIR_SEPARATOR
  _tmemmove(s,tmp+1,len);

  return FALSE;
}


int my_dirname(TCHAR *c)
{
  TCHAR *tmp;

  if (NULL == c)
    return TRUE;

  /* If there are no DIR_SEPARATORs in the directory name, then the 
     directory name should be the empty string */
  tmp = _tcsrchr(c,DIR_SEPARATOR);
  if (NULL != tmp)
    tmp[1] = 0;
  else
    c[0] = 0;

  return FALSE;
}


void make_newline(state *s)
{
  if (s->mode & mode_zero)
    printf("%c", 0);
  else
    printf("%s", NEWLINE);
  fflush(stdout);
}


/* Shift the contents of a string so that the values after 'new_start'
   will now begin at location 'start' */
void shift_string(char *fn, size_t start, size_t new_start)
{
  // TODO: Can shift_string be replaced with memmove? 
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


#ifndef _WIN32
void make_magic(void){print_status("%s",MM_STR);}
#endif


#ifndef _WIN32

/* Return the size, in bytes of an open file stream. On error, return 0 */
#if defined (__LINUX__)


off_t find_file_size(FILE *f) 
{
  off_t num_sectors = 0, sector_size = 0;
  int fd = fileno(f);
  struct stat sb;

  if (fstat(fd,&sb))
    return 0;

  if (S_ISREG(sb.st_mode) || S_ISDIR(sb.st_mode))
    return sb.st_size;

#ifdef HAVE_SYS_IOCTL_H
#ifdef HAVE_SYS_MOUNT_H
  if (S_ISCHR(sb.st_mode) || S_ISBLK(sb.st_mode))
  {
    if (ioctl(fd, BLKGETSIZE, &num_sectors))
    {
      print_debug("%s: ioctl BLKGETSIZE failed: %s", 
		  __progname, strerror(errno));
      return 0;
    }

    if (ioctl(fd, BLKSSZGET, &sector_size))
    {
      print_debug("%s: ioctl BLKSSZGET failed: %s",
		  __progname, strerror(errno));		  
      return 0;
    }

    return (num_sectors * sector_size);
  }
#endif // #ifdef HAVE_SYS_MOUNT_H
#endif // #ifdef HAVE_SYS_IOCTL_H

  return 0;
}  

#elif defined (__APPLE__)

off_t find_file_size(FILE *f) {
  struct stat info;
  off_t total = 0;
  off_t original = ftello(f);
  int fd = fileno(f);
  uint32_t blocksize = 0;
  uint64_t blockcount = 0;

  /* I'd prefer not to use fstat as it will follow symbolic links. We don't
     follow symbolic links. That being said, all symbolic links *should*
     have been caught before we got here. */

  if (fstat(fd, &info))
  {
    print_status("%s: %s", __progname,strerror(errno));
    return 0;
  }

#ifdef HAVE_SYS_IOCTL_H
  /* Block devices, like /dev/hda, don't return a normal filesize.
     If we are working with a block device, we have to ask the operating
     system to tell us the true size of the device. 
     
     This isn't the recommended way to do check for block devices, 
     but using S_ISBLK(info.stmode) wasn't working. */
  if (info.st_mode & S_IFBLK)
  {    
    /* Get the block size */
    if (ioctl(fd, DKIOCGETBLOCKSIZE,&blocksize) < 0) 
    {
      print_debug("%s: ioctl DKIOCGETBLOCKSIZE failed: %s", 
		  __progname, strerror(errno));
      return 0;
    } 
    
    /* Get the number of blocks */
    if (ioctl(fd, DKIOCGETBLOCKCOUNT, &blockcount) < 0) 
    {
      print_debug("%s: ioctl DKIOCGETBLOCKCOUNT failed: %s", 
		  __progname, strerror(errno));
    }

    total = blocksize * blockcount;
  }
#endif     // ifdef HAVE_IOCTL_H

  else 
  {
    if ((fseeko(f,0,SEEK_END)))
      return 0;
    total = ftello(f);
    if ((fseeko(f,original,SEEK_SET)))
      return 0;
  }

  return (total - original);
}


#else   // ifdef __APPLE__

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
    if (nread < blk_size) 
    {
      if (nread <= 0) 
	{
	  if (curr == amount) 
	  {
	    free(buf);
	    lseek(fd, 0, SEEK_SET);
	    return amount;
	  }
	  curr = midpoint(amount, curr, blk_size);
	}
      else 
	{ /* 0 < nread < blk_size */
	  free(buf);
	  lseek(fd, 0, SEEK_SET);
	  return amount + nread;
	}
    } 
    else 
    {
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

#endif // ifdef __LINUX__
#endif // ifndef _WIN32

#if defined(_WIN32)
off_t find_file_size(FILE *f) 
{
  off_t total = 0, original = ftello(f);
  
  /* Windows does not support running fstat on block devices,
     so there's no point in mucking about with them.

     TODO: Find a way to estimate device sizes on Windows
     Perhaps an IOTCL_DISK_GET_DRIVE_GEOMETRY_EX would work? */

  if ((fseeko(f,0,SEEK_END)))
    return 0;

  total = ftello(f);
  if ((fseeko(f,original,SEEK_SET)))
    return 0;
  
  return total;
}
#endif /* ifdef _WIN32 */
