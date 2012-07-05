
// MD5DEEP - helpers.c
//
// By Jesse Kornblum
//
// This is a work of the US Government. In accordance with 17 USC 105,
// copyright protection is not available for any work of the US Government.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// $Id$ 

#include "main.h"

// Remove the newlines, if any. Works on both DOS and *nix newlines
void chop_line(char *s)
{
  while(true)
  {
    size_t pos = strlen(s);
    if (pos>0)
    {
      if (s[pos-1]=='\r' || s[pos-1]=='\n')
      {
	s[pos-1]='\000';
	continue;
      }
      return;
    }
    if(pos==0) break;
  }
}



// Return the size, in bytes of an open file stream. On error, return 0 
#ifndef _WIN32
#if defined (__LINUX__)

off_t find_file_size(FILE *f,class display *ocb) 
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
#if defined(_IO) && defined(BLKGETSIZE)
    if (ioctl(fd, BLKGETSIZE, &num_sectors)) {
	if(ocb) ocb->print_debug("ioctl BLKGETSIZE failed: %s", strerror(errno));
	return 0;
    }
#else
    // If we can't run the ioctl call, we can't do anything here
    return 0;
#endif // ifdefined _IO and BLKGETSIZE


#if defined(_IO) && defined(BLKSSZGET)
    if (ioctl(fd, BLKSSZGET, &sector_size))
    {
	if(ocb) ocb->print_debug("ioctl BLKSSZGET failed: %s",
				 strerror(errno));		  
      return 0;
    }
    if (0 == sector_size)
      sector_size = 512;
#else
    sector_size = 512;
#endif  // ifdef _IO and BLKSSZGET

    return (num_sectors * sector_size);
  }
#endif // #ifdef HAVE_SYS_MOUNT_H
#endif // #ifdef HAVE_SYS_IOCTL_H
  return 0;
}  

#elif defined (__APPLE__)

off_t find_file_size(FILE *f,class display *ocb) 
{
  struct stat info;
  off_t total = 0;
  off_t original = ftello(f);
  int fd = fileno(f);
  uint32_t blocksize = 0;
  uint64_t blockcount = 0;

  // I'd prefer not to use fstat as it will follow symbolic links. We don't
  // follow symbolic links. That being said, all symbolic links *should*
  // have been caught before we got here. 

  if (fstat(fd, &info)) {
      if(ocb) ocb->status("%s: %s", progname.c_str(),strerror(errno));
      return 0;
  }

#ifdef HAVE_SYS_IOCTL_H
  // Block devices, like /dev/hda, don't return a normal filesize.
  // If we are working with a block device, we have to ask the operating
  // system to tell us the true size of the device. 
  //
  // This isn't the recommended way to do check for block devices, 
  // but using S_ISBLK(info.stmode) wasn't working. 
  if (info.st_mode & S_IFBLK)
  {    
    // Get the block size 
    if (ioctl(fd, DKIOCGETBLOCKSIZE,&blocksize) < 0) 
    {
	if(ocb) ocb->print_debug("ioctl DKIOCGETBLOCKSIZE failed: %s", 
				 strerror(errno));
      return 0;
    } 
    
    // Get the number of blocks 
    if (ioctl(fd, DKIOCGETBLOCKCOUNT, &blockcount) < 0) 
    {
	if(ocb) ocb->print_debug("ioctl DKIOCGETBLOCKCOUNT failed: %s", 
				 strerror(errno));
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

//  This is code for general UNIX systems 
// (e.g. NetBSD, FreeBSD, OpenBSD, etc) 

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
	if (nread < blk_size)     {
	    if (nread <= 0) 	{
		if (curr == amount) 	  {
		    free(buf);
		    lseek(fd, 0, SEEK_SET);
		    return amount;
		}
		curr = midpoint(amount, curr, blk_size);
	    }
	    else 	{ // 0 < nread < blk_size 
		free(buf);
		lseek(fd, 0, SEEK_SET);
		return amount + nread;
	    }
	} 
	else     {
	    amount = curr + blk_size;
	    curr = amount * 2;
	}
    }

    free(buf);
    lseek(fd, 0, SEEK_SET);
    return amount;
}


off_t find_file_size(FILE *f,class display *ocb) 
{
  // The error checking for this is above. If f is not NULL
  // fd should be vald.
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
off_t find_file_size(FILE *f,class display *ocb) 
{
  off_t total = 0, original = ftello(f);
  
  // Windows does not support running fstat on block devices,
  // so there's no point in mucking about with them.
  // 
  // TODO: Find a way to estimate device sizes on Windows
  // Perhaps an IOTCL_DISK_GET_DRIVE_GEOMETRY_EX would work? 

  // RBF - We don't really have the fseeko and ftello functions
  // on windows. They are functions like _ftelli64 or some such
  // RBF - Fix find_file_size for large files on Win32

  if ((fseeko(f,0,SEEK_END)))
    return 0;

  total = ftello(f);
  if ((fseeko(f,original,SEEK_SET)))
    return 0;
  
  return total;
}
#endif // ifdef _WIN32 
