/*
 * main.h:
 *
 * This is the main file included by all other modules in md5deep/hashdeep/etc.
 * 
 * It includes:
 * common.h - the common system include files
 * xml.h    - the C++ XML system.
 * hash function headers
 *
 * C++ STL stuff.
 *
 * It then creates all the C++ classes and structures used.
 *
 * $Id: main.h,v 1.14 2007/12/28 01:49:36 jessekornblum Exp $
 */


#ifndef __MAIN_H
#define __MAIN_H

#include "common.h"
#include "xml.h"

#ifdef HAVE_PTHREAD
#include "threadpool.h"
#endif

#include <map>
#include <vector>

#define VERBOSE		 1
#define MORE_VERBOSE	 2
#define INSANELY_VERBOSE 3

/* These describe the version of the file format being used, not
 *   the version of the program.
 */
#define HASHDEEP_PREFIX     "%%%% "
#define HASHDEEP_HEADER_10  "%%%% HASHDEEP-1.0"

/* HOW TO ADD A NEW HASHING ALGORITHM
  * Add a value for the algorithm to the hashid_t enumeration
  * Add the functions to compute the hashes. There should be three functions,
    an initialization route, an update routine, and a finalize routine.
    The convention, for an algorithm "foo", is 
    foo_init, foo_update, and foo_final. 
  * Add your new code to Makefile.am under hashdeep_SOURCES
  * Add a call to insert the algorithm in state::load_hashing_algorithms
  * See if you need to increase MAX_ALGORITHM_NAME_LENGTH or
    MAX_ALGORITHM_CONTEXT_SIZE for your algorithm.
  * Update the usage function and man page to include the function
  */

typedef enum { 
  alg_md5=0, 
  alg_sha1, 
  alg_sha256, 
  alg_tiger,
  alg_whirlpool, 
  
  /* alg_unknown must always be last in this list. It's used
     as a loop terminator in many functions. */
  alg_unknown 
} hashid_t;

#define NUM_ALGORITHMS  alg_unknown

/* Which ones are enabled by default */
#define DEFAULT_ENABLE_MD5         TRUE
#define DEFAULT_ENABLE_SHA1        FALSE
#define DEFAULT_ENABLE_SHA256      TRUE
#define DEFAULT_ENABLE_TIGER       FALSE
#define DEFAULT_ENABLE_WHIRLPOOL   FALSE

/* This class holds the information known about each hash algorithm.
 * It's sort of like the EVP system in OpenSSL.
 *
 * In version 3 the list of known hashes was stored here as well.
 * That has been moved to the hashlist database (further down).
 *
 * Right now we are using some global variables; the better way to do this
 * would be with a C++ singleton.
 *
 * Perhaps the correct way to do this would be a global C++ vector of objects?
 */
class algorithm_t {
public:
    bool		inuse;		// true if we are using this algorithm
    std::string		name;		// name of algorithm
    size_t		bit_length;	// 128 for MD5
    hashid_t		id;		// usually the position in the array...

    /* The hashing functions */
    int ( *f_init)(void *ctx);
    int ( *f_update)(void *ctx, const unsigned char *buf, size_t len );
    int ( *f_finalize)(void *ctx, unsigned char *);

    /* The methods */
    static void add_algorithm(hashid_t pos, const char *name, uint16_t bits, 
			      int ( *func_init)(void *ctx),
			      int ( *func_update)(void *ctx, const unsigned char *buf, size_t len ),
			      int ( *func_finalize)(void *ctx, unsigned char *),
			      int inuse);
    static void load_hashing_algorithms();
    static void clear_algorithms_inuse();
    static void enable_hashing_algorithms(std::string var);  // enable the algorithms in 'var'; var can be 'all'
    static hashid_t get_hashid_for_name(std::string name);   // return the hashid_t for 'name'
    static bool valid_hash(hashid_t alg,const std::string &buf); // returns true if buf is a valid hash for hashid_t a
};

extern algorithm_t     hashes[NUM_ALGORITHMS];		// which hash algorithms are available and in use



/** status_t describes exit codes for the program
 * 
 */
