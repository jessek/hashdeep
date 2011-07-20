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
 * Compute the hash on fdht and store the results in fdht. 
 */


void display::compute_hash(file_data_hasher_t *fdht)
{
    time_t current_time;
    uint64_t current_read, mysize, remaining, this_start;
    unsigned char buffer[file_data_hasher_t::MD5DEEP_IDEAL_BLOCK_SIZE];

    // Although we need to read MD5DEEP_BLOCK_SIZE bytes before
    // we exit this function, we may not be able to do that in 
    // one read operation. Instead we read in blocks of 8192 bytes 
    // (or as needed) to get the number of bytes we need. 

    if (fdht->block_size < file_data_hasher_t::MD5DEEP_IDEAL_BLOCK_SIZE){
	mysize = fdht->block_size;
    }
    else {
	mysize = file_data_hasher_t::MD5DEEP_IDEAL_BLOCK_SIZE;
    }

    remaining = fdht->block_size;

    // We get weird results calling ftell on stdin!
    if (!(fdht->is_stdin)) {
	fdht->read_start = ftello(fdht->handle);
    }
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
	    print_error_filename(fdht->file_name,
				 "error at offset %"PRIu64": %s",
				 ftello(fdht->handle),
				 strerror(errno));
	   
	    if (file_fatal_error()){
		set_return_code(status_t::status_EXIT_FAILURE);
		return;
	    }
      
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
		clear_realtime_stats();
	    }
	    return;			// done hashing!
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
	    if (fdht->last_time != current_time) {
		fdht->last_time = current_time;
		display_realtime_stats(fdht,current_time - fdht->start_time);
	    }
	}
    }      
}



/**
 *
 * THIS IS IT!!!!
 *
 * hash(state *s,file_data_hasher_t *fdht)
 * 
 * This function is called to hash each file.
 *
 * Called by: hash_stdin and hash_file.
 * 
 * Result is stored in the fdht structure.
 * This routine is made multi-threaded to make the system run faster.
 */
void display::hash(file_data_hasher_t *fdht)
{
    int done = FALSE;
  
    fdht->actual_bytes = 0;

    if (opt_estimate)  {
	time(&(fdht->start_time));
	fdht->last_time = fdht->start_time;
    }
    
    if (s->mode & mode_triage)  {
	/*
	 * Triage mode:
	 * We use the piecewise mode to get a partial hash of the first 
	 * 512 bytes of the file. But we'll have to remove piecewise mode
	 * before returning to the main hashing code
	 */

	fdht->block_size = 512;
	fdht->piecewise = true;
	fdht->multihash_initialize();
    
	int success = compute_hash(fdht);
	fdht->piecewise = false;
	fdht->multihash_finalize();
	if(success){
	    printf ("%"PRIu64"\t%s", fdht->stat_bytes,
		    fdht->hash_hex[opt_md5deep_mode_algorithm].c_str());
	}

	/*
	 * Rather than muck about with updating the state of the input
	 * file, just reset everything and process it normally.
	 */
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
	 * It returns FALSE if there is a failure.
	 */
	if (!compute_hash(fdht)) {
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
		fdht->file_name_annotation = std::string(" offset ") + itos(fdht->read_start)
		    + std::string("-") + itos(tmp_end);
	    }
      
	    fdht->multihash_finalize();

	    if(s->md5deep_mode){
		// Under not matched mode, we only display those known hashes that
		// didn't match any input files. Thus, we don't display anything now.
		// The lookup is to mark those known hashes that we do encounter.
		// searching for the hash will cause matched_file_number to be set
		if (s->mode & mode_not_matched){
		    s->known.find_hash(opt_md5deep_mode_algorithm,
				       fdht->hash_hex[opt_md5deep_mode_algorithm],
				       fdht->file_number);
		}
		else {
		    md5deep_display_hash(fdht);
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


/**
 * Given a file name, hash it into a fdht, then ask s to deal with it.
 */
int state::hash_file(file_data_hasher_t *fdht,const tstring &fn)
{
    if(opt_verbose>=MORE_VERBOSE){
	print_error("hash_file(%s) mode=%x primary_function=%d",
		    main::make_utf8(fn).c_str(),
		    this->mode,this->primary_function);
    }

    int status		= STATUS_OK;
    fdht->is_stdin	= FALSE;
    fdht->file_name	= main::make_utf8(fn);

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

    if ((fdht->handle = _tfopen(fn.c_str(),_TEXT("rb"))) != NULL) {
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
