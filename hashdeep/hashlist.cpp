// $Id$ 

/** hashlist.cpp
 * Contains the logic for performing the audit.
 * Also has implementation of the hashlist, searching, etc.
 * Formerly this code was in audit.cpp and match.cpp.
 */

#include "main.h"
#include <new>

/**
 * Return the number of entries in the hashlist that have used==0
 */
uint64_t hashlist::count_unused()
{
    uint64_t count=0;
    for(std::vector<file_data_t *>::const_iterator i = this->begin(); i != this->end(); i++){
	if((*i)->used==0){
	    count++;
	    if (s->mode & mode_more_verbose) {
		display_filename(stdout,*i,false);
		print_status(": Known file not used");
	    }
	}
    }
    return count;
}

/**
 * Adds a file_data_t pointer to the hashlist.
 * Does not copy the object; that must be done elsewhere.
 */
void hashlist::add_fdt(file_data_t *fi){ 
    fi->retain();
    push_back(fi);
    for(int i=0;i<NUM_ALGORITHMS;i++){
	if(s->hashes[i].inuse){
	    hashmaps[i].add_file(fi,i);
	}
    };
};


/**
 * Search for the provided fdt in the hashlist and return the status of the match.
 */
hashlist::searchstatus_t hashlist::search(const file_data_t *fdt) const
{
    bool file_size_mismatch = false;
    bool file_name_mismatch = false;
  
    /* Iterate through each of the hashes in the haslist until we find a match.
     */
    for (int i = 0 ; i < NUM_ALGORITHMS ; ++i)  {
	/* Only search hash functions that are in use and hashes that are in the fdt */
	if (s->hashes[i].inuse && fdt->hash_hex[i].size()){
	    hashmap::const_iterator it = hashmaps[i].find(fdt->hash_hex[i]);
	    if(it != hashmaps[i].end()){
		/* found a match*/

		const file_data_t *match = it->second;

		/* Verify that all of the other hash functions for *it match fdt as well. */
		for(int j=0;j<NUM_ALGORITHMS;j++){
		    if(s->hashes[j].inuse && j!=i && fdt->hash_hex[i].size() && match->hash_hex[i].size()){
			if(fdt->hash_hex[j] != match->hash_hex[j]){
			    /* Amazing. We found a match on one hash a a non-match on another.
			     * Call the newspapers! This is a newsorthy event.
			     */
			    return status_partial_match;
			}
		    }
		}
		/* If we got here we matched on all of the hashes.
		 * Which is to be expected.
		 * Check to see if the sizes are the same.
		 */
		if(fdt->file_size != match->file_size){
		    /* Amazing. We found two files that have the same hash but different
		     * file sizes. This has never happened before in the history of the world.
		     * Call the newspapers!
		     */
		    file_size_mismatch = true;
		}
		/* See if the hashes are the same but the name changed.
		 */
		if(fdt->file_name != match->file_name){
		    file_name_mismatch = true;
		}
	    }
	}
    }
    /* If we get here, then all of the hash matches for all of the algorithms have been
     * checked and found to be equal if present.
     */
    if(file_size_mismatch) return status_file_size_mismatch;
    if(file_name_mismatch) return status_file_name_mismatch;
    return status_match;
}


/**
 * perform an audit
 */
 

int audit_check(state *s)
{
    /* Count the number of unused */
    s->match.unused = s->known.count_unused();
    return (0 == s->match.unused  && 
	    0 == s->match.unknown && 
	    0 == s->match.moved);
}


int display_audit_results(state *s)
{
    int status = EXIT_SUCCESS;
    
    if (audit_check(s)==0) {
	print_status("%s: Audit failed", __progname);
	status = EXIT_FAILURE;
    }
    else {
	print_status("%s: Audit passed", __progname);
    }
  
    if (s->mode & mode_verbose)    {
	/*
	  print_status("   Input files examined: %"PRIu64, s->match_total);
	  print_status("  Known files expecting: %"PRIu64, s->match_expect);
	  print_status(" ");
	*/
	print_status("          Files matched: %"PRIu64, s->match.exact);
	print_status("Files partially matched: %"PRIu64, s->match.partial);
	print_status("            Files moved: %"PRIu64, s->match.moved);
	print_status("        New files found: %"PRIu64, s->match.unknown);
	print_status("  Known files not found: %"PRIu64, s->match.unused);
    }
    return status;
}


