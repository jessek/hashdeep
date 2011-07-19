#include "main.h"
#include "utf8.h"
#include <stdarg.h>

/**
 *
 * display.cpp:
 * Manages user output.
 *
 * All output is in UTF-8.
 *
 * If opt_escape8 is set, then non-ASCII UTF-8 characters are turned into U+XXXX notation.
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
static std::string shorten_filename(const std::string &fn)
{
    if (fn.size() < MAX_FILENAME_LENGTH){
	return fn;
    }
    return(fn.substr(0,MAX_FILENAME_LENGTH-3) + "...");
}


/* Does not lock */
void display::newline()
{
    if (opt_zero){
	printf("%c", 0);
    }
    else {
	printf("%s", NEWLINE);
    }
    fflush(stdout);
}


/****************************************************************
 ** Display Routines
 ****************************************************************/

void display::open(const std::string &fn)
{
    outfile = fopen(fn.c_str(),"wb");
    if(!outfile){
	perror(fn.c_str());
	exit(1);
    }
}

void display::status(const char *fmt,...)
{
    lock();
    va_list(ap); 
    va_start(ap,fmt); 
    if (vfprintf(outfile,fmt,ap) < 0) { 
	fprintf(stderr, "%s: %s", __progname, strerror(errno)); 
	exit(EXIT_FAILURE);
    }
    va_end(ap);
    fprintf(outfile,"%s", NEWLINE);
    unlock();
}

void display::error(const char *fmt,...)
{
    lock();
    va_list(ap); 
    va_start(ap,fmt); 
    if (vfprintf(stderr,fmt,ap) < 0) { 
	fprintf(stderr, "%s: %s", __progname, strerror(errno)); 
	exit(EXIT_FAILURE);
    }
    va_end(ap);
    fprintf(stderr,"%s", NEWLINE);
    unlock();
}

/**
 * output the string, typically a fn, optionally performing unicode escaping
 */

void display::output_filename(const std::string &fn)
{
    if(opt_unicode_escape){
	std::string f2 = main::escape_utf8(fn);
	fwrite(f2.c_str(),f2.size(),1,outfile);
    } else {
	fwrite(fn.c_str(),fn.size(),1,outfile);
    }
}

#ifdef _WIN32
/* NOTE - This is where to do the UTF-8 or U+ substitution */
void output_filename(const std::wstring &fn)
{
    output_filename(main::make_utf8(fn));
}
#endif

/* By default, we display in UTF-8.
 * We escape UTF-8 if requested.
 */
void display::display_filename(const file_data_t &fdt, bool shorten)
{
    if(shorten){
	output_filename(shorten_filename(fdt.file_name));
    } else {
	output_filename(fdt.file_name);
    }
    if(fdt.file_name_annotation.size()>0){
	output_filename(fdt.file_name_annotation);
    }
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
	char msg[64];
	snprintf(msg,sizeof(msg),"%"PRIu64"MB of %"PRIu64"MB done, %02"PRIu64":%02"PRIu64":%02"PRIu64" left%s",
		 mb_read, fdht->stat_megs(), hour, min, seconds, BLANK_LINE);
    }

    lock();
    fprintf(stderr,"\r");
    display_filename(*fdht,shorten);
    output_filename(msg);	// was previously put in the anotation
    fflush(stderr);
    unlock();
}


void display::display_banner_if_needed(const std::string &utf8_banner)
{
    if(this->dfxml!=0) return;		// output is in DFXML; no banner
    lock();
    if(banner_displayed==false){
	fprintf(outfile,"%s",utf8_banner.c_str());
	banner_displayed=true;
    }
    unlock();
}


