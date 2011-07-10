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

#include <map>
#include <vector>

/* Global Options */
extern bool opt_relative;		// print relative file names
extern bool opt_silent;			// previously was mode_silent
extern int  opt_verbose;		// can be 1, 2 or 3
extern bool opt_zero;			// newlines are \000 
extern bool opt_estimate;		// print ETA
extern int  opt_debug;			// for debugging
extern bool opt_unicode_escape;

// Return values for the program 
// RBF - Document these return values for hashdeep 
#define STATUS_OK                      0
#define STATUS_UNUSED_HASHES           1
#define STATUS_INPUT_DID_NOT_MATCH     2
#define STATUS_USER_ERROR             64
#define STATUS_INTERNAL_ERROR        128 

#define mode_none              0
#define mode_recursive         1<<0
//#define mode_estimate          1<<1          // now is opt_estimate
//#define mode_silent            1<<2          // now is opt_silent
#define mode_warn_only         1<<3
#define mode_match             1<<4
#define mode_match_neg         1<<5
#define mode_display_hash      1<<6
#define mode_display_size      1<<7
//#define mode_zero              1<<8          // now is opt_zero
//#define mode_relative          1<<9
#define mode_which             1<<10
#define mode_barename          1<<11
#define mode_asterisk          1<<12
#define mode_not_matched       1<<13
#define mode_quiet             1<<14
#define mode_piecewise         1<<15
//these were moved to opt_verbose
//#define mode_verbose           1<<16
//#define mode_more_verbose      1<<17
//#define mode_insanely_verbose  1<<18
#define mode_size              1<<19
#define mode_size_all          1<<20
#define mode_timestamp         1<<21
#define mode_csv               1<<22
#define mode_read_from_file    1<<25
#define mode_triage            1<<26

// Modes 27-48 are reserved for future use.
//
// Note that the LL is required to avoid overflows of 32-bit words.
// LL must be used for any value equal to or above 1<<31. 
// Also note that these values can't be returned as an int type. 
// For example, any function that returns an int can't use
//
// return (s->mode & mode_regular);   
// 
// That value is 64-bits wide and may not be returned correctly. 

#define mode_expert        (1LL)<<49
#define mode_regular       (1LL)<<50
#define mode_directory     (1LL)<<51
#define mode_door          (1LL)<<52
#define mode_block         (1LL)<<53
#define mode_character     (1LL)<<54
#define mode_pipe          (1LL)<<55
#define mode_socket        (1LL)<<56
#define mode_symlink       (1LL)<<57



#define VERBOSE		1
#define MORE_VERBOSE	2
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

typedef enum{ 
  alg_md5=0, 
  alg_sha1, 
  alg_sha256, 
  alg_tiger,
  alg_whirlpool, 
  
  /* alg_unknown must always be last in this list. It's used
     as a loop terminator in many functions. */
  alg_unknown 
} hashid_t;

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
    int ( *f_init)(void *);
    int ( *f_update)(void *, unsigned char *, uint64_t );
    int ( *f_finalize)(void *, unsigned char *);

    /* The methods */
    static void add_algorithm(hashid_t pos, const char *name, uint16_t bits, 
		       int ( *func_init)(void *),
		       int ( *func_update)(void *, unsigned char *, uint64_t ),
		       int ( *func_finalize)(void *, unsigned char *),
		       int inuse);
    static void load_hashing_algorithms();
    static void clear_algorithms_inuse();
    static void enable_hashing_algorithms(std::string var);  // enable the algorithms in 'var'; var can be 'all'
    static hashid_t get_hashid_for_name(std::string name);   // return the hashid_t for 'name'
    static bool valid_hash(hashid_t alg,const std::string &buf); // returns true if buf is a valid hash for hashid_t a
};

extern algorithm_t     hashes[NUM_ALGORITHMS];		// which hash algorithms are available and in use



/** status_t describes general kinds of errors.
 * 
 */
typedef enum   {
    status_ok = 0,

    /* General errors */
    status_out_of_memory,
    status_invalid_hash,  
    status_unknown_error,
    status_omg_ponies

} status_t;