/**
 * Called after every file is hashed by display_hash
 * when s->primary_function==primary_audit
 * Records every file seen in the 'seen' structure, referencing the 'known' structure.
 */

int audit_update(state *s,file_data_t *fdt)
{
#if 0
    bool no_match = false;
    bool exact_match = false;
    bool moved = false;
    bool partial = false;
    file_data_t * moved_file = NULL, * partial_file = NULL;
    uint64_t my_round;			// don't know what the round is
  
    my_round = s->hash_round;		// count number of files
    s->hash_round++;
    if (my_round > s->hash_round){	// check for 64-bit overflow (highly unlikely)
	fatal_error("%s: Too many input files", __progname);
    }

    // Although nobody uses match_total right now, we may in the future 
    //  s->match_total++;

    // Check all of the algorithms in use
    for (int i = 0 ; i < NUM_ALGORITHMS; i++) {
	if (s->hashes[i].inuse) {
	    hashlist::hashmap::const_iterator match  = s->known.hashmaps[i].find(fdt->hash_hex[i]);
	    if(match==s->known.hashmaps[i].end()){
		no_match = TRUE;
	    }
	    else {
		

	    hashtable_entry_t *matches = hashtable_contains(s,(hashid_t)i);
	    hashtable_entry_t *tmp = matches;
	    }
      if (NULL == tmp)
      {
	no_match = TRUE;
      }
      else while (tmp != NULL)
      {
	// print_status("Got status %d for %s", tmp->status, tmp->data->file_name);

	if (tmp->data->used != s->hash_round)
	{

	  switch (tmp->status) {
	  case status_match:
	    tmp->data->used = s->hash_round;
	    exact_match = TRUE;
	    break;
    
	    // This shouldn't happen 
	  case status_no_match:
	    no_match = TRUE;
	    print_error_unicode(s->current_file->file_name,
				"Internal error: Got match for \"no match\"");
	    break;
    
	  case status_file_name_mismatch:
	    moved = TRUE;
	    moved_file = tmp->data;
	    break;
    
	    // Hash collision 
	  case status_partial_match:
	  case status_file_size_mismatch:
	    partial = TRUE;
	    partial_file = tmp->data;
	    break;
	    
	  case status_unknown_error:
	    return status_unknown_error;
	    
	  default:
	    break;
	  }
	}

	tmp = tmp->next;

      }
      hashtable_destroy(matches);
    }
    }

  // If there was an exact match, that overrides any other matching
  // 'moved' file. Usually this happens when the same file exists
  // in two places. In this case we find an exact match and a case
  // where the filenames don't match. 

  if (exact_match)
  {
    s->match_exact++;
    if (s->mode & mode_insanely_verbose)
    {
	display_filename(stdout,s->current_file,false);
      print_status(": Ok");
    }
  }

  else if (no_match)
  {
    s->match_unknown++;
    if (s->mode & mode_more_verbose)
    {
	display_filename(stdout,s->current_file,false);
      print_status(": No match");
    }
  } 

  else if (moved)
  {
    s->match_moved++;
    if (s->mode & mode_more_verbose)
    {
	display_filename(stdout,s->current_file,false);
	fprintf(stdout,": Moved from ");
	display_filename(stdout,moved_file,false);
	print_status("");
    }

  }
  
  // A file can have a hash collision even if it matches an otherwise
  // known good. For example, an evil version of ntoskrnl.exe should
  // have an exact match to EVILEVIL.EXE but still have a collision 
  // with the real ntoskrnl.exe. When this happens we should report
  // both results. 
  if (partial)  {
    // We only record the hash collision if it wasn't anything else.
    // At the same time, however, a collision is such a significant
    // event that we print it no matter what. 
    if (!exact_match && !moved && !no_match)
      s->match_partial++;
    display_filename(stdout,s->current_file,false);
    fprintf(stdout,": Hash collision with ");
    display_filename(stdout,partial_file,false);
    print_status("");
  }
  
#endif
  return FALSE;
}