/* No locks, so do not call on the same FDHT in different threads */
void file_data_hasher_t::compute_dfxml(bool known_hash)
{
    if(this->piecewise){
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
    if(this->piecewise){
	this->dfxml_hash += "</byte_run>\n";
    }
}

/*
 * Externally called to display a simple hash
 */
int display::display_hash_simple(file_data_hasher_t *fdht)
{
    if(this->dfxml){
	fdht->compute_dfxml(opt_show_matched);
	return FALSE;
    }

    /* In piecewise mode the size of each 'file' is the size
     * of the block it came from. This is important when doing an
     * audit in piecewise mode. In all other cases we use the 
     * total number of bytes from the file we *actually* read
     *
     * NOTE: Ignore the warning in the format when running on mingw with GCC-4.3.0
     * see http://lists.gnu.org/archive/html/qemu-devel/2009-01/msg01979.html
     */
     
    display_banner_if_needed(s->utf8_banner);
    lock();
    if (fdht->piecewise){
	fprintf(outfile,"%"PRIu64",", fdht->bytes_read);
    }
    else {
	fprintf(outfile,"%"PRIu64",", fdht->actual_bytes);
    }

    for (int i = 0 ; i < NUM_ALGORITHMS ; ++i)  {
	if (hashes[i].inuse){
	    fprintf(outfile,"%s,", fdht->hash_hex[i].c_str());
	}
    }
    display_filename(*fdht,false);
    fprintf(outfile,"%s",NEWLINE);
    unlock();
    return FALSE;
}

int display::display_audit_results()
{
    int ret = EXIT_SUCCESS;
    
    lock();
    if (audit_check()==0) {
	status("%s: Audit failed", __progname);
	ret = EXIT_FAILURE;
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
    unlock();
    return ret;
}


/* This doesn't lock/unlock because we should already be locked or unlocked */
void display::display_size(const state *s,const file_data_t *fdt)
{
    if (s->mode & mode_display_size)  {
	// Under CSV mode we have to include a comma, otherwise two spaces
	if (opt_csv){
	    fprintf(outfile,"%"PRIu64",", fdt->actual_bytes);
	}
	else {
	    fprintf(outfile,"%10"PRIu64"  ", fdt->actual_bytes);
	}
    }
}


/* The old display_match_result from md5deep */
int display::md5deep_display_match_result(file_data_hasher_t *fdht)
{  
    file_data_t *fs = known.find_hash(md5deep_mode_algorithm,
				      fdht->hash_hex[md5deep_mode_algorithm],
				      fdht->file_number);
    int known_hash = fs ? 1 : 0;

    if ((known_hash && (mode & mode_match)) ||
	(!known_hash && (mode & mode_match_neg))) {
	if(dfxml){
	    compute_dfxml(fdht,(opt_show_matched || known_hash);
	    return FALSE;
	}
	{
	    lock();
	    
	    display_size(this,fdht);
	    if (mode & mode_display_hash) {
		fprintf(outfile,"%s", fdht->hash_hex[md5deep_mode_algorithm].c_str());
		if (opt_csv) {
		    fprintf(outfile,",");
		} else if (mode & mode_asterisk) {
		    fprintf(outfile," *");
		} else {
		    fprintf(outfile,"  ");
		}
	    }
	    
	    if (opt_show_matched) 	    {
		if (known_hash && (mode & mode_match)) {
		    display_filename(outfile,fdht,false);
		    fprintf(outfile," matched ");
		    output_filename(outfile,fs->file_name);
		}
		else {
		    display_filename(outfile,fdht,false);
		    fprintf(outfile," does NOT match");
		}
	    }
	    else{
		display_filename(stdout,fdht,false);
	    }
	    newline();
	    unlock();
	}
    }
    return FALSE;
}

/* The original display_match_result from md5deep.
 * This should probably be merged with the function above.
 * This function is very similar to audit_update(), which follows
 */
status_t display::display_match_result(file_data_hasher_t *fdht)
{
    file_data_t *matched_fdt = NULL;
    int should_display; 
    should_display = (primary_match_neg == primary_function);
    
    lock();				// protects the search and the printing

    hashlist::searchstatus_t m = known.search(fdht,&matched_fdt);
    switch(m){
	// If only the name is different, it's still really a match
	//  as far as we're concerned. 
    case hashlist::status_file_name_mismatch:
    case hashlist::status_match:
	should_display = (primary_match_neg != primary_function);
	break;
	  
    case hashlist::status_file_size_mismatch:
	display_filename(stderr,fdht,false);
	fprintf(stderr,": Hash collision with ");
	display_filename(stderr,matched_fdt,false);
	fprintf(stderr,"%s", NEWLINE);
	break;
	
    case hashlist::status_partial_match:
	display_filename(stderr,fdht,false);
	fprintf(stderr,": partial hash match with ");
	display_filename(stderr,matched_fdt,false);
	fprintf(stderr,"%s", NEWLINE);
	break;
	
    default:
	break;
    }
    if (should_display) {
	if (mode & mode_display_hash)
	    display_hash_simple(fdht);
	else {
	    display_filename(stdout,fdht,false);
	    if (opt_show_matched && primary_match == primary_function) {
		fprintf(stdout," matches ");
		if (NULL == matched_fdt) {
		    fprintf(stdout,"(unknown file)");
		} else {
		    display_filename(stdout,matched_fdt,false);
		}
	    }
	    status("");			// generates a newline
	}
    }
    unlock();
    return status_ok;
}


/**
 * Called after every file is hashed by display_hash
 * when s->primary_function==primary_audit
 * Records every file seen in the 'seen' structure, referencing the 'known' structure.
 */

int display::audit_update(file_data_hasher_t *fdht)
{
    lock();				// protects the search and the printing
    file_data_t *matched_fdht=0;
    hashlist::searchstatus_t m = known.search(fdht,&matched_fdht);
    switch(m){
    case hashlist::status_match:
    case hashlist::searchstatus_ok:
	this->match.exact++;
	if (opt_verbose >= INSANELY_VERBOSE) {
	    display_filename(stdout,fdht,false);
	    status(": Ok");
	}
	break;
    case hashlist::status_no_match:
	this->match.unknown++;
	if (opt_verbose >= MORE_VERBOSE) {
	    display_filename(stdout,fdht,false);
	    status(": No match");
	}
	break;
    case hashlist::status_file_name_mismatch:
	this->match.moved++;
	if (opt_verbose >= MORE_VERBOSE) {
	    display_filename(stdout,fdht,false);
	    fprintf(stdout,": Moved from ");
	    display_filename(stdout,matched_fdht,false);
	    status("");
	}
	break;
    case hashlist::status_partial_match:
    case hashlist::status_file_size_mismatch:
	this->match.partial++;
	// We only record the hash collision if it wasn't anything else.
	// At the same time, however, a collision is such a significant
	// event that we print it no matter what. 
	display_filename(stdout,fdht,false);
	fprintf(stdout,": Hash collision with ");
	display_filename(stdout,matched_fdht,false);
	status("");
    }
    unlock();
    return FALSE;
}



/**
 * perform an audit
 */
 

int display::audit_check()
{
    /* Count the number of unused */
    lock();
    this->match.unused = this->compute_unused(false,": Known file not used");
    unlock();
    return (0 == this->match.unused  && 
	    0 == this->match.unknown && 
	    0 == this->match.moved);
}





/* The old display_hash from the md5deep program, with a few modifications */
int display::md5deep_display_hash(file_data_hasher_t *fdht)
{
    if (mode & mode_triage) {
	if(dfxml){
	    compute_dfxml(fdht,1);	// no lock required here
	    return FALSE;
	}
	lock();
	fprintf(outfile,"\t%s\t", fdht->hash_hex[this->md5deep_mode_algorithm].c_str());
	display_filename(stdout,fdht,false);
	newline();
	unlock();
	return FALSE;
    }

    // We can't call display_size here because we don't know if we're
    // going to display *anything* yet. If we're in matching mode, we
    // have to evaluate if there was a match first. 
    if ((this->mode & mode_match) || (this->mode & mode_match_neg)){
	return md5deep_display_match_result(fdht);
    }
    
    if(this->dfxml){
	compute_dfxml(fdht,opt_show_matched);
	return FALSE;
    }

    lock();
    display_size(this,fdht);
    fprintf(outfile,"%s", fdht->hash_hex[this->md5deep_mode_algorithm].c_str());

    if (this->mode & mode_quiet){
	fprintf(outfile,"  ");
    }
    else  {
	if ((fdht->piecewise) || !(fdht->is_stdin))    {
	    if (this->mode & mode_timestamp)      {
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
		
		fprintf(outfile,"%c%s", (opt_csv ?',':' '), time_str);
	    }
	    if (opt_csv) {
		fprintf(outfile,",");
	    } else if (mode & mode_asterisk) {
		fprintf(outfile," *");
	    } else {
		fprintf(outfile,"  ");
	    }
	    display_filename(stdout,fdht,false);
	}
    }
    newline();
    unlock();
    return FALSE;
}

/**
 * Return the number of entries in the hashlist that have used==0
 * Optionally display them, optionally with additional output.
 */
uint64_t display::compute_unused(bool display, std::string annotation)
{
    lock();
    uint64_t count=0;
    for(hashlist::const_iterator i = known->begin(); i != known->end(); i++){
	if((*i)->matched_file_number==0){
	    count++;
	    if (display || opt_verbose >= MORE_VERBOSE) {
		display_filename(*i,false);
		print_status(annotation.c_str());
	    }
	}
    }
    unlock();
    return count;
}



/**
 * Called by hash() in hash.c when the hashing operation is complete.
 * Display the hash and perform any auditing steps.
 */ 
int display::display_hash(file_data_hasher_t *fdht)
{
    if(md5deep_mode){
	md5deep_display_hash(fdht);
	return TRUE;
    }

    switch (primary_function) {
    case primary_compute: 
	return display_hash_simple(fdht);
    case primary_match: 
    case primary_match_neg: 
	return display_match_result(fdht);
    case primary_audit: 
	return audit_update(fdht);
    }
    return FALSE;
}



void display::dfxml_setup()
{
    lock();
    if(dfxml){
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
    }
    unlock();
}

void display::dfxml_shutdown()
{
    lock();
    if(s->dfxml){
	s->dfxml->pop();		// outermost
	s->dfxml->close();
	delete s->dfxml;
	s->dfxml = 0;
    }
    unlock();
}
