
/* $Id: main.h,v 1.14 2007/12/28 01:49:36 jessekornblum Exp $ */

#ifndef __MAIN_H
#define __MAIN_H

#include "common.h"
#include "md5deep_hashtable.h"
#include "xml.h"

#include "md5.h"
#include "sha1.h"
#include "sha256.h"
#include "tiger.h"
#include "whirlpool.h"

#include <map>
#include <vector>

/* Global Options */
extern bool opt_silent;			// previously was mode_silent
extern int  opt_verbose;		// can be 1, 2 or 3
#define VERBOSE 1
#define MORE_VERBOSE 2
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

typedef enum
{ 
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
    bool		inuse;
    std::string		name;
    size_t		bit_length;	// 128 for MD5
    hashid_t		id;		// usually the position in the array...

    int ( *f_init)(void *);
    int ( *f_update)(void *, unsigned char *, uint64_t );
    int ( *f_finalize)(void *, unsigned char *);

    static void add_algorithm(hashid_t pos, const char *name, uint16_t bits, 
		       int ( *func_init)(void *),
		       int ( *func_update)(void *, unsigned char *, uint64_t ),
		       int ( *func_finalize)(void *, unsigned char *),
		       int inuse);
    static void load_hashing_algorithms();
    static void clear_algorithms_inuse();
    static void enable_hashing_algorithms(std::string var);  // enable the algorithms in 'var'; var can be 'all'
    static hashid_t get_hashid_for_name(std::string name);   // return the hashid_t for 'name'
    static bool valid_hash(hashid_t alg,const char *buf); // returns true if buf is a valid hash for hashid_t a
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
    file_data_t():refcount(0),used(0),file_size(0),stat_bytes(0),stat_megs(0),actual_bytes(0) {
    };
    // Implement a simple reference count garbage collection system
    int		   refcount;			     // reference counting
    void retain() { refcount++;}
    void release() { if(--refcount==0) delete this; }

    std::string    hash_hex[NUM_ALGORITHMS];	     // the hash in hex of the entire file
    std::string	   hash512_hex[NUM_ALGORITHMS];	     // hash of the first 512 bytes, for partial matching
    std::string	   file_name;		// just the file_name, apparently
    std::string	   file_name_annotation;// print after file name; for piecewise hashing

    uint64_t       used;	 // was hash used in file system? For auditing. The round # is stored
    std::string    known_fn;	 // if we do an md5deep_is_known_hash, this is set to be the filename of the known hash
#ifdef _WIN32
    __time64_t    timestamp;
#else
    time_t        timestamp;
#endif

    // How many bytes (and megs) we think are in the file, via stat(2)
    // and how many bytes we've actually read in the file
    uint64_t       file_size;		// in bytes
    uint64_t       stat_bytes;		// how much stat returned
    uint64_t       stat_megs;		// why are we keeping this?
    uint64_t       actual_bytes;	// how many we read.
};


/** file_data_hasher_t is a subclass of file_data_t.
 * It contains additional information necessary to actually hash a file.
 */
class file_data_hasher_t : public file_data_t {
public:
    static const size_t MD5DEEP_IDEAL_BLOCK_SIZE = 8192;
    static const size_t MAX_ALGORITHM_CONTEXT_SIZE = 256;
    static const size_t MAX_ALGORITHM_RESIDUE_SIZE = 256;
    file_data_hasher_t():handle(0),is_stdin(0),read_start(0),read_end(0),bytes_read(0),
			 block_size(MD5DEEP_IDEAL_BLOCK_SIZE){
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

    // Where the last read operation started and ended
    // bytes_read is a shorthand for read_end - read_start
    uint64_t        read_start;
    uint64_t        read_end;
    uint64_t        bytes_read;

    /* Size of blocks used in normal hashing */
    uint64_t        block_size;
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
    } filetype_t; 

