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


/****************************************************************
 *** Service routines
 ****************************************************************/

// Returns TRUE if errno is currently set to a fatal error. That is,
// an error that can't possibly be fixed while trying to read this file
static int file_fatal_error(void)
{
    switch(errno) {
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

static std::string make_stars(int count)
{
    std::string ret;
    for(int i=0;i<count;i++){
	ret.push_back('*');
    }
    return ret;
}


/*
 * Compute the hash on fdht and store the results in the display ocb.
 * returns true if successful.
 */

bool file_data_hasher_t::compute_hash()
{
    time_t current_time;
    uint64_t current_read, mysize, remaining, this_start;
    unsigned char buffer[file_data_hasher_t::MD5DEEP_IDEAL_BLOCK_SIZE];

    // Although we need to read MD5DEEP_BLOCK_SIZE bytes before
    // we exit this function, we may not be able to do that in 
    // one read operation. Instead we read in blocks of 8192 bytes 
    // (or as needed) to get the number of bytes we need. 

    if (this->block_size < file_data_hasher_t::MD5DEEP_IDEAL_BLOCK_SIZE){
	mysize = this->block_size;
    }
    else {
	mysize = file_data_hasher_t::MD5DEEP_IDEAL_BLOCK_SIZE;
    }

    remaining = this->block_size;

    // We get weird results calling ftell on stdin!
    if (!(this->is_stdin)) {
	this->read_start = ftello(this->handle);
    }
    this->read_end   = this->read_start;
    this->bytes_read = 0;

    while (TRUE)   {    
	// Clear the buffer in case we hit an error and need to pad the hash 
	memset(buffer,0,mysize);

	this_start = this->read_end;

	current_read = fread(buffer, 1, mysize, this->handle);
    
	this->actual_bytes += current_read;
	this->read_end     += current_read;
	this->bytes_read   += current_read;
      
	// If an error occured, display a message but still add this block 
	if (ferror(this->handle)) {
	    print_error_filename(this->file_name,
				 "error at offset %"PRIu64": %s",
				 ftello(this->handle),
				 strerror(errno));
	   
	    if (file_fatal_error()){
		this->ocb->set_return_code(status_t::status_EXIT_FAILURE);
		return false;		// error
	    }
      
	    this->multihash_update(buffer,current_read);
      
	    clearerr(this->handle);
      
	    // The file pointer's position is now undefined. We have to manually
	    // advance it to the start of the next buffer to read. 
	    fseeko(this->handle,SEEK_SET,this_start + mysize);
	} 
	else    {
	    // If we hit the end of the file, we read less than MD5DEEP_BLOCK_SIZE
	    // bytes and must reflect that in how we update the hash.
	    this->multihash_update(buffer,current_read);
	}
    
	// Check if we've hit the end of the file 
	if (feof(this->handle))    {	
	    // If we've been printing time estimates, we now need to clear the line.
	    if (opt_estimate){
		ocb->clear_realtime_stats();
	    }
	    return true;			// done hashing!
	} 

	// In piecewise mode we only hash one block at a time
	if (this->piecewise_size>0)    {
	    remaining -= current_read;
	    if (remaining == 0) return true; // success!
	    if (remaining < file_data_hasher_t::MD5DEEP_IDEAL_BLOCK_SIZE){
		mysize = remaining;
	    }
	}
    
	if (opt_estimate)    {
	    time(&current_time);
      
	    // We only update the display only if a full second has elapsed 
	    if (this->last_time != current_time) {
		this->last_time = current_time;
		ocb->display_realtime_stats(this,current_time - this->start_time);
	    }
	}
    }      
    return true;
}

/**
 *
 * THIS IS IT!!!!
 *
 * hash()
 * 
 * This function is called to hash each file.
 *
 * Called by: hash_stdin and hash_file.
 * 
 * Result is stored in the fdht structure.
 * This routine is made multi-threaded to make the system run faster.
 */
 void file_data_hasher_t::hash()
 {
     file_data_hasher_t *fdht = this;
     fdht->actual_bytes = 0;

     if (opt_estimate)  {
	 time(&(fdht->start_time));
	 fdht->last_time = fdht->start_time;
     }

     if (fdht->ocb->mode_triage)  {
	 /*
	  * Triage mode:
	  * We use the piecewise mode to get a partial hash of the first 
	  * 512 bytes of the file. But we'll have to remove piecewise mode
	  * before returning to the main hashing code
	  */

	 fdht->block_size     = 512;
	 fdht->piecewise_size = 512;
	 fdht->multihash_initialize();

	 bool success = fdht->compute_hash();
	 fdht->piecewise_size = 0;
	 fdht->multihash_finalize();
	 if(success){
	     char buf[1024];
	     snprintf(buf,sizeof(buf),"%"PRIu64"\t%s",
		      fdht->stat_bytes,
		      fdht->hash_hex[opt_md5deep_mode_algorithm].c_str());
	     fdht->triage_info = buf;
	 }

	 /*
	  * Rather than muck about with updating the state of the input
	  * file, just reset everything and process it normally.
	  */
	 fdht->actual_bytes = 0;
	 fseeko(fdht->handle, 0, SEEK_SET);
     }

     if ( fdht->piecewise_size>0 )  {
	 fdht->block_size = fdht->piecewise_size;
     }

     int done = FALSE;
     while (!done)  {
	 fdht->multihash_initialize();
	 fdht->read_start = fdht->actual_bytes;

	 /**
	  * call compute_hash(), which computes the hash of the full file,
	  * or all of the piecewise hashes.
	  * It returns FALSE if there is a failure.
	  */
	 if (fdht->compute_hash()) {
	     fclose(fdht->handle);
	     fdht->release();
	 }

	 // We should only display a hash if we've processed some
	 // data during this read OR if the whole file is zero bytes long.
	 // If the file is zero bytes, we won't have read anything, but
	 // still need to display a hash.
	 if (fdht->bytes_read != 0 || 0 == fdht->stat_bytes) {
	     if (fdht->piecewise_size>0) {
		 uint64_t tmp_end = 0;
		 if (fdht->read_end != 0){
		     tmp_end = fdht->read_end - 1;
		 }
		 fdht->file_name_annotation = std::string(" offset ") + itos(fdht->read_start)
		     + std::string("-") + itos(tmp_end);
	     }

	     fdht->multihash_finalize();

	     if(md5deep_mode){
		 // Under not matched mode, we only display those known hashes that
		 // didn't match any input files. Thus, we don't display anything now.
		 // The lookup is to mark those known hashes that we do encounter.
		 // searching for the hash will cause matched_file_number to be set
		 if (ocb->mode_not_matched){
		    ocb->find_hash(opt_md5deep_mode_algorithm,
					 fdht->hash_hex[opt_md5deep_mode_algorithm],
					 fdht->file_number);
		}
		else {
		    ocb->md5deep_display_hash(this);
		}
	    } else {
		ocb->display_hash(fdht);
	    }
	}
	if (fdht->piecewise_size>0){
	    done = feof(fdht->handle);
	} else {
	    done = TRUE;
	}
    }

    /**
     * If we are in dfxml mode, output the DFXML, which may optionally include
     * all of the piecewise information.
     */
    ocb->dfxml_write(this);
    fclose(this->handle);
    this->release();
}


/**
 * Primary entry point for a file being hashed.
 * Given a file name:
 * 1 - create the fdht
 * 2 - set up the fdht
 * 3 - hash the fdht
 * 4 - record it in stdout using display.
 */
void display::hash_file(const tstring &fn)
{
    display *ocb = this;
    file_data_hasher_t *fdht = new file_data_hasher_t(this);
    fdht->retain();

    // stat the file to get the bytes and timestamp
    state::file_type(fn,&fdht->stat_bytes,&fdht->timestamp);

    if(opt_verbose>=MORE_VERBOSE){
	print_error("hash_file(%s) primary_function=%d",
		    main::make_utf8(fn).c_str(),
		    ocb->primary_function);
    }

    fdht->file_name	= main::make_utf8(fn);
    if (ocb->mode_barename)  {
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

    /* Open the file for hashing! */
    fdht->handle = _tfopen(fn.c_str(),_TEXT("rb"));
    if(fdht->handle==0){
	print_error_filename(fn,"%s", strerror(errno));
	fdht->release();
	return;
    }
    /*
     * If this file is above the size threshold set by the user, skip it
     * and set the hash to be stars
     */
    if ((ocb->mode_size) && (fdht->stat_bytes > ocb->size_threshold)) {
	if (ocb->mode_size_all) {
	    for (int i = 0 ; i < NUM_ALGORITHMS ; ++i) {
		if (hashes[i].inuse){
		    fdht->hash_hex[i] = make_stars(hashes[i].bit_length/4);
		}
	    }
	    this->display_hash(fdht);
	}
	fclose(fdht->handle);
	fdht->release();
	return ;
    }

    // DO THE HASH EITHER IN THIS THREAD OR ANOTHER THREAD
    // hash() WILL CLOSE AND RELEASE
    fdht->hash();			// DO THE HASH!
}


void display::hash_stdin()
{
    file_data_hasher_t *fdht = new file_data_hasher_t(this);
    fdht->retain();
    fdht->file_name = "stdin";
    fdht->is_stdin  = true;
    fdht->handle    = stdin;
    fdht->hash();
}
