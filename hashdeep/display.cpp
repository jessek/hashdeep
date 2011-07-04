#include "main.h"
#include "utf8.h"

/**
 *
 * display.cpp:
 * Manages user output.
 * All output is in UTF-8.
 * If opt_escape8 is set, then non-ASCII UTF-8 characters are turned into U+XXXX notation.
 * 
 * NOTE WITH MINGW GCC-4.3.0:
 * You will get a warning from the format. IGNORE IT.
 * See http://lists.gnu.org/archive/html/qemu-devel/2009-01/msg01979.html
 */

static void display_size(const state *s,const file_data_t *fdt)
{
  if (s->mode & mode_display_size)  {
      // Under CSV mode we have to include a comma, otherwise two spaces
      if (s->mode & mode_csv){
	  printf ("%"PRIu64",", fdt->actual_bytes);
      }
      else {
	  printf ("%10"PRIu64"  ", fdt->actual_bytes);
      }
  }
}


static std::string shorten_filename(const std::string &fn)
{
    if (fn.size() < MAX_FILENAME_LENGTH){
	return fn;
    }
    return(fn.substr(0,MAX_FILENAME_LENGTH-3) + "...");
}

/**
 * output the string, typically a fn, optionally performing unicode escaping
 */

void output_filename(FILE *out,const char *fn)
{
    fwrite(fn,strlen(fn),1,out);
}

void output_filename(FILE *out,const std::string &fn)
{
    fwrite(fn.c_str(),fn.size(),1,out);
}

/* By default, we display in UTF-8.
 * We escape UTF-8 if requested.
 */
void display_filename(FILE *out, const file_data_t &fdt, bool shorten)
{
#if 0
    /* old windows code */
  size_t pos,len;

  len = _tcslen(fn);

  for (pos = 0 ; pos < len ; ++pos)
  {
    // We can only display the English (00) code page
    if (0 == (fn[pos] & 0xff00))
      fprintf (out,"%c", (char)(fn[pos]));
    else
      fprintf (out,"?");
  }
  
#else
  if(shorten){
      output_filename(out,shorten_filename(fdt.file_name));
  } else {
      output_filename(out,fdt.file_name);
  }
  if(fdt.file_name_annotation.size()>0){
      output_filename(out,fdt.file_name_annotation);
  }
#endif
}


void state::display_banner()
{
    tstring cwd = main::getcwd();
    print_status("%s", HASHDEEP_HEADER_10);

    fprintf (stdout,"%ssize,",HASHDEEP_PREFIX);  
    for (int i = 0 ; i < NUM_ALGORITHMS ; ++i) {
	if (hashes[i].inuse){
	    printf ("%s,", hashes[i].name.c_str());
	}
    }  
    print_status("filename");
    
    fprintf(stdout,"## Invoked from: ");
    output_filename(stdout,cwd);
    fprintf(stdout,"%s",NEWLINE);
  
    // Display the command prompt as we think the user saw it
    fprintf(stdout,"## ");
#ifdef _WIN32
    fprintf(stdout,"%c:\\>", (char)cwd[0]);
#else
    if (geteuid()==0){
	fprintf(stdout,"#");
    }
    else {
	fprintf(stdout,"$");
    }
#endif

  // Accounts for '## ', command prompt, and space before first argument
  size_t bytes_written = 8;

  for (int argc = 0 ; argc < this->argc ; ++argc) {
      fprintf(stdout," ");
      bytes_written++;

      // We are going to print the string. It's either ASCII or UTF16
      // convert it to a tstring and then to UTF8 string.
      tstring arg_t = tstring(this->argv[argc]);
      std::string arg_utf8 = main::make_utf8(arg_t);
      size_t current_bytes = arg_utf8.size();

      // The extra 32 bytes is a fudge factor
      if (current_bytes + bytes_written + 32 > MAX_STRING_LENGTH) {
	  fprintf(stdout,"%s## ", NEWLINE);
	  bytes_written = 3;
      }

      output_filename(stdout,arg_utf8);
      bytes_written += current_bytes;
  }
  fprintf(stdout,"%s## %s",NEWLINE, NEWLINE);
}


