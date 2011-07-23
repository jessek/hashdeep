#include "main.h"
#include "utf8.h"
#include <stdarg.h>

/**
 *
 * display.cpp:
 * Manages user output.
 * All output is in UTF-8.
 *
 * If opt_escape8 is set, then non-ASCII UTF-8 characters are turned
 * into U+XXXX notation.
 * 
 * NOTE WITH MINGW GCC-4.3.0:
 * You will get a warning from the format. IGNORE IT.
 * See http://lists.gnu.org/archive/html/qemu-devel/2009-01/msg01979.html
 *
 * All output is threadsafe.
 */

/****************************************************************
 ** Support routines
 ****************************************************************/

void display::set_outfilename(std::string outfilename)
{
    myoutstream.open(outfilename.c_str(),std::ios::out|std::ios::binary);
    if(!myoutstream.is_open()){
	fatal_error("%s: Cannot open: ",opt_outfilename.c_str());
    }
    out = &myoutstream;
}

void display::writeln(std::ostream *os,const std::string &str)
{
    lock();
    (*os) << str;
    if (opt_zero){
	(*os) << '\000';
    }
    else {
	(*os) << NEWLINE;
    }
    os->flush();
    unlock();
}

void display::status(const char *fmt,...)
{
    va_list(ap); 
    va_start(ap,fmt); 

    char *ret = 0;
    if(vasprintf(&ret,fmt,ap) < 0){
	(*out) << __progname << ": " << strerror(errno);
	exit(EXIT_FAILURE);
    }
    writeln(out,ret);
    free(ret);
    va_end(ap);
}

void display::error(const char *fmt,...)
{
    va_list(ap); 
    va_start(ap,fmt); 
    char *ret = 0;
    if(vasprintf(&ret,fmt,ap) < 0){
	(*out) << __progname << ": " << strerror(errno);
	exit(EXIT_FAILURE);
    }
    writeln(&std::cerr,ret);
    free(ret);
    va_end(ap);
}

void display::try_msg(void)
{
    writeln(&std::cerr,std::string("Try `") + __progname + " -h` for more information.");
}


void display::fatal_error(const char *fmt,...)
{
    va_list(ap); 
    va_start(ap,fmt); 

    char *ret = 0;
    if(vasprintf(&ret,fmt,ap) < 0){
	(*out) << __progname << ": " << strerror(errno);
	exit(EXIT_FAILURE);
    }
    writeln(&std::cerr,ret);
    free(ret);
    va_end(ap);
    exit(1);
}

void display::internal_error(const char *fmt,...)
{
    va_list(ap); 
    va_start(ap,fmt); 

    char *ret = 0;
    if(vasprintf(&ret,fmt,ap) < 0){
	(*out) << __progname << ": " << strerror(errno);
	exit(EXIT_FAILURE);
    }
    writeln(&std::cerr,ret);
    writeln(&std::cerr,std::string(__progname)+": Internal error. Contact developer!");
    va_end(ap);
    free(ret);
    exit(1);
}

void display::print_debug(const char *fmt, ... )
{
#ifdef __DEBUG
    va_list(ap); 
    va_start(ap,fmt); 
    char *ret = 0;
    if(vasprintf(&ret,fmt,ap) < 0){
	(*out) << __progname << ": " << strerror(errno);
	exit(EXIT_FAILURE);
    }
    writeln(&std::cerr,std::string("DEBUG:" ) + ret);
    free(ret);
    va_end(ap);
#endif
}
/**
 * output the string, typically a fn, optionally performing unicode escaping
 */

std::string display::fmt_filename(const filename_t &fn) const
{
    if(opt_unicode_escape){
	return main::escape_utf8(fn);
    } else {
	return main::make_utf8(fn);
    }
}




void display::print_error(const char *fmt, ...)
{
    if(opt_silent) return;
    va_list(ap);
    va_start(ap,fmt);
    char *ret = 0;
    if(vasprintf(&ret,fmt,ap) < 0){
	std::cerr << __progname << ": " << strerror(errno);
	exit(EXIT_FAILURE);
    }
    lock();
    std::cerr << ret << NEWLINE;
    unlock();
    free(ret);
    va_end(ap);
}


