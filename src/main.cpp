/*
 * 
 *  $Id$
 * 
 * This is the main() function and support functions for hashdeep and md5deep.
 *
 * This is a work of the US Government. In accordance with 17 USC 105,
 * copyright protection is not available for any work of the US Government.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Original program by Jesse Kornblum.
 * Significantly modified by Simson Garfinkel.
 */

#include "main.h"
#include <string>
#include <algorithm>
#include <iostream>
#include <vector>
#include <sstream>
#include "utf8.h"

#include "md5.h"
#include "sha1.h"
#include "sha256.h"
#include "tiger.h"
#include "whirlpool.h"

using namespace std;

#if defined(HAVE_EXTERN_PROGNAME) 
extern char *__progname;
#else
char *__progname;
#endif

#define AUTHOR      "Jesse Kornblum and Simson Garfinkel"
#define COPYRIGHT   "This program is a work of the US Government. "\
"In accordance with 17 USC 105,%s"\
"copyright protection is not available for any work of the US Government.%s"\
"This is free software; see the source for copying conditions. There is NO%s"\
"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.",\
NEWLINE, NEWLINE, NEWLINE

#ifdef _WIN32 
// This can't go in main.h or we get multiple definitions of it
// Allows us to open standard input in binary mode by default 
// See http://gnuwin32.sourceforge.net/compile.html for more 
int _CRT_fmode = _O_BINARY;
#endif


/* These were inappropriately moved here and need to be moved to classes */
int opt_debug = 0;			// debug mode; 1 is self-test
bool opt_silent = false;
bool md5deep_mode = false;
int  opt_verbose = 0;
bool opt_estimate = false;
bool opt_relative = false;
bool opt_unicode_escape = false;
bool opt_mode_match = false;
bool opt_mode_match_neg = false;


/* output options */
bool opt_csv = false;
bool opt_asterisk = false;


/****************************************************************
 ** Various helper functions.
 ****************************************************************/

hashid_t  opt_md5deep_mode_algorithm = alg_unknown;
uint64_t file_data_hasher_t::next_file_number = 0; // needs to live somewhere

/* This is the one place we allow a printf, becuase we are about to exit, and we call it before we multithread */
static void try_msg(void)
{
    fprintf(stderr,"Try `%s -h` for more information.", __progname);
}


static void sanity_check(int condition, const char *msg)
{
    if (condition) {
	if (!opt_silent) {
	    fprintf(stderr,"%s: %s", __progname, msg);
	    try_msg();
	}
	exit (status_t::STATUS_USER_ERROR);
    }
}

static int is_absolute_path(const TCHAR *fn)
{
#ifdef _WIN32
  return FALSE;
#endif
  return (DIR_SEPARATOR == fn[0]);
}


/**
 * return the full pathname for a filename.
 */
 
tstring state::generate_filename(const TCHAR *input)
{
    if ((opt_relative) || is_absolute_path(input)){
#ifdef _WIN32
	return tstring((const wchar_t *)input);
#else
	return tstring(input);
#endif
    }
    // Windows systems don't have symbolic links, so we don't
    // have to worry about carefully preserving the paths
    // they follow. Just use the system command to resolve the paths
    //
    // Actually, they can have symbolic links...
#ifdef _WIN32
    wchar_t fn[PATH_MAX];
    memset(fn,0,sizeof(fn));
    _wfullpath(fn,(const wchar_t *)input,PATH_MAX);
    return tstring(fn);
#else	  
    char buf[PATH_MAX+1];
    std::string cwd = main::getcwd();
    if (cwd=="") {
	// If we can't get the current working directory, we're not
	// going to be able to build the relative path to this file anyway.
	// So we just call realpath and make the best of things 
	if (realpath(input,buf)==0){
	    ocb.internal_error("Error calling realpath in generate_filename");
	}
	return string(buf);
    }
    return cwd + DIR_SEPARATOR + input;
#endif
}


