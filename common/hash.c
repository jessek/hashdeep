// MD5DEEP - hash.c
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

#ifdef __MD5DEEP_H

// Code for md5deep 

#define HASH_INITIALIZE()      s->hash_init(s->hash_context)
#define HASH_UPDATE(BUF,SIZE)  s->hash_update(s->hash_context,BUF,SIZE)
#define HASH_FINALIZE()        s->hash_finalize(s->hash_context,s->hash_sum)

#else
		    
// Code for hashdeep 

#define HASH_INITIALIZE()      multihash_initialize(s)
#define HASH_UPDATE(BUF,SIZE)  multihash_update(s,BUF,SIZE)
#define HASH_FINALIZE()        multihash_finalize(s);

#endif // ifdef __MD5DEEP_H 
		    


// At least one user has suggested changing update_display() to 
// use human readable units (e.g. GB) when displaying the updates.
// The problem is that once the display goes above 1024MB, there
// won't be many updates. The counter doesn't change often enough
// to indicate progress. Using MB is a reasonable compromise. 

static void update_display(state *s, time_t elapsed)
{
  uint64_t hour, min, seconds, mb_read;

  if (NULL == s)
    return;

  memset(s->msg,0,LINE_LENGTH);

  // If we've read less than one MB, then the computed value for mb_read 
  // will be zero. Later on we may need to divide the total file size, 
  // total_megs, by mb_read. Dividing by zero can create... problems 
  if (s->bytes_read < ONE_MEGABYTE)
    mb_read = 1;
  else
    mb_read = s->actual_bytes / ONE_MEGABYTE;
  
  if (0 == s->stat_megs)
  {
    _sntprintf(s->msg,
	       LINE_LENGTH-1,
	       _TEXT("%s: %"PRIu64"MB done. Unable to estimate remaining time.%s"),
	       s->short_name,
	       mb_read,
	       _TEXT(BLANK_LINE));
  }
  else 
  {
    // Estimate the number of seconds using only integer math.
    // The old method:
    //    seconds = (uint64_t)floor(((double)s->total_megs/mb_read - 1) * elapsed);
    // sometimes produced wacky values, especially on Win32 systems.
    // We now compute the number of bytes read per second and then
    // use that to determine how long the whole file should take. 
    // By subtracting the number of elapsed seconds from that, we should
    // get a good estimate of how many seconds remain.

    seconds = (s->stat_bytes / (s->actual_bytes / elapsed)) - elapsed;

    // We don't care if the remaining time is more than one day.
    // If you're hashing something that big, to quote the movie Jaws:
    //        
    //            "We're gonna need a bigger boat."            
    hour = seconds / 3600;
    seconds -= (hour * 3600);
    
    min = seconds/60;
    seconds -= min * 60;

    _sntprintf(s->msg,LINE_LENGTH-1,
	       _TEXT("%s: %"PRIu64"MB of %"PRIu64"MB done, %02"PRIu64":%02"PRIu64":%02"PRIu64" left%s"),
	       s->short_name,
	       mb_read,
	       s->stat_megs,
	       hour,
	       min,
	       seconds,
	       _TEXT(BLANK_LINE));
  }

  fprintf(stderr,"\r");
  display_filename(stderr,s->msg);
}



static void shorten_filename(TCHAR *dest, TCHAR *src)
{
  TCHAR *basen;

  if (NULL == dest || NULL == src)
    return;

  if (_tcslen(src) < MAX_FILENAME_LENGTH)
  {
    _tcsncpy(dest,src, MAX_FILENAME_LENGTH);
    return;
  }

  basen = _tcsdup(src);
  if (NULL == basen)
    return;

  if (my_basename(basen))
    return;

  if (_tcslen(basen) < MAX_FILENAME_LENGTH)
  {
    _tcsncpy(dest,basen,MAX_FILENAME_LENGTH);
    return;
  }

  basen[MAX_FILENAME_LENGTH - 3] = 0;
  _sntprintf(dest,MAX_FILENAME_LENGTH,_TEXT("%s..."),basen);
  free(basen);
}


// Returns TRUE if errno is currently set to a fatal error. That is,
// an error that can't possibly be fixed while trying to read this file
static int file_fatal_error(void)
{
  switch(errno) 
    {
    case EINVAL:   // Invalid argument (happens on Windows)
    case EACCES:   // Permission denied
    case ENODEV:   // Operation not supported (e.g. trying to read 
                   //   a write only device such as a printer)
    case EBADF:    // Bad file descriptor
    case EFBIG:    // File too big
    case ETXTBSY:  // Text file busy
      // The file is being written to by another process.
      // This happens with Windows system files 
    case EIO:      // Input/Output error
      // Added 22 Nov 2010 in response to user email

      return TRUE;  
    }
  
  return FALSE;
}


