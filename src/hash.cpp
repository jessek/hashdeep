/*
 * By Jesse Kornblum and Simson Garfinkel
 *
 * This is a work of the US Government. In accordance with 17 USC 105,
 * copyright protection is not available for any work of the US Government.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Id$
 *
 * 2011 JUN 1 - SLG - Removed #ifdef MD5DEEP, since we now have a single binary for both MD5DEEP and HASHDEEP
 */

#include "main.h"
#include "threadpool.h"
#include <sstream>

/****************************************************************
 *** Service routines
 ****************************************************************/

/*
 * Returns TRUE if errno is currently set to a fatal error. That is,
 * an error that can't possibly be fixed while trying to read this file
 */
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


static inline uint64_t min(uint64_t a,uint64_t b){
    if(a<b) return a;
    return b;
}

/*
 * Compute the hash on fdht and store the results in the display ocb.
 * returns true if successful, flase if failure.
 * Doesn't need to seek because the caller handles it. 
 */

#if 0
    size_t read_size = request_len;

    if (this->block_size < ){
	mysize = this->block_size;
    }

    uint64_t remaining = this->block_size;

    // Seek if necessary...
	if(this->read_start != request_start){
	    fseek(this->handle,
	}
    }
    this->read_end   = this->read_start;
#endif


	

bool file_data_hasher_t::compute_hash(uint64_t request_start,uint64_t request_len)
{
    /*
     * We may need to read multiple times; don't read more than
     * file_data_hasher_t::MD5DEEP_IDEAL_BLOCK_SIZE
     * at a time.
     */
    // We get weird results calling ftell on stdin!
    if (this->handle != stdin){
	if(ftello(this->handle) != (int64_t)request_start){
	    fseeko(this->handle,request_start,SEEK_SET);
	}
    }

    this->read_offset = request_start;
    this->read_len    = 0;		// so far
    this->multihash_initialize();
    while (!feof(this->handle) && request_len>0){
	// Clear the buffer in case we hit an error and need to pad the hash 
	unsigned char buffer[file_data_hasher_t::MD5DEEP_IDEAL_BLOCK_SIZE];
	memset(buffer,0,sizeof(buffer));

	// Figure out how many bytes we need to read 
	uint64_t toread = min(request_len,file_data_hasher_t::MD5DEEP_IDEAL_BLOCK_SIZE);
	size_t current_read_bytes = fread(buffer, 1, toread, this->handle);
    
	/* Update the pointers and the hash */
	this->file_bytes    += current_read_bytes;
	this->read_len     += current_read_bytes;
	this->multihash_update(buffer,current_read_bytes); // hash in the non-error
      
	// Get set up for the next read
	request_start += toread;
	request_len   -= toread;

	// If an error occured, display a message and see if we need to quit.
	if (ferror(this->handle)) {
	    ocb->error_filename(this->file_name,"error at offset %"PRIu64": %s",
			       ftello(this->handle), strerror(errno));
	   
	    if (file_fatal_error()){
		this->ocb->set_return_code(status_t::status_EXIT_FAILURE);
		return false;		// error
	    }
	    clearerr(this->handle);
      
	    // The file pointer's position is now undefined. We have to manually
	    // advance it to the start of the next buffer to read. 
	    if(this->handle != stdin){
		fseeko(this->handle,request_start,SEEK_SET);
	    }
	}

	// If we are printing estimates, update the time
	if (opt_estimate)    {
	    time_t current_time = time(0);
	    // We only update the display only if a full second has elapsed 
	    if (this->last_time != current_time) {
		this->last_time = current_time;
		ocb->display_realtime_stats(this,current_time - this->start_time);
	    }
	}
    }
    if (opt_estimate) ocb->clear_realtime_stats();
    this->multihash_finalize();
    return true;			// done hashing!
}

/**
 *
 * THIS IS IT!!!!
 *
 * hash()
 * 
 * This function is called to hash each file.
 * Calls:
 *    - compute_hash() to hash the file or the file segment
 *    - display_hash() to display each hash after it is computed.
 *
 * Called by: hash_stdin and hash_file.
 * 
 * Triage Mode: First computes the hash on the first 512-bytes
 * Piecewise Mode: Calls pieceiwse hasher iteratively
 * In the case of piecewise hashing, 
 * This routine is made multi-threaded to make the system run faster.
 */