class status_t  {
private:
    int32_t code;
public:;
    status_t():code(0){};
    static const int32_t status_ok = EXIT_SUCCESS; // 0
    static const int32_t status_EXIT_FAILURE = EXIT_FAILURE;
    static const int32_t status_out_of_memory = -2;
    static const int32_t status_invalid_hash = -3;
    static const int32_t status_unknown_error = -4;
    static const int32_t status_omg_ponies = -5;

    // Return values for the program 
    // RBF - Document these return values for hashdeep
    // A successful run has these or'ed together
    static const int32_t STATUS_UNUSED_HASHES = 1;
    static const int32_t STATUS_INPUT_DID_NOT_MATCH = 2;
    static const int32_t STATUS_USER_ERROR = 64;
    static const int32_t STATUS_INTERNAL_ERROR = 128;
    void add(int32_t val){ code |= val; }
    void set(int32_t val){ code = val; }
    int32_t get_status(){ return code; }
    bool operator==(int32_t v){ return this->code==v; }
    bool operator!=(int32_t v){ return this->code!=v; }
};


#ifdef _WIN32
typedef __time64_t	timestamp_t;
typedef std::wstring	filename_t;
#else
typedef time_t		timestamp_t;
typedef std::string	filename_t;
#endif


/** file_data_t contains information about a file.
 * It can be created by hashing an actual file, or by reading a hash file a file of hashes. 
 * The object is simple so that the built in C++ shallow copy will make a proper copy of it.
 * Note that all hashes are currently stored as a hex string. That incurs a 2x memory overhead.
 * This will be changed.
 */
class file_data_t {
public:
    file_data_t():matched_file_number(0),timestamp(0),stat_bytes(0),file_bytes(0),read_offset(0),read_len(0) {
    };
    virtual ~file_data_t(){}		// required because we subclass

    std::string hash_hex[NUM_ALGORITHMS];	     // the hash in hex of the entire file
    std::string	hash512_hex[NUM_ALGORITHMS];	     // hash of the first 512 bytes, for partial matching
    std::string	file_name;		// just the file_name; native on POSIX; UTF-8 on Windows.

    uint64_t    matched_file_number;	 // file number that we matched.; 0 if no match
    timestamp_t	timestamp;

    // How many bytes (and megs) we think are in the file, via stat(2)
    // and how many bytes we've actually read in the file
    uint64_t    stat_bytes;		// how much stat returned
    uint64_t    file_bytes;		// in bytes (how many we actually read)
    //uint64_t    actual_bytes;	// how many we read.

    // where this segment was actually read
    uint64_t	read_offset;		// where the segment we read started
    uint64_t	read_len;		// how many bytes were read and hashed

    uint64_t	   stat_megs() const{
	return stat_bytes / ONE_MEGABYTE;
    }
};


/** file_data_hasher_t is a subclass of file_data_t.
 * It contains additional information necessary to actually hash a file.
 */
class file_data_hasher_t : public file_data_t {
private:
    static uint64_t next_file_number;
public:
    static const size_t MD5DEEP_IDEAL_BLOCK_SIZE = 8192;
    static const size_t MAX_ALGORITHM_CONTEXT_SIZE = 256;
    static const size_t MAX_ALGORITHM_RESIDUE_SIZE = 256;
    file_data_hasher_t(class display *ocb_):
	ocb(ocb_),			// where we put results
	handle(0),
	base(0),bounds(0),file_number(0),start_time(0),last_time(0){
	file_number = ++next_file_number;
    };
    virtual ~file_data_hasher_t(){
	if(handle){
	    fclose(handle);
	    handle = 0;
	}
    }

    /* The actual file to hash */
    filename_t file_name_to_hash;

    /* Where the results go */
    class display *ocb;
    
    /* The actual hashing */
    void multihash_initialize();
    void multihash_update(const unsigned char *buffer,size_t bufsize);
    void multihash_finalize();

    /* How we read the data */
    FILE        *handle;		// the file we are reading
    int		fd;			// fd used for mmap
    const uint8_t *base;		// base of mapped file
    size_t	bounds;			// size of the mapped file

