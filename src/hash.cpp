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

/****************************************************************
 *** Service routines
 ****************************************************************/

/*
 * Returns TRUE if errno is currently set to a fatal error. That is,
 * an error that can't possibly be fixed while trying to read this file
 */
static int file_fatal_error()
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

static std::string make_stars(size_t count)
{
    std::string ret;
    for (size_t i = 0 ; i < count ; i++)
    {
      ret.push_back('*');
    }
    return ret;
}


static inline uint64_t min(uint64_t a,uint64_t b){
    if(a<b) return a;
    return b;
}
static inline uint64_t max(uint64_t a,uint64_t b){
    if(a>b) return a;
    return b;
}

/*
 * Compute the hash on fdht and store the results in the display ocb.
 * returns true if successful, flase if failure.
 * Doesn't need to seek because the caller handles it. 
 */


/**
 * compute_hash is where the data gets read and hashed.
 */

bool file_data_hasher_t::compute_hash(uint64_t request_start,uint64_t request_len,
				      hash_context_obj *hc1,hash_context_obj *hc2)
{
    /* Shortcut; If no hash algorithms are specified, just advance the pointer and return */
    if(algorithm_t::algorithms_in_use_count()==0){
	eof = true;			// read everything (because we are not hashing)
	return true;			// done hashing
    }

    /*
     * We may need to read multiple times; don't read more than
     * file_data_hasher_t::MD5DEEP_IDEAL_BLOCK_SIZE
     * at a time.
     */

    hc1->read_offset = request_start;
    hc1->read_len    = 0;		// so far

    while (request_len>0){
	// Clear the buffer in case we hit an error and need to pad the hash 
	// The use of MD5DEEP_IDEAL_BLOCK_SIZE means that we loop even for memory-mapped
	// files. That's okay, becuase our super-fast SHA1 implementation
	// actually corrupts its input buffer, forcing a copy...

	unsigned char buffer_[file_data_hasher_t::MD5DEEP_IDEAL_BLOCK_SIZE];
	const unsigned char *buffer = buffer_;
	uint64_t toread = min(request_len,file_data_hasher_t::MD5DEEP_IDEAL_BLOCK_SIZE); // and shrink

	if(this->base==0){		// not mmap, so clear buffer
	    memset(buffer_,0,sizeof(buffer_));
	}

	ssize_t current_read_bytes = 0;	// read the data into buffer

	if(this->handle){
	    current_read_bytes = fread(buffer_, 1, toread, this->handle);
	} else {
	    assert(this->fd!=0);
	    if(this->base){
		buffer = this->base + request_start;
		current_read_bytes = min(toread,this->bounds - request_start); // can't read more than this
		if(hc1->read_offset+current_read_bytes==this->bounds){
		    this->eof = true;	// we hit the end
		}
	    } else {
		current_read_bytes = read(this->fd,buffer_,toread);
	    }
	}
    
	// If an error occured, display a message and see if we need to quit.
	if ((current_read_bytes<0) || (this->handle && ferror(this->handle))){
	    ocb->error_filename(this->file_name,"error at offset %"PRIu64": %s",
				request_start, strerror(errno));
	   
	    if (file_fatal_error()){
		this->ocb->set_return_code(status_t::status_EXIT_FAILURE);
		return false;		// error
	    }
	    if(this->handle) clearerr(this->handle);
      
	    // The file pointer's position is now undefined. We have to manually
	    // advance it to the start of the next buffer to read. 
	    if(this->is_stdin()==false){
		if(this->handle) fseeko(this->handle,request_start,SEEK_SET);
		if(this->fd)     lseek(this->fd,request_start,SEEK_SET);
	    }
	}

	/* Update the pointers and the hash */
	if(current_read_bytes>0){
	    this->file_bytes   += current_read_bytes;
	    hc1->read_len     += current_read_bytes;
	    hc1->multihash_update(buffer,current_read_bytes); // hash in the non-error
	    if(hc2) hc2->multihash_update(buffer,current_read_bytes); // hash in the non-error
	}
      
	// If we are printing estimates, update the time
	if (ocb->opt_estimate)    {
	    time_t current_time = time(0);
	    // We only update the display only if a full second has elapsed 
	    if (this->last_time != current_time) {
		this->last_time = current_time;
		ocb->display_realtime_stats(this,hc1,current_time - this->start_time);
	    }

	}
	// If we are at the end of the file, break
	if((current_read_bytes==0) || (this->handle && feof(this->handle))){
	    this->eof = true;
	    break;
	}

	// Get set up for the next read
	request_start += toread;
	request_len   -= toread;
    }
    if (ocb->opt_estimate) ocb->clear_realtime_stats();
    if (this->file_bytes == this->stat_bytes) this->eof = true; // end of the file
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
#ifndef O_BINARY
#define O_BINARY 0
#endif
mutex_t file_data_hasher_t::fdh_lock;

void file_data_hasher_t::hash()
{
    file_data_hasher_t *fdht = this;

    /*
     * If the handle is set, we are probably hashing stdin.
     * If not, figure out file size and full file name for the handle
     */
    if(fdht->handle==0){		
	/* Open the file and print an error if we can't */
	// stat the file to get the bytes and ctime
	//state::file_type(fdht->file_name_to_hash,ocb,&fdht->stat_bytes,
	//&fdht->ctime,&fdht->mtime,&fdht->atime);
	file_metadata_t m;
	file_metadata_t::stat(fdht->file_name_to_hash,&m,*ocb);
	fdht->stat_bytes = m.size;
	fdht->ctime      = m.ctime;
	fdht->mtime      = m.mtime;
	fdht->atime      = m.atime;

	if(ocb->opt_verbose>=MORE_VERBOSE){
	    errno = 0;			// no error
	}
	fdht->file_name	= global::make_utf8(fdht->file_name_to_hash);
	fdht->file_bytes = 0;		// actual number of bytes we have read

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

	switch(ocb->opt_iomode){
	case iomode::buffered:
	    assert(fdht->handle==0);

	    /* Corrects bug 3476412 on MacOS 10.6 
	     * in which the 'too many files' error was generated when two threads
	     * tried to run fopen() simultaneously. This bug was apparently fixed
	     * in MacOS 10.7. There is only minimal overhead with the lock, however,
	     * and there is a chance that other platforms may have a similar bug,
	     * so we always do it, just to be safe.
	     * Simson L. Garfinkel, Jan 21, 2012
	     */
	    fdh_lock.lock();
	    fdht->handle = _tfopen(file_name_to_hash.c_str(),_TEXT("rb"));
	    fdh_lock.unlock();

	    if(fdht->handle==0){
		ocb->error_filename(fdht->file_name_to_hash,"%s", strerror(errno));
		return;
	    }
	    break;
	case iomode::unbuffered:
	    assert(fdht->fd==-1);
	    fdht->fd    = _topen(file_name_to_hash.c_str(),O_BINARY|O_RDONLY,0);
	    if(fdht->fd<0){
		ocb->error_filename(fdht->file_name_to_hash,"%s", strerror(errno));
		return;
	    }
	    break;
	case iomode::mmapped:
	    fdht->fd    = _topen(file_name_to_hash.c_str(),O_BINARY|O_RDONLY,0);
	    if(fdht->fd<0){
		ocb->error_filename(fdht->file_name_to_hash,"%s", strerror(errno));
		return;
	    }
#ifdef HAVE_MMAP
	    fdht->base = (uint8_t *)mmap(0,fdht->stat_bytes,PROT_READ,
#if HAVE_DECL_MAP_FILE
		MAP_FILE|
#endif
		MAP_SHARED,fd,0);
	    if(fdht->base>0){		
		/* mmap is successful, so set the bounds.
		 * if it is not successful, we default to reading the fd
		 */
		fdht->bounds = fdht->stat_bytes;
	    }
#endif
	    break;
	default:
	    ocb->fatal_error("hash.cpp: iomode setting invalid (%d)",ocb->opt_iomode);
	}

	// If this file is above the size threshold set by the user, skip it
	// and set the hash to be stars
	if ((ocb->mode_size) and (fdht->stat_bytes > ocb->size_threshold)) 
	{
	  if (ocb->mode_size_all) 
	  {
	    for (int i = 0 ; i < NUM_ALGORITHMS ; ++i) 
	    {
	      if (hashes[i].inuse)
	      {
		fdht->hash_hex[i] = make_stars(hashes[i].bit_length/4);
	      }
	    }

	    if (md5deep_mode)
	    {
	      fdht->ocb->md5deep_display_hash(fdht,0); // no hash
	    } 
	    else 
	    {
	      // RBF - This line causes a CRASH. The function display_hash
	      // RBF - accesses fields in the second argument, which is supposed
	      // RBF - to be a pointer, without checking whether the pointer
	      // RBF - is valid or not.
	      // RBF - Replicate with ./hashdeep -I 1 *
	      fdht->ocb->display_hash(fdht,0);
	    }
	  }

	  // close will happend when the fdht is killed
	  return ;
	}
    }

    if (ocb->opt_estimate)  {
	time(&(fdht->start_time));
	fdht->last_time = fdht->start_time;
    }

    if (fdht->ocb->mode_triage && fdht->is_stdin()==false)  {
	/*
	 * Triage mode output consists of file size, hash of the first 512 bytes, then hash of the whole file.
	 * 
	 * We use the piecewise mode to get a partial hash of the first 
	 * 512 bytes of the file. But we'll have to remove piecewise mode
	 * before returning to the main hashing code.
	 *
	 * Triage mode is only available for md5deep series programs.
	 *
	 * Disabled for stdin becuase we can't seek back and search again.
	 */
	
	hash_context_obj hc_triage;
	hc_triage.multihash_initialize();
	bool success = fdht->compute_hash(0,512,&hc_triage,0);
	hc_triage.multihash_finalize(this->hash_hex);			// finalize and save the results

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
	if(fdht->handle) fseeko(fdht->handle, 0, SEEK_SET);
	if(fdht->fd){
	    lseek(this->fd,0,SEEK_SET);
	}
	fdht->eof = false;		// 
    }

    /*
     * Read the file, handling piecewise hashing as necessary
     */

    uint64_t request_start = 0;
    hash_context_obj *hc_file= 0;	// if we are doing picewise hashing, this stores the file context

    if(fdht->ocb->piecewise_size>0){
	hc_file = new hash_context_obj();
	hc_file->multihash_initialize();
    }

    while (fdht->eof==false)  {
	
	uint64_t request_len = fdht->stat_bytes; // by default, hash the file
	if ( fdht->ocb->piecewise_size>0 )  {
	    request_len = fdht->ocb->piecewise_size;
	}

	/**
	 * call compute_hash(), which computes the hash of the full file, or next next piecewise hashe.
	 * It returns FALSE if there is a failure.
	 */
	hash_context_obj hc_piece;
	hc_piece.multihash_initialize();
	bool r = fdht->compute_hash(request_start,request_len,&hc_piece,hc_file);
	hc_piece.multihash_finalize(this->hash_hex);			// finalize and save the results

	if (r==false) {
	    break;
	}
	request_start += request_len;

	/*
	 * We should only display a hash if we've processed some
	 * data during this read OR if the whole file is zero bytes long.
	 * If the file is zero bytes, we won't have read anything, but
	 * still need to display a hash.
	 */

	if (hc_piece.read_len > 0 || fdht->stat_bytes==0 || fdht->is_stdin()) {
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
				   fdht->file_name,
				   fdht->file_number);
		}
		else {
		    ocb->md5deep_display_hash(this,&hc_piece);
		}
	    } else {
		ocb->display_hash(fdht,&hc_piece);
	    }
	}
    }

    /**
     * If we had an additional hash context for the file,
     * then we must be in DFXML mode and doing piecewise hashing.
     * We want both the hash of the file and of the context.
     */
    if(hc_file){
	std::string file_hashes[NUM_ALGORITHMS];
	hc_file->multihash_finalize(file_hashes);
	this->dfxml_write_hashes(file_hashes,0);
    }

    ocb->dfxml_write(this);
    if(hc_file) delete hc_file;
}


