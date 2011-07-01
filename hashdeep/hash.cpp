//
// By Jesse Kornblum and Simson Garfinkel
//
// This is a work of the US Government. In accordance with 17 USC 105,
// copyright protection is not available for any work of the US Government.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// $Id$
//
// 2011 JUN 1 - SLG - Removed #ifdef MD5DEEP, since we now have a single binary for both MD5DEEP and HASHDEEP


// Code for hashdeep 
#include "main.h"

// At least one user has suggested changing update_display() to 
// use human readable units (e.g. GB) when displaying the updates.
// The problem is that once the display goes above 1024MB, there
// won't be many updates. The counter doesn't change often enough
// to indicate progress. Using MB is a reasonable compromise. 

static void update_display(file_data_hasher_t *fdht, time_t elapsed)
{
  uint64_t hour, min, seconds, mb_read;
  bool shorten = false;

  // If we've read less than one MB, then the computed value for mb_read 
  // will be zero. Later on we may need to divide the total file size, 
  // total_megs, by mb_read. Dividing by zero can create... problems 
  if (fdht->bytes_read < ONE_MEGABYTE)
    mb_read = 1;
  else
    mb_read = fdht->actual_bytes / ONE_MEGABYTE;
  
  if (fdht->stat_megs()==0 || opt_estimate==false)  {
      shorten = true;
      char msg[64];
      snprintf(msg,sizeof(msg)-1,"%"PRIu64"MB done. Unable to estimate remaining time.%s",
	       mb_read,BLANK_LINE);
      fdht->file_name_annotation = msg;
  }
  else {
    // Estimate the number of seconds using only integer math.
    //
    // We now compute the number of bytes read per second and then
    // use that to determine how long the whole file should take. 
    // By subtracting the number of elapsed seconds from that, we should
    // get a good estimate of how many seconds remain.

    seconds = (fdht->stat_bytes / (fdht->actual_bytes / elapsed)) - elapsed;

    // We don't care if the remaining time is more than one day.
    // If you're hashing something that big, to quote the movie Jaws:
    //        
    //            "We're gonna need a bigger boat."            
    hour = seconds / 3600;
    seconds -= (hour * 3600);
    
    min = seconds/60;
    seconds -= min * 60;

    shorten = 1;
      char msg[64];
    snprintf(msg,sizeof(msg)-1,
	       "%"PRIu64"MB of %"PRIu64"MB done, %02"PRIu64":%02"PRIu64":%02"PRIu64" left%s",
	       mb_read,
	     fdht->stat_megs(),
	       hour,
	       min,
	       seconds,
	     BLANK_LINE);
    fdht->file_name_annotation = msg;
  }

  fprintf(stderr,"\r");
  display_filename(stderr,fdht,shorten);
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


/**
 * compute the hash. return FALSE if error, TRUE if success
 */

static int compute_hash(state *s,file_data_hasher_t *fdht)
{
  time_t current_time;
  uint64_t current_read, mysize, remaining, this_start;
  unsigned char buffer[file_data_hasher_t::MD5DEEP_IDEAL_BLOCK_SIZE];

  // Although we need to read MD5DEEP_BLOCK_SIZE bytes before
  // we exit this function, we may not be able to do that in 
  // one read operation. Instead we read in blocks of 8192 bytes 
  // (or as needed) to get the number of bytes we need. 

  if (fdht->block_size < file_data_hasher_t::MD5DEEP_IDEAL_BLOCK_SIZE)
    mysize = fdht->block_size;
  else
      mysize = file_data_hasher_t::MD5DEEP_IDEAL_BLOCK_SIZE;

  remaining = fdht->block_size;

  // We get weird results calling ftell on stdin!
  if (!(fdht->is_stdin))
    fdht->read_start = ftello(fdht->handle);
  fdht->read_end   = fdht->read_start;
  fdht->bytes_read = 0;

  while (TRUE)   {    
    // Clear the buffer in case we hit an error and need to pad the hash 
    memset(buffer,0,mysize);

    this_start = fdht->read_end;

    current_read = fread(buffer, 1, mysize, fdht->handle);
    
    fdht->actual_bytes += current_read;
    fdht->read_end     += current_read;
    fdht->bytes_read   += current_read;
      
    // If an error occured, display a message but still add this block 
    if (ferror(fdht->handle)) {
	print_error_filename(fdht->file_name.c_str(),
			    "error at offset %"PRIu64": %s",
			    ftello(fdht->handle),
			    strerror(errno));
	   
      if (file_fatal_error()) return FALSE; 
      
      fdht->multihash_update(buffer,current_read);
      
      clearerr(fdht->handle);
      
      // The file pointer's position is now undefined. We have to manually
      // advance it to the start of the next buffer to read. 
      fseeko(fdht->handle,SEEK_SET,this_start + mysize);
    } 
    else    {
      // If we hit the end of the file, we read less than MD5DEEP_BLOCK_SIZE
      // bytes and must reflect that in how we update the hash.
	fdht->multihash_update(buffer,current_read);
    }
    
    // Check if we've hit the end of the file 
    if (feof(fdht->handle))    {	
      // If we've been printing time estimates, we now need to clear the line.
	if (opt_estimate){
	    fprintf(stderr,"\r%s\r",BLANK_LINE);
	}

      return TRUE;
   } 

    // In piecewise mode we only hash one block at a time
    if (fdht->piecewise)    {
      remaining -= current_read;
      if (remaining == 0) return TRUE;

      if (remaining < file_data_hasher_t::MD5DEEP_IDEAL_BLOCK_SIZE){
	mysize = remaining;
      }
    }
    
    if (opt_estimate)    {
      time(&current_time);
      
      // We only update the display only if a full second has elapsed 
      if (s->last_time != current_time)       {
	s->last_time = current_time;
	update_display(fdht,current_time - s->start_time);
      }
    }
  }      
  return TRUE;
}


static int md5deep_hash_triage(state *s,file_data_hasher_t *fdht)
{
    // We use the piecewise mode to get a partial hash of the first 
    // 512 bytes of the file. But we'll have to remove piecewise mode
    // before returning to the main hashing code
    fdht->block_size = 512;
    fdht->piecewise = true;
    
    fdht->multihash_initialize();
    
    if (!compute_hash(s,fdht))  {
	return TRUE;
    }
    
    fdht->piecewise = false;
    
    fdht->multihash_finalize();
    printf ("%"PRIu64"\t%s", fdht->stat_bytes, fdht->hash_hex[s->md5deep_mode_algorithm].c_str());
    return FALSE;
}


/**
 * This function is called to hash each file.
 * Called by: hash_stdin and hash_file.
 */
static int hash(state *s,file_data_hasher_t *fdht)
{
  int done = FALSE, status = FALSE;
  
  fdht->actual_bytes = 0;

  if (opt_estimate)  {
    time(&(s->start_time));
    s->last_time = s->start_time;
  }

  if (s->mode & mode_triage)  {
    // Hash and display the first 512 bytes of this file
      md5deep_hash_triage(s,fdht);

    // Rather than muck about with updating the state of the input
    // file, just reset everything and process it normally.
    fdht->actual_bytes = 0;
    fseeko(fdht->handle, 0, SEEK_SET);
  }
  
  if ( fdht->piecewise )  {
    fdht->block_size = s->piecewise_size;
  }
  
  while (!done)  {
      fdht->multihash_initialize();
      fdht->read_start = fdht->actual_bytes;

    /**
     * call compute_hash(), which computes the hash of the full file,
     * or all of the piecewise hashes.
     */
    if (!compute_hash(s,fdht)) {
      return TRUE;
    }

    // We should only display a hash if we've processed some
    // data during this read OR if the whole file is zero bytes long.
    // If the file is zero bytes, we won't have read anything, but
    // still need to display a hash.
    if (fdht->bytes_read != 0 || 0 == fdht->stat_bytes)    {
      if (fdht->piecewise)      {
	uint64_t tmp_end = 0;
	if (fdht->read_end != 0){
	    tmp_end = fdht->read_end - 1;
	}
	fdht->file_name_annotation =
	    std::string(" offset ")
	    + itos(fdht->read_start)
	    + std::string("-")
	    + itos(tmp_end);
      }
      
      fdht->multihash_finalize();

      if(s->md5deep_mode){
	  // Under not matched mode, we only display those known hashes that
	  // didn't match any input files. Thus, we don't display anything now.
	  // The lookup is to mark those known hashes that we do encounter.
	  // searching for the hash will cause matched_file_number to be set
	  if (s->mode & mode_not_matched){
	      s->known.find_hash(s->md5deep_mode_algorithm,
				 fdht->hash_hex[s->md5deep_mode_algorithm],
				 fdht->file_number);
	  }
	  else {
	      status = s->md5deep_display_hash(fdht);
	  }
      } else {
	  s->display_hash(fdht);
      }
    }
    

    if (fdht->piecewise){
	done = feof(fdht->handle);
    } else {
	done = TRUE;
    }
  }

  /**
   * If we are in dfxml mode, output the DFXML, which may optionally include
   * all of the piecewise information.
   */
  if(s->dfxml){
      s->dfxml->push("fileobject");
      s->dfxml->xmlout("filename",fdht->file_name);
      s->dfxml->writexml(fdht->dfxml_hash);
      s->dfxml->pop();
  }
  return status;
}


static std::string make_stars(int count)
{
    std::string ret;
    for(int i=0;i<count;i++){
	ret.push_back('*');
    }
    return ret;
}


/**
 * Given a file name, hash it into a fdht, then ask s to deal with it.
 */
int state::hash_file(file_data_hasher_t *fdht,TCHAR *fn)
{
    if(opt_verbose>=MORE_VERBOSE){
	print_error("hash_file(%s) mode=%x primary_function=%d",
		    fn,this->mode,this->primary_function);
    }

    int status = STATUS_OK;
    fdht->is_stdin = FALSE;
    fdht->file_name = fn;

    if (this->mode & mode_barename)  {
	/* Convert fdht->file_name to its basename */

	/* The basename function kept misbehaving on OS X, so Jesse rewrote it.
	 * This approach isn't perfect, nor is it designed to be. Because
	 * we're guarenteed to be working with a file here, there's no way
	 * that str will end with a DIR_SEPARATOR (e.g. /foo/bar/). This function
	 * will not work properly for a string that ends in a DIR_SEPARATOR
	 */
	size_t delim = fdht->file_name.rfind(DIR_SEPARATOR);
	if(delim!=std::string::npos){
	    fdht->file_name = fdht->file_name.substr(delim+1);
	}
    }

    if ((fdht->handle = _tfopen(fn,_TEXT("rb"))) != NULL) {
	// We should have the file size already from the stat functions
	// called during digging. If for some reason that failed, we'll
	// try some ioctl calls now to get the full size.
	if (UNKNOWN_FILE_SIZE == fdht->stat_bytes)
	    fdht->stat_bytes = find_file_size(fdht->handle);
	
	/*
	 * If this file is above the size threshold set by the user, skip it
	 * and set the hash to be stars
	 */
	if ((this->mode & mode_size) && (fdht->stat_bytes > this->size_threshold)) {
	    if (this->mode & mode_size_all)      {
		for (int i = 0 ; i < NUM_ALGORITHMS ; ++i)	{
		    if (hashes[i].inuse){
			fdht->hash_hex[i] = make_stars(hashes[i].bit_length/4);
		    }
		}
		this->display_hash(fdht);
	    }
	    fdht->close();
	    return STATUS_OK;
	}
	
	status = hash(this,fdht);
	fdht->close();
    }
    else  {
	print_error_filename(fn,"%s", strerror(errno));
    }
    return status;
}


int state::hash_stdin()
{
    file_data_hasher_t *fdht = new file_data_hasher_t(this->mode & mode_piecewise);
    fdht->file_name = "stdin";
    fdht->is_stdin  = TRUE;
    fdht->handle    = stdin;
    return hash(this,fdht);
}