// So that the usage message fits in a standard DOS window, this
// function should produce no more than 22 lines of text.
void state::usage()
{
    if(usage_count==0){
	ocb.status("%s version %s by %s.",__progname,VERSION,AUTHOR);
	ocb.status("%s %s [-c <alg>] [-k <file>] [-amxwMXrespblvv] [-jnn] [-V|-h] [-o <mode>] [FILES]",
		     CMD_PROMPT,__progname);
	
	/* Make a list of the hashes */
	ocb.status("-c <alg1,[alg2]> - Compute hashes only. Defaults are MD5 and SHA-256");
	fprintf(stderr,"     legal values: ");
	for (int i = 0 ; i < NUM_ALGORITHMS ; i++){
	    fprintf(stderr,"%s%s",hashes[i].name.c_str(),(i+1<NUM_ALGORITHMS) ? "," : NEWLINE);
	}
	
	ocb.status("-p <size> - piecewise mode. Files are broken into blocks for hashing");
	ocb.status("-r  - recursive mode. All subdirectories are traversed");
	ocb.status("-d  - output in DFXML (Digital Forensics XML)");
	ocb.status("-k <FN>  - add a file of known hashes");
	ocb.status("-a  - audit mode. Validates FILES against known hashes. Requires -k");
	ocb.status("-m  - matching mode. Requires -k");
	ocb.status("-x  - negative matching mode. Requires -k");
	ocb.status("-w  - in -m mode, displays which known file was matched");
	ocb.status("-M and -X act like -m and -x, but display hashes of matching files");
	ocb.status("-e  - compute estimated time remaining for each file");
	ocb.status("-s  - silent mode. Suppress all error messages");
	ocb.status("-b  - prints only the bare name of files; all path information is omitted");
	ocb.status("-l  - print relative paths for filenames");
	ocb.status("-i  - only process files smaller than the given threshold");
	ocb.status("-o  - only process certain types of files. See README/manpage");
	ocb.status("-v  - verbose mode. Use again to be more verbose; -V display version & exit.");
	ocb.status("-d  - output in DFXML; -u - Escape Unicode; -W FILE - write to FILE.");
#ifdef HAVE_PTHREAD
	ocb.status("-jnn run nn threads (default %d)",threadpool::numCPU());
#else
	ocb.status("-jnn ignored (compiled without pthreads)");
#endif	

    }
    if(usage_count==1){
	ocb.status("-u  - escape Unicode");
    }
    usage_count++;
}


// So that the usage message fits in a standard DOS window, this
// function should produce no more than 22 lines of text. 
void state::md5deep_usage(void) 
{
    if(usage_count==0){
	ocb.status("%s version %s by %s.",__progname,VERSION,AUTHOR);
	ocb.status("%s %s [OPTION]... [FILE]...",CMD_PROMPT,__progname);
	ocb.status("See the man page or README.txt file for the full list of options");
	ocb.status("-p <size> - piecewise mode. Files are broken into blocks for hashing");
	ocb.status("-r  - recursive mode. All subdirectories are traversed");
	ocb.status("-e  - show estimated time remaining for each file");
	ocb.status("-s  - silent mode. Suppress all error messages; -S warn on bad hashes only");
	ocb.status("-z  - display file size before hash");
	ocb.status("-m <file> - enables matching mode. See README/man page");
	ocb.status("-x <file> - enables negative matching mode. See README/man page");
	ocb.status("-M and -X are the same as -m and -x but also print hashes of each file");
	ocb.status("-w  - displays which known file generated a match");
	ocb.status("-n  - displays known hashes that did not match any input files");
	ocb.status("-a and -A add a single hash to the positive or negative matching set");
	ocb.status("-b  - prints only the bare name of files; all path information is omitted");
	ocb.status("-l  - print relative paths for filenames");
	ocb.status("-k  - print asterisk before filename; -0 - use a NULL for newline.");
	ocb.status("-t  - print GMT timestamp");
	ocb.status("-i/I- only process files smaller than the given threshold");
	ocb.status("-o  - only process certain types of files. See README/manpage");
	ocb.status("-v  - display version number and exit");
	ocb.status("-d  - output in DFXML; -u - Escape Unicode; -W FILE - write to FILE.");
#ifdef HAVE_PTHREAD
	ocb.status("-jnn run nn threads (default %d)",threadpool::numCPU());
#else
	ocb.status("-jnn ignored (compiled without pthreads)");
#endif	
    }
    usage_count++;
}


void state::check_flags_okay()
{
  sanity_check(
	       (((ocb.primary_function & primary_match) ||
		 (ocb.primary_function & primary_match_neg) ||
		 (ocb.primary_function & primary_audit)) &&
		!hashes_loaded()),
	       "Unable to load any matching files");

  sanity_check(
	       (opt_relative) && (ocb.mode_barename),
	       "Relative paths and bare filenames are mutally exclusive");
  
  /* Additional sanity checks will go here as needed... */
}



