/*
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
#include "utf8.h"

#include "md5.h"
#include "sha1.h"
#include "sha256.h"
//#include "sha3.h"
#include "tiger.h"
#include "whirlpool.h"

using namespace std;

std::string progname;

#define AUTHOR      "Jesse Kornblum and Simson Garfinkel"
#define COPYRIGHT   "This program is a work of the US Government. "\
"In accordance with 17 USC 105,\n"\
"copyright protection is not available for any work of the US Government.\n"\
"This is free software; see the source for copying conditions. There is NO\n"\
"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"


#ifdef _WIN32
// This can't go in main.h or we get multiple definitions of it
// Allows us to open standard input in binary mode by default
// See http://gnuwin32.sourceforge.net/compile.html for more
int _CRT_fmode = _O_BINARY;
#endif


/* The only remaining global options */
bool	md5deep_mode = false;
int	opt_debug = 0;			// debug mode; 1 is self-test
hashid_t  opt_md5deep_mode_algorithm = alg_unknown;


/****************************************************************
 ** Various helper functions.
 ****************************************************************/

uint64_t file_data_hasher_t::next_file_number = 0; // needs to live somewhere

/* This is the one place we allow a printf, becuase we are about to exit, and we call it before we multithread */
static void try_msg(void)
{
    std::cerr << "Try `" << progname << " -h` for more information." << std::endl;
}


void state::sanity_check(int condition, const char *msg)
{
  if (condition)
  {
    if (!ocb.opt_silent)
    {
      ocb.error("%s",msg);
      try_msg();
    }
    exit (status_t::STATUS_USER_ERROR);
  }
}

static int is_absolute_path(const tstring &fn)
{
#ifdef _WIN32
  return FALSE;
#endif
  return (fn.size()>0 && fn[0] == DIR_SEPARATOR);
}


/**
 * return the full pathname for a filename.
 */

tstring state::generate_filename(const tstring &input)
{
    if ((ocb.opt_relative) || is_absolute_path(input)){
	return tstring(input);
    }
    // Windows systems don't have symbolic links, so we don't
    // have to worry about carefully preserving the paths
    // they follow. Just use the system command to resolve the paths
    //
    // Actually, they can have symbolic links...
#ifdef _WIN32
    wchar_t fn[PATH_MAX];
    memset(fn,0,sizeof(fn));
    _wfullpath(fn,input.c_str(),PATH_MAX);
    return tstring(fn);
#else
    char buf[PATH_MAX+1];
    std::string cwd = global::getcwd();
    if (cwd=="") {
	// If we can't get the current working directory, we're not
	// going to be able to build the relative path to this file anyway.
	// So we just call realpath and make the best of things
	if (realpath(input.c_str(),buf)==0){
	    ocb.internal_error("Error calling realpath in generate_filename");
	}
	return string(buf);
    }
    return cwd + DIR_SEPARATOR + input;
#endif
}