    /* Information for the hashing underway */
    uint8_t     hash_context[NUM_ALGORITHMS][MAX_ALGORITHM_CONTEXT_SIZE];	 
    std::string	triage_info;		// if true, must print on output
    std::string	dfxml_hash;          // the DFXML hash digest for the piece just hashed;
					// used to build piecewise
    uint64_t	file_number;
    void	append_dfxml_for_byterun();
    void	compute_dfxml(bool known_hash);

    // Request of where to read the file (or segment)
    // Where it was actually read is put in read_start and read_len
    // bytes_read is a shorthand for read_end - read_start
    //uint64_t	request_start;
    //uint64_t	request_len;		// the maximum I wish to read
    //uint64_t  read_end;
    //uint64_t	bytes_read;

    /* Size of blocks used in normal hashing */
    //uint64_t	block_size;

    /* When we started the hashing, and when was the last time a display was printed? */
    time_t	start_time, last_time;


    /* multithreaded hash implementation is these functions in hash.cpp.
     * hash() is called to hash each file and record the results.
     * Return codes are now both stored in display return_code and returned
     * 0 - for success, -1 for error
     */
    // called to actually do the computation; returns true if successful
    bool compute_hash(uint64_t request_start,uint64_t request_len);
    void hash();	// called to hash each file and record results
};


/** The hashlist holds a list of file_data_t objects.
 * state->known is used to hold the audit file that is loaded.
 * state->seen is used to hold the hashes seen on the current run.
 * We store multiple maps for each algorithm number which map the hash hex code
 * to the pointer as well. 
 *
 * the hashlist.cpp file contains the implementation. It's largely taken
 * from the v3 audit.cpp and match.cpp files.
 */
class hashlist : public std::vector<file_data_t *> {
    /**
     * The largest number of columns we can expect in a file of hashes
     * (knowns).  Normally this should be the number of hash
     * algorithms plus a column for file size, file name, and, well,
     * some fudge factors. Any values after this number will be
     * ignored. For example, if the user invokes the program as:
     * 
     * hashdeep -c md5,md5,md5,md5,...,md5,md5,md5,md5,md5,md5,md5,whirlpool
     * 
     * the whirlpool will not be registered.
     */

public:;
    static const int MAX_KNOWN_COLUMNS= NUM_ALGORITHMS+ 6;
    typedef enum {
	/* return codes from loading a hash list */
	loadstatus_ok = 0,
	status_unknown_filetype,
	status_contains_bad_hashes,
	status_contains_no_hashes,
	status_file_error
    } loadstatus_t;
    
    typedef enum   {
	searchstatus_ok = 0,
	
	/* Matching hashes */
	status_match,			// all hashes match
	status_partial_match,	 /* One or more hashes match, but not all */
	status_file_size_mismatch,   /* Implies all hashes match */
	status_file_name_mismatch,   /* Implies all hashes and file size match */   
	status_no_match             /* Implies none of the hashes match */
    } searchstatus_t;
    static const char *searchstatus_to_str(searchstatus_t val);

    // Types of files that contain known hashes 
    typedef enum   {
	file_plain,
	file_bsd,
	file_hashkeeper,
	file_nsrl_15,
	file_nsrl_20,
	file_encase3,
	file_encase4,
	file_ilook,
	
	// Files generated by md5deep with the ten digit filesize at the start 
	// of each line
	file_md5deep_size,
	file_hashdeep_10,
	file_unknown
    } hashfile_format; 

    class hashmap : public  std::map<std::string,file_data_t *> {
    public:;
	void add_file(file_data_t *fi,int alg_num);
    };
    hashmap		hashmaps[NUM_ALGORITHMS];
    //
    // look up a fdt by hash code(s) and return if it is present or not.
    // optionally return a pointer to it as well.
    searchstatus_t	search(const file_data_hasher_t *fdht, file_data_t **matched) ; 

    uint64_t		total_matched(); // return the total matched

    /**
     * Figure out the format of a hashlist file and load it.
     * Both of these functions take the file name and the open handle.
     * They read from the handle and just use the filename for printing error messages.
     */
    void		enable_hashing_algorithms_from_hashdeep_file(class display *ocb,const std::string &fn,std::string val);

