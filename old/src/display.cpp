#include "main.h"
#include "utf8.h"
#include <stdarg.h>

// $Id$

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
	(*os) << std::endl;
    }
    os->flush();
    unlock();
}

/* special handling for vasprintf under mingw.
 * Sometimes the function prototype is missing. Sometimes the entire function is missing!
 */
#if defined(HAVE_VASPRINTF) && defined(MINGW)
extern "C" {
    int vasprintf(char **ret,const char *fmt,va_list ap);
}
#endif    
	
/*
 * If we don't have vasprintf, bring it in.
 */
/*
#if !defined(HAVE_VASPRINTF) 
extern "C" {
    //
    // We do not have vasprintf.
    // We have determined that vsnprintf() does not perform properly on windows.
    // So we just allocate a huge buffer and then strdup() and hope!
    //
    int vasprintf(char **ret,const char *fmt,va_list ap)
    {
	// Figure out how long the result will be 
	char buf[65536];
	int size = vsnprintf(buf,sizeof(buf),fmt,ap);
	if(size<0) return size;
	// Now allocate the memory 
	*ret = (char *)strdup(buf);
	return size;
    }
}
#endif
*/

void display::status(const char *fmt,...)
{
    va_list(ap); 
    va_start(ap,fmt); 

    char *ret = 0;
    if(vasprintf(&ret,fmt,ap) < 0){
	(*out) << progname << ": " << strerror(errno);
	exit(EXIT_FAILURE);
    }
    writeln(out,ret);
    free(ret);
    va_end(ap);
}

void display::error(const char *fmt,...)
{
    if(opt_silent) return;
    va_list(ap); 
    va_start(ap,fmt); 
    char *ret = 0;
    if (vasprintf(&ret,fmt,ap) < 0)    {
      (*out) << progname << ": " << strerror(errno);
      exit(EXIT_FAILURE);
    }
    lock();
    std::cerr << progname << ": " << ret << std::endl;
    unlock();
    va_end(ap);
}


void display::try_msg(void)
{
    writeln(&std::cerr,std::string("Try `") + progname + " -h` for more information.");
}


void display::fatal_error(const char *fmt,...)
{
    va_list(ap); 
    va_start(ap,fmt); 

    char *ret = 0;
    if(vasprintf(&ret,fmt,ap) < 0){
	(*out) << progname << ": " << strerror(errno);
	exit(EXIT_FAILURE);
    }
    writeln(&std::cerr,progname + ": " + ret);
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
	(*out) << progname << ": " << strerror(errno);
	exit(EXIT_FAILURE);
    }
    writeln(&std::cerr,ret);
    writeln(&std::cerr,progname+": Internal error. Contact developer!");
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
	(*out) << progname << ": " << strerror(errno);
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

std::string display::fmt_filename(const std::string &fn) const
{
    if(opt_unicode_escape){
	return global::escape_utf8(fn);
    } else {
	return fn;			// assumed to be utf8
    }
}


#ifdef _WIN32
std::string display::fmt_filename(const std::wstring &fn) const
{
    if(opt_unicode_escape){
	return global::escape_utf8(global::make_utf8(fn));
    } else {
	return global::make_utf8(fn);
    }
}
#endif



void display::error_filename(const std::string &fn,const char *fmt, ...)
{
    if(opt_silent) return;

    va_list(ap); 
    va_start(ap,fmt); 
    char *ret = 0;
    if(vasprintf(&ret,fmt,ap) < 0){
	(*out) << progname << ": " << strerror(errno);
	exit(EXIT_FAILURE);
    }
    if(dfxml){
	lock();
	dfxml->push("fileobject");
	dfxml->xmlout("filename",fn);
	dfxml->xmlout("error",ret);
	dfxml->pop();
	unlock();
    } else {
	writeln(&std::cerr,fmt_filename(fn) + ": " + ret);
    }
    free(ret);
    va_end(ap);
}

#ifdef _WIN32
void display::error_filename(const std::wstring &fn,const char *fmt, ...)
{
    if(opt_silent) return;

    va_list(ap); 
    va_start(ap,fmt); 
    char *ret = 0;
    if(vasprintf(&ret,fmt,ap) < 0){
	(*out) << progname << ": " << strerror(errno);
	exit(EXIT_FAILURE);
    }
    writeln(&std::cerr,fmt_filename(fn) + ": " + ret);
    free(ret);
    va_end(ap);
}
#endif