/****************************************************************
 ** Hash algorithms database.
 ****************************************************************/

algorithm_t     hashes[NUM_ALGORITHMS];		// which hash algorithms are available and in use
/**
 * Add a hash algorithm. This could be table driven, but it isn't.
 */
void algorithm_t::add_algorithm(
	      hashid_t pos,
	      const char *name, 
	      uint16_t bits, 
	      int ( *func_init)(void *ctx),
	      int ( *func_update)(void *ctx, const unsigned char *buf, size_t buflen),
	      int ( *func_finalize)(void *ctx, unsigned char *),
	      int inuse)
{
    hashes[pos].name		= name;
    hashes[pos].f_init      = func_init;
    hashes[pos].f_update    = func_update;
    hashes[pos].f_finalize  = func_finalize;
    hashes[pos].bit_length  = bits;
    hashes[pos].inuse       = inuse;
    hashes[pos].id          = pos;
}


extern "C" {
int sha1_init(void * md);
    int sha1_process(void *md, const unsigned char *buf,uint64_t);
    int sha1_done(void * md, unsigned char *out);
    };

/*
 * Load the hashing algorithms array.
 */
void algorithm_t::load_hashing_algorithms()
{
    /* The DEFAULT_ENABLE variables are in main.h */
    add_algorithm(alg_md5,    "md5", 128, hash_init_md5, hash_update_md5, hash_final_md5, DEFAULT_ENABLE_MD5);
    add_algorithm(alg_sha1,   "sha1",160, hash_init_sha1, hash_update_sha1, hash_final_sha1, DEFAULT_ENABLE_SHA1);
    add_algorithm(alg_sha256, "sha256", 256, hash_init_sha256, hash_update_sha256, hash_final_sha256, DEFAULT_ENABLE_SHA256);
    add_algorithm(alg_tiger,  "tiger", 192, hash_init_tiger, hash_update_tiger, hash_final_tiger, DEFAULT_ENABLE_TIGER);
    add_algorithm(alg_whirlpool, "whirlpool", 512, hash_init_whirlpool, hash_update_whirlpool, hash_final_whirlpool,
		  DEFAULT_ENABLE_WHIRLPOOL);
}


/**
 * Given an algorithm name, convert it to a hashid_t
 * returns alg_unknown if the name is not valid.
 */
hashid_t algorithm_t::get_hashid_for_name(string name)
{
    /* convert name to lowercase and remove any dashes */
    lowercase(name);
    size_t dash;
    while((dash=name.find("-")) != string::npos){ 
	name.replace(dash,1,"");
    }
    for(int i=0;i<NUM_ALGORITHMS;i++){
	if(hashes[i].name==name) return hashes[i].id;
    }
    return alg_unknown;
}

void algorithm_t::clear_algorithms_inuse()
{
  for (int i = 0 ; i < NUM_ALGORITHMS ; ++i)  { 
      hashes[i].inuse = false;
  }
}


bool algorithm_t::valid_hash(hashid_t alg, const std::string &buf)
{
    for (size_t pos = 0 ; pos < hashes[alg].bit_length/4 ; pos++)  {
	if (!isxdigit(buf[pos])) return false; // invalid character
	if (pos==(hashes[alg].bit_length/4)-1) return true; // we found them all
    }
    return false;				// too short or too long
}



/* C++ string splitting code from http://stackoverflow.com/questions/236129/how-to-split-a-string-in-c */
std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}


std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    return split(s, delim, elems);
}


void lowercase(std::string &s)
{
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
}


/*
 * Set inuse for each of the algorithms in the argument.
 */
void algorithm_t::enable_hashing_algorithms(std::string var)
{
    /* convert name to lowercase and remove any dashes */
    std::transform(var.begin(), var.end(), var.begin(), ::tolower);

    /* Split on the commas */
    std::vector<std::string>algs = split(var,',');

    for(std::vector<std::string>::const_iterator it = algs.begin();it!=algs.end();it++){
	hashid_t id = get_hashid_for_name(*it);
	if(id==alg_unknown){
	    /* Check to see if *i is "all" */
	    if(*it == "all"){
		for(int j=0;j<NUM_ALGORITHMS;j++){
		    hashes[j].inuse = TRUE;
		}
		return;
	    }
	    /* No idea what this algorithm is. */
	    fprintf(stderr,"%s: Unknown algorithm: %s", __progname, (*it).c_str());
	    try_msg();
	    exit(EXIT_FAILURE);
	}
	hashes[id].inuse = TRUE;
    }
}