    std::string		last_enabled_algorithms; // a string with the algorithms that were enabled last
    hashid_t		hash_column[NUM_ALGORITHMS]; // maps a column number to a hashid;
						     // the order columns appear in the file being loaded.
    int			num_columns;		     // number of columns in file being loaded
    hashfile_format	identify_format(class display *ocb,const std::string &fn,FILE *handle);
    loadstatus_t	load_hash_file(class display *ocb,const std::string &fn); // not tstring! always ASCII

    const file_data_t	*find_hash(hashid_t alg,std::string &hash_hex,uint64_t file_number); 
    void		dump_hashlist(); // send contents to stdout
    
    /**
     * add_fdt adds a file_data_t record to the hashlist, and its hashes to the hashmaps.
     * @param fi - a file_data_t to add. Don't erase it; we're going to use it (and modify it)
     */
    void add_fdt(file_data_t *fi);
};

/* Primary modes of operation (primary_function) */
typedef enum  { 
  primary_compute=0, 
  primary_match=1, 
  primary_match_neg=2, 
  primary_audit=3				
} primary_t; 


// These are the types of files that we can match against 
#define TYPE_PLAIN        0
#define TYPE_BSD          1
#define TYPE_HASHKEEPER   2
#define TYPE_NSRL_15      3
#define TYPE_NSRL_20      4
#define TYPE_ILOOK        5
#define TYPE_ILOOK3       6
#define TYPE_ILOOK4       7
#define TYPE_MD5DEEP_SIZE 8
#define TYPE_ENCASE       9
#define TYPE_UNKNOWN    254

/* audit mode stats */
class audit_stats {
public:
    audit_stats():exact(0), expect(0), partial(0), moved(0), unused(0), unknown(0), total(0){
    };
    /* For audit mode, the number of each type of file */
    uint64_t	exact, expect, partial; // 
    uint64_t	moved, unused, unknown, total; //
    void clear(){
	exact = 0;
	expect = 0;
	partial = 0;
	moved = 0;
	unused = 0;
	unknown = 0;
	total = 0;
    }
};

/** display describes how information is output.
 * There is only one OCB (it is a singleton).
 * It needs to be mutex protected.
 *
 * The hashing happens in lots of threads and then calls the output
 * classes in output_control_block to actually do the outputing. The
 * problem here is that one of the things that is done is looking up,
 * so the searches into "known" and "seen" also need to be
 * protected. Hence "known" and "seen" appear in the
 * output_control_block, and not elsewhere, and all of the access to
 * them needs to be mediated.
 *
 * It also needs to maintain all of the stuff for audit mode.
 *
 * It is a class because it is protected and is passed around.
 */
class display {
 private:
#ifdef HAVE_PTHREAD
    mutable pthread_mutex_t	M;	// lock for anything in output section
#endif    
    void lock() const {
#ifdef HAVE_PTHREAD
	if(pthread_mutex_lock(&M)){
	    perror("pthread_mutex_lock failed");
	    exit(1);
	}
#endif
    }
    void unlock() const {
#ifdef HAVE_PTHREAD
	if(pthread_mutex_unlock(&M)){
	    perror("pthread_mutex_unlock failed");
	    exit(1);
	}
#endif
    }

    /* all display state variables are protected by M and must be private */
    std::ostream	*out;		// where things get sent
    std::ofstream       myoutstream;	// if we open it
    std::string		utf8_banner;	// banner to be displayed
    bool		banner_displayed;	// has the header been shown (text output)
    XML			*dfxml;			/* output in DFXML */

    /* The set of known values; typically read from the audit file */
    hashlist		known;		// hashes read from the -k file
    hashlist		seen;		// hashes seen on this hashing run; from the command line
    class audit_stats	match;		// for the audit mode
    status_t		return_code;	// prevously returned by hash() and dig().

public:
    display():
	out(&std::cout),
	banner_displayed(0),dfxml(0),
	mode_triage(false),
	mode_not_matched(false),mode_quiet(false),mode_timestamp(false),
	mode_barename(false),
	mode_size(false),mode_size_all(false),
	opt_silent(false),
	opt_zero(false),
	opt_display_size(false),
	opt_display_hash(false),
	opt_show_matched(false),
#ifdef HAVE_PTRHEAD
	opt_threadcount(threadpool::numCPU()),
	tp(0),
#else
	opt_threadcount(0),
#endif
	size_threshold(0),
	piecewise_size(0),	
	primary_function(primary_compute){

#ifdef HAVE_PTHREAD
	pthread_mutex_init(&M,NULL);
#endif	
    }
    ~display(){
#ifdef HAVE_PTHREAD
	pthread_mutex_destroy(&M);
#endif	
    }