static int compute_hash(state *s)
{
  time_t current_time;
  uint64_t current_read, mysize, remaining, this_start;
  unsigned char buffer[MD5DEEP_IDEAL_BLOCK_SIZE];

  if (NULL == s)
    return TRUE;

  // Although we need to read MD5DEEP_BLOCK_SIZE bytes before
  // we exit this function, we may not be able to do that in 
  // one read operation. Instead we read in blocks of 8192 bytes 
  // (or as needed) to get the number of bytes we need. 

  if (s->block_size < MD5DEEP_IDEAL_BLOCK_SIZE)
    mysize = s->block_size;
  else
    mysize = MD5DEEP_IDEAL_BLOCK_SIZE;

  remaining = s->block_size;

  // We get weird results calling ftell on stdin!
  if (!(s->is_stdin))
    s->read_start = ftello(s->handle);
  s->read_end   = s->read_start;
  s->bytes_read = 0;

  while (TRUE) 
  {    
    // Clear the buffer in case we hit an error and need to pad the hash 
    memset(buffer,0,mysize);

    this_start = s->read_end;

    current_read = fread(buffer, 1, mysize, s->handle);
    
    s->actual_bytes += current_read;
    s->read_end     += current_read;
    s->bytes_read   += current_read;
      
    // If an error occured, display a message but still add this block 
    if (ferror(s->handle))
    {
      if ( ! (s->mode & mode_silent))
	print_error_unicode(s,
			    s->full_name,
			    "error at offset %"PRIu64": %s",
			    ftello(s->handle),
			    strerror(errno));
	   
      if (file_fatal_error())
	return FALSE; 
      
      HASH_UPDATE(buffer,current_read);
      
      clearerr(s->handle);
      
      // The file pointer's position is now undefined. We have to manually
      // advance it to the start of the next buffer to read. 
      fseeko(s->handle,SEEK_SET,this_start + mysize);
    } 
    else
    {
      // If we hit the end of the file, we read less than MD5DEEP_BLOCK_SIZE
      // bytes and must reflect that in how we update the hash.
      HASH_UPDATE(buffer,current_read);
    }
    
    // Check if we've hit the end of the file 
    if (feof(s->handle))
    {	
      // If we've been printing time estimates, we now need to clear the line.
      if (s->mode & mode_estimate)
	fprintf(stderr,"\r%s\r",BLANK_LINE);

      return TRUE;
   } 

    // In piecewise mode we only hash one block at a time
    if (s->mode & mode_piecewise)
    {
      remaining -= current_read;
      if (remaining == 0)
	return TRUE;

      if (remaining < MD5DEEP_IDEAL_BLOCK_SIZE)
	mysize = remaining;
    }
    
    if (s->mode & mode_estimate)
    {
      time(&current_time);
      
      // We only update the display only if a full second has elapsed 
      if (s->last_time != current_time) 
      {
	s->last_time = current_time;
	update_display(s,current_time - s->start_time);
      }
    }
  }      
}


// Macro to convert raw hex bytes to ASCII output
#ifdef __MD5DEEP_H
static char hex[] = "0123456789abcdef";	
#define HASH_TO_STR(SRC,DEST,LEN)				\
  size_t __i;							\
  for (__i = 0; __i < LEN ; ++__i)				\
    {								\
      DEST[2 * __i    ] = hex[(SRC[__i] >> 4) & 0xf];		\
      DEST[2 * __i + 1] = hex[ SRC[__i]       & 0xf];		\
    }

static int hash_triage(state *s)
{
  if (NULL == s)
    return TRUE;

  memset(s->hash_result,0,(2 * s->hash_length) + 1);

  // We use the piecewise mode to get a partial hash of the first 
  // 512 bytes of the file. But we'll have to remove piecewise mode
  // before returning to the main hashing code
  s->block_size = 512;
  s->mode |= mode_piecewise;

  HASH_INITIALIZE();
    
  if (!compute_hash(s))
  {
    if (s->mode & mode_piecewise)
      free(s->full_name);
    return TRUE;
  }

  s->mode -= mode_piecewise;
  
  HASH_FINALIZE();
  HASH_TO_STR(s->hash_sum, s->hash_result, s->hash_length);

  printf ("%"PRIu64"\t%s", s->stat_bytes, s->hash_result);
  
  return FALSE;
}
#endif