void display::clear_realtime_stats()
{
    lock();
    std::cerr << "\r";
    for(int i=0;i<LINE_LENGTH;i++){
	std::cerr << ' ';
    }
    std::cerr << "\r";
    unlock();
}

// At least one user has suggested changing update_display() to 
// use human readable units (e.g. GB) when displaying the updates.
// The problem is that once the display goes above 1024MB, there
// won't be many updates. The counter doesn't change often enough
// to indicate progress. Using MB is a reasonable compromise. 

void display::display_realtime_stats(const file_data_hasher_t *fdht, const hash_context_obj *hc,time_t elapsed)
{
    uint64_t mb_read=0;

    /*
     * This is the only place where filenames are shortened.
     * So do it explicitly here.
     *
     */

    std::stringstream ss;

    std::string fn = fdht->file_name;
    if (fn.size() > MAX_FILENAME_LENGTH){
	/* Shorten from the middle */
	size_t half = (MAX_FILENAME_LENGTH)/2;
	fn = fn.substr(0,half) + "..." + fn.substr(fn.size()-half,half);
    }

    ss << fmt_filename(fn) << " ";

    // If we've read less than one MB, then the computed value for mb_read 
    // will be zero. Later on we may need to divide the total file size, 
    // total_megs, by mb_read. Dividing by zero can create... problems 

    if (hc->read_len < ONE_MEGABYTE){
	mb_read = 1;
    }
    else {
	mb_read = hc->read_len / ONE_MEGABYTE;
    }

    if (fdht->stat_megs()==0 || opt_estimate==false)  {
	ss << mb_read << "MB done. Unable to estimate remaining time.";
    }
    else {
	// Estimate the number of seconds using only integer math.
	//
	// We now compute the number of bytes read per second and then
	// use that to determine how long the whole file should take. 
	// By subtracting the number of elapsed seconds from that, we should
	// get a good estimate of how many seconds remain.

	uint64_t seconds = (fdht->stat_bytes / (fdht->file_bytes / elapsed)) - elapsed;

	// We don't care if the remaining time is more than one day.
	// If you're hashing something that big, to quote the movie Jaws:
	//        
	//            "We're gonna need a bigger boat."            
	uint64_t hour = seconds / 3600;
	seconds -= (hour * 3600);
    
	uint64_t min = seconds/60;
	seconds -= min * 60;

	ss << mb_read << "MB of " << fdht->stat_megs() << "MB done, ";
	char msg[64];
	snprintf(msg,sizeof(msg),"%02"PRIu64":%02"PRIu64":%02"PRIu64" left", hour, min, seconds);
	ss << msg;
    }
    ss << "\r";
    lock();
    std::cerr << ss.str() << "\r";
    unlock();
}


void display::display_banner_if_needed()
{
    if(this->dfxml!=0) return;		// output is in DFXML; no banner
    lock();
    if(banner_displayed==false){
	(*out) << utf8_banner;
	banner_displayed=true;
    }
    unlock();
}

void file_data_hasher_t::dfxml_write_hashes(std::string hex_hashes[],int indent)
{
    for(int i=0;i<NUM_ALGORITHMS;i++){
	if(hashes[i].inuse){
	    for(int j=0;j<indent;j++){
		this->dfxml_hash << ' ';
	    }
	    this->dfxml_hash << "<hashdigest type='" << makeupper(hashes[i].name) << "'>"
			     << hex_hashes[i] <<"</hashdigest>\n";
	}
    }
}


void file_data_hasher_t::compute_dfxml(bool known_hash,const hash_context_obj *hc)
{
    int indent=0;
    if(hc && this->ocb->piecewise_size>0){
	this->dfxml_hash << "<byte_run file_offset='" << hc->read_offset << "'" 
			 << " len='" << hc->read_len << "'>   \n";
	indent=2;
    }
    
    this->dfxml_write_hashes(this->hash_hex,indent);
    if(known_hash){
	this->dfxml_hash << "<matched>1</matched>";
    }
    if(hc && this->ocb->piecewise_size){
	this->dfxml_hash << "</byte_run>\n";
    }
}