void state::setup_expert_mode(char *arg)
{
    for(unsigned int i=0;i<strlen(arg);i++){
	switch(arg[i]){
	case 'b': // Block Device
	    mode_block = true;     break;
	case 'c': // Character Device
	    mode_character = true; break;
	case 'p': // Named Pipe
	    mode_pipe=true;      break;
	case 'f': // Regular File
	    mode_regular=true;   break;
	case 'l': // Symbolic Link
	    mode_symlink=true;   break;
	case 's': // Socket
	    mode_socket=true;    break;
	case 'd': // Door (Solaris)
	    mode_door=true;      break;
	default:
	    ocb.print_error("%s: Unrecognized file type: %c", __progname,arg[i]);
	}
    }
}




int state::hashdeep_process_command_line(int argc, char **argv)
{
  int i;
  
  while ((i=getopt(argc,argv,"do:I:i:c:MmXxtablk:resp:wvVhW:0D:uj:")) != -1)  {
    switch (i) {
    case 'o':
      mode_expert=true; 
      setup_expert_mode(optarg);
      break;

    case 'I': 
	ocb.mode_size_all=true;
      // Note no break here;
    case 'i':
	ocb.mode_size = true;
	ocb.size_threshold = find_block_size(optarg);
	if (ocb.size_threshold==0) {
	    ocb.print_error("%s: Requested size threshold implies not hashing anything",
			__progname);
	    exit(status_t::STATUS_USER_ERROR);
	}
	break;

    case 'c': 
      ocb.primary_function = primary_compute;
      /* Before we parse which algorithms we're using now, we have 
       * to erase the default (or previously entered) values
       */
      algorithm_t::clear_algorithms_inuse();
      algorithm_t::enable_hashing_algorithms(optarg);
      break;
      
    case 'd': ocb.xml_open(stdout); break;
    case 'M': ocb.opt_display_hash=true;
	/* intentional fall through */
    case 'm': ocb.primary_function = primary_match;      break;
    case 'X': ocb.opt_display_hash=true;
	/* intentional fall through */
    case 'x': ocb.primary_function = primary_match_neg;  break;
    case 'a': ocb.primary_function = primary_audit;      break;
      
      // TODO: Add -t mode to hashdeep
      //    case 't': mode |= mode_timestamp;    break;

    case 'b': ocb.mode_barename=true;     break;
    case 'l': opt_relative=true;     break;
    case 'e': opt_estimate = true;	    break;
    case 'r': mode_recursive=true;    break;
    case 's': opt_silent = true;	    break;
      
    case 'p':
	ocb.piecewise_size = find_block_size(optarg);
      if (ocb.piecewise_size==0)
	  ocb.fatal_error("%s: Piecewise blocks of zero bytes are impossible",__progname);
      
      break;
      
    case 'w': ocb.opt_show_matched = true;    break; // displays which known hash generated a match
      
    case 'k':
	switch (ocb.load_hash_file(optarg)) {
	case hashlist::loadstatus_ok: 
	    if(opt_verbose>=MORE_VERBOSE){
		ocb.print_error("%s: %s: Match file loaded %d known hash values.",
			    __progname,optarg,ocb.known_size());
	    }
	    break;
	  
      case hashlist::status_contains_no_hashes:
	  /* Trying to load an empty file is fine, but we shouldn't
	     change hashes_loaded */
	  break;
	  
      case hashlist::status_contains_bad_hashes:
	  ocb.print_error("%s: %s: contains some bad hashes, using anyway", 
		      __progname, optarg);
	  break;
	  
      case hashlist::status_unknown_filetype:
      case hashlist::status_file_error:
	  /* The loading code has already printed an error */
	    break;
	    
	default:
	    fprintf(stderr,"%s: %s: unknown error, skipping", __progname, optarg);
	  break;
	}
      break;
      
    case 'v':
	if(++opt_verbose > INSANELY_VERBOSE){
	    ocb.print_error("%s: User request for insane verbosity denied", __progname);
	}
	break;
      
    case 'V':
      ocb.status("%s", VERSION);
      exit(EXIT_SUCCESS);
	  
    case 'W': ocb.set_outfilename(optarg); break;
    case '0': ocb.opt_zero = true; break;
    case 'u': opt_unicode_escape = true;break;
    case 'j': ocb.opt_threadcount = atoi(optarg); break;

    case 'h':
	usage();
	exit(EXIT_SUCCESS);
      
    case 'D': opt_debug = atoi(optarg);break;
    default:
      try_msg();
      exit(EXIT_FAILURE);
    }            
  }
  
  check_flags_okay();
  return FALSE;
}