/** file_data_t contains information about a file.
 * It can be created by hashing an actual file, or by reading a hash file a file of hashes. 
 * Pointers to these objects are stored in a single vector and in a map for each algorithm.
 * Note that all hashes are currently stored as a hex string. That incurs a 2x memory overhead.
 * This will be changed.
 */
class file_data_t {
public:
    file_data_t():refcount(0),matched_file_number(0),file_size(0),stat_bytes(0),actual_bytes(0) {
    };
    // Implement a simple reference count garbage collection system
    int		   refcount;			     // reference counting
    void retain() { refcount++;}
    void release() { if(--refcount==0) delete this; }

    std::string    hash_hex[NUM_ALGORITHMS];	     // the hash in hex of the entire file
    std::string	   hash512_hex[NUM_ALGORITHMS];	     // hash of the first 512 bytes, for partial matching
    std::string	   file_name;		// just the file_name, apparently; native on POSIX; UTF-8 on Windows.
    std::string	   file_name_annotation;// print after file name; for piecewise hashing

    uint64_t       matched_file_number;	 // file number that we matched.
#ifdef _WIN32
    __time64_t    timestamp;
#else
    time_t        timestamp;
#endif

    // How many bytes (and megs) we think are in the file, via stat(2)
    // and how many bytes we've actually read in the file
    uint64_t       file_size;		// in bytes
    uint64_t       stat_bytes;		// how much stat returned
    uint64_t       actual_bytes;	// how many we read.

    uint64_t	   stat_megs(){
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
    file_data_hasher_t(bool piecewise_):handle(0),is_stdin(0),read_start(0),read_end(0),bytes_read(0),
					block_size(MD5DEEP_IDEAL_BLOCK_SIZE),piecewise(piecewise_){
	file_number = ++next_file_number;
    };
    ~file_data_hasher_t(){
	if(handle) fclose(handle);	// make sure that it' closed.
    }
    void close(){
	if(handle){
	    fclose(handle);
	    handle = 0;
	}
    }
    /* The actual hashing */
    void multihash_initialize();
    void multihash_update(const unsigned char *buffer,size_t bufsize);
    void multihash_finalize();


    FILE           *handle;		// the file we are reading
    bool           is_stdin;		// flag if the file is stdin
    unsigned char  buffer[MD5DEEP_IDEAL_BLOCK_SIZE]; // next buffer to hash
    uint8_t        hash_context[NUM_ALGORITHMS][MAX_ALGORITHM_CONTEXT_SIZE];	 
    std::string	   dfxml_hash;	      // the DFXML hash digest for the piece just hashed; used to build piecewise
    uint64_t	   file_number;

    // Where the last read operation started and ended
    // bytes_read is a shorthand for read_end - read_start
    uint64_t        read_start;
    uint64_t        read_end;
    uint64_t        bytes_read;

    /* Size of blocks used in normal hashing */
    uint64_t        block_size;

    /* Flags */
    bool		piecewise;	// create picewise hashes

};



/** The hashlist holds a list of file_data_t pointers.
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
    uint64_t		compute_unused(bool display,std::string annotation);
    hashmap		hashmaps[NUM_ALGORITHMS];
    searchstatus_t	search(const file_data_hasher_t *fdht,
			       file_data_t **matched) const; // look up a fdt
    uint64_t		total_matched;

    /**
     * Figure out the format of a hashlist file and load it.
     * Both of these functions take the file name and the open handle.
     * They read from the handle and just use the filename for printing error messages.
     */
    void		enable_hashing_algorithms_from_hashdeep_file(const std::string &fn,std::string val);
    std::string		last_enabled_algorithms; // a string with the algorithms that were enabled last
    hashid_t		hash_column[NUM_ALGORITHMS]; // maps a column number to a hashid;
						     // the order columns appear in the file being loaded.
    int			num_columns;		     // number of columns in file being loaded
    hashfile_format	identify_format(const std::string &fn,FILE *handle);
    //int			parse_hashing_algorithm(const char *fn,const char *val);
    loadstatus_t	load_hash_file(const std::string &fn); // not tstring! always ASCII
    file_data_t		*find_hash(hashid_t alg,std::string &hash_hex,uint64_t file_number); 
    void		dump_hashlist(); // send contents to stdout
    