    /* These variables are read-only after threading starts */
    bool	mode_triage;
    bool	mode_not_matched;
    bool	mode_quiet;
    bool	mode_timestamp;
    bool	mode_barename;
    bool	mode_size;
    bool	mode_size_all;
    std::string	opt_outfilename;
    bool	opt_silent;
    bool	opt_zero;
    bool	opt_display_size;
    bool	opt_display_hash;
    bool	opt_show_matched;
    int		opt_threadcount;

#ifdef HAVE_PTHREAD
    threadpool		*tp;
#endif

    // When only hashing files larger/smaller than a given threshold
    uint64_t        size_threshold;
    uint64_t        piecewise_size;    // non-zero for piecewise mode
    primary_t       primary_function;    /* what do we want to do? */


    /* Functions for working */

    void	set_outfilename(std::string outfilename);

    /* Return code support */
    int32_t	get_return_code(){ lock(); int ret = return_code.get_status(); unlock(); return ret; }
    void	set_return_code(status_t code){ lock(); return_code = code; unlock(); }
    void	set_return_code(int32_t code){ lock(); return_code.set(code); unlock(); }
    void	set_return_code_if_not_ok(status_t code){
	lock();
	if(code!=status_t::status_ok) return_code = code;
	unlock();
    }

    /* DFXML support */

    void	xml_open(FILE *out){
	lock();
	dfxml = new XML(out);
	unlock();
    }
    void dfxml_startup(int argc,char **argv);
    void dfxml_shutdown();
    void dfxml_write(file_data_hasher_t *fdht);


    /* Known hash database interface */
    /* Display the unused files and return the count */
    uint64_t	compute_unused(bool display,std::string annotation);
    void	set_utf8_banner(std::string utf8_banner_){
	utf8_banner = utf8_banner_;
    }

    void	try_msg(void);
    void	display_banner_if_needed();
    void	display_hash(file_data_hasher_t *fdht);
    void	display_hash_simple(file_data_hasher_t *fdt);

    /* The following routines are for printing and outputing filenames.
     * 
     * fmt_filename formats the filename.
     * On Windows this version outputs as UTF-8 unless unicode quoting is requested,
     * in which case Unicode characters are emited as U+xxxx.
     * For example, the Unicode smiley character ☺ is output as U+263A.
     *
     */
    std::string	fmt_size(const file_data_t *fdh) const;
    std::string fmt_filename(const std::string  &fn) const;
#ifdef _WIN32
    std::string fmt_filename(const std::wstring &fn) const;
#endif
    std::string fmt_filename(const file_data_t *fdt) const {
	return fmt_filename(fdt->file_name);
    }
    void	writeln(std::ostream *s,const std::string &str);    // writes a line with NEWLINE
    void	status(const char *fmt, ...);// Display an ordinary message with newline added
    void	error(const char *fmt, ...);// Display an error message if not in silent mode 
    void	fatal_error(const char *fmt, ...);// Display an error message if not in silent mode and exit
    void	internal_error(const char *fmt, ...);// Display an error message, ask user to contact the developer, 
    void	print_debug(const char *fmt, ...);
    void	print_error(const char *fmt, ...);// Display an error message if not in silent mode
    void	error_filename(const std::string &fn, const char *fmt, ...);
#ifdef _WIN32
    void	error_filename(const std::wstring &fn, const char *fmt, ...);
#endif

    /* these versions extract the filename and the annotation if it is present.
     */

    /* known hash database and realtime stats */
    hashlist::loadstatus_t load_hash_file(const std::string &fn){
	lock();
	hashlist::loadstatus_t ret = known.load_hash_file(this,fn);
	unlock();
	return ret;
    }
    uint64_t known_size() const {
	lock();
	uint64_t ret= known.size();
	unlock();
	return ret;
    }
    const file_data_t *find_hash(hashid_t alg,std::string &hash_hex,uint64_t file_number){
	lock();
	const file_data_t *ret = known.find_hash(alg,hash_hex,file_number);
	unlock();
	return ret;
    }
    void	clear_realtime_stats();
    void	display_realtime_stats(const file_data_hasher_t *fdht,time_t elapsed);
    bool	hashes_loaded() const{ lock(); bool ret = known.size()>0; unlock(); return ret; }
    void	add_fdt(file_data_t *fdt){ lock(); known.add_fdt(fdt); unlock(); }

