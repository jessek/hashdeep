// HASHDEEP
// $Id$

#include "main.h"
#include <string>
#include <algorithm>
#include <iostream>

using namespace std;

#ifdef HAVE_EXTERN_PROGNAME
extern char *__progname;
#else
char *__progname;
#endif


#ifdef _WIN32 
// This can't go in main.h or we get multiple definitions of it
// Allows us to open standard input in binary mode by default 
// See http://gnuwin32.sourceforge.net/compile.html for more 
int _CRT_fmode = _O_BINARY;
#endif


// bring in strsep if it is needed
#ifndef HAVE_STRSEP
#include "lib-strsep.c"
#endif

// So that the usage message fits in a standard DOS window, this
// function should produce no more than 22 lines of text.
static void usage(state *s)
{
  print_status("%s version %s by %s.",__progname,VERSION,AUTHOR);
  print_status("%s %s [-c <alg>] [-k <file>] [-amxwMXrespblvv] [-V|-h] [-o <mode>] [FILES]",
	       CMD_PROMPT,__progname);
  print_status("");

  print_status("-c <alg1,[alg2]> - Compute hashes only. Defaults are MD5 and SHA-256");
  print_status("     legal values are ");
  for (int i = 0 ; i < NUM_ALGORITHMS ; i++)
      print_status("%s%s",hashes[i].name.c_str(),
		   (i+1<NUM_ALGORITHMS)?",":NEWLINE);

  print_status("-a - audit mode. Validates FILES against known hashes. Requires -k");
  print_status("-d - output in DFXML (Digital Forensics XML)");
  print_status("-m - matching mode. Requires -k");
  print_status("-x - negative matching mode. Requires -k");
  print_status("-w - in -m mode, displays which known file was matched");
  print_status("-M and -X act like -m and -x, but display hashes of matching files");
  print_status("-k - add a file of known hashes");
  print_status("-r - recursive mode. All subdirectories are traversed");
  print_status("-e - compute estimated time remaining for each file");
  print_status("-s - silent mode. Suppress all error messages");
  print_status("-p - piecewise mode. Files are broken into blocks for hashing");
  print_status("-b - prints only the bare name of files; all path information is omitted");
  print_status("-l - print relative paths for filenames");
  print_status("-i - only process files smaller than the given threshold");
  print_status("-o - only process certain types of files. See README/manpage");
  print_status("-v - verbose mode. Use again to be more verbose.");
  print_status("-V - display version number and exit");
  print_status("-W FILE - write output to a file");
}


// So that the usage message fits in a standard DOS window, this
// function should produce no more than 22 lines of text. 
static void md5deep_usage(void) 
{
  print_status("%s version %s by %s.",__progname,VERSION,AUTHOR);
  print_status("%s %s [OPTION]... [FILE]...",CMD_PROMPT,__progname);

  print_status("See the man page or README.txt file for the full list of options");
  print_status("-p <size> - piecewise mode. Files are broken into blocks for hashing");
  print_status("-r  - recursive mode. All subdirectories are traversed");
  print_status("-e  - compute estimated time remaining for each file");
  print_status("-s  - silent mode. Suppress all error messages");
  print_status("-S  - displays warnings on bad hashes only");
  print_status("-z  - display file size before hash");
  print_status("-m <file> - enables matching mode. See README/man page");
  print_status("-x <file> - enables negative matching mode. See README/man page");
	 
  print_status("-M and -X are the same as -m and -x but also print hashes of each file");
  print_status("-w  - displays which known file generated a match");
  print_status("-n  - displays known hashes that did not match any input files");
  print_status("-a and -A add a single hash to the positive or negative matching set");
  print_status("-b  - prints only the bare name of files; all path information is omitted");
  print_status("-l  - print relative paths for filenames");
  print_status("-k  - print asterisk before filename");
  print_status("-t  - print GMT timestamp");
  print_status("-i/I- only process files smaller than the given threshold");
  print_status("-o  - only process certain types of files. See README/manpage");
  print_status("-v  - display version number and exit");
  print_status("-W FILE - write output to a file");
}


static void check_flags_okay(state *s)
{
  sanity_check(s,
	       (((s->primary_function & primary_match) ||
		 (s->primary_function & primary_match_neg) ||
		 (s->primary_function & primary_audit)) &&
		!s->hashes_loaded()),
	       "Unable to load any matching files");

  sanity_check(s,
	       (s->mode & mode_relative) && (s->mode & mode_barename),
	       "Relative paths and bare filenames are mutally exclusive");
  
  /* Additional sanity checks will go here as needed... */
}