    /**
     * add_fdt adds a file_data_t record to the hashlist, and its hashes to the hashmaps.
     * @param state - needed to find the algorithms in use
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

/**
 * The 'state' class holds the state of the hashdeep/md5deep program.
 * This includes:
 * startup parameters
 * known - the list of hashes in the hash database.
 * seen  - the list of hashes that have been seen this time through.
 */


class main {
public:
    static tstring getcwd();			  // returns the current directory
    static tstring get_realpath(const tstring &fn); // returns the full path
    static std::string get_realpath8(const tstring &fn);  // returns the full path in UTF-8
    static std::string escape_utf8(const std::string &fn); // turns "⦿" to "U+29BF"
#ifdef _WIN32
    static std::string make_utf8(const std::wstring &tfn) ;
#else
    static std::string make_utf8(const std::string &tfn){return tfn;}
#endif
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


    state():primary_function(primary_compute),mode(mode_none),
	    start_time(0),last_time(0),
	    argc(0),argv(0),
	    piecewise_size(0),
	    banner_displayed(false),
	    dfxml(0),
	    size_threshold(0),
	    known(),seen(),
	    h_plain(0),h_bsd(0),h_md5deep_size(0),
	    h_hashkeeper(0),h_ilook(0),h_ilook3(0),h_ilook4(0), h_nsrl15(0),
	    h_nsrl20(0), h_encase(0),
	    md5deep_mode(0),md5deep_mode_algorithm(alg_unknown)
	    {};

    /* Basic Program State */
    primary_t       primary_function;
    uint64_t        mode;
    time_t          start_time, last_time;

    /* Command line arguments */
    int             argc;
#ifdef _WIN32
    wchar_t        **argv;			// never allocated, never freed
#else
    char	   **argv;
#endif

    /* Configuration */
    uint64_t        piecewise_size;    /* Size of blocks used in piecewise hashing */

    /* output */
    std::string		outfile;	// where output goes
    bool             banner_displayed;	// has the header been shown (text output)
    XML             *dfxml;  /* output in DFXML */

    // When only hashing files larger/smaller than a given threshold
    uint64_t        size_threshold;

    /* The set of known values; typically read from the audit file */
    hashlist	    known;		// hashes read from the -k file
    hashlist	    seen;		// hashes seen on this hashing run; from the command line

    // Which filetypes this algorithm supports and their position in the file
    uint8_t      h_plain, h_bsd, h_md5deep_size, h_hashkeeper;
    uint8_t      h_ilook, h_ilook3, h_ilook4, h_nsrl15, h_nsrl20, h_encase;

    class audit_stats match;		// for the audit mode
  
    /* Due to an inadvertant code fork several years ago, this program has different usage
     * and output when run as 'md5deep' then when run as 'hashdeep'. We call this the
     * 'md5deep_mode' and track it with the variables below.
     */
    bool	md5deep_mode;		// if true, then we were run as md5deep, sha1deep, etc.
    hashid_t	md5deep_mode_algorithm;	// which algorithm number we are using, derrived from argv[0]

    void	md5deep_add_hash(char *h, char *fn); // explicitly add a hash


    /* files.cpp
     * Not quite sure what to do with this stuff yet...
     */
    
    status_t	display_match_result(file_data_hasher_t *fdht);
    void	md5deep_load_match_file(const char *fn);
    int		md5deep_display_match_result(file_data_hasher_t *fdht);
    int		md5deep_display_hash(file_data_hasher_t *fdht);
    int		find_hash_in_line(char *buf, int fileType, char *filename);
    int		parse_encase_file(const char *fn,FILE *f,uint32_t num_expected_hashes);
    int		find_plain_hash(char *buf,char *known_fn); // returns FALSE if error
    int         find_md5deep_size_hash(char *buf, char *known_fn);
    int		find_bsd_hash(char *buf, char *fn);
    int		find_rigid_hash(char *buf,  char *fn, unsigned int fn_location, unsigned int hash_location);
    int		find_ilook_hash(char *buf, char *known_fn);
    int		check_for_encase(FILE *f,uint32_t *expected_hashes);
    int		identify_hash_file_type(FILE *f,uint32_t *expected_hashes); // identify the hash file type
    int		finalize_matching();