#ifdef _WIN32
/**
 * WIN32 requires the argv in wchar_t format to allow the program to get UTF16
 * filenames resulting from star expansion.
 */
int state::prepare_windows_command_line()
{
    this->argv = CommandLineToArgvW(GetCommandLineW(),&this->argc);
    return FALSE;
}
#endif

class uni32str:public vector<uint32_t> {};
    
std::string main::escape_utf8(const std::string &utf8)
{
    uni32str utf32_line;
    std::string ret;
    utf8::utf8to32(utf8.begin(),utf8.end(),back_inserter(utf32_line));
    for(uni32str::const_iterator it = utf32_line.begin(); it!=utf32_line.end(); it++){
	if((*it) < 256){
	    ret.push_back(*it);
	} else {
	    char buf[16];
	    snprintf(buf,sizeof(buf),"U+%04X",*it);
	    ret.append(buf);
	}
    }
    return ret;
}

#ifdef _WIN32
/**
 * We only need make_utf8 on windows because on POSIX systems
 * all filenames are assumed to be UTF8.
 */
std::string main::make_utf8(const tstring &str) 
{
    if(str.size()==0) return std::string(); // nothing to convert

    /* Figure out how many bytes req required */
    size_t len = WideCharToMultiByte(CP_UTF8,0,str.c_str(),str.size(),0,0,0,0);
    if(len==0){
	switch(GetLastError()){
	case ERROR_INSUFFICIENT_BUFFER: std::cerr << "ERROR_INSUFFICIENT_BUFFER\n";break;
	case ERROR_INVALID_FLAGS: std::cerr << "ERROR_INVALID_FLAGS\n";break;
	case ERROR_INVALID_PARAMETER: std::cerr << "ERROR_INVALID_PARAMETER\n";break;
	case ERROR_NO_UNICODE_TRANSLATION: std::cerr << "ERROR_NO_UNICODE_TRANSLATION\n";break;
	}
	std::cerr << "WideCharToMultiByte failed\n";
	return std::string("");
    }
    /* allocate the space we need (plus one for null-termination */
    char *buf = new char[len+1];

    /* Perform the conversion */
    len = WideCharToMultiByte(CP_UTF8,0,str.c_str(),str.size(),buf,len,0,0);
    if(len==0){
	return std::string("");		// nothing to return
    }
    buf[len] = 0;			// be sure it is null-terminated
    std::string s2(buf);		// Make a STL string 
    delete [] buf;			// Delete the buffern
    return s2;				// return the string
}
#endif


tstring main::getcwd()
{
#ifdef _WIN32
    wchar_t buf[MAX_PATH];
    memset(buf,0,sizeof(buf));
    _wgetcwd(buf,MAX_PATH);
    tstring ret = tstring(buf);
    return ret;
#else
    char buf[PATH_MAX+1];
    memset(buf,0,sizeof(buf));
    ::getcwd(buf,sizeof(buf));
    return std::string(buf);
#endif    
}

/* Return the canonicalized absolute pathname in UTF-8 on Windows and POSIX systems */
tstring main::get_realpath(const tstring &fn)
{
#ifdef _WIN32    
    /*
     * expand a relative path to the full path.
     * http://msdn.microsoft.com/en-us/library/506720ff(v=vs.80).aspx
     */
    wchar_t absPath[PATH_MAX];
    if(_wfullpath(absPath,fn.c_str(),PATH_MAX)==0) tstring(); // fullpath failed...
    return tstring(absPath);
#else
    char resolved_name[PATH_MAX];	//
    if(realpath(fn.c_str(),resolved_name)==0) return "";
    if(opt_debug) std::cout << "main::get_realpath(" << fn << ")=" << resolved_name << "\n";
    return tstring(resolved_name);
#endif
}

std::string main::get_realpath8(const tstring &fn)
{
    return main::make_utf8(main::get_realpath(fn));
}