static int match_valid_hash(state *s, hashid_t a, char *buf)
{
  size_t pos = 0;

  if (strlen(buf) != s->hashes[a].bit_length/4)
    return FALSE;
  
  for (pos = 0 ; pos < s->hashes[a].bit_length/4 ; pos++)
  {
    if (!isxdigit(buf[pos]))
      return FALSE;
  }
  return TRUE;
}
      
#define TEST_ALGORITHM(CONDITION,ALG)	  \
  if (CONDITION)			  \
    {					  \
      s->hashes[ALG].inuse = TRUE;	  \
      hash_order[order] = ALG;		  \
      buf += strlen(buf) + 1;		  \
      pos = 0;				  \
      ++total_pos;			  \
      ++order;				  \
      continue;				  \
    }

static int hashlist::parse_hashing_algorithms(state *s, char *fn, char *val)
{
  char * buf = val;
  size_t len = strlen(val);
  int done = FALSE;

  // The first position is always the file size, so we start with an 
  // the first position of one.
  uint8_t order = 1;

  size_t pos = 0, total_pos = 0;
  
  if (NULL == s || NULL == fn || NULL == val)
    return TRUE;

  while (!done)  {
    if ( ! (',' == buf[pos] || 0 == buf[pos]))    {
      // If we don't find a comma or the end of the line, 
      // we must continue to the next character
      ++pos;
      ++total_pos;
      continue;
    }

    /// Terminate the string so that we can do comparisons easily
    buf[pos] = 0;

    TEST_ALGORITHM(STRINGS_CASE_EQUAL(buf,"md5"),alg_md5);
    
    TEST_ALGORITHM(STRINGS_CASE_EQUAL(buf,"sha1") ||
		   STRINGS_CASE_EQUAL(buf,"sha-1"), alg_sha1);

    TEST_ALGORITHM(STRINGS_CASE_EQUAL(buf,"sha256") || 
		   STRINGS_CASE_EQUAL(buf,"sha-256"), alg_sha256);

    TEST_ALGORITHM(STRINGS_CASE_EQUAL(buf,"tiger"),alg_tiger);

    TEST_ALGORITHM(STRINGS_CASE_EQUAL(buf,"whirlpool"),alg_whirlpool);
      
    if (STRINGS_CASE_EQUAL(buf,"filename"))
    {
      // The filename column should be the end of the line
      if (total_pos != len)
      {
	print_error("%s: %s: Badly formatted file", __progname, fn);
	try_msg();
	exit(EXIT_FAILURE);
      }
      done = TRUE;
    }
      
    else
    {
      // If we can't parse the algorithm name, there's something
      // wrong with it. Don't tempt fate by trying to print it,
      // it could contain non-ASCII characters or something malicious.
      print_error(
		  "%s: %s: Unknown algorithm in file header, line 2.", 
		  __progname, fn);
      
      try_msg();
      exit(EXIT_FAILURE);
    }
  }

  s->expected_columns = order;
  s->hash_order[order] = alg_unknown;

  if (done)
    return FALSE;
  return TRUE;
}



/**
 * Returns the file type of a given input file.
 * fn is provided so that error messages can be printed.
 */
hashlist::filetype_t hashlist::identify_filetype(const char *fn,FILE *handle)
{
    hashid_t current_order[NUM_ALGORITHMS];
 
    char buf[MAX_STRING_LENGTH];

    // Find the header 
    if ((fgets(buf,MAX_STRING_LENGTH,handle)) == NULL) {
	return file_unknown;
    }

    chop_line(buf);

    if ( ! STRINGS_EQUAL(buf,HASHDEEP_HEADER_10)) {
	return file_unknown;
    }

    // Find which hashes are in this file
    if ((fgets(buf,MAX_STRING_LENGTH,handle)) == NULL) {
	return file_unknown;
    }

    chop_line(buf);

    // We don't use STRINGS_EQUAL here because we only care about
    // the first ten characters for right now. 
    if (strncasecmp("%%%% size,",buf,10))  {
	return file_unknown;
    }

    // If this is the first file of hashes being loaded, clear out the 
    // list of known values. Otherwise, record the current values to
    // let the user know if they have changed when we load the new file.
    if ( ! s->hashes_loaded )  {
	clear_algorithms_inuse(s);
    }
    else  {
	for (int i = 0 ; i < NUM_ALGORITHMS ; ++i)
	    current_order[i] = s->hash_order[i];
    }

    // We have to clear out the algorithm order to remove the values
    // from the previous file. This file may have different ones 
    for (int i = 0 ; i < NUM_ALGORITHMS ; ++i){
	s->hash_order[i] = alg_unknown;
    }
    
    // Skip the "%%%% size," when parsing the list of hashes 
    parse_hashing_algorithms(fn,buf + 10);
    
    if (s->hashes_loaded)  {
	int i = 0;
	while (i < NUM_ALGORITHMS && s->hash_order[i] == current_order[i])
	    i++;
	if (i < NUM_ALGORITHMS)
	    print_error("%s: %s: Hashes not in same format as previously loaded",
			__progname, fn);
    }
    return file_hashdeep_10;
}