void display::error_filename(const filename_t &fn,const char *fmt, ...)
{
    if(opt_silent) return;

    va_list(ap); 
    va_start(ap,fmt); 
    char *ret = 0;
    if(vasprintf(&ret,fmt,ap) < 0){
	(*out) << __progname << ": " << strerror(errno);
	exit(EXIT_FAILURE);
    }
    writeln(&std::cerr,fmt_filename(fn) + ": " + ret);
    free(ret);
    va_end(ap);
}


void display::clear_realtime_stats()
{
    lock();
    std::cerr << "\r" << BLANK_LINE;
    unlock();
}

// At least one user has suggested changing update_display() to 
// use human readable units (e.g. GB) when displaying the updates.
// The problem is that once the display goes above 1024MB, there
// won't be many updates. The counter doesn't change often enough
// to indicate progress. Using MB is a reasonable compromise. 

void display::display_realtime_stats(const file_data_hasher_t *fdht, time_t elapsed)
{
    uint64_t mb_read=0;
    bool shorten = false;

    // If we've read less than one MB, then the computed value for mb_read 
    // will be zero. Later on we may need to divide the total file size, 
    // total_megs, by mb_read. Dividing by zero can create... problems 
    if (fdht->bytes_read < ONE_MEGABYTE){
	mb_read = 1;
    }
    else {
	mb_read = fdht->actual_bytes / ONE_MEGABYTE;
    }
  
    char msg[64];
    memset(msg,0,sizeof(msg));

    if (fdht->stat_megs()==0 || opt_estimate==false)  {
	shorten = true;
	snprintf(msg,sizeof(msg)-1,"%"PRIu64"MB done. Unable to estimate remaining time.%s",
		 mb_read,BLANK_LINE);
    }
    else {
	// Estimate the number of seconds using only integer math.
	//
	// We now compute the number of bytes read per second and then
	// use that to determine how long the whole file should take. 
	// By subtracting the number of elapsed seconds from that, we should
	// get a good estimate of how many seconds remain.

	uint64_t seconds = (fdht->stat_bytes / (fdht->actual_bytes / elapsed)) - elapsed;

	// We don't care if the remaining time is more than one day.
	// If you're hashing something that big, to quote the movie Jaws:
	//        
	//            "We're gonna need a bigger boat."            
	uint64_t hour = seconds / 3600;
	seconds -= (hour * 3600);
    
	uint64_t min = seconds/60;
	seconds -= min * 60;

	shorten = 1;
	snprintf(msg,sizeof(msg),"%"PRIu64"MB of %"PRIu64"MB done, %02"PRIu64":%02"PRIu64":%02"PRIu64" left%s",
		 mb_read, fdht->stat_megs(), hour, min, seconds, BLANK_LINE);
    }
    
    std::string line = std::string("\r");

    /*
     * This is the only place where filenames are shortened.
     * So do it explicitly here.
     *
     */

    tstring fn = fdht->file_name;
    if (fn.size() < MAX_FILENAME_LENGTH){
	/* Shorten from the middle */
	size_t half = MAX_FILENAME_LENGTH/2;
	fn = fn.substr(0,half) + _T("...") + fn.substr(fn.size()-half,half);
    }

    line += fmt_filename(fn);
    line += msg;

    lock();
    std::cerr << line << "\r";
    unlock();
}


void display::display_banner_if_needed()
{
    if(this->dfxml!=0) return;		// output is in DFXML; no banner
    lock();
    if(banner_displayed==false){
	(*out) << utf8_banner << NEWLINE ;
	banner_displayed=true;
    }
    unlock();
}


/* No locks, so do not call on the same FDHT in different threads */
void file_data_hasher_t::compute_dfxml(bool known_hash)
{
    if(this->piecewise_size){
	uint64_t bytes = this->read_end - this->read_start;
	this->dfxml_hash +=
	    std::string("<byte_run file_offset='")
	    + itos(this->read_start)
	    + std::string("' bytes='")
	    + itos(bytes) + std::string("'>\n   ");
    }
    for(int i=0;i<NUM_ALGORITHMS;i++){
	if(hashes[i].inuse){
	    this->dfxml_hash += "<hashdigest type='";
	    this->dfxml_hash += makeupper(this->hash_hex[i]);
	    this->dfxml_hash += std::string("'>") + this->hash_hex[i] + std::string("</hashdigest>\n");
	}
    }
    if(known_hash){
	this->dfxml_hash += std::string("<matched>1</matched>");
    }
    if(this->piecewise_size){
	this->dfxml_hash += "</byte_run>\n";
    }
}