/**
 * Return the number of entries in the hashlist that have used==0
 * Optionally display them, optionally with additional output.
 */
uint64_t display::compute_unused(bool show_display, std::string annotation)
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
	    if (show_display || opt_verbose >= MORE_VERBOSE) {
		filelist.push_back((*i)->file_name);
	    }
	}
    }
    unlock();
    for(std::vector<std::string>::const_iterator i = filelist.begin(); i!= filelist.end(); i++){
	std::string line = *i + annotation;
	writeln(out,line);
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
	status("%s: Audit failed", progname.c_str());
	set_return_code(status_t::status_EXIT_FAILURE);
    }
    else {
	status("%s: Audit passed", progname.c_str());
    }
  
    if (opt_verbose)    {
	if(opt_verbose >= MORE_VERBOSE){
	    status("   Input files examined: %"PRIu64, this->match.total);
	    status("  Known files expecting: %"PRIu64, this->match.expect);
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
	std::stringstream ss;
	ss.width(10);
	ss.fill(' ');
	ss << fdt->file_bytes;
	ss.width(1);
	ss << (opt_csv ? ", " : "  ");
	return ss.str();
    }
    return std::string();
}


// The old display_match_result from md5deep 
void display::md5deep_display_match_result(file_data_hasher_t *fdht,
					   const hash_context_obj *hc)
{  
    lock();
    const file_data_t *fs = known.find_hash(opt_md5deep_mode_algorithm,
					    fdht->hash_hex[opt_md5deep_mode_algorithm],
					    fdht->file_name,
					    fdht->file_number);
    unlock();

    int known_hash = fs ? 1 : 0;
    if ((known_hash && opt_mode_match) || (!known_hash && opt_mode_match_neg)) {
	if(dfxml){
	    fdht->compute_dfxml(opt_show_matched || known_hash,hc);
	    return ;
	}

	std::stringstream line;

	line << fmt_size(fdht);
	if (opt_display_hash) {
	    line << fdht->hash_hex[opt_md5deep_mode_algorithm];
	    if (opt_csv) {
		line << ",";
	    } else if (opt_asterisk) {
		line << " *";
	    } else {
		line << "  ";
	    }
	}
	    
	if (opt_show_matched) 	    {
	    if (known_hash && (opt_mode_match)) {
		line << fdht->file_name << " matched " << fs->file_name;
	    }
	    else {
		line << fdht->file_name << " does NOT match";
	    }
	}
	else{
	    line << fdht->file_name;
	    if(piecewise_size) {
		line << " offset " << hc->read_offset << "-" << hc->read_offset + hc->read_len - 1;
	    }
	}
	writeln(out,line.str());
    }
}

/* The original display_match_result from md5deep.
 * This should probably be merged with the function above.
 * This function is very similar to audit_update(), which follows
 */