// So that the usage message fits in a standard DOS window, this
// function should produce no more than 22 lines of text.
void state::hashdeep_usage()
{
  if (1 == usage_count)
  {
    ocb.status("%s version %s by %s.",progname.c_str(),VERSION,AUTHOR);
    ocb.status("%s %s [OPTION]... [FILES]...",CMD_PROMPT,progname.c_str());

    // Make a list of the hashes
    ocb.status("-c <alg1,[alg2]> - Compute hashes only. Defaults are MD5 and SHA-256");
    fprintf(stdout,"                   legal values: ");
    for (int i = 0 ; i < NUM_ALGORITHMS ; i++)
    {
      fprintf(stdout,"%s%s",hashes[i].name.c_str(),(i+1<NUM_ALGORITHMS) ? "," : NEWLINE);
    }

    ocb.status("-p <size> - piecewise mode. Files are broken into blocks for hashing");
    ocb.status("-r        - recursive mode. All subdirectories are traversed");
    ocb.status("-d        - output in DFXML (Digital Forensics XML)");
    ocb.status("-k <file> - add a file of known hashes");
    ocb.status("-a        - audit mode. Validates FILES against known hashes. Requires -k");
    ocb.status("-m        - matching mode. Requires -k");
    ocb.status("-x        - negative matching mode. Requires -k");
    ocb.status("-w        - in -m mode, displays which known file was matched");
    ocb.status("-M and -X act like -m and -x, but display hashes of matching files");
    ocb.status("-e        - compute estimated time remaining for each file");
    ocb.status("-s        - silent mode. Suppress all error messages");
    ocb.status("-b        - prints only the bare name of files; all path information is omitted");
    ocb.status("-l        - print relative paths for filenames");
    ocb.status("-i/-I     - only process files smaller than the given threshold");
    ocb.status("-o        - only process certain types of files. See README/manpage");
    ocb.status("-v        - verbose mode. Use again to be more verbose");
    ocb.status("-d        - output in DFXML; -W FILE - write to FILE.");
#ifdef HAVE_PTHREAD
    ocb.status("-j <num>  - use num threads (default %d)",threadpool::numCPU());
#else
    ocb.status("-j <num>  - ignored (compiled without pthreads)");
#endif
  }

  // -hh makes us more verbose
  if (2 == usage_count)
  {
    ocb.status("-f <file> - Use file as a list of files to process.");
    ocb.status("-V        - display version number and exit");
    ocb.status("-0        - use a NUL (\\0) for newline.");
    ocb.status("-u        - escape Unicode");
    ocb.status("-E        - Use case insensitive matching for filenames in audit mode");
    ocb.status("-B        - verbose mode; repeat for more verbosity");
    ocb.status("-C        - OS X only --- use Common Crypto hash functions");
    ocb.status("-Fb       - I/O mode buffered; -Fu unbuffered; -Fm memory-mapped");
    ocb.status("-o[bcpflsde] - Expert mode. only process certain types of files:");
    ocb.status("               b=block dev; c=character dev; p=named pipe");
    ocb.status("               f=regular file; l=symlink; s=socket; d=door e=Windows PE");
    ocb.status("-D <num>  - set debug level");
  }

  /// -hhh mode includes debugging information.
  if (3 == usage_count)
  {
    ocb.status("sizeof(off_t)= %d",sizeof(off_t));
#ifdef HAVE_PTHREAD
    ocb.status("HAVE_PTHREAD");
#endif
#ifdef HAVE_PTHREAD_H
    ocb.status("HAVE_PTHREAD_H");
#endif
#ifdef HAVE_PTHREAD_WIN32_PROCESS_ATTACH_NP
    ocb.status("HAVE_PTHREAD_WIN32_PROCESS_ATTACH_NP");
#endif
  }
}


// So that the usage message fits in a standard DOS window, this
// function should produce no more than 22 lines of text.
void state::md5deep_usage(void)
{
    if(usage_count==1){
	ocb.status("%s version %s by %s.",progname.c_str(),VERSION,AUTHOR);
	ocb.status("%s %s [OPTION]... [FILES]...",CMD_PROMPT,progname.c_str());
	ocb.status("See the man page or README.txt file or use -hh for the full list of options");
	ocb.status("-p <size> - piecewise mode. Files are broken into blocks for hashing");
	ocb.status("-r        - recursive mode. All subdirectories are traversed");
	ocb.status("-e        - show estimated time remaining for each file");
	ocb.status("-s        - silent mode. Suppress all error messages");
	ocb.status("-z        - display file size before hash");
	ocb.status("-m <file> - enables matching mode. See README/man page");
	ocb.status("-x <file> - enables negative matching mode. See README/man page");
	ocb.status("-M and -X are the same as -m and -x but also print hashes of each file");
	ocb.status("-w        - displays which known file generated a match");
	ocb.status("-n        - displays known hashes that did not match any input files");
	ocb.status("-a and -A add a single hash to the positive or negative matching set");
	ocb.status("-b        - prints only the bare name of files; all path information is omitted");
	ocb.status("-l        - print relative paths for filenames");
	ocb.status("-t        - print GMT timestamp (ctime)");
	ocb.status("-i/I <size> - only process files smaller/larger than SIZE");
	ocb.status("-v        - display version number and exit");
	ocb.status("-d        - output in DFXML; -u - Escape Unicode; -W FILE - write to FILE.");
#ifdef HAVE_PTHREAD
	ocb.status("-j <num>  - use num threads (default %d)",threadpool::numCPU());
#else
	ocb.status("-j <num>  - ignored (compiled without pthreads)");
#endif
	ocb.status("-Z - triage mode;   -h - help;   -hh - full help");
    }
    if(usage_count==2){			// -hh
	ocb.status("-S        - Silent mode, but warn on bad hashes");
	ocb.status("-0        - use a NUL (\\0) for newline.");
	ocb.status("-k        - print asterisk before filename");
	ocb.status("-u        - escape Unicode characters in filenames");
	ocb.status("-B        - verbose mode; repeat for more verbosity");
	ocb.status("-C        - OS X only --- use Common Crypto hash functions");
	ocb.status("-Fb       - I/O mode buffered; -Fu unbuffered; -Fm memory-mapped");
	ocb.status("-f <file> - take list of files to hash from filename");
	ocb.status("-o[bcpflsde] - expert mode. Only process certain types of files:");
	ocb.status("               b=block dev; c=character dev; p=named pipe");
	ocb.status("               f=regular file; l=symlink; s=socket; d=door e=Windows PE");
	ocb.status("-D <num>  - set debug level to nn");
    }
    if (usage_count==3){			// -hhh
	ocb.status("sizeof(off_t)= %d",sizeof(off_t));
#ifdef HAVE_PTHREAD
	ocb.status("HAVE_PTHREAD");
#endif
#ifdef HAVE_PTHREAD_H
	ocb.status("HAVE_PTHREAD_H");
#endif
#ifdef HAVE_PTHREAD_WIN32_PROCESS_ATTACH_NP
	ocb.status("HAVE_PTHREAD_WIN32_PROCESS_ATTACH_NP");
#endif
    }
}