#if 0
static status_t add_file(state *s, file_data_t *f)
{
  f->next = NULL;

  if (NULL == s->known)
    s->known = f;
  else
    s->last->next = f;

  s->last = f;

  int i = 1;
  while (s->hash_order[i] != alg_unknown)  {
      status_t st = hashtable_add(s,s->hash_order[i],f);
      if (st != status_ok) return st;
      ++i;
  }

  return status_ok;
}
#endif


#ifdef _DEBUG
/**
 * debug functions. These don't work anymore.
 */
static void display_file_data(state *s, file_data_t * t)
{
    int i;
    
    fprintf(stdout,"  Filename: ");
    display_filename(stdout,t,false);
    fprintf(stdout,"%s",NEWLINE);
    
    print_status("      Size: %"PRIu64, t->file_size);
    
    for (i = 0 ; i < NUM_ALGORITHMS ; ++i)
	print_status("%10s: %s", s->hashes[i]->name, t->hash[i]);
    print_status("");
}

static void display_all_known_files(state *s)
{
    if (NULL == s->known) {
	print_status("No known hashes");
	return;
    }
    
    file_data_t * tmp = s->known;
    while (tmp != NULL)	{
	display_file_data(s,tmp);
	tmp = tmp->next;
    }
}
#endif


/**
 * Read a hash set file.
 * Assumes file type has been identified (kind of a weird assumption)
 */

status_t read_hash_set_file(const char *fn)
{
    FILE *handle = fopen(fn,"rb");
    status_t st = status_ok;
    int contains_bad_lines = FALSE;
    int record_valid=0;

    // We start our counter at line number two for the two lines
    // of header we've already read
    uint64_t line_number = 2;

    char line[MAX_STRING_LENGTH];	// holds the line we are reading

    while (fgets(line,MAX_STRING_LENGTH,handle)) {
	line_number++;			// count where we are

	// Lines starting with a pound sign are comments and can be ignored
	if ('#' == line[0]){
	    continue;
	}

	// We're going to be advancing the string variable, so we
	// make sure to use a temporary pointer. If not, we'll end up
	// overwriting random data the next time we read.
	char *buf = line;
	record_valid = TRUE;
	chop_line(buf);

	// C++ typically fails with a bad_alloc, but you can make it return null
	// http://www.cplusplus.com/reference/std/new/bad_alloc/
	// http://www.cplusplus.com/reference/std/new/nothrow/
	file_data_t *t = new (std::nothrow) file_data_t(); // C++ new fails with a bad_a
	if (NULL == t){
	    fatal_error("%s: %s: Out of memory in line %"PRIu64, 
			__progname, fn, line_number);
	}

	int done = FALSE;
	size_t pos = 0;
	uint8_t column_number = 0;

	/* Process possibly multiple hashes on the line */
	while (!done) {
	    // scan past any comma 
	    if ( ! (',' == buf[pos] || 0 == buf[pos])) {
		++pos;
		continue;
	    }

	    // Terminate the string so that we can do comparisons easily
	    buf[pos] = 0;

	    // The first column should always be the file size
	    if (0 == column_number) {
		t->file_size = (uint64_t)strtoll(buf,NULL,10);
		buf += strlen(buf) + 1;
		pos = 0;
		column_number++;
		continue;
	    }

	    // All other columns should contain a valid hash in hex
	    if ( ! match_valid_hash(s,s->hash_order[column_number],buf)) {
		print_error(
			    "%s: %s: Invalid %s hash in line %"PRIu64,
			    __progname, fn, 
			    s->hashes[s->hash_order[column_number]].name.c_str(),
			    line_number);
		contains_bad_lines = TRUE;
		record_valid = FALSE;
		// Break out (done = true) and then process the next line
		break;
	    }

	    // Convert the hash to a std::string and save it
	    t->hash_hex[s->hash_order[column_number]] = std::string(buf);

	    ++column_number;
	    buf += strlen(buf) + 1;
	    pos = 0;

	    // The 'last' column (even if there are more commas in the line)
	    // is the filename. Note that valid filenames can contain commas! 
	    if (column_number == s->expected_columns) {
		t->file_name = buf;
		done = TRUE;
	    }
	}

	if ( ! record_valid) {
	    continue;
	}

	/* add the file to the known files hashlist */
	s->known.add_file(t);
    }
    if (contains_bad_lines){
	return status_contains_bad_hashes;
    }
    
    return st;
}