/**
 * Return the number of entries in the hashlist that have used==0
 * Optionally display them, optionally with additional output.
 */
uint64_t display::compute_unused(bool display, std::string annotation)
{
    uint64_t count=0;

    /* This is the only place we iterate through known.
     * We make a list of all the filenames so we can get out of the lock fast.
     */

    std::vector<std::string> filelist;

    lock();
    for(hashlist::const_iterator i = known.begin(); i != known.end(); i++){
	if((*i)->matched_file_number==0){
	    count++;
	    if (display || opt_verbose >= MORE_VERBOSE) {
		filelist.push_back((*i)->file_name);
	    }
	}
    }
    unlock();
    for(std::vector<std::string>::const_iterator i = filelist.begin(); i!= filelist.end(); i++){
	std::string line = *i + annotation;
	writeln(&std::cerr,line);
    }
    return count;
}


/**
 * perform an audit
 */
 

int display::audit_check()
{
    /* Count the number of unused */
    match.unused = compute_unused(false,": Known file not used");
    return (0 == this->match.unused  && 
	    0 == this->match.unknown && 
	    0 == this->match.moved);
}



void display::display_audit_results()
{
    if (audit_check()==0) {
	status("%s: Audit failed", __progname);
	set_return_code(status_t::status_EXIT_FAILURE);
    }
    else {
	status("%s: Audit passed", __progname);
    }
  
    if (opt_verbose)    {
	if(opt_verbose >= MORE_VERBOSE){
	    status("   Input files examined: %"PRIu64, this->match.total);
	    status("  Known files expecting: %"PRIu64, this->match.expect);
	    status(" ");
	}
	status("          Files matched: %"PRIu64, this->match.exact);
	status("Files partially matched: %"PRIu64, this->match.partial);
	status("            Files moved: %"PRIu64, this->match.moved);
	status("        New files found: %"PRIu64, this->match.unknown);
	status("  Known files not found: %"PRIu64, this->match.unused);
    }
}


std::string display::fmt_size(const file_data_t *fdt) const
{
    if (opt_display_size)  {
	if (opt_csv){
	    // Under CSV mode we have to include a comma, otherwise two spaces
	    char buf[64];
	    snprintf(buf,sizeof(buf),"%"PRIu64",", fdt->actual_bytes);
	    return std::string(buf);
	}
	else {
	    char buf[64];
	    snprintf(buf,sizeof(buf),"%10"PRIu64"  ", fdt->actual_bytes);
	    return std::string(buf);
	}
    }
    return std::string();
}


/* The old display_match_result from md5deep */
void display::md5deep_display_match_result(file_data_hasher_t *fdht)
{  
    file_data_t *fs = known.find_hash(opt_md5deep_mode_algorithm,
				      fdht->hash_hex[opt_md5deep_mode_algorithm],
				      fdht->file_number);
    int known_hash = fs ? 1 : 0;

    if ((known_hash && opt_mode_match) || (!known_hash && opt_mode_match_neg)) {
	if(dfxml){
	    fdht->compute_dfxml(opt_show_matched || known_hash);
	    return ;
	}

	std::string line;

	line = fmt_size(fdht);
	if (opt_display_hash) {
	    line += fdht->hash_hex[opt_md5deep_mode_algorithm];
	    if (opt_csv) {
		line += ",";
	    } else if (opt_asterisk) {
		line += " *";
	    } else {
		line += "  ";
	    }
	}
	    
	if (opt_show_matched) 	    {
	    if (known_hash && (opt_mode_match)) {
		line += fdht->file_name + " matched " + fs->file_name;
	    }
	    else {
		line += fdht->file_name + " does NOT match";
	    }
	}
	else{
	    line += fdht->file_name;
	}
	writeln(&std::cerr,line);
    }
}

/* The original display_match_result from md5deep.
 * This should probably be merged with the function above.
 * This function is very similar to audit_update(), which follows
 */