#ifdef _WIN32
/**
 * Detect if we are a 32-bit program running on a 64-bit system.
 *
 * Running a 32-bit program on a 64-bit system is problematic because WoW64
 * changes the program's view of critical directories. An affected
 * program does not see the true %WINDIR%, but instead gets a mapped
 * version. Thus the user cannot get an accurate picture of their system.
 * See http://jessekornblum.livejournal.com/273084.html for an example.
 *
 * The following is adapted from
 * http://msdn.microsoft.com/en-us/library/ms684139(v=VS.85).aspx
 */

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
LPFN_ISWOW64PROCESS fnIsWow64Process;

void state::check_wow64()
{
    BOOL result;

    fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress(GetModuleHandle(TEXT("kernel32")),
							  "IsWow64Process");
    // If this system doesn't have the function IsWow64Process then
    // it's definitely not running under WoW64.
    if (NULL == fnIsWow64Process) return;
    
    if (! fnIsWow64Process(GetCurrentProcess(), &result))  {
	// The function failed? WTF? Well, let's not worry about it.
	return;
    }
    
    if (result) {
	ocb.print_error("%s: WARNING: You are running a 32-bit program on a 64-bit system.", __progname);
	ocb.print_error("%s: You probably want to use the 64-bit version of this program.", __progname);
    }
}
#endif   // ifdef _WIN32


/****************************************************************
 * Legacy code from md5deep follows....
 *
 ****************************************************************/


void state::md5deep_check_flags_okay()
{
  sanity_check(((opt_mode_match) || (opt_mode_match_neg)) &&
	       hashes_loaded()==0,
	       "Unable to load any matching files");

  sanity_check((opt_relative) && (ocb.mode_barename),
	       "Relative paths and bare filenames are mutally exclusive");
  
  sanity_check((ocb.piecewise_size>0) && (ocb.opt_display_size),
	       "Piecewise mode and file size display is just plain silly");


  /* If we try to display non-matching files but haven't initialized the
     list of matching files in the first place, bad things will happen. */
  sanity_check((ocb.mode_not_matched) && 
	       ! ((opt_mode_match) || (opt_mode_match_neg)),
	       "Matching or negative matching must be enabled to display non-matching files");

  sanity_check(ocb.opt_show_matched && 
	       ! ((opt_mode_match) || (opt_mode_match_neg)), 
	       "Matching or negative matching must be enabled to display which file matched");
}


void state::md5deep_check_matching_modes()
{
    sanity_check((opt_mode_match) && (opt_mode_match_neg),
		 "Regular and negative matching are mutually exclusive.");
}