    class hashmap : public  std::map<std::string,file_data_t *> {
    public:;
	void add_file(file_data_t *fi,int alg_num);
    };
    uint64_t		count_unused(); // count unused and optionally perform audit
    hashmap		hashmaps[NUM_ALGORITHMS];
    searchstatus_t	search(const file_data_t *fdt) const; // look up a fdt

    /**
     * Figure out the format of a hashlist file and load it.
     * Both of these functions take the file name and the open handle.
     * They read from the handle and just use the filename for printing error messages.
     */
    void		enable_hashing_algorithms_from_hashdeep_file(const char *fn,const char *val);
    std::string		last_enabled_algorithms; // a string with the algorithms that were enabled last
    hashid_t		hash_column[NUM_ALGORITHMS]; // maps a column number to a hashid;
						     // the order columns appear in the file being loaded.
    int			num_columns;		     // number of columns in file being loaded
    filetype_t		identify_filetype(const char *fn,FILE *handle);
    int			parse_hashing_algorithm(const char *fn,const char *val);
    loadstatus_t	load_hash_file(const char *fn);
    

    /**
     * add_fdt adds a file_data_t record to the hashlist, and its hashes to the hashmaps.
     * @param state - needed to find the algorithms in use
     */
    void add_fdt(file_data_t *fi);
};