void display::display_match_result(file_data_hasher_t *fdht)
{
    file_data_t *matched_fdt = NULL;
    int should_display; 
    should_display = (primary_match_neg == primary_function);
    
    lock();				// protects the search and the printing
    hashlist::searchstatus_t m = known.search(fdht,&matched_fdt);
    unlock();

    std::string line;

    switch(m){
	// If only the name is different, it's still really a match
	//  as far as we're concerned. 
    case hashlist::status_file_name_mismatch:
    case hashlist::status_match:
	should_display = (primary_match_neg != primary_function);
	break;
	  
    case hashlist::status_file_size_mismatch:
	line = fmt_filename(fdht) + ": Hash collision with " + fmt_filename(matched_fdt);
	writeln(&std::cerr,line);
	break;
	
    case hashlist::status_partial_match:
	line = fmt_filename(fdht) + ": partial hash match with " + fmt_filename(matched_fdt);
	writeln(&std::cerr,line);
	break;
	
    default:
	break;
    }
    if (should_display) {
	if (opt_display_hash){
	    display_hash_simple(fdht);
	}
	else {
	    line = fmt_filename(fdht);
	    if (opt_show_matched && primary_match == primary_function) {
		line += " matches ";
		if (NULL == matched_fdt) {
		    line += "(unknown file)";
		} else {
		    line += fmt_filename(matched_fdt);
		}
	    }
	    writeln(&std::cerr,line);
	}
    }
}


/**
 * Called after every file is hashed by display_hash
 * when primary_function==primary_audit
 * Records every file seen in the 'seen' structure, referencing the 'known' structure.
 */

int display::audit_update(file_data_hasher_t *fdht)
{
    file_data_t *matched_fdht=0;
    lock();				// protects the search and the printing
    hashlist::searchstatus_t m = known.search(fdht,&matched_fdht);
    unlock();
    std::string line;
    switch(m){
    case hashlist::status_match:
    case hashlist::searchstatus_ok:
	this->match.exact++;
	if (opt_verbose >= INSANELY_VERBOSE) {
	    line = fmt_filename(fdht) + ": Ok";
	}
	break;
    case hashlist::status_no_match:
	this->match.unknown++;
	if (opt_verbose >= MORE_VERBOSE) {
	    line = fmt_filename(fdht) + ": No match";
	}
	break;
    case hashlist::status_file_name_mismatch:
	this->match.moved++;
	if (opt_verbose >= MORE_VERBOSE) {
	    line = fmt_filename(fdht) + ": Moved from " + fmt_filename(matched_fdht);
	}
	break;
    case hashlist::status_partial_match:
    case hashlist::status_file_size_mismatch:
	this->match.partial++;
	// We only record the hash collision if it wasn't anything else.
	// At the same time, however, a collision is such a significant
	// event that we print it no matter what. 
	line = fmt_filename(fdht) + ": Hash collision with " + fmt_filename(matched_fdht);
    }
    if(line.size()>0){
	writeln(&std::cerr,line);
    }
    return FALSE;
}

/**
 * Examines the hash table and determines if any known hashes have not
 * been used or if any input files did not match the known hashes. If
 * requested, displays any unused known hashes. Returns a status variable
 */
void display::finalize_matching()
{
    if (known.total_matched!=known.size()){
	return_code.add(status_t::STATUS_UNUSED_HASHES); // were there any unmatched?
    }
    if (known.total_matched==0){
	return_code.add(status_t::STATUS_INPUT_DID_NOT_MATCH); // were they all unmatched?
    }
    if (mode_not_matched){	// should we display those that were not matched?
	compute_unused(true,"");
    }
}



/* The old display_hash from the md5deep program, with modifications
 * to build the line before outputing it.
 *
 */
void  display::md5deep_display_hash(file_data_hasher_t *fdht) // needs hasher because of triage
{
    if (mode_triage) {
	if(dfxml) return;		// traige mode and dfxml are incompatable 
	std::string line = std::string("\t") + fdht->hash_hex[opt_md5deep_mode_algorithm] + std::string("\t") + fdht->file_name;
	writeln(out,line);
	return;
    }

    /**
     * We can't call display_size here because we don't know if we're
     * going to display *anything* yet. If we're in matching mode, we
     * have to evaluate if there was a match first.
     */
    if (opt_mode_match || opt_mode_match_neg){
	md5deep_display_match_result(fdht);
	return;
    }
    
    if(this->dfxml){
	fdht->compute_dfxml(opt_show_matched);
	return;
    }

    std::string line = fmt_size(fdht) + fdht->hash_hex[opt_md5deep_mode_algorithm];

    if (mode_quiet){
	line += "  ";
    }
    else  {
	if ((fdht->piecewise_size) || (fdht->handle!=stdin))    {
	    if (mode_timestamp)      {
		struct tm my_time;
		memset(&my_time,0,sizeof(my_time)); // clear it out
#ifdef HAVE__GMTIME64_S		
		_gmtime64_s(&fdht->timestamp,&my_time);
#endif
#ifdef HAVE__GMTIME64
		my_time = *_gmtime64(&fdht->timestamp);
#endif
#ifdef HAVE_GMTIME_R		
		gmtime_r(&fdht->timestamp,&my_time);
#endif

		char time_str[MAX_TIME_STRING_LENGTH];
		
		// The format is four digit year, two digit month, 
		// two digit hour, two digit minute, two digit second
		strftime(time_str, sizeof(time_str), "%Y:%m:%d:%H:%M:%S", &my_time);

		line += (opt_csv ? ",":" ");
		line += time_str;
	    }
	    if (opt_csv) {
		line += ",";
	    } else if (opt_asterisk) {
		line += " *";
	    } else {
		line += "  ";
	    }
	    line += fmt_filename(fdht);
	}
    }
    writeln(out,line);
}