int state::md5deep_process_command_line(int argc, char **argv)
{
    int i;
    while ((i = getopt(argc,
		       argv,
		       "dI:i:M:X:x:m:o:A:a:tnwczsSp:erhvV0lbkqZW:D:uj:")) != -1) { 
	switch (i) {
	case 'D': opt_debug = atoi(optarg);break;
	case 'd': ocb.xml_open(stdout); break;
	case 'I':
	    ocb.mode_size_all=true;
	    // Note that there is no break here
	case 'i':
	    ocb.mode_size=true;
	    ocb.size_threshold = find_block_size(optarg);
	    if (ocb.size_threshold==0) {
		ocb.print_error("%s: Requested size threshold implies not hashing anything",
			    __progname);
		exit(status_t::STATUS_USER_ERROR);
	    }
	    break;

	case 'p':
	    ocb.piecewise_size = find_block_size(optarg);
	    if (ocb.piecewise_size==0) {
		ocb.print_error("%s: Illegal size value for piecewise mode.", __progname);
		exit(status_t::STATUS_USER_ERROR);
	    }

	    break;

	case 'Z': ocb.mode_triage      = true; break;
	case 't': ocb.mode_timestamp   = true; break;
	case 'n': ocb.mode_not_matched = true; break;
	case 'w': ocb.opt_show_matched = true;break; 		// display which known hash generated match
	case 'j': ocb.opt_threadcount          = atoi(optarg);break;

	case 'a':
	    opt_mode_match=true;
	    md5deep_check_matching_modes();
	    md5deep_add_hash(optarg,optarg);
	    break;

	case 'A':
	    opt_mode_match_neg=true;
	    md5deep_check_matching_modes();
	    md5deep_add_hash(optarg,optarg);
	    break;

	case 'o': 
	    mode_expert=true; 
	    setup_expert_mode(optarg);
	    break;
      
	case 'M':
	    ocb.opt_display_hash=true;
	    /* Intentional fall through */
	case 'm':
	    opt_mode_match=true;
	    md5deep_check_matching_modes();
	    md5deep_load_match_file(optarg);
	    break;

	case 'X':
	    ocb.opt_display_hash=true;
	case 'x':
	    opt_mode_match_neg=true;
	    md5deep_check_matching_modes();
	    md5deep_load_match_file(optarg);
	    break;

	case 'c': opt_csv = true;		break;
	case 'z': ocb.opt_display_size = true;	break;
	case '0': ocb.opt_zero = true;	break;

	case 'S':
	    mode_warn_only=true;
	    opt_silent = true;
	    break;

	case 's': opt_silent = true;	break;
	case 'e': opt_estimate = true;	break;
	case 'r': mode_recursive = true; break;
	case 'k': opt_asterisk = true;      break;
	case 'b': ocb.mode_barename=true; break;
      
	case 'l': opt_relative = true;      break;
	case 'q': ocb.mode_quiet = true; break;
	case 'W': ocb.set_outfilename(optarg);	break;
	case 'u': opt_unicode_escape = 1;	break;

	case 'h':
	    md5deep_usage();
	    exit (EXIT_SUCCESS);

	case 'v':
	    ocb.status("%s",VERSION);
	    exit (EXIT_SUCCESS);

	case 'V':
	    // COPYRIGHT is a format string, complete with newlines
	    ocb.status(COPYRIGHT);
	    exit (EXIT_SUCCESS);

	default:
	    try_msg();
	    exit (status_t::STATUS_USER_ERROR);
	}
    }
    md5deep_check_flags_okay();
    return EXIT_SUCCESS;
}


/****************************************************************/
/* Make the UTF8 banner in case we need it 
 * Only hashdeep has a header.
 */
std::string state::make_banner()
{
    std::string utf8_banner;
    utf8_banner = HASHDEEP_HEADER_10 + std::string(NEWLINE);
    utf8_banner += HASHDEEP_PREFIX;
    utf8_banner += "size,";
    for (int i = 0 ; i < NUM_ALGORITHMS ; ++i) {
	if (hashes[i].inuse){
	    utf8_banner += hashes[i].name + std::string(",");
	}
    }  
    utf8_banner += std::string("filename") + NEWLINE;
    utf8_banner += "## Invoked from: " + main::make_utf8(main::getcwd()) + NEWLINE;
    utf8_banner += "## ";
#ifdef _WIN32
    utf8_banner += main::make_utf8(main::getcwd())[0] + ":\\>";
#else
    utf8_banner += (geteuid()==0) ? "#" : "$";
#endif

    // Accounts for '## ', command prompt, and space before first argument
    size_t bytes_written = 8;
	
    for (int largc = 0 ; largc < this->argc ; ++largc) {
	utf8_banner += " ";
	bytes_written++;
	
	// We are going to print the string. It's either ASCII or UTF16
	// convert it to a tstring and then to UTF8 string.
	tstring arg_t = tstring(this->argv[largc]);
	std::string arg_utf8 = main::make_utf8(arg_t);
	size_t current_bytes = arg_utf8.size();
    
	// The extra 32 bytes is a fudge factor
	if (current_bytes + bytes_written + 32 > MAX_STRING_LENGTH) {
	    utf8_banner += std::string(NEWLINE) + "## ";
	    bytes_written = 3;
	}
	utf8_banner += arg_utf8;
	bytes_written += current_bytes;
    }
    utf8_banner += std::string(NEWLINE) + "## " + NEWLINE;
    return utf8_banner;
    /****************************************************************/

}

uint64_t state::find_block_size(std::string input_str)
{
    if(input_str.size()==0) return 0;	// no input???
    uint64_t multiplier = 1;
    char last_char = input_str[input_str.size()-1];

    // There are deliberately no break statements in this switch
    switch (tolower(last_char)) {
    case 'e':
	multiplier *= 1024;    
    case 'p':
	multiplier *= 1024;    
    case 't':
	multiplier *= 1024;    
    case 'g':
	multiplier *= 1024;    
    case 'm':
	multiplier *= 1024;
    case 'k':
	multiplier *= 1024;
    case 'b':
	input_str.erase(input_str.size()-1,1); // erase the last character
	break;
    default:
	ocb.print_error("%s: Improper piecewise multiplier ignored", __progname);
	break;
    case '0':case '1':case '2':case '3':case '4':
    case '5':case '6':case '7':case '8':case '9':
	break;
    }
    
#ifdef __HPUX
    return (strtoumax ( input_str.c_str(), (char**)0, 10) * multiplier);
#else
    return (atoll(input_str.c_str()) * multiplier);
#endif
}