static void compute_dfxml(file_data_hasher_t *fdht,bool known_hash)
{
    if(fdht->piecewise){
	uint64_t bytes = fdht->read_end - fdht->read_start;
	fdht->dfxml_hash +=
	    std::string("<byte_run file_offset='")
	    + itos(fdht->read_start)
	    + std::string("' bytes='")
	    + itos(bytes) + std::string("'>\n   ");
    }
    for(int i=0;i<NUM_ALGORITHMS;i++){
	if(hashes[i].inuse){
	    fdht->dfxml_hash += "<hashdigest type='";
	    fdht->dfxml_hash += makeupper(fdht->hash_hex[i]);
	    fdht->dfxml_hash += std::string("'>") + fdht->hash_hex[i] + std::string("</hashdigest>\n");
	}
    }
    if(known_hash){
	fdht->dfxml_hash += std::string("<matched>1</matched>");
    }
    if(fdht->piecewise){
	fdht->dfxml_hash += "</byte_run>\n";
    }
}

/*
 * Externally called to display a simple hash
 */
int state::display_hash_simple(file_data_hasher_t *fdht)
{
    if ( this->dfxml==0 && this->banner_displayed==0){
	display_banner();
	this->banner_displayed = 1;
    }

    if(this->dfxml){
	compute_dfxml(fdht,this->mode & mode_which);
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
     
    if (fdht->piecewise){
	printf ("%"PRIu64",", fdht->bytes_read);
    }
    else {
	printf ("%"PRIu64",", fdht->actual_bytes);
    }

    for (int i = 0 ; i < NUM_ALGORITHMS ; ++i)  {
	if (hashes[i].inuse){
	    printf("%s,", fdht->hash_hex[i].c_str());
	}
    }
    display_filename(stdout,fdht,false);
    fprintf(stdout,"%s",NEWLINE);
    return FALSE;
}

int state::display_audit_results()
{
    int status = EXIT_SUCCESS;
    
    if (audit_check()==0) {
	print_status("%s: Audit failed", __progname);
	status = EXIT_FAILURE;
    }
    else {
	print_status("%s: Audit passed", __progname);
    }
  
    if (opt_verbose)    {
	if(opt_verbose >= MORE_VERBOSE){
	    print_status("   Input files examined: %"PRIu64, this->match.total);
	    print_status("  Known files expecting: %"PRIu64, this->match.expect);
	    print_status(" ");
	}
	print_status("          Files matched: %"PRIu64, this->match.exact);
	print_status("Files partially matched: %"PRIu64, this->match.partial);
	print_status("            Files moved: %"PRIu64, this->match.moved);
	print_status("        New files found: %"PRIu64, this->match.unknown);
	print_status("  Known files not found: %"PRIu64, this->match.unused);
    }
    return status;
}


/* The old display_match_result from md5deep */
int state::md5deep_display_match_result(file_data_hasher_t *fdht)
{  
    file_data_t *fs = known.find_hash(md5deep_mode_algorithm,
				      fdht->hash_hex[md5deep_mode_algorithm],
				      fdht->file_number);
    int known_hash = fs ? 1 : 0;

    if ((known_hash && (mode & mode_match)) ||
	(!known_hash && (mode & mode_match_neg))) {
	if(dfxml){
	    compute_dfxml(fdht,(this->mode & mode_which) || known_hash);
	    return FALSE;
	}

	display_size(this,fdht);

	if (mode & mode_display_hash) {
	    printf ("%s", fdht->hash_hex[md5deep_mode_algorithm].c_str());
	    if (mode & mode_csv) {
		printf (",");
	    } else if (mode & mode_asterisk) {
		printf(" *");
	    } else {
		printf("  ");
	    }
	}

	if (mode & mode_which) 	    {
		if (known_hash && (mode & mode_match)) {
			display_filename(stdout,fdht,false);
			printf (" matched ");
			output_filename(stdout,fs->file_name);
		    }
		else {
			display_filename(stdout,fdht,false);
			printf (" does NOT match");
		    }
	}
	else{
	    display_filename(stdout,fdht,false);
	}
	print_newline();
    }
    return FALSE;
}

/* The original display_match_result from md5deep.
 * This should probably be merged with the function above.
 * This function is very similar to audit_update(), which follows
 */
status_t state::display_match_result(file_data_hasher_t *fdht)
{
    file_data_t *matched_fdt = NULL;
    int should_display; 
    should_display = (primary_match_neg == primary_function);
    
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
	    if (mode & mode_which && primary_match == primary_function) {
		fprintf(stdout," matches ");
		if (NULL == matched_fdt) {
		    fprintf(stdout,"(unknown file)");
		} else {
		    display_filename(stdout,matched_fdt,false);
		}
	    }
	    print_status("");
	}
    }
    return status_ok;
}


