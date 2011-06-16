
// $Id$

#include "main.h"
#include <new>

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
      s->hash_order[order] = ALG;	  \
      buf += strlen(buf) + 1;		  \
      pos = 0;				  \
      ++total_pos;			  \
      ++order;				  \
      continue;				  \
    }

static int parse_hashing_algorithms(state *s, char *fn, char *val)
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

  while (!done)
  {
    if ( ! (',' == buf[pos] || 0 == buf[pos]))
    {
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
	print_error(s,"%s: %s: Badly formatted file", __progname, fn);
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
      print_error(s,
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



static filetype_t 
identify_file(state *s, char *fn, FILE *handle)
{
  hashid_t current_order[NUM_ALGORITHMS];
 
  assert(s!=0);
  assert(fn!=0);
  assert(handle!=0);

  char * buf = (char *)malloc(MAX_STRING_LENGTH);
  if (NULL == buf)
    return file_unknown;

  // Find the header 
  if ((fgets(buf,MAX_STRING_LENGTH,handle)) == NULL) 
    {
      free(buf);
      return file_unknown;
    }

  chop_line(buf);

  if ( ! STRINGS_EQUAL(buf,HASHDEEP_HEADER_10))
    {
      free(buf);
      return file_unknown;
    }

  // Find which hashes are in this file
  if ((fgets(buf,MAX_STRING_LENGTH,handle)) == NULL) 
    {
      free(buf);
      return file_unknown;
    }

  chop_line(buf);

  // We don't use STRINGS_EQUAL here because we only care about
  // the first ten characters for right now. 
  if (strncasecmp("%%%% size,",buf,10))
  {
    free(buf);
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
  for (int i = 0 ; i < NUM_ALGORITHMS ; ++i)
    s->hash_order[i] = alg_unknown;

  // Skip the "%%%% size," when parsing the list of hashes 
  parse_hashing_algorithms(s,fn,buf + 10);

  if (s->hashes_loaded)  {
      int i = 0;
    while (i < NUM_ALGORITHMS && 
	   s->hash_order[i] == current_order[i])
      i++;
    if (i < NUM_ALGORITHMS)
      print_error(s,"%s: %s: Hashes not in same format as previously loaded",
		  __progname, fn);
  }

  free(buf);
  return file_hashdeep_10;
}



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


#ifdef _DEBUG
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
  if (NULL == s)
    return;

  if (NULL == s->known)
  {
    print_status("No known hashes");
    return;
  }

  file_data_t * tmp = s->known;
  while (tmp != NULL)
  {
    display_file_data(s,tmp);
    tmp = tmp->next;
  }
}
#endif


/**
 * Read a hash set file.
 */

status_t read_hash_set_file(state *s, char *fn, FILE *handle)
{
  status_t st = status_ok;
  int contains_bad_lines = FALSE, record_valid;
  char * buf;

  // We start our counter at line number two for the two lines
  // of header we've already read
  uint64_t line_number = 2;

  char line[MAX_STRING_LENGTH];

  while (fgets(line,MAX_STRING_LENGTH,handle)) {
    line_number++;

    // Lines starting with a pound sign are comments and can be ignored
    if ('#' == line[0])
      continue;

    // We're going to be advancing the string variable, so we
    // make sure to use a temporary pointer. If not, we'll end up
    // overwriting random data the next time we read.
    buf = line;
    record_valid = TRUE;
    chop_line(buf);

    // C++ typically fails with a bad_alloc, but you can make it return null
    // http://www.cplusplus.com/reference/std/new/bad_alloc/
    // http://www.cplusplus.com/reference/std/new/nothrow/
    file_data_t * t = new (std::nothrow) file_data_t(); // C++ new fails with a bad_a
    if (NULL == t){
      fatal_error(s,"%s: %s: Out of memory in line %"PRIu64, 
		  __progname, fn, line_number);
    }

    int done = FALSE;
    size_t pos = 0;
    uint8_t column_number = 0;

    while (!done)
    {
      if ( ! (',' == buf[pos] || 0 == buf[pos]))
      {
	++pos;
	continue;
      }

      // Terminate the string so that we can do comparisons easily
      buf[pos] = 0;

      // The first column should always be the file size
      if (0 == column_number)
      {
	t->file_size = (uint64_t)strtoll(buf,NULL,10);
	buf += strlen(buf) + 1;
	pos = 0;
	column_number++;
	continue;
      }

      // All other columns should contain a valid hash
      if ( ! match_valid_hash(s,s->hash_order[column_number],buf))
      {
	print_error(s,
		    "%s: %s: Invalid %s hash in line %"PRIu64,
		    __progname, fn, 
		    s->hashes[s->hash_order[column_number]].name.c_str(),
		    line_number);
	contains_bad_lines = TRUE;
	record_valid = FALSE;
	// Break out (done = true) and then process the next line
	break;
      }

      t->hash_hex[s->hash_order[column_number]] = std::string(buf);

      ++column_number;
      buf += strlen(buf) + 1;
      pos = 0;

      // The 'last' column (even if there are more commas in the line)
      // is the filename. Note that valid filenames can contain commas! 
      if (column_number == s->expected_columns)
	{
#ifdef _WIN32
	  size_t len = strlen(buf);
	  t->file_name = (TCHAR *)malloc(sizeof(TCHAR) * (len+1));
	  if (NULL == t->file_name)
	    fatal_error(s,"%s: Out of memory", __progname);
	  
	  // On Windows we must convert the filename from ANSI to Unicode.
	  // The -1 parameter for the input length asserts that the input
	  // string, argv[i] is NULL terminated and that the function should
	  // process the whole thing. The full definition of this function:
	  // http://msdn2.microsoft.com/en-us/library/ms776413(VS.85).aspx
	  if ( ! MultiByteToWideChar(CP_ACP,
				     MB_PRECOMPOSED,
				     buf,
				     -1,   
				     t->file_name,
				     len+1))
	    fatal_error(s,"%s: MultiByteToWideChar failed (%d)", 
			__progname, 
			GetLastError()); 
#else
	  t->file_name = buf;
#endif
	  done = TRUE;
	}
    }

    if ( ! record_valid)
      continue;

    st = add_file(s,t);
    if (st != status_ok)
      return st;
  }
  if (contains_bad_lines)
    return status_contains_bad_hashes;
  
  return st;
}



status_t load_match_file(state *s, char *fn)
{
  status_t status = status_ok;
  filetype_t type;

  assert(s!=0);
  assert(fn!=0);

  FILE *handle = fopen(fn,"rb");
  if (NULL == handle) {
    print_error(s,"%s: %s: %s", __progname, fn, strerror(errno));
    return status_file_error;
  }
  
  type = identify_file(s,fn,handle);
  if (file_unknown == type)  {
    print_error(s,"%s: %s: Unable to identify file format", __progname, fn);
    fclose(handle);
    handle=0;
    return status_unknown_filetype;
  }

  status = read_hash_set_file(s,fn,handle);
  fclose(handle);
  handle=0;

  //  display_all_known_files(s);
  return status;
}


// We don't use this function anymore, but it's handy to have just in case
/*
char * status_to_str(status_t s)
{
  switch (s) {
  case status_ok: return "ok";
  case status_match: return "complete match";
  case status_partial_match: return "partial match";
  case status_file_size_mismatch: return "file size mismatch";
  case status_file_name_mismatch: return "file name mismatch";
  case status_no_match: return "no match"; 

  default:
    return "unknown";
  }      
}
*/


status_t display_match_result(state *s)
{
    file_data_t *matched_fdt = NULL;
    int should_display; 
    uint64_t my_round;

    my_round = s->hash_round;
  s->hash_round++;
  if (my_round > s->hash_round)
      fatal_error(s,"%s: Too many input files", __progname);

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
  
  return status_ok;
}