#ifdef HAVE_PTHREAD
/* Here is where we tie-in to the threadpool system.
 */
void worker::do_work(file_data_hasher_t *fdht)
{
    fdht->set_workerid(workerid);
    fdht->hash();
    delete fdht;
}
#endif


/**
 * Primary entry point for a file being hashed.
 * Given a file name:
 * 1 - create a minimal fdht and pass it off to either schedule_work() or hash().
 *
 * hash() will 
 * 1 - open the file (or print an error message if it can't).
 * 2 - hash the fdht
 * 3 - record it in stdout using display.
 */
void display::hash_file(const tstring &fn)
{
    file_data_hasher_t *fdht = new file_data_hasher_t(this);
    fdht->file_name_to_hash = fn;

    /**
     * If we are using a thread pool, hash in another thread
     * with do_work 
     */
#ifdef HAVE_PTHREAD
    if(tp){
	tp->schedule_work(fdht);
	return;
    }
#endif
    /* no threading; just work normally */
    fdht->hash();		
    delete fdht;
}


/* Hashing stdin can only be done with buffered I/O.
 * Note that it is only hashed in the main thread.
 * No reason to hash stdin in other threads, since we
 * are only hashing one file.
 */
void display::hash_stdin()
{
    file_data_hasher_t *fdht = new file_data_hasher_t(this);
    fdht->file_name_to_hash = _T("stdin");
    fdht->file_name  = "stdin";
    fdht->handle     = stdin;
#ifdef _WIN32
    /* see http://support.microsoft.com/kb/58427 */
    if (setmode(fileno(stdin),O_BINARY) == -1 ){
	fatal_error("Cannot set stdin to binary mode");
    }
#endif
#ifdef SIZE_T_MAX
    fdht->stat_bytes = SIZE_T_MAX;
#else
    fdht->stat_bytes = 0x7fffffffffffffffLL;
#endif
    fdht->hash();
    delete fdht;
}