/****************************************************************
 ** Hash algorithms database.
 ****************************************************************/

/**
 * Add a hash algorithm. This could be table driven, but it isn't.
 */
void algorithm_t::add_algorithm(
	      hashid_t pos,
	      const char *name, 
	      uint16_t bits, 
	      int ( *func_init)(void *),
	      int ( *func_update)(void *, unsigned char *, uint64_t ),
	      int ( *func_finalize)(void *, unsigned char *),
	      int inuse)
{
    hashes[pos].name		= name;
    hashes[pos].f_init      = func_init;
    hashes[pos].f_update    = func_update;
    hashes[pos].f_finalize  = func_finalize;
    hashes[pos].bit_length  = bits;
    hashes[pos].inuse       = inuse;
}


/*
 * Load the hashing algorithms array.
 */
void algorithm_t::load_hashing_algorithms()
{
    /* The DEFAULT_ENABLE variables are in main.h */
    add_algorithm(alg_md5, "md5", 128, hash_init_md5, hash_update_md5, hash_final_md5, DEFAULT_ENABLE_MD5);
    add_algorithm(alg_sha1,"sha1",160, hash_init_sha1, hash_update_sha1, hash_final_sha1, DEFAULT_ENABLE_SHA1);
    add_algorithm(alg_sha256, "sha256", 256, hash_init_sha256, hash_update_sha256, hash_final_sha256, DEFAULT_ENABLE_SHA256);
    add_algorithm(alg_tiger, "tiger", 192, hash_init_tiger, hash_update_tiger, hash_final_tiger, DEFAULT_ENABLE_TIGER);
    add_algorithm(alg_whirlpool, "whirlpool", 512, hash_init_whirlpool, hash_update_whirlpool, hash_final_whirlpool,
		  DEFAULT_ENABLE_WHIRLPOOL);
}


void algorithm_t::clear_algorithms_inuse()
{
  for (int i = 0 ; i < NUM_ALGORITHMS ; ++i)  { 
      hashes[i].inuse = false;
  }
}


/*
 * Add each hashing algorithm by name.
 * @return 0 if success, -1 if error
 */
void algorithm_t::parse_hashing_algorithms(const char *val_)
{
    char *val = strdup(val_);
    char *buf[hashlist::MAX_KNOWN_COLUMNS];

    for (char **ap = buf ; (*ap = strsep(&val,",")) != NULL ; )
	if (*ap != '\0')
	    if (++ap >= &buf[hashlist::MAX_KNOWN_COLUMNS])
		break;
    
    for (int i = 0 ; i < hashlist::MAX_KNOWN_COLUMNS && buf[i] != NULL ; i++)  {
	if (STRINGS_CASE_EQUAL(buf[i],"md5"))
	    hashes[alg_md5].inuse = TRUE;
    
	else if (STRINGS_CASE_EQUAL(buf[i],"sha1") || 
		 STRINGS_CASE_EQUAL(buf[i],"sha-1"))
	    hashes[alg_sha1].inuse = TRUE;
    
	else if (STRINGS_CASE_EQUAL(buf[i],"sha256") || 
		 STRINGS_CASE_EQUAL(buf[i],"sha-256"))
	    hashes[alg_sha256].inuse = TRUE;
    
	else if (STRINGS_CASE_EQUAL(buf[i],"tiger"))
	    hashes[alg_tiger].inuse = TRUE;
    
	else if (STRINGS_CASE_EQUAL(buf[i],"whirlpool"))
	    hashes[alg_whirlpool].inuse = TRUE;
    
	else if (STRINGS_CASE_EQUAL(buf[i],"all")) {
	    for (int count = 0 ; count < NUM_ALGORITHMS ; ++count)
		hashes[count].inuse = TRUE;
	}
	else {
	    print_error("%s: Unknown algorithm: %s", __progname, buf[i]);
	    try_msg();
	    exit(EXIT_FAILURE);
	}    
    }
    free(val);
}


#define TEST_ALGORITHM(CONDITION,ALG)	  \
  if (CONDITION)			  \
    {					  \
      hashes[ALG].inuse = TRUE;	  \
      hash_order[order] = ALG;		  \
      buf += strlen(buf) + 1;		  \
      pos = 0;				  \
      ++total_pos;			  \
      ++order;				  \
      continue;				  \
    }