/*
 * Externally called to display a simple hash
 */
void display::display_hash_simple(file_data_hasher_t *fdht)
{
    if(this->dfxml){
	fdht->compute_dfxml(opt_show_matched);
	return;
    }

    /* In piecewise mode the size of each 'file' is the size
     * of the block it came from. This is important when doing an
     * audit in piecewise mode. In all other cases we use the 
     * total number of bytes from the file we *actually* read
     *
     * NOTE: Ignore the warning in the format when running on mingw with GCC-4.3.0
     * see http://lists.gnu.org/archive/html/qemu-devel/2009-01/msg01979.html
     */
     
    display_banner_if_needed();
    std::string line;

    if (fdht->piecewise_size){
	char buf[1024];
	snprintf(buf,sizeof(buf),"%"PRIu64",", fdht->bytes_read);
	line = std::string(buf);
    }
    else {
	char buf[1024];
	snprintf(buf,sizeof(buf),"%"PRIu64",", fdht->actual_bytes);
	line = std::string(buf);
    }

    for (int i = 0 ; i < NUM_ALGORITHMS ; ++i)  {
	if (hashes[i].inuse){
	    line += fdht->hash_hex[i] + std::string(",");
	}
    }
    line += fmt_filename(fdht);
    writeln(&std::cerr,line);
}


/**
 * Called by hash() in hash.c when the hashing operation is complete.
 * Display the hash and perform any auditing steps.
 */ 
void display::display_hash(file_data_hasher_t *fdht)
{
    switch (primary_function) {
    case primary_compute: 
	display_hash_simple(fdht);
	break;
    case primary_match: 
    case primary_match_neg: 
	display_match_result(fdht);
	break;
    case primary_audit: 
	audit_update(fdht);
	break;
    }
}



void display::dfxml_startup(int argc,char **argv)
{
    if(dfxml){
	lock();
	dfxml->push("dfxml","xmloutputversion='1.0'");
	dfxml->push("metadata",
		       "\n  xmlns='http://md5deep.sourceforge.net/md5deep/' "
		       "\n  xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance' "
		       "\n  xmlns:dc='http://purl.org/dc/elements/1.1/'" );
	dfxml->xmlout("dc:type","Hash List","",false);
	dfxml->pop();
	dfxml->add_DFXML_creator(PACKAGE_NAME,PACKAGE_VERSION,XML::make_command_line(argc,argv));
	dfxml->push("configuration");
	dfxml->push("algorithms");
	for(int i=0;i<NUM_ALGORITHMS;i++){
	    dfxml->make_indent();
	    dfxml->printf("<algorithm name='%s' enabled='%d'/>\n",
			   hashes[i].name.c_str(),hashes[i].inuse);
	}
	dfxml->pop();			// algorithms
	dfxml->pop();			// configuration
	unlock();
    }
}

void display::dfxml_write(file_data_hasher_t *fdht)
{
    if(dfxml){
	lock();
	dfxml->push("fileobject");
	dfxml->xmlout("filename",fdht->file_name);
	dfxml->writexml(fdht->dfxml_hash);
	dfxml->pop();
	unlock();
    }
}

void display::dfxml_shutdown()
{
    if(dfxml){
	lock();
	dfxml->pop();		// outermost
	dfxml->close();
	delete dfxml;
	dfxml = 0;
	unlock();
    }
}