void file_data_hasher_t::hash()
{
    file_data_hasher_t *fdht = this;
    fdht->file_bytes = 0;		// actual number of bytes we have read

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
	
	bool success = fdht->compute_hash(0,512);

	if(success){
	    std::stringstream ss;
	    ss << fdht->stat_bytes << "\t" << fdht->hash_hex[opt_md5deep_mode_algorithm];
	    fdht->triage_info = ss.str();
	}

	/*
	 * Rather than muck about with updating the state of the input
	 * file, just reset everything and process it normally.
	 */
	fdht->file_bytes = 0;
	fseeko(fdht->handle, 0, SEEK_SET);
    }

    /*
     * Read the file, handling piecewise hashing as necessary
     */

    uint64_t request_start = 0;
    while (!feof(fdht->handle))  {
	
	uint64_t request_len = 0xffffffffffffffffUL; // really big
	if ( fdht->ocb->piecewise_size>0 )  {
	    request_len = fdht->ocb->piecewise_size;
	}

	/**
	 * call compute_hash(), which computes the hash of the full file, or next next piecewise hashe.
	 * It returns FALSE if there is a failure.
	 */
	if (fdht->compute_hash(request_start,request_len)==false) {
	    break;
	}
	request_start += request_len;

	/*
	 * We should only display a hash if we've processed some
	 * data during this read OR if the whole file is zero bytes long.
	 * If the file is zero bytes, we won't have read anything, but
	 * still need to display a hash.
	 */

	if (fdht->read_len > 0 || fdht->stat_bytes==0) {
	    if(md5deep_mode){
		/**
		 * Under not matched mode, we only display those known hashes that
		 *  didn't match any input files. Thus, we don't display anything now.
		 * The lookup is to mark those known hashes that we do encounter.
		 * searching for the hash will cause matched_file_number to be set
		 */
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
    }

    /**
     * If we are in dfxml mode, output the DFXML, which may optionally include
     * all of the piecewise information.
     */
    ocb->dfxml_write(this);
}


/* Here is where we tie-in to the threadpool system */
void worker::do_work(file_data_hasher_t *fdht)
{
    // this is all we should need to do
    //std::cerr << "worker " << workerid << "\n";
    //std::stringstream s;
    // s << "TK worker " << workerid << " ";
    //fdht->file_name_annotation = s.str();
    fdht->hash();
    delete fdht;
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

    // stat the file to get the bytes and timestamp
    state::file_type(fn,ocb,&fdht->stat_bytes,&fdht->timestamp);

    if(opt_verbose>=MORE_VERBOSE){
	print_error("hash_file(%s) primary_function=%d",
		    main::make_utf8(fn).c_str(),ocb->primary_function);
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

    //lock();
    //std::cout << "TK0 hash_file " << fdht->file_name << " piecewise_size= " << fdht->piecewise_size << "\n"; 
    //unlock();

    /* Open the file for hashing! */
    fdht->handle = _tfopen(fn.c_str(),_TEXT("rb"));
    if(fdht->handle==0){
	ocb->error_filename(fn,"%s", strerror(errno));
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
	    if(md5deep_mode){
		this->md5deep_display_hash(fdht);
	    } else {
		this->display_hash(fdht);
	    }
	}
	fclose(fdht->handle);
	return ;
    }

    // DO THE HASH EITHER IN THIS THREAD OR ANOTHER THREAD
    // hash() WILL CLOSE AND RELEASE
    if(tp){
	tp->schedule_work(fdht);
	return;
    }
    
    // Do the has locally
    fdht->hash();			// DO THE HASH!
    delete fdht;
}


void display::hash_stdin()
{
    file_data_hasher_t *fdht = new file_data_hasher_t(this);
    fdht->file_name = "stdin";
    fdht->handle    = stdin;
    fdht->hash();
}