void display::display_match_result(file_data_hasher_t *fdht,const hash_context_obj *hc)
{
    file_data_t *matched_fdt = NULL;
    int should_display = (primary_match_neg == primary_function);
    
    lock();				// protects the search and the printing
    hashlist::searchstatus_t m = known.search(fdht,
					      &matched_fdt,
					      opt_case_sensitive);
    unlock();

    std::stringstream line1;

    switch(m){
	// If only the name is different, it's still really a match
	//  as far as we're concerned. 
    case hashlist::status_file_name_mismatch:
    case hashlist::status_match:
	should_display = (primary_match_neg != primary_function);
	break;
	  
    case hashlist::status_file_size_mismatch:
	line1 << fmt_filename(fdht) << ": Hash collision with " << fmt_filename(matched_fdt);
	writeln(&std::cerr,line1.str());
	break;
	
    case hashlist::status_partial_match:
	line1 << fmt_filename(fdht) << ": partial hash match with " << fmt_filename(matched_fdt);
	writeln(&std::cerr,line1.str());
	break;
	
    default:
	break;
    }
    if (should_display) {
	std::stringstream line2;

	if (opt_display_hash){
	    display_hash_simple(fdht,hc);
	}
	else {
	    line2 << fmt_filename(fdht);
	    if (opt_show_matched && primary_match == primary_function) {
		line2 << " matches ";
		if (NULL == matched_fdt) {
		    line2 << "(unknown file)";
		} else {
		    line2 << fmt_filename(matched_fdt);
		}
	    }
	    writeln(out,line2.str());
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
    hashlist::searchstatus_t m = known.search(fdht,
					      &matched_fdht,
					      opt_case_sensitive);
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
	writeln(&std::cout,line);
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
    /* Could the total matched */
    lock();
    uint64_t total_matched = known.total_matched();
    unlock();

    if (total_matched != known_size())
    {
      // were there any unmatched?
      return_code.add(status_t::STATUS_UNUSED_HASHES); 
    }

    // RBF - Implement check for Input Did not Match
    /*
    if (this->match.unknown)
    {
      return_code.add(status_t::STATUS_INPUT_DID_NOT_MATCH);
    }
    */

    if (mode_not_matched)
    {	
      // should we display those that were not matched?
      compute_unused(true,"");
    }
}


mutex_t    display::portable_gmtime_mutex;

struct tm  *display::portable_gmtime(struct tm *my_time,const timestamp_t *t)
{
    memset(my_time,0,sizeof(*my_time)); // clear it out
#ifdef HAVE_GMTIME_R		
    gmtime_r(t,my_time);
    return my_time;
#endif
#ifdef HAVE__GMTIME64
    // This is not threadsafe, hence the lock 
    portable_gmtime_mutex.lock();
    *my_time = *_gmtime64(t);
    portable_gmtime_mutex.unlock();

    //we tried this:
    //_gmtime64_s(&fdht->timestamp,&my_time);
    //but it had problems on mingw64
    return my_time;
#endif
#if !defined(HAVE__GMTIME64) && !defined(HAVE_GMTIME_R)
#error This program requires either _gmtime64 or gmtime_r for compilation
#endif
}

/* The old display_hash from the md5deep program, with modifications
 * to build the line before outputing it.
 *
 * needs hasher because of triage mode
 */

void  display::md5deep_display_hash(file_data_hasher_t *fdht,const hash_context_obj *hc) 
{
    /**
     * We can't call display_size here because we don't know if we're
     * going to display *anything* yet. If we're in matching mode, we
     * have to evaluate if there was a match first.
     */
    if (opt_mode_match || opt_mode_match_neg){
	md5deep_display_match_result(fdht,hc);
	return;
    }
    
    if(this->dfxml){
	fdht->compute_dfxml(opt_show_matched,hc);
	return;
    }

    std::stringstream line;

    if (mode_triage) {
	line << fdht->triage_info << "\t";
    }
    line << fmt_size(fdht) << fdht->hash_hex[opt_md5deep_mode_algorithm];

    if (mode_quiet){
	line << "  ";
    }
    else  {
	if ((fdht->ocb->piecewise_size) || (fdht->is_stdin()==false))    {
	    if (mode_timestamp)      {
		struct tm my_time;
		portable_gmtime(&my_time,&fdht->ctime);
		char time_str[MAX_TIME_STRING_LENGTH];
		
		// The format is four digit year, two digit month, 
		// two digit hour, two digit minute, two digit second
		strftime(time_str, sizeof(time_str), "%Y:%m:%d:%H:%M:%S", &my_time);
		line << (opt_csv ? ",":" ") << time_str;
	    }
	    if (opt_csv) {
		line << ",";
	    } else if (opt_asterisk) {
		line << " *";
	    } else if (mode_triage) {
		line << "\t";
	    } else {
		line << "  ";
	    }
	    line << fmt_filename(fdht);
	}
    }
    if(hc && fdht->ocb->piecewise_size > 0){
	uint64_t len = (hc->read_offset+hc->read_len-1);
	if(hc->read_offset==0 && hc->read_len==0) len=0;
	line << " offset " << hc->read_offset << "-" << len;
    }
    writeln(out,line.str());
}


/*
 * Externally called to display a simple hash
 */
void display::display_hash_simple(file_data_hasher_t *fdht,
				  const hash_context_obj *hc)
{
    if (this->dfxml)
    {
      fdht->compute_dfxml(opt_show_matched,hc);
      return;
    }

    /* Old comment:
     * In piecewise mode the size of each 'file' is the size
     * of the block it came from. This is important when doing an
     * audit in piecewise mode. In all other cases we use the 
     * total number of bytes from the file we *actually* read
     *
     * New comment:
     * read_len is always correct.
     * In piecewise mode its the size of the piece,
     * In normal mode it's the size of the file.
     * 
     * NOTE: Ignore the warning in the format when running on mingw with GCC-4.3.0
     * see http://lists.gnu.org/archive/html/qemu-devel/2009-01/msg01979.html
     *
     */
     
    display_banner_if_needed();
    std::stringstream line;

    line << hc->read_len << ",";

    for (int i = 0 ; i < NUM_ALGORITHMS ; ++i)
    {
      if (hashes[i].inuse)
      {
	line << fdht->hash_hex[i] << ",";
      }
    }
    line << fmt_filename(fdht);
    if(fdht->ocb->piecewise_size > 0){
	std::stringstream ss;
	uint64_t len = (hc->read_offset+hc->read_len-1);
	if(hc->read_offset==0 && hc->read_len==0) len=0;
	line << " offset " << hc->read_offset << "-" << len;
    }
    writeln(out,line.str());
}


/**
 * Called by hash() in hash.c when the hashing operation is complete.
 * Display the hash and perform any auditing steps.
 */ 
void display::display_hash(file_data_hasher_t *fdht,const hash_context_obj *hc)
{
    switch (primary_function) {
    case primary_compute: 
	display_hash_simple(fdht,hc);
	break;
    case primary_match: 
    case primary_match_neg: 
	display_match_result(fdht,hc);
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
	dfxml->push("dfxml",
		       "\n  xmlns='http://www.forensicswiki.org/wiki/Category:Digital_Forensics_XML' "
		       "\n  xmlns:deep='http://md5deep.sourceforge.net/md5deep/' "
		       "\n  xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance' "
		       "\n  xmlns:dc='http://purl.org/dc/elements/1.1/' "
		       "version='1.0'");
	dfxml->push("deep:configuration");
	dfxml->push("algorithms");
	for(int i=0;i<NUM_ALGORITHMS;i++){
	    dfxml->make_indent();
	    dfxml->printf("<algorithm name='%s' enabled='%d'/>\n",
			   hashes[i].name.c_str(),hashes[i].inuse);
	}
	dfxml->pop();			// algorithms
	dfxml->pop();			// configuration
	dfxml->push("metadata", "");
	dfxml->xmlout("dc:type","Hash List","",false);
	dfxml->pop();
	dfxml->add_DFXML_creator(PACKAGE_NAME,PACKAGE_VERSION,XML::make_command_line(argc,argv));
	unlock();
    }
}

void display::dfxml_timeout(const std::string &tag,const timestamp_t &val)
{
    char buf[256];
    struct tm tm;
    strftime(buf,sizeof(buf),"%Y-%m-%dT%H:%M:%SZ",portable_gmtime(&tm,&val));
    dfxml->xmlout(tag,buf);
}

void display::dfxml_write(file_data_hasher_t *fdht)
{
    if(dfxml){
	std::string attrs;
	if(opt_verbose && fdht->workerid>=0){
	    std::stringstream ss;
	    ss << "workerid='" << fdht->workerid << "'";
	    attrs = ss.str();
	}
	lock();
	dfxml->push("fileobject",attrs);
	dfxml->xmlout("filename",fdht->file_name);
	dfxml->xmlout("filesize",(int64_t)fdht->stat_bytes);
	if(fdht->mtime) dfxml_timeout("mtime",fdht->mtime);
	if(fdht->ctime) dfxml_timeout("ctime",fdht->ctime);
	if(fdht->atime) dfxml_timeout("atime",fdht->atime);
	dfxml->writexml(fdht->dfxml_hash.str());
	dfxml->pop();
	unlock();
    }
}

void display::dfxml_shutdown()
{
    if(dfxml){
	lock();
	dfxml->add_rusage();
	dfxml->pop();		// outermost
	dfxml->close();
	delete dfxml;
	dfxml = 0;
	unlock();
    }
}