/**
 * Called after every file is hashed by display_hash
 * when s->primary_function==primary_audit
 * Records every file seen in the 'seen' structure, referencing the 'known' structure.
 */

int state::audit_update(file_data_hasher_t *fdht)
{
    file_data_t *matched_fdht;
    hashlist::searchstatus_t m = known.search(fdht,&matched_fdht);
    switch(m){
    case hashlist::status_match:
    case hashlist::searchstatus_ok:
	this->match.exact++;
	if (opt_verbose >= INSANELY_VERBOSE) {
	    display_filename(stdout,fdht,false);
	    print_status(": Ok");
	}
	break;
    case hashlist::status_no_match:
	this->match.unknown++;
	if (opt_verbose >= MORE_VERBOSE) {
	    display_filename(stdout,fdht,false);
	    print_status(": No match");
	}
	break;
    case hashlist::status_file_name_mismatch:
	this->match.moved++;
	if (opt_verbose >= MORE_VERBOSE) {
	    display_filename(stdout,fdht,false);
	    fprintf(stdout,": Moved from ");
	    display_filename(stdout,matched_fdht,false);
	    print_status("");
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
	print_status("");
    }
    return FALSE;
}



/**
 * perform an audit
 */
 

int state::audit_check()
{
    /* Count the number of unused */
    this->match.unused = this->known.compute_unused(false,": Known file not used");
    return (0 == this->match.unused  && 
	    0 == this->match.unknown && 
	    0 == this->match.moved);
}





/* The old display_hash from the md5deep program, with a few modifications */
int state::md5deep_display_hash(file_data_hasher_t *fdht)
{
    if (mode & mode_triage) {
	if(dfxml){
	    compute_dfxml(fdht,1);
	    return FALSE;
	}
	printf ("\t%s\t", fdht->hash_hex[this->md5deep_mode_algorithm].c_str());
	display_filename(stdout,fdht,false);
	print_newline();
	return FALSE;
    }

    // We can't call display_size here because we don't know if we're
    // going to display *anything* yet. If we're in matching mode, we
    // have to evaluate if there was a match first. 
    if ((this->mode & mode_match) || (this->mode & mode_match_neg)){
	return md5deep_display_match_result(fdht);
    }
    
    if(this->dfxml){
	compute_dfxml(fdht,this->mode & mode_which);
	return FALSE;
    }

    display_size(this,fdht);

    printf ("%s", fdht->hash_hex[this->md5deep_mode_algorithm].c_str());

    if (this->mode & mode_quiet)
	printf ("  ");
    else  {
	if ((fdht->piecewise) || !(fdht->is_stdin))    {
	    if (this->mode & mode_timestamp)      {
		struct tm my_time;

#ifdef _WIN32
# ifdef HAVE__GMTIME64_S
		_gmtime64_s(&fdht->timestamp,&my_time);
# else
		my_time = *_gmtime64(&fdht->timestamp);
# endif
#else
		gmtime_r(&fdht->timestamp,&my_time);
#endif

		char time_str[MAX_TIME_STRING_LENGTH];
		
		// The format is four digit year, two digit month, 
		// two digit hour, two digit minute, two digit second
		strftime(time_str, sizeof(time_str), "%Y:%m:%d:%H:%M:%S", &my_time);
		
		printf ("%c%s", (this->mode & mode_csv?',':' '), time_str);
	    }
	    if (mode & mode_csv) {
		printf(",");
	    } else if (mode & mode_asterisk) {
		printf(" *");
	    } else {
		printf("  ");
	    }
	    display_filename(stdout,fdht,false);
	}
    }
    print_newline();
    return FALSE;
}

/**
 * Called by hash() in hash.c when the hashing operation is complete.
 * Display the hash and perform any auditing steps.
 */ 
int state::display_hash(file_data_hasher_t *fdht)
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