void state::hashdeep_check_flags_okay()
{
  sanity_check(
	       (((ocb.primary_function & primary_match) ||
		 (ocb.primary_function & primary_match_neg) ||
		 (ocb.primary_function & primary_audit)) &&
		!hashes_loaded()),
	       "Unable to load any matching files.");

  sanity_check(
	       (ocb.opt_relative) && (ocb.mode_barename),
	       "Relative paths and bare filenames are mutally exclusive.");

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
	      void ( *func_init)(void *ctx),
	      void ( *func_update)(void *ctx, const unsigned char *buf, size_t buflen),
	      void ( *func_finalize)(void *ctx, unsigned char *),
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


#ifdef POLARSSL_SHA1_H
void hash_init_sha1(void * ctx)
{
    assert(sizeof(sha1_context) < MAX_ALGORITHM_CONTEXT_SIZE);
    sha1_starts((sha1_context *)ctx);
}

void hash_update_sha1(void * ctx, const unsigned char *buf, size_t len)
{
    sha1_update((sha1_context *)ctx,buf,len);
}

void hash_final_sha1(void * ctx, unsigned char *sum)
{
    sha1_finish((sha1_context *)ctx,sum);
}
#endif

#if defined(HAVE_COMMONCRYPTO_COMMONDIGEST_H)
#include <CommonCrypto/CommonDigest.h>
#endif

bool opt_enable_mac_cc=false;	// enable mac common crypto

#ifdef HAVE_CC_SHA1_INIT
/* These are to overcome C++ cast issues */
void cc_md5_init(void * ctx)
{
    if(opt_enable_mac_cc){
	CC_MD5_Init((CC_MD5_CTX *)ctx);
    } else {
	hash_init_md5(ctx);
    }
}

void cc_sha1_init(void * ctx)
{
    if(opt_enable_mac_cc){
	CC_SHA1_Init((CC_SHA1_CTX *)ctx);
    } else {
	hash_init_sha1(ctx);
    }
}

void cc_sha256_init(void * ctx)
{
    if(opt_enable_mac_cc){
	CC_SHA256_Init((CC_SHA256_CTX *)ctx);
    } else {
	hash_init_sha256(ctx);
    }
}

void cc_md5_update(void *ctx, const unsigned char *buf, size_t len)
{
    if(opt_enable_mac_cc){
	CC_MD5_Update((CC_MD5_CTX *)ctx,buf,len);
    } else {
	hash_update_md5(ctx,buf,len);
    }
}

void cc_sha1_update(void *ctx, const unsigned char *buf, size_t len)
{
    if(opt_enable_mac_cc){
	CC_SHA1_Update((CC_SHA1_CTX *)ctx,buf,len);
    } else {
	hash_update_sha1(ctx,buf,len);
    }
}

void cc_sha256_update(void *ctx, const unsigned char *buf, size_t len)
{
    if(opt_enable_mac_cc){
	CC_SHA256_Update((CC_SHA256_CTX *)ctx,buf,len);
    } else {
	hash_update_sha256(ctx,buf,len);
    }
}

/* These swap argument orders, which are different for Apple and our implementation */
void cc_md5_final(void *ctx, unsigned char *digest)
{
    if(opt_enable_mac_cc){
	CC_MD5_Final(digest,(CC_MD5_CTX *)ctx);
    } else {
	hash_final_md5(ctx,digest);
    }
}

void cc_sha1_final(void *ctx, unsigned char *digest)
{
    if(opt_enable_mac_cc){
	CC_SHA1_Final(digest,(CC_SHA1_CTX *)ctx);
    } else {
	hash_final_sha1(ctx,digest);
    }
}

void cc_sha256_final(void *ctx, unsigned char *digest)
{
    if(opt_enable_mac_cc){
	CC_SHA256_Final(digest,(CC_SHA256_CTX *)ctx);
    } else {
	hash_final_sha256(ctx,digest);
    }
}
#endif



/*
 * Load the hashing algorithms array.
 */
void algorithm_t::load_hashing_algorithms()
{
    /* The DEFAULT_ENABLE variables are in main.h */
#if defined(HAVE_CC_SHA1_INIT)
    /* Use the Apple's validated Common Crypto for SHA1 and SHA256 */
    assert(sizeof(struct CC_MD5state_st)<MAX_ALGORITHM_CONTEXT_SIZE);
    assert(sizeof(struct CC_SHA1state_st)<MAX_ALGORITHM_CONTEXT_SIZE);
    assert(sizeof(struct CC_SHA256state_st)<MAX_ALGORITHM_CONTEXT_SIZE);
    add_algorithm(alg_md5,       "md5",       128, cc_md5_init,         cc_md5_update,         cc_md5_final,         DEFAULT_ENABLE_MD5);
    add_algorithm(alg_sha1,      "sha1",      160, cc_sha1_init,        cc_sha1_update,        cc_sha1_final,        DEFAULT_ENABLE_SHA1);
    add_algorithm(alg_sha256,    "sha256",    256, cc_sha256_init,      cc_sha256_update,      cc_sha256_final,      DEFAULT_ENABLE_SHA256);
#else
    add_algorithm(alg_md5,       "md5",       128, hash_init_md5,       hash_update_md5,       hash_final_md5,       DEFAULT_ENABLE_MD5);
    add_algorithm(alg_sha1,      "sha1",      160, hash_init_sha1,      hash_update_sha1,      hash_final_sha1,      DEFAULT_ENABLE_SHA1);
    add_algorithm(alg_sha256,    "sha256",    256, hash_init_sha256,    hash_update_sha256,    hash_final_sha256,    DEFAULT_ENABLE_SHA256);
#endif
    add_algorithm(alg_tiger,     "tiger",     192, hash_init_tiger,     hash_update_tiger,     hash_final_tiger,     DEFAULT_ENABLE_TIGER);
    add_algorithm(alg_whirlpool, "whirlpool", 512, hash_init_whirlpool, hash_update_whirlpool, hash_final_whirlpool, DEFAULT_ENABLE_WHIRLPOOL);

    //add_algorithm(alg_sha3,
    //		  "sha3",
    //256,
    //hash_init_sha3,
    //hash_update_sha3,
    //hash_final_sha3,
    //	  DEFAULT_ENABLE_SHA3);
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


bool algorithm_t::valid_hex(const std::string &buf)
{
    for(std::string::const_iterator it = buf.begin(); it!=buf.end(); it++){
	if(!isxdigit(*it)) return false;
    }
    return true;
}

bool algorithm_t::valid_hash(hashid_t alg, const std::string &buf)
{
    for (size_t pos = 0 ; pos < hashes[alg].bit_length/4 ; pos++)  {
	if (!isxdigit(buf[pos])) return false; // invalid character
	if (pos==(hashes[alg].bit_length/4)-1) return true; // we found them all
    }
    return false;				// too short or too long
}


int algorithm_t::algorithms_in_use_count()
{
    int count = 0;
    for (int i = 0 ; i < NUM_ALGORITHMS ; ++i)  {
	if(hashes[i].inuse) count++;
    }
    return count;
}


// C++ string splitting code from
// http://stackoverflow.com/questions/236129/how-to-split-a-string-in-c
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


//
// Set inuse for each of the algorithms in the argument.
//
void algorithm_t::enable_hashing_algorithms(std::string var)
{
  // convert name to lowercase and remove any dashes
  std::transform(var.begin(), var.end(), var.begin(), ::tolower);

  // Split on the commas
  std::vector<std::string>algs = split(var,',');

  for (std::vector<std::string>::const_iterator it = algs.begin();it!=algs.end();it++)
  {
    hashid_t id = get_hashid_for_name(*it);
    if (id==alg_unknown)
    {
      // Did the user specify to compute all hash algorithms?
      if (*it == "all")
      {
	for (int j=0 ; j<NUM_ALGORITHMS ; j++)
	{
	  hashes[j].inuse = TRUE;
	}
	return;
      }

      // No idea what this algorithm is.
      fprintf(stderr,
	      "%s: Unknown algorithm: %s%s",
	      progname.c_str(),
	      (*it).c_str(),
	      NEWLINE);
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
	case 'e': // Windows PE executables
	  mode_winpe = true;     break;
	case 'b': // Block Device
	  mode_block = true;     break;
	case 'c': // Character Device
	  mode_character = true; break;
	case 'p': // Named Pipe
	  mode_pipe=true;        break;
	case 'f': // Regular File
	  mode_regular=true;     break;
	case 'l': // Symbolic Link
	  mode_symlink=true;     break;
	case 's': // Socket
	  mode_socket=true;      break;
	case 'd': // Door (Solaris)
	  mode_door=true;        break;
	default:
	  ocb.error("%s: Unrecognized file type: %c", progname.c_str(),arg[i]);
	}
    }
}

void state::hashdeep_check_matching_modes()
{
  sanity_check((not (primary_compute == ocb.primary_function)),
	       "Multiple processing modes specified.");
}



int state::hashdeep_process_command_line(int argc_, char **argv_)
{
    bool did_usage = false;
  int i;

  while ((i=getopt(argc_,argv_,"abc:CdeEF:f:o:I:i:MmXxtlk:rsp:wvVhW:0D:uj:")) != -1)  {
    switch (i)
    {
    case 'a':
      hashdeep_check_matching_modes();
      ocb.primary_function = primary_audit;
      break;

    case 'C':
      opt_enable_mac_cc = true;
      break;

    case 'd':
      ocb.xml_open(stdout);
      break;

    case 'f':
      opt_input_list = optarg;
      break;

    case 'o':
      mode_expert=true;
      setup_expert_mode(optarg);
      break;

    case 'I':
      ocb.mode_size_all=true;
      // falls through
    case 'i':
      ocb.mode_size = true;
      ocb.size_threshold = find_block_size(optarg);
      if (ocb.size_threshold==0)
      {
	ocb.error("Requested size threshold implies not hashing anything");
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

    case 'M':
      ocb.opt_display_hash = true;
      // falls through
    case 'm':
      hashdeep_check_matching_modes();
      ocb.primary_function = primary_match;
      break;

    case 'X':
      ocb.opt_display_hash=true;
      // falls through
    case 'x':
      hashdeep_check_matching_modes();
      ocb.primary_function = primary_match_neg;
      break;

      // TODO: Add -t mode to hashdeep
      //    case 't': mode |= mode_timestamp;    break;

    case 'b': ocb.mode_barename=true;   break;
    case 'l': ocb.opt_relative=true;    break;
    case 'e': ocb.opt_estimate = true;	break;
    case 'r': mode_recursive=true;	break;
    case 's': ocb.opt_silent = true;	break;


    case 'p':
	ocb.piecewise_size = find_block_size(optarg);
      if (ocb.piecewise_size==0)
	  ocb.fatal_error("Piecewise blocks of zero bytes are impossible");

      break;

    case 'w': ocb.opt_show_matched = true;    break; // displays which known hash generated a match

    case 'k':
	switch (ocb.load_hash_file(optarg)) {
	case hashlist::loadstatus_ok:
	    if(opt_debug){
		ocb.error("%s: Match file loaded %d known hash values.",
				optarg,ocb.known_size());
	    }
	    break;

      case hashlist::status_contains_no_hashes:
	  /* Trying to load an empty file is fine, but we shouldn't
	     change hashes_loaded */
	  break;

      case hashlist::status_contains_bad_hashes:
	  ocb.error("%s: contains some bad hashes, using anyway",optarg);
	  break;

      case hashlist::status_unknown_filetype:
      case hashlist::status_file_error:
	  /* The loading code has already printed an error */
	    break;

	default:
	    ocb.error("%s: unknown error, skipping%s", optarg, NEWLINE);
	  break;
	}
      break;

    case 'v':
      ++ocb.opt_verbose;
      if (ocb.opt_verbose > INSANELY_VERBOSE)
	ocb.error("User request for insane verbosity denied");
      break;

    case 'V':
      ocb.status("%s", VERSION);
      exit(EXIT_SUCCESS);

    case 'W': ocb.set_outfilename(optarg); break;
    case '0': ocb.opt_zero = true; break;
    case 'u': ocb.opt_unicode_escape = true;break;
    case 'j': ocb.opt_threadcount = atoi(optarg); break;
    case 'F': ocb.opt_iomode = iomode::toiomode(optarg);break;
    case 'E': ocb.opt_case_sensitive = false; break;

    case 'h':
	usage_count++;
	hashdeep_usage();
	did_usage = true;
	break;

    case 'D': opt_debug = atoi(optarg); break;
    default:
      try_msg();
      exit(EXIT_FAILURE);
    }
  }

  if(did_usage ) exit(EXIT_SUCCESS);

  hashdeep_check_flags_okay();
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

std::string global::escape_utf8(const std::string &utf8)
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
std::string global::make_utf8(const tstring &str)
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


tstring global::getcwd()
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


#if 0
// See http://insanecoding.blogspot.com/2007/11/pathmax-simply-isnt.html
tstring global::getcwd()
{
    std::string path;
    typedef std::pair<dev_t, ino_t> file_id;

    bool success = false;
    int start_fd = open(".", O_RDONLY); //Keep track of start directory, so can jump back to it later
    if (start_fd == -1) {
	fprintf(stderr,"global::getcwd(): Cannot open '.': %s\n",strerror(errno));
	exit(1);
    }
    struct stat sb;
    if (fstat(start_fd, &sb)==0) {
	file_id current_id(sb.st_dev, sb.st_ino);
	if (!stat("/", &sb)){ //Get info for root directory, so we can determine when we hit it
	    std::vector<std::string> path_components;
	    file_id root_id(sb.st_dev, sb.st_ino);

	    // while we are not at the root, keep going up...
	    while (current_id != root_id){
		bool pushed = false;
		if (!chdir("..")){ 		    //Keep recursing towards root each iteration
		    DIR *dir = opendir(".");
		    if (dir) {
			dirent *entry;
			while ((entry = readdir(dir))){
			    //We loop through each entry trying to find where we came from
			    if (strcmp(entry->d_name,".")==0) continue; // ignore .
			    if (strcmp(entry->d_name,"..")==0) continue;
			    if (lstat(entry->d_name, &sb)==0){
				file_id child_id(sb.st_dev, sb.st_ino);
				if (child_id == current_id){
				    //We found where we came from, add its name to the list
				    path_components.push_back(entry->d_name);
				    pushed = true;
				    break;
				}
			    }
			}
			closedir(dir);
			if (pushed && !stat(".", &sb)){
			    //If we have a reason to continue, we update the current dir id
			    current_id = file_id(sb.st_dev, sb.st_ino);
			}
		    } //Else, Uh oh, can't read information at this level
		}
		if (!pushed) { break; } //If we didn't obtain any info this pass, no reason to continue
	    }

	    if (current_id == root_id){
		//Unless they're equal, we failed above
		//Built the path, will always end with a slash
		path = "/";
		for (std::vector<std::string>::reverse_iterator i = path_components.rbegin();
		     i != path_components.rend();
		     ++i){
		    path += *i+"/";
		}
		success = true;
	    }
	    fchdir(start_fd);
	}
    }
    close(start_fd);
    return path;
}
#endif



/* Return the canonicalized absolute pathname in UTF-8 on Windows and POSIX systems */
tstring global::get_realpath(const tstring &fn)
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
    if(opt_debug) std::cout << "global::get_realpath(" << fn << ")=" << resolved_name << "\n";
    return tstring(resolved_name);
#endif
}

std::string global::get_realpath8(const tstring &fn)
{
    return global::make_utf8(global::get_realpath(fn));
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
	ocb.error("WARNING: You are running a 32-bit program on a 64-bit system.");
	ocb.error("You probably want to use the 64-bit version of this program.");
    }
}
#endif   // ifdef _WIN32


/****************************************************************
 * Legacy code from md5deep follows....
 *
 ****************************************************************/


void state::md5deep_check_flags_okay()
{
  sanity_check(((ocb.opt_mode_match) || (ocb.opt_mode_match_neg)) &&
	       hashes_loaded()==0,
	       "Unable to load any matching files.");

  sanity_check((ocb.opt_relative) && (ocb.mode_barename),
	       "Relative paths and bare filenames are mutally exclusive.");

  sanity_check((ocb.piecewise_size>0) && (ocb.opt_display_size),
	       "Piecewise mode and file size display is just plain silly.");


  /* If we try to display non-matching files but haven't initialized the
     list of matching files in the first place, bad things will happen. */
  sanity_check((ocb.mode_not_matched) &&
	       ! ((ocb.opt_mode_match) || (ocb.opt_mode_match_neg)),
	       "Matching or negative matching must be enabled to display non-matching files.");

  sanity_check(ocb.opt_show_matched &&
	       ! ((ocb.opt_mode_match) || (ocb.opt_mode_match_neg)),
	       "Matching or negative matching must be enabled to display which file matched.");
}


void state::md5deep_check_matching_modes()
{
    sanity_check((ocb.opt_mode_match) && (ocb.opt_mode_match_neg),
		 "Regular and negative matching are mutually exclusive.");
}


int state::md5deep_process_command_line(int argc_, char **argv_)
{
    bool did_usage = false;
    int i;

    while ((i = getopt(argc_,
		       argv_,
		       "A:a:bcCdeF:f:I:i:M:X:x:m:o:tnwzsSp:rhvV0lkqZW:D:uj:")) != -1) {
	switch (i) {
	case 'C': opt_enable_mac_cc = true; break;
	case 'D': opt_debug = atoi(optarg);	break;
	case 'd': ocb.xml_open(stdout);		break;
	case 'f': opt_input_list = optarg;	break;
	case 'I':
	  // RBF - Document -I mode for hashdeep man page
	    ocb.mode_size_all=true;
	    // falls through
	case 'i':
	    ocb.mode_size=true;
	    ocb.size_threshold = find_block_size(optarg);
	    if (ocb.size_threshold==0) {
		ocb.error("Requested size threshold implies not hashing anything.");
		exit(status_t::STATUS_USER_ERROR);
	    }
	    break;

	case 'p':
	    ocb.piecewise_size = find_block_size(optarg);
	    if (ocb.piecewise_size==0) {
		ocb.error("Illegal size value for piecewise mode.");
		exit(status_t::STATUS_USER_ERROR);
	    }

	    break;

	case 'Z': ocb.mode_triage	= true;		break;
	case 't': ocb.mode_timestamp	= true;		break;
	case 'n': ocb.mode_not_matched	= true;		break;
	case 'w': ocb.opt_show_matched	= true;		break; 	// display which known hash generated match
	case 'j': ocb.opt_threadcount	= atoi(optarg);	break;
	case 'F': ocb.opt_iomode	= iomode::toiomode(optarg);break;

	case 'a':
	    ocb.opt_mode_match=true;
	    md5deep_check_matching_modes();
	    md5deep_add_hash(optarg,optarg);
	    break;

	case 'A':
	    ocb.opt_mode_match_neg=true;
	    md5deep_check_matching_modes();
	    md5deep_add_hash(optarg,optarg);
	    break;

	case 'o':
	    mode_expert=true;
	    setup_expert_mode(optarg);
	    break;

	case 'M':			// match mode
	    ocb.opt_display_hash=true;
	    /* Intentional fall through */
	case 'm':
	    ocb.opt_mode_match=true;
	    md5deep_check_matching_modes();
	    md5deep_load_match_file(optarg);
	    break;

	case 'X':
	    ocb.opt_display_hash=true;
	case 'x':
	    ocb.opt_mode_match_neg=true;
	    md5deep_check_matching_modes();
	    md5deep_load_match_file(optarg);
	    break;

	case 'c': ocb.opt_csv = true;		break;
	case 'z': ocb.opt_display_size = true;	break;
	case '0': ocb.opt_zero = true;		break;

	case 'S':
	    mode_warn_only = true;
	    ocb.opt_silent = true;
	    break;

	case 's': ocb.opt_silent = true;	break;
	case 'e': ocb.opt_estimate = true;	break;
	case 'r': mode_recursive = true;	break;
	case 'k': ocb.opt_asterisk = true;      break;
	case 'b': ocb.mode_barename=true;	break;

	case 'l': ocb.opt_relative = true;      break;
	case 'q': ocb.mode_quiet = true;	break;
	case 'W': ocb.set_outfilename(optarg);	break;
	case 'u': ocb.opt_unicode_escape = true;break;

	case 'h':
	usage_count++;
	    md5deep_usage();
	    did_usage = true;
	    break;

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
    if(did_usage) exit (EXIT_SUCCESS);

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
    utf8_banner += "## Invoked from: " + global::make_utf8(global::getcwd()) + NEWLINE;
    utf8_banner += "## ";
#ifdef _WIN32
    std::wstring cwd = global::getcwd();
    std::string  cwd8 = global::make_utf8(cwd);

    utf8_banner += cwd8 + ">";
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
	std::string arg_utf8 = global::make_utf8(arg_t);
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

    // All cases fall through in this switch statement
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
	ocb.error("Improper piecewise multiplier ignored.");
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
  // Because the main() function can handle wchar_t arguments on Win32,
  // we need a way to reference those values. Thus we make a duplciate
  // of the argc and argv values.

  // Initialize the plugable algorithm system and create the state object!

  // Be sure that we were compiled correctly
  assert(sizeof(off_t)==8);

  algorithm_t::load_hashing_algorithms();

  state *s = new state();
  exit(s->main(argc,argv));
}


int state::main(int _argc,char **_argv)
{
    /**
     * Originally this program was two sets of progarms:
     * 'hashdeep' with the new interface, and 'md5deep', 'sha1deep', etc
     * with the old interface. Now we are a single program and we figure out
     * which interface to use based on how we are started.
     */

    /* Get the program name */
    progname = _argv[0];		// default
#ifdef HAVE_GETPROGNAME
    progname = getprogname();		// possibly better
#endif
#ifdef HAVE_PROGRAM_INVOCATION_NAME
    progname = program_invocation_name;	// possibly better
#endif

#ifdef HAVE_PTHREAD
    threadpool::win32_init();			//
    ocb.opt_threadcount = threadpool::numCPU(); // be sure it's set
#endif

    /* There are two versions of basename, so use our own */
    size_t delim = progname.rfind(DIR_SEPARATOR);
    if(delim!=std::string::npos) progname.erase(0,delim+1);

    // Convert progname to lower case
    std::transform(progname.begin(), progname.end(), progname.begin(), ::tolower);
    std::string algname = progname.substr(0,progname.find("deep"));

    if (algname=="hash")
    {
      // We were called as "hashdeep"
      hashdeep_process_command_line(_argc,_argv);
    }
    else
    {
      // We were called as "[somethingelse]deep". Figure out which
      // algorithm and if we support that something else

      algorithm_t::clear_algorithms_inuse();
      char buf[256];
      strcpy(buf,algname.c_str());
      algorithm_t::enable_hashing_algorithms(buf);
      for (int i=0;i<NUM_ALGORITHMS;++i)
      {
	if (hashes[i].inuse)
	{
	  md5deep_mode = true;
	  opt_md5deep_mode_algorithm = hashes[i].id;
	  break;
	}
      }

      if (not md5deep_mode)
      {
	cerr << progname << ": unknown hash: " <<algname << "\n";
	exit(1);
      }
      md5deep_process_command_line(_argc,_argv);
    }

    if (opt_debug==1)
    {
      printf("self-test...\n");
      state::dig_self_test();
    }


    // See if we can open a regular file output, if requested
    // Set up the DFXML output if requested
    ocb.dfxml_startup(_argc,_argv);

#ifdef _WIN32
    if (prepare_windows_command_line()){
	ocb.fatal_error("Unable to process command line arguments");
    }
    check_wow64();
#else
    argc = _argc;
    argv = _argv;
#endif

    /* Verify that we can get the current working directory. */
    if(global::getcwd().size()==0){
	ocb.fatal_error("%s", strerror(errno));
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

    /* If we were given an input list, process it */
    if(opt_input_list!=""){
	std::ifstream in;
	in.open(opt_input_list.c_str());
	if(!in.is_open()){
	    std::cerr << "Cannot open " << opt_input_list << ": " << strerror(errno) << "\n";
	    exit(1);
	}
	while(!in.eof()){
	    std::string line;
	    std::getline(in,line);
	    /* Remove any possible \r\n or \n */
	    if(line.size()>0 && line[line.size()-1]=='\n') line.erase(line.size()-1);
	    if(line.size()>0 && line[line.size()-1]=='\r') line.erase(line.size()-1);
	    if(line.size()==0) continue;
	    /* If we are running on Windows, turn it into a UTF-16 filename */
#ifdef _WIN32
	    /* I think that this will work, but it needs to be tested */
	    std::wstring wstr;
	    utf8::utf8to16(line.begin(),line.end(),back_inserter(wstr));
	    dig_win32(wstr);
#else
	    dig_normal(line.c_str());
#endif
	}
	in.close();
    }

    /*
     * Anything left on the command line at this point is a file
     * or directory we're supposed to process. If there's nothing
     * specified, we should hash standard input
     */

    if (optind == argc && opt_input_list==""){
	if(ocb.mode_triage){
	    ocb.fatal_error("Processing stdin not supported in Triage mode");
	}
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
    if(ocb.tp) ocb.tp->wait_till_all_free();
#endif

    if (opt_debug>2)
    {
      std::cout << "\ndump hashlist after matching:\n";
      ocb.dump_hashlist();
    }

    // If we were auditing, display the audit results
    if (ocb.primary_function == primary_audit)
    {
      ocb.display_audit_results();
    }

    /* We only have to worry about checking for unused hashes if one
     * of the matching modes was enabled. We let the display_not_matched
     * function determine if it needs to display anything. The function
     * also sets our return values in terms of inputs not being matched
     * or known hashes not being used
     */
    if (ocb.opt_mode_match or
	ocb.opt_mode_match_neg or
	(primary_match == ocb.primary_function) or
	(primary_match_neg == ocb.primary_function))
    {
      ocb.finalize_matching();
    }

    /* If we were generating DFXML, finish the job */
    if(opt_debug>1) std::cerr << "*** main calling dfxml_shutdown\n";
    ocb.dfxml_shutdown();

    /* On windows, do a hard exit
     *
     * "If one of the terminated threads in the process holds a lock
     * and the DLL detach code in one of the loaded DLLs attempts to
     * acquire the same lock, then calling ExitProcess results in a
     * deadlock. In contrast, if a process terminates by calling
     * TerminateProcess, the DLLs that the process is attached to are
     * not notified of the process termination. Therefore, if you do
     * not know the state of all threads in your process, it is better
     * to call TerminateProcess than ExitProcess. Note that returning
     * from the main function of an application results in a call to
     * ExitProcess."
     *
     * http://msdn.microsoft.com/en-us/library/ms682658(v=vs.85).aspx
     */
#if defined(_WIN32)
    TerminateProcess(GetCurrentProcess(),ocb.get_return_code());
#endif
    return ocb.get_return_code();
}