int hashlist::parse_hashing_algorithms_in_file(const char *fn, const char *val)
{
    const char * buf = val;
    size_t len = strlen(val);
    int done = FALSE;

    // The first position is always the file size, so we start with an 
    // the first position of one.
    uint8_t order = 1;

    size_t pos = 0, total_pos = 0;
  
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
      
    if (STRINGS_CASE_EQUAL(buf,"filename")) {
      // The filename column should be the end of the line
      if (total_pos != len)
      {
	print_error("%s: %s: Badly formatted file", __progname, fn);
	try_msg();
	exit(EXIT_FAILURE);
      }
      done = TRUE;
    }
      
    else {
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

  //s->expected_columns = order;
  //  s->hash_order[order] = alg_unknown;

  if (done) return FALSE;
  return TRUE;
}


int algorithm_t::hashes_inuse_mask()
{
    uint32_t ret = 0;
    for(int i=0;i<NUM_ALGORITHMS;i++){
	ret = (ret<<1) | hashes[i].inuse;
    }
    return ret;
}

static int process_command_line(state *s, int argc, char **argv)
{
  int i;
  
  while ((i=getopt(argc,argv,"do:I:i:c:MmXxtablk:resp:wvVhW:")) != -1)  {
    switch (i) {
    case 'o':
      s->mode |= mode_expert; 
      setup_expert_mode(s,optarg);
      break;

    case 'I': 
      s->mode |= mode_size_all;
      // Note no break here;
    case 'i':
      s->mode |= mode_size;
      s->size_threshold = find_block_size(s,optarg);
      if (0 == s->size_threshold)
      {
	print_error("%s: Requested size threshold implies not hashing anything",
		    __progname);
	exit(STATUS_USER_ERROR);
      }
      break;

    case 'c': 
      s->primary_function = primary_compute;
      /* Before we parse which algorithms we're using now, we have 
       * to erase the default (or previously entered) values
       */
      clear_algorithms_inuse(s);
      algorithm::parse_hashing_algorithms(optarg);
      break;
      
    case 'd': s->dfxml = new XML(stdout); break;
    case 'M': s->mode |= mode_display_hash;	  
    case 'm': s->primary_function = primary_match;      break;
      
    case 'X': s->mode |= mode_display_hash;
    case 'x': s->primary_function = primary_match_neg;  break;
      
    case 'a': s->primary_function = primary_audit;      break;
      
      // TODO: Add -t mode to hashdeep
      //    case 't': s->mode |= mode_timestamp;    break;

    case 'b': s->mode |= mode_barename;     break;
    case 'l': s->mode |= mode_relative;     break;
    case 'e': s->mode |= mode_estimate;     break;
    case 'r': s->mode |= mode_recursive;    break;
    case 's': opt_silent = true;	    break;
      
    case 'p':
      s->mode |= mode_piecewise;
      s->piecewise_size = find_block_size(s, optarg);
      if (0 == s->piecewise_size)
	fatal_error("%s: Piecewise blocks of zero bytes are impossible", 
		    __progname);
      
      break;
      
    case 'w': s->mode |= mode_which;        break;
      
    case 'k':
      switch (s->known.load_hash_file(optarg)) {
      case hashlist::loadstatus_ok: 
	  break;
	  
      case hashlist::status_contains_no_hashes:
	  /* Trying to load an empty file is fine, but we shouldn't
	     change hashes_loaded */
	  break;
	  
      case hashlist::status_contains_bad_hashes:
	  hashes_loaded = true;
	  print_error("%s: %s: contains some bad hashes, using anyway", 
		      __progname, optarg);
	  break;
	  
      case hashlist::status_unknown_filetype:
      case hashlist::status_file_error:
	  /* The loading code has already printed an error */
	    break;
	    
	default:
	  print_error("%s: %s: unknown error, skipping", __progname, optarg);
	  break;
	}
      break;
      
    case 'v':
	if(++opt_verbose > INSANELY_VERBOSE){
	    print_error("%s: User request for insane verbosity denied", __progname);
	}
	break;
      
    case 'V':
      print_status("%s", VERSION);
      exit(EXIT_SUCCESS);
	  
    case 'W':
	s->outfile = optarg;
	break;

    case 'h':
      usage(s);
      exit(EXIT_SUCCESS);
      
    default:
      try_msg();
      exit(EXIT_FAILURE);
    }            
  }
  
  check_flags_okay(s);

  return FALSE;
}




#ifdef _WIN32
static int prepare_windows_command_line(state *s)
{
  int argc;
  TCHAR **argv;

  argv = CommandLineToArgvW(GetCommandLineW(),&argc);
  
  s->argc = argc;
  s->argv = argv;

  return FALSE;
}
#endif


int main(int argc, char **argv)
{
  int count, status = EXIT_SUCCESS;


  /* Because the main() function can handle wchar_t arguments on Win32,
   * we need a way to reference those values. Thus we make a duplciate
   * of the argc and argv values.
   */ 

#ifndef HAVE_EXTERN_PROGNAME
#ifdef HAVE_GETPROGNAME
  __progname  = getprogname();
#else
  __progname  = basename(argv[0]);
#endif
#endif

  algorithm_t::load_hashing_algorithms();		// 
  state *s = new state();

  /**
   * Originally this program was two sets of progarms:
   * 'hashdeep' with the new interface, and 'md5deep', 'sha1deep', etc
   * with the old interface. Now we are a single program and we figure out
   * which interface to use based on how we are started.
   */
  std::string progname(__progname);
  /* Convert progname to lower case */
  std::transform(progname.begin(), progname.end(), progname.begin(), ::tolower);
  std::string algname = progname.substr(0,progname.find("deep"));
  if(algname=="hash"){			// we are hashdeep
      process_command_line(s,argc,argv);
  } else {
      clear_algorithms_inuse(s);
      char buf[256];
      strcpy(buf,algname.c_str());
      algorithm_t::parse_hashing_algorithms(buf);
      for(int i=0;i<NUM_ALGORITHMS;++i){
	  if(hashes[i].inuse){
	      s->md5deep_mode = 1;
	      s->md5deep_mode_algorithm = i;
	      break;
	  }
      }
      if(s->md5deep_mode==0){
	  cerr << progname << ": unknown hash: " <<algname << "\n";
	  exit(1);
      }
      md5deep_process_command_line(s,argc,argv);
  }

  /* Set up the XML */
    if(s->dfxml){
	XML &xreport = *s->dfxml;
	xreport.push("dfxml","xmloutputversion='1.0'");
	xreport.push("metadata",
		       "\n  xmlns='http://md5deep.sourceforge.net/md5deep/' "
		       "\n  xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance' "
		       "\n  xmlns:dc='http://purl.org/dc/elements/1.1/'" );
	xreport.xmlout("dc:type","Hash List","",false);
	xreport.pop();
	xreport.add_DFXML_creator(PACKAGE_NAME,PACKAGE_VERSION,XML::make_command_line(argc,argv));
	xreport.push("configuration");
	xreport.push("algorithms");
	for(int i=0;i<NUM_ALGORITHMS;i++){
	    xreport.make_indent();
	    xreport.printf("<algorithm name='%s' enabled='%d'/>\n",hashes[i].name.c_str(),hashes[i].inuse);
	}
	xreport.pop();			// algorithms
	xreport.pop();			// configuration
    }

   
    //  if (primary_audit == s->primary_function){
    //setup_audit(s);
    //}

#ifdef _WIN32
  if (prepare_windows_command_line(s))
    fatal_error("%s: Unable to process command line arguments", __progname);

  check_wow64(s);
#else
  s->argc = argc;
  s->argv = argv;
#endif

  //memset(s->cwd,0,sizeof(s->cwd));	// zero this out
  char buf[PATH_MAX];
  _tgetcwd(buf,sizeof(buf));	// try to get the cwd
  if (buf[0]==0){			// verify that we got it.
      fatal_error("%s: %s", __progname, strerror(errno));
  }
  s->cwd = buf;				// remember

  /* Anything left on the command line at this point is a file
     or directory we're supposed to process. If there's nothing
     specified, we should tackle standard input */
  
  if (optind == argc){
      hash_stdin(s);
  }   else {
      TCHAR fn[PATH_MAX];;

      count = optind;

      while (count < s->argc) {  
	  generate_filename(s,fn,s->cwd,s->argv[count]);
#ifdef _WIN32
	  status = process_win32(s,fn);
#else
	  status = process_normal(s,fn);
#endif
	  ++count;
      }
  }
  
  if (primary_audit == s->primary_function)
      status = display_audit_results(s);
  
  if(s->dfxml) s->dfxml->pop();		// outermost
  return status;
}
/****************************************************************
 * Legacy code from md5deep follows....
 *
 ****************************************************************/


static void md5deep_check_flags_okay(state *s)
{
  if (NULL == s)
    exit (STATUS_USER_ERROR);

  sanity_check(s,
	       ((s->mode & mode_match) || (s->mode & mode_match_neg)) &&
	       s->hashes_loaded(),
	       "Unable to load any matching files");

  sanity_check(s,
	       (s->mode & mode_relative) && (s->mode & mode_barename),
	       "Relative paths and bare filenames are mutally exclusive");
  
  sanity_check(s,
	       (s->mode & mode_piecewise) && (s->mode & mode_display_size),
	       "Piecewise mode and file size display is just plain silly");


  /* If we try to display non-matching files but haven't initialized the
     list of matching files in the first place, bad things will happen. */
  sanity_check(s,
	       (s->mode & mode_not_matched) && 
	       ! ((s->mode & mode_match) || (s->mode & mode_match_neg)),
	       "Matching or negative matching must be enabled to display non-matching files");

  sanity_check(s,
	       (s->mode & mode_which) && 
	       ! ((s->mode & mode_match) || (s->mode & mode_match_neg)), 
	       "Matching or negative matching must be enabled to display which file matched");
 
 
  // Additional sanity checks will go here as needed... 
}


static void md5deep_check_matching_modes(state *s)
{
    if (NULL == s){
	exit (STATUS_USER_ERROR);
    }

    sanity_check(s,
		 (s->mode & mode_match) && (s->mode & mode_match_neg),
		 "Regular and negative matching are mutually exclusive.");
}


int md5deep_process_command_line(state *s, int argc, char **argv)
{
  int i;

  if (NULL == s)
    return TRUE;
  
  while ((i = getopt(argc,
		     argv,
		     "df:I:i:M:X:x:m:o:A:a:tnwczsSp:erhvV0lbkqZW:")) != -1) { 
    switch (i) {

    case 'd': s->dfxml = new XML(stdout); break;
    case 'f':
      s->input_list = strdup(optarg);
      s->mode |= mode_read_from_file;
      break;

    case 'I':
      s->mode |= mode_size_all;
      // Note that there is no break here
    case 'i':
      s->mode |= mode_size;
      s->size_threshold = find_block_size(s,optarg);
      if (0 == s->size_threshold) {
	print_error("%s: Requested size threshold implies not hashing anything",
		    __progname);
	exit(STATUS_USER_ERROR);
      }
      break;

    case 'p':
      s->mode |= mode_piecewise;
      s->piecewise_size = find_block_size(s, optarg);
      if (0 == s->piecewise_size) {
	print_error("%s: Illegal size value for piecewise mode.", __progname);
	exit(STATUS_USER_ERROR);
      }

      break;

    case 'Z':
      s->mode |= mode_triage;
      break;

    case 't':
      s->mode |= mode_timestamp;
      break;
    case 'n': 
      s->mode |= mode_not_matched; 
      break;
    case 'w': 
      s->mode |= mode_which; 
      break;

    case 'a':
      s->mode |= mode_match;
      md5deep_check_matching_modes(s);
      md5deep_add_hash(s,optarg,optarg);
      break;

    case 'A':
      s->mode |= mode_match_neg;
      md5deep_check_matching_modes(s);
      md5deep_add_hash(s,optarg,optarg);
      hashes_loaded = true;
      break;

    case 'o': 
      s->mode |= mode_expert; 
      setup_expert_mode(s,optarg);
      break;
      
    case 'M':
      s->mode |= mode_display_hash;
    case 'm':
      s->mode |= mode_match;
      md5deep_check_matching_modes(s);
      if (md5deep_load_match_file(s,optarg))
	hashes_loaded = true;
      break;

    case 'X':
      s->mode |= mode_display_hash;
    case 'x':
      s->mode |= mode_match_neg;
      md5deep_check_matching_modes(s);
      if (md5deep_load_match_file(s,optarg))
	hashes_loaded = true;
      break;

    case 'c':
      s->mode |= mode_csv;
      break;

    case 'z': 
      s->mode |= mode_display_size; 
      break;

    case '0': 
      s->mode |= mode_zero; 
      break;

    case 'S': 
      s->mode |= mode_warn_only;
      opt_silent = true;
      break;

    case 's':
	opt_silent = true;
      break;

    case 'e':
      s->mode |= mode_estimate;
      break;

    case 'r':
      s->mode |= mode_recursive;
      break;

    case 'k':
      s->mode |= mode_asterisk;
      break;

    case 'b': 
      s->mode |= mode_barename; 
      break;
      
    case 'l': 
      s->mode |= mode_relative; 
      break;

    case 'q': 
      s->mode |= mode_quiet; 
      break;

    case 'h':
	md5deep_usage();
      exit (STATUS_OK);

    case 'v':
      print_status("%s",VERSION);
      exit (STATUS_OK);

    case 'V':
      // COPYRIGHT is a format string, complete with newlines
      print_status(COPYRIGHT);
      exit (STATUS_OK);

    case 'W':
	s->outfile = optarg;
	break;

    default:
      try_msg();
      exit (STATUS_USER_ERROR);

    }
  }

  md5deep_check_flags_okay(s);
  return STATUS_OK;
}