    /* dig.cpp */
    int		should_hash_symlink(const tstring &fn,file_types *link_type);
    int		should_hash_expert(const tstring &fn, file_types type);
    int		should_hash(file_data_hasher_t *fdht,const tstring &fn);

    int		process_dir(const tstring &path);
    int		dig_normal(const tstring &path);	// posix  & win32 
    int		dig_win32(const tstring &path);	// win32 only; calls dig_normal
    static	void dig_self_test();


    /* display.cpp */
    void	display_banner();
    int		display_hash(file_data_hasher_t *fdht);
    int		display_hash_simple(file_data_hasher_t *fdt);

    /* audit mode */
    int		audit_update(file_data_hasher_t *fdt);
    int		audit_check();		// performs an audit; return 0 if pass, -1 if fail
    int		display_audit_results();

    /* hash.cpp */
    
    int hash_file(file_data_hasher_t *fdht,const tstring &file_name);
    int hash_stdin();


    bool hashes_loaded(){
	return known.size()>0;
    }
};

/**
 * the files class knows how to read various hash file types
 */

class files {
};

/* main.cpp */
std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);
std::vector<std::string> split(const std::string &s, char delim);
void lowercase(std::string &s);
extern char *__progname;

// ----------------------------------------------------------------
// CYCLE CHECKING
// ----------------------------------------------------------------
int have_processed_dir(const tstring &fn);
void processing_dir(const tstring &fn);
void done_processing_dir(const tstring &fn);

// ------------------------------------------------------------------
// HELPER FUNCTIONS
//
// helper.cpp 
// ------------------------------------------------------------------

std::string itos(uint64_t i);
uint64_t find_block_size(state *s, char *input_str);
void     chop_line(char *s);
void     shift_string(char *fn, size_t start, size_t new_start);
void     check_wow64(state *s);

int find_comma_separated_string(char *s, unsigned int n);
int find_quoted_string(char *buf, unsigned int n);

// Return the size, in bytes of an open file stream. On error, return -1 
off_t find_file_size(FILE *f);

// ------------------------------------------------------------------
// MAIN PROCESSING
// ------------------------------------------------------------------ 
/* dig.cpp */
int md5deep_process_command_line(state *s, int argc, char **argv);
void dig_self_test();			// check the string-processing


/* display.cpp */
/* output_filename simply sends the filename to the specified output.
 * With TCHAR it sends it out as UTF-8 unless unicode quoting is requested,
 * in which case Unicode characters are emited as U+xxxx.
 * For example, the Unicode smiley character ☺ is output as U+263A.
 */
void  output_filename(FILE *out,const char *fn);
void  output_filename(FILE *out,const std::string &fn);
#ifdef _WIN32
void  output_filename(FILE *out,const std::wstring &fn);
#endif

/* display_filename is similar output_filename,
 * except it takes a file_data_structure and optionally shortens to the line width
 */

void  display_filename(FILE *out, const file_data_t &fdt,bool shorten);
inline void display_filename(FILE *out, const file_data_t *fdt,bool shorten){
    display_filename(out,*fdt,shorten);
};


/* ui.c */
/* User Interface Functions */

// Display an ordinary message with newline added
void print_status(const char *fmt, ...);

// Display an error message if not in silent mode
void print_error(const char *fmt, ...);

// Display an error message if not in silent mode with a Unicode filename
void print_error_filename(const std::string &fn, const char *fmt, ...);
#ifdef _WIN32
void print_error_filename(const std::wstring &wfn, const char *fmt, ...);
#endif

// Display an error message, if not in silent mode,  
// and exit with EXIT_FAILURE
void fatal_error(const char *fmt, ...);

// Display an error message, ask user to contact the developer, 
// and exit with EXIT_FAILURE
void internal_error(const char *fmt, ... );

// Display a filename, possibly including Unicode characters
void print_debug(const char *fmt, ...);
void print_newline();
void try_msg(void);

#endif /* ifndef __MAIN_H */