static int hash(state *s)
{
  int done = FALSE, status = FALSE;
  TCHAR *tmp_name = NULL;
  //  uint64_t start_offset;
  
  if (NULL == s)
    return TRUE;

  s->actual_bytes = 0LL;

  if (s->mode & mode_estimate)
  {
    time(&(s->start_time));
    s->last_time = s->start_time;
  }

#ifdef __MD5DEEP_H
  if (s->mode & mode_triage)
  {
    // Hash and display the first 512 bytes of this file
    hash_triage(s);

    // Rather than muck about with updating the state of the input
    // file, just reset everything and process it normally.
    s->actual_bytes = 0;
    fseeko(s->handle, 0, SEEK_SET);
  }
#endif
  
  if ( s->mode & mode_piecewise )
  {
    s->block_size = s->piecewise_size;
    
    // We copy out the original file name and saved it in tmp_name
    tmp_name = s->full_name;
    s->full_name = (TCHAR *)malloc(sizeof(TCHAR) * PATH_MAX);
    if (NULL == s->full_name)
    {
      return TRUE;
    }
  }
  
  while (!done)
  {
#ifdef __MD5DEEP_H
    memset(s->hash_result,0,(2 * s->hash_length) + 1);
#endif
    HASH_INITIALIZE();
    
    s->read_start = s->actual_bytes;

    if (!compute_hash(s))
    {
      if (s->mode & mode_piecewise)
	free(s->full_name);
      return TRUE;
    }

    // We should only display a hash if we've processed some
    // data during this read OR if the whole file is zero bytes long.
    // If the file is zero bytes, we won't have read anything, but
    // still need to display a hash.
    if (s->bytes_read != 0 || 0 == s->stat_bytes)
    {
      if (s->mode & mode_piecewise)
      {
	uint64_t tmp_end = 0;
	if (s->read_end != 0)
	  tmp_end = s->read_end - 1;
	_sntprintf(s->full_name,PATH_MAX,_TEXT("%s offset %"PRIu64"-%"PRIu64),
		   tmp_name, s->read_start, tmp_end);
      }
      
      HASH_FINALIZE();

#ifdef __MD5DEEP_H
      HASH_TO_STR(s->hash_sum, s->hash_result, s->hash_length);
      
      // Under not matched mode, we only display those known hashes that
      // didn't match any input files. Thus, we don't display anything now.
      // The lookup is to mark those known hashes that we do encounter
      if (s->mode & mode_not_matched)
	is_known_hash(s->hash_result,NULL);
      else
	status = display_hash(s);
#else
      display_hash(s);
#endif    
    }

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


static int setup_barename(state *s, TCHAR *fn)
{
  if (NULL == s || NULL == fn)
    return TRUE;

  TCHAR *basen = _tcsdup(fn);
  if (basen == NULL)
  {
    print_error_unicode(s,fn,"Out of memory");
    return TRUE;
  }

  if (my_basename(basen))
  {
    free(basen);
    print_error_unicode(s,fn,"%s: Illegal filename");
    return TRUE;
  }

  s->full_name = basen;
  return FALSE;
}


int hash_file(state *s, TCHAR *fn)
{
  int status = STATUS_OK;

  if (NULL == s || NULL == fn)
    return TRUE;

  s->is_stdin = FALSE;

  if (s->mode & mode_barename)
  {
    if (setup_barename(s,fn))
      return TRUE;
  }
  else
    s->full_name = fn;

  if ((s->handle = _tfopen(fn,_TEXT("rb"))) != NULL)
  {
    // We should have the file size already from the stat functions
    // called during digging. If for some reason that failed, we'll
    // try some ioctl calls now to get the full size.
    if (UNKNOWN_FILE_SIZE == s->stat_bytes)
      s->stat_bytes = find_file_size(s->handle);

    // If this file is above the size threshold set by the user, skip it
    if ((s->mode & mode_size) && (s->stat_bytes > s->size_threshold))
    {
      if (s->mode & mode_size_all)
      {
	// Whereas md5deep has only one hash to wipe, hashdeep has several
#ifdef __MD5DEEP_H
	memset(s->hash_result, '*', HASH_STRING_LENGTH);
#else
	int i;
	for (i = 0 ; i < NUM_ALGORITHMS ; ++i)
	{
	  if (s->hashes[i]->inuse)
	    memset(s->current_file->hash[i], '*', s->hashes[i]->byte_length);
	}
#endif

	display_hash(s);
      }

      fclose(s->handle);
      return STATUS_OK;
    }


    if (s->mode & mode_estimate)
    {
      s->stat_megs = s->stat_bytes / ONE_MEGABYTE;
      shorten_filename(s->short_name,s->full_name);    
    }    

    status = hash(s);

    fclose(s->handle);
  }
  else
  {
    print_error_unicode(s,fn,"%s", strerror(errno));
  }

  
  return status;
}


int hash_stdin(state *s)
{
  if (NULL == s)
    return TRUE;

  _tcsncpy(s->full_name,_TEXT("stdin"),PATH_MAX);
  s->is_stdin  = TRUE;
  s->handle    = stdin;

  if (s->mode & mode_estimate)
  {
    s->short_name = s->full_name;
    s->stat_megs = 0LL;
  }

  return (hash(s));
}