int main(int argc, char **argv)
{
    /* Because the main() function can handle wchar_t arguments on Win32,
     * we need a way to reference those values. Thus we make a duplciate
     * of the argc and argv values.
     */ 

#if !defined(HAVE_EXTERN_PROGNAME)
#if defined(HAVE_GETPROGNAME)
    __progname  = getprogname();
#else
    __progname  = basename(argv[0]);
#endif
#endif

    // Initialize the plugable algorithm system and create the state object!

    algorithm_t::load_hashing_algorithms();		
    state *s = new state();

    return s->main(argc,argv);
}

int state::main(int _argc,char **_argv)
{
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
	hashdeep_process_command_line(_argc,_argv);
    } else {
	algorithm_t::clear_algorithms_inuse();
	char buf[256];
	strcpy(buf,algname.c_str());
	algorithm_t::enable_hashing_algorithms(buf);
	for(int i=0;i<NUM_ALGORITHMS;++i){
	    if(hashes[i].inuse){
		md5deep_mode = true;
		opt_md5deep_mode_algorithm = hashes[i].id;
		break;
	    }
	}
	if(!md5deep_mode){
	    cerr << progname << ": unknown hash: " <<algname << "\n";
	    exit(1);
	}
	md5deep_process_command_line(_argc,_argv);
    }

    if(opt_debug==1){
	printf("self-test...\n");
	state::dig_self_test();
    }

    /* See if we can open a regular file output, if requested */

    /* Set up the DFXML output if requested */
    ocb.dfxml_startup(_argc,_argv);
   
#ifdef _WIN32
    if (prepare_windows_command_line()){
	ocb.fatal_error("%s: Unable to process command line arguments", __progname);
    }
    check_wow64();
#else
    argc = _argc;
    argv = _argv;
#endif

    /* Verify that we can get the current working directory. */
    if(main::getcwd().size()==0){
	ocb.fatal_error("%s: %s", __progname, strerror(errno));
    }

    /* Make the banner if we are not in md5deep mode */
    if (!md5deep_mode){
	ocb.set_utf8_banner( make_banner());
    }

#ifdef HAVE_PTHREAD
    /* set up the threadpool */
    if(ocb.opt_threadcount>0){
	ocb.tp = new threadpool(ocb.opt_threadcount);
    }
#endif

    if(opt_debug>2){
	std::cout << "dump hashlist before matching:\n";
	ocb.dump_hashlist();
    }

    /*
     * Anything left on the command line at this point is a file
     * or directory we're supposed to process. If there's nothing
     * specified, we should hash standard input
     */
    
    if (optind == argc){
	ocb.hash_stdin();
    } else {
	for(int i=optind;i<argc;i++){
	    tstring fn = generate_filename(this->argv[i]);
#ifdef _WIN32
	    dig_win32(fn);
#else
	    dig_normal(fn);
#endif
	}
    }
  
    /* If we are multi-threading, wait for all threads to finish */
#ifdef HAVE_PTHREAD
    if(ocb.tp){
	while(ocb.tp->all_free()==false){
#ifdef HAVE_USLEEP
	    usleep(50);
#else	    
	    sleep(1);
#endif
	}
    }
#endif

    if(opt_debug>2){
	std::cout << "\ndump hashlist after matching:\n";
	ocb.dump_hashlist();
    }


    /* If we were auditing, display the audit results */
    if (ocb.primary_function == primary_audit){
	ocb.display_audit_results();
    }
  
    /* We only have to worry about checking for unused hashes if one 
     * of the matching modes was enabled. We let the display_not_matched
     * function determine if it needs to display anything. The function
     * also sets our return values in terms of inputs not being matched
     * or known hashes not being used
     */
    if (opt_mode_match || opt_mode_match_neg){
	ocb.finalize_matching();
    }

    /* If we were generating DFXML, finish the job */
    ocb.dfxml_shutdown();
    return ocb.get_return_code();
}