/* Primary modes of operation  */
typedef enum  { 
  primary_compute, 
  primary_match, 
  primary_match_neg, 
  primary_audit
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

class state {
    void add_algorithm(hashid_t pos,const char *name,uint16_t bits, 
		       int ( *func_init)(void *),
		       int ( *func_update)(void *, unsigned char *, uint64_t ),
		       int ( *func_finalize)(void *, unsigned char *),
		       int inuse);
public:;
    state():primary_function(primary_compute),mode(mode_none),
	    start_time(0),last_time(0),
	    
	    argc(0),argv(0),input_list(0),
	    piecewise_size(0),

	    expected_hashes(0),
	    size_threshold(0),

	    known(),seen(),
	    hash_round(0),h_plain(0),h_bsd(0),h_md5deep_size(0),
	    h_hashkeeper(0),h_ilook(0),h_ilook3(0),h_ilook4(0), h_nsrl15(0),
	    h_nsrl20(0), h_encase(0),

	    banner_displayed(false),
	    dfxml(0) {};

    /* Basic Program State */
    primary_t       primary_function;
    uint64_t        mode;
    time_t          start_time, last_time;

    /* Command line arguments */
    int             argc;
    TCHAR        ** argv;			// never allocated, never freed
    char          * input_list;
    std::string     cwd;

    /* Configuration */
    uint64_t        piecewise_size;    /* Size of blocks used in piecewise hashing */

    // Which hash algorithms we are using

    /* The file currently being hashed */

    // Lists of known hashes 
    hashTable       known_hashes;
    uint32_t        expected_hashes;

    // When only hashing files larger/smaller than a given threshold
    uint64_t        size_threshold;

    /* The set of known values; typically read from the audit file */
    hashlist	    known;		// hashes read from the -k file

    hashlist	    seen;		// hashes seen on this hashing run; from the command line
    uint64_t        hash_round;

    // Which filetypes this algorithm supports and their position in the file
    uint8_t      h_plain, h_bsd, h_md5deep_size, h_hashkeeper;
    uint8_t      h_ilook, h_ilook3, h_ilook4, h_nsrl15, h_nsrl20, h_encase;

    /* output */
    bool             banner_displayed;	// has the header been shown (text output)
    XML             *dfxml;  /* output in DFXML */


    class audit_stats match;		// for the audit mode
  
    /* Legacy 'md5deep', 'sha1deep', etc. mode.  */
    bool	md5deep_mode;		// if true, then we were run as md5deep, sha1deep, etc.
    int		md5deep_mode_algorithm;	// which algorithm number we are using

    std::string	outfile;	// where output goes

    bool hashes_loaded(){
	return known.size()>0;
    }

};

/* HASH TABLE */
#if 0
void hashtable_init(hashtable_t *t);
status_t hashtable_add(state *s, hashid_t alg, file_data_t *f);
hashtable_entry_t * hashtable_contains(state *s, hashid_t alg);
void hashtable_destroy(hashtable_entry_t *e);
#endif


/* MATCHING MODES */
status_t display_match_result(state *s,file_data_hasher_t *fdht);

int md5deep_display_hash(state *s,file_data_hasher_t *fdt);
int display_hash_simple(state *s,file_data_t *fdt);

/* AUDIT MODE */

//void setup_audit(state *s);
int audit_check(state *s);		// performs an audit; return 0 if pass, -1 if fail
int display_audit_results(state *s);
int audit_update(state *s,file_data_t *fdt);

/* HASHING CODE */

int hash_file(state *s, file_data_hasher_t *fdht,TCHAR *file_name);
int hash_stdin(state *s);


/* HELPER FUNCTIONS */
void generate_filename(state *s, TCHAR *fn, std::string cwd, TCHAR *input);
void sanity_check(state *s, int condition, const char *msg);

// ----------------------------------------------------------------
// CYCLE CHECKING
// ----------------------------------------------------------------
int have_processed_dir(TCHAR *fn);
int processing_dir(TCHAR *fn);
int done_processing_dir(TCHAR *fn);

// ------------------------------------------------------------------
// HELPER FUNCTIONS
// ------------------------------------------------------------------ 
void     setup_expert_mode(state *s, char *arg);
void     generate_filename(state *s, TCHAR *fn, TCHAR *cwd, TCHAR *input);
uint64_t find_block_size(state *s, char *input_str);
void     chop_line(char *s);
void     shift_string(char *fn, size_t start, size_t new_start);
void     check_wow64(state *s);

// Works like dirname(3), but accepts a TCHAR* instead of char*
int my_dirname(TCHAR *c);
int my_basename(TCHAR *s);

int find_comma_separated_string(char *s, unsigned int n);
int find_quoted_string(char *buf, unsigned int n);

// Return the size, in bytes of an open file stream. On error, return -1 
off_t find_file_size(FILE *f);

// ------------------------------------------------------------------
// MAIN PROCESSING
// ------------------------------------------------------------------ 
int process_win32(state *s, TCHAR *fn);
int process_normal(state *s, TCHAR *fn);
int md5deep_process_command_line(state *s, int argc, char **argv);


/* display.cpp */
std::string itos(uint64_t i);
void output_unicode(FILE *out,const std::string &ucs);
void display_filename(FILE *out, const file_data_t &fdt,bool shorten);
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
void print_error_unicode(const std::string &fn, const char *fmt, ...);

// Display an error message, if not in silent mode,  
// and exit with EXIT_FAILURE
void fatal_error(const char *fmt, ...);

// Display an error message, ask user to contact the developer, 
// and exit with EXIT_FAILURE
void internal_error(const char *fmt, ... );

// Display a filename, possibly including Unicode characters
void print_debug(const char *fmt, ...);
void make_newline(const state *s);
void try_msg(void);
int display_hash( state *s, file_data_hasher_t *fdht);



// ----------------------------------------------------------------
// FILE MATCHING
// ---------------------------------------------------------------- 

// md5deep_match.c
int md5deep_load_match_file(state *s, const char *fn);
int md5deep_is_known_hash(const char *h, std::string *known_fn);
//    // This function returns FALSE. hash_file, called above, returns STATUS_OK                                             
// process_win32 also returns STATUS_OK.                                                                               
// display_audit_results, used by hashdeep, returns EXIT_SUCCESS/FAILURE.                                              
// Pick one and stay with it!                                                                                          
int was_input_not_matched(void);
int md5deep_finalize_matching(state *s);

// Add a single hash to the matching set
void md5deep_add_hash(state *s, char *h, char *fn);

// Functions for file evaluation (files.c) 
int hash_file_type(state *s, FILE *f);
int find_hash_in_line(state *s, char *buf, int fileType, char *filename);

#endif /* ifndef __MAIN_H */