    void	display_match_result(file_data_hasher_t *fdht);
    void	md5deep_display_match_result(file_data_hasher_t *fdht);
    void	md5deep_display_hash(file_data_hasher_t *fdht);

    /* audit mode */
    int		audit_update(file_data_hasher_t *fdt);
    int		audit_check();		// performs an audit; return 0 if pass, -1 if fail
    void	display_audit_results(); // sets return code if fails
    void	finalize_matching();

    /* hash.cpp: Actually trigger the hashing. */
    void	hash_file(const tstring &file_name);
    void	hash_stdin();
    void	dump_hashlist(){ lock(); known.dump_hashlist(); unlock(); }
};

/**
 * The 'state' class holds the state of the hashdeep/md5deep program.
 * This includes:
 * startup parameters
 * known - the list of hashes in the hash database.
 * seen  - the list of hashes that have been seen this time through.
 */


class main {
public:
    static tstring getcwd();			// returns the current directory
    static tstring get_realpath(const tstring &fn); // returns the full path
    static std::string get_realpath8(const tstring &fn);  // returns the full path in UTF-8
    static std::string escape_utf8(const std::string &fn); // turns "⦿" to "U+29BF"
#ifdef _WIN32
    static std::string make_utf8(const std::wstring &tfn) ;
#endif
    static std::string make_utf8(const std::string &tfn){return tfn;}
};

/* On Win32, allow output of wstr's by converting them to UTF-8 */
#ifdef _WIN32
inline std::ostream & operator <<(std::ostream &os,const std::wstring &wstr) {
    os << main::make_utf8(wstr);
    return os;
}
#endif

class state {
public:;
    typedef enum {
	stat_regular=0,
	stat_directory,
	stat_door,
	stat_block,
	stat_character,
	stat_pipe,
	stat_socket,
	stat_symlink,
	stat_unknown=254
    } file_types;

    state():mode_recursive(false),	// do we recurse?
	    mode_warn_only(false),	// for loading hash files

	    // these determine which files get hashed
	    mode_expert(false),
	    mode_regular(false),
	    mode_directory(false),
	    mode_door(false),
	    mode_block(false),
	    mode_character(false),
	    mode_pipe(false),
	    mode_socket(false),
	    mode_symlink(false),

	    // command line argument
	    argc(0),argv(0),

	    // these have something to do with hash files that are loaded
	    h_plain(0),h_bsd(0),
	    h_md5deep_size(0),
	    h_hashkeeper(0),h_ilook(0),h_ilook3(0),h_ilook4(0), h_nsrl15(0),
	    h_nsrl20(0), h_encase(0),
	    usage_count(0)		// allows -hh to print extra help
	    {};

    bool	mode_recursive;
    bool	mode_warn_only;

    // which files do we hash.
    bool	mode_expert;
    bool	mode_regular;
    bool	mode_directory;
    bool	mode_door;
    bool	mode_block;
    bool	mode_character;
    bool	mode_pipe;
    bool	mode_socket;
    bool	mode_symlink;
 
    /* Command line arguments */
    int		argc;
#ifdef _WIN32
    wchar_t     **argv;			// never allocated, never freed
#else
    char	**argv;
#endif

    /* configuration and output */
    display	ocb;		// output control block

    // Which filetypes this algorithm supports and their position in the file
    uint8_t      h_plain, h_bsd, h_md5deep_size, h_hashkeeper;
    uint8_t      h_ilook, h_ilook3, h_ilook4, h_nsrl15, h_nsrl20, h_encase;

    void	md5deep_add_hash(char *h, char *fn); // explicitly add a hash
    void	setup_expert_mode(char *arg);