/** Loads a file of known hashes
 */
 hashlist::loadstatus_t hashlist::load_hash_file(char *fn)
{
    loadstatus_t status = status_ok;
    filetype_t type;

    FILE *handle = fopen(fn,"rb");
    if (NULL == handle) {
	print_error("%s: %s: %s", __progname, fn, strerror(errno));
	return status_file_error;
    }
  
    type = identify_filetype(fn,handle);
    if (file_unknown == type)  {
	print_error("%s: %s: Unable to identify file format", __progname, fn);
	fclose(handle);
	return status_unknown_filetype;
    }
    status = read_hash_set_file(fn,handle);
    fclose(handle);
    return status;
}


 /**
  * We don't use this function anymore, but it's handy to have just in case
  */
 const char *hashlsit::searchstatus_to_str(status_t val)
{
    switch (val) {
    case searchstatus_ok: return "ok";
    case status_match: return "complete match";
    case status_partial_match: return "partial match";
    case status_file_size_mismatch: return "file size mismatch";
    case status_file_name_mismatch: return "file name mismatch";
    case status_no_match: return "no match"; 
	
    default:
	return "unknown";
    }      
}



status_t display_match_result(state *s,file_data_hasher_t *fdht)
{
#if 0
    file_data_t *matched_fdt = NULL;
    int should_display; 
    uint64_t my_round;
    
    my_round = s->hash_round;
    s->hash_round++;
    if (my_round > s->hash_round)
	fatal_error("%s: Too many input files", __progname);

    should_display = (primary_match_neg == s->primary_function);
    
    for (int i = 0 ; i < NUM_ALGORITHMS; ++i)  {
	if (s->hashes[i].inuse)    {
	hashtable_entry_t *ret = hashtable_contains(s,(hashid_t)i);
      hashtable_entry_t *tmp = ret;
      while (tmp != NULL)
      {
	switch (tmp->status) {

	  // If only the name is different, it's still really a match
	  //  as far as we're concerned. 
	case status_file_name_mismatch:
	case status_match:
	    matched_fdt = tmp->data;
	  should_display = (primary_match_neg != s->primary_function);
	  break;
	  
	case status_file_size_mismatch:
	case status_partial_match:

	  // We only want to display a partial hash error once per input file 
	  if (tmp->data->used != s->hash_round)
	  {
	    tmp->data->used = s->hash_round;
	    display_filename(stderr,s->current_file,false);
	    fprintf(stderr,": Hash collision with ");
	    display_filename(stderr,tmp->data,false);
	    fprintf(stderr,"%s", NEWLINE);

	    // Technically this wasn't a match, so we're still ok
	    // with the match result we already have

	  }
	  break;
	
	case status_unknown_error:
	  return status_unknown_error;

	default:
	  break;
	}
      
	tmp = tmp->next;
      }

      //hashtable_destroy(ret);
    }
  }

  if (should_display)
  {
    if (s->mode & mode_display_hash)
      display_hash_simple(s);
    else
    {
	display_filename(stdout,s->current_file,false);
      if (s->mode & mode_which && primary_match == s->primary_function)
      {
	fprintf(stdout," matches ");
	if (NULL == matched_fdt)
	  fprintf(stdout,"(unknown file)");
	else
	    display_filename(stdout,matched_fdt,false);
      }
      print_status("");
    }
  }
  
#endif
  return status_ok;
}