    /* main.cpp */
    uint64_t	find_block_size(std::string input_str);
    int		usage_count;
    tstring	generate_filename(const TCHAR *input);
    void	usage();
    std::string	make_banner();
    void	md5deep_usage();
    void	check_flags_okay();
    void	check_wow64();
    void	md5deep_check_flags_okay();
    int		hashdeep_process_command_line(int argc,char **argv);
    void	md5deep_check_matching_modes();
    int		md5deep_process_command_line(int argc,char **argv);
#ifdef _WIN32
    int		prepare_windows_command_line();
#endif    

    /* files.cpp
     * Not quite sure what to do with this stuff yet...
     */
    
    void	md5deep_load_match_file(const char *fn);
    int		find_hash_in_line(char *buf, int fileType, char *filename);
    int		parse_encase_file(const char *fn,FILE *f,uint32_t num_expected_hashes);
    int		find_plain_hash(char *buf,char *known_fn); // returns FALSE if error
    int         find_md5deep_size_hash(char *buf, char *known_fn);
    int		find_bsd_hash(char *buf, char *fn);
    int		find_rigid_hash(char *buf,  char *fn, unsigned int fn_location, unsigned int hash_location);
    int		find_ilook_hash(char *buf, char *known_fn);
    int		check_for_encase(FILE *f,uint32_t *expected_hashes);

    /* dig.cpp 
     *
     * Note the file typing system needs to be able to display errors...
     */

    class dir_table_t : public std::set<tstring>{
    };
    dir_table_t dir_table;
    void	done_processing_dir(const tstring &fn_);
    void	processing_dir(const tstring &fn_);
    bool	have_processed_dir(const tstring &fn_);
    

    int		identify_hash_file_type(FILE *f,uint32_t *expected_hashes); // identify the hash file type
    bool	should_hash_symlink(const tstring &fn,file_types *link_type);
    bool	should_hash_expert(const tstring &fn, file_types type);
    bool	should_hash(const tstring &fn);

    /* file_type returns the file type of a string.
     * If an error is found and ocb is provided, send the error to ocb.
     * If filesize and timestamp are provided, give them.
     */
    static file_types decode_file_type(const struct __stat64 &sb);
    static file_types file_type(const filename_t &fn,class display *ocb,uint64_t *filesize,timestamp_t *timestamp);
#ifdef _WIN32
    bool	is_junction_point(const std::wstring &fn);
#endif
    void	process_dir(const tstring &path);
    void	dig_normal(const tstring &path);	// posix  & win32 
    void	dig_win32(const tstring &path);	// win32 only; calls dig_normal
    static	void dig_self_test();

    bool hashes_loaded(){
	return ocb.hashes_loaded();
    }

    int main(int argc,char **argv);	// main

};

/**
 * the files class knows how to read various hash file types
 */

/* Due to an inadvertant code fork several years ago, this program has different usage
 * and output when run as 'md5deep' then when run as 'hashdeep'. We call this the
 * 'md5deep_mode' and track it with the variables below.
 */

/* main.cpp */
extern bool	md5deep_mode;		// if true, then we were run as md5deep, sha1deep, etc.
extern int	opt_debug;		// for debugging
extern bool opt_relative;		// print relative file names
extern int  opt_verbose;		// can be 1, 2 or 3
extern bool opt_estimate;		// print ETA
extern bool opt_unicode_escape;
extern bool opt_mode_match;
extern bool opt_mode_match_neg;
extern hashid_t opt_md5deep_mode_algorithm;	// for when we are in MD5DEEP mode
extern bool opt_csv;
extern bool opt_asterisk;
extern bool md5deep_mode;


std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);
std::vector<std::string> split(const std::string &s, char delim);
void lowercase(std::string &s);
extern char *__progname;

// ------------------------------------------------------------------
// HELPER FUNCTIONS
//
// helper.cpp 
// ------------------------------------------------------------------

std::string itos(uint64_t i);
void     chop_line(char *s);
off_t	find_file_size(FILE *f,class display *ocb); // Return the size, in bytes of an open file stream. On error, return -1 

// ------------------------------------------------------------------
// MAIN PROCESSING
// ------------------------------------------------------------------ 
/* dig.cpp */
void dig_self_test();			// check the string-processing




#endif /* ifndef __MAIN_H */
