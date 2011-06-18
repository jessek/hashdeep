
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

  * Update parse_algorithm_name in main.c and parse_hashing_algorithms
    in match.c to handle your algorithm. 

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

/* The largest number of columns we can expect in a file of knowns.
   Normally this should be the number of hash algorithms plus a column
   for file size, file name, and, well, some fudge factors. Any values
   after this number will be ignored. For example, if the user invokes
   the program as:

   hashdeep -c md5,md5,md5,md5,...,md5,md5,md5,md5,md5,md5,md5,whirlpool

   the whirlpool will not be registered. */

#define MAX_KNOWN_COLUMNS  (NUM_ALGORITHMS + 6)
   

/** status_t describes what kind of match was made.
 * 
 */
typedef enum   {
    status_ok = 0,

    /* Matching hashes */
    status_match,			// all hashes match
    status_partial_match,	 /* One or more hashes match, but not all */
    status_file_size_mismatch,   /* Implies all hashes match */
    status_file_name_mismatch,   /* Implies all hashes and file size match */   
    status_no_match,             /* Implies none of the hashes match */

    /* Loading sets of hashes */
    status_unknown_filetype,
    status_contains_bad_hashes,
    status_contains_no_hashes,
    status_file_error,

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
    file_data_t():used(0),file_size(0),stat_bytes(0),stat_megs(0),actual_bytes(0) {
    };
    /* We don't want to use s->full_name, but it's required for hash.c */
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
 * We store ONE of these in the state and use it as a list of all the hashes seen.
 * We also store multiple maps for each algorithm number which map the hash hex code
 * to the pointer as well. 
 */
class hashlist : public std::vector<file_data_t *> {
public:;
    class state *s;			// our state; used to determine which algorithms in use
    class hashmap : public  std::map<std::string,file_data_t *> {
    public:;
	void add_file(file_data_t *fi,int alg_num){
	    insert(std::pair<std::string,file_data_t *>(fi->hash_hex[alg_num],fi));
	};
    };
    hashlist(class state *s_):s(s_){
    }
    uint64_t    count_unused(); // count unused and optionally perform audit
    hashmap	hashmaps[NUM_ALGORITHMS];
    status_t	search(const file_data_t *fdt) const; // look up a fdt

    /**
     * add_file adds a file to the hashlist, and its hashes to the hashmaps.
     * @param state - needed to find the algorithms in use
     */
    void add_file(file_data_t *fi);
};



/* This class holds the information known about each hash algorithm.
 * It's sort of like the EVP system in OpenSSL.
 * In version 3 the list of known hashes was stored here as well.
 * That has been moved to the hashmap database.
 */
class algorithm_t {
public:
    bool		inuse;		// are we using this hash algorithm?
    std::string		name;
    size_t		bit_length;	// 128 for MD5

    int ( *f_init)(void *);
    int ( *f_update)(void *, unsigned char *, uint64_t );
    int ( *f_finalize)(void *, unsigned char *);
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
    void load_hashing_algorithms();
public:;
    state():primary_function(primary_compute),mode(mode_none),
	    start_time(0),last_time(0),
	    
	    argc(0),argv(0),input_list(0),
	    piecewise_size(0),
	    current_file(0),

	    expected_hashes(0),expected_columns(0),
	    size_threshold(0),

	    hashes_loaded(false), known(this),seen(this),

	    hash_round(0),h_plain(0),h_bsd(0),h_md5deep_size(0),
	    h_hashkeeper(0),h_ilook(0),h_ilook3(0),h_ilook4(0), h_nsrl15(0),
	    h_nsrl20(0), h_encase(0),

	    banner_displayed(false),
	    dfxml(0) {
	load_hashing_algorithms();
	current_file = new file_data_hasher_t();
    };
    ~state(){
	if(current_file) delete current_file;
    }

    /* Basic Program State */
    primary_t       primary_function;
    uint64_t        mode;
    time_t          start_time, last_time;
    algorithm_t     hashes[NUM_ALGORITHMS]; // which hash algorithms are in use

    /* Command line arguments */
    int             argc;
    TCHAR        ** argv;			// never allocated, never freed
    char          * input_list;
    std::string     cwd;

    /* Configuration */
    uint64_t        piecewise_size;    /* Size of blocks used in piecewise hashing */

    /* The file currently being hashed */
    file_data_hasher_t   * current_file;

    // Lists of known hashes 
    hashTable     known_hashes;
    uint32_t      expected_hashes;
    uint8_t         expected_columns;

    // When only hashing files larger/smaller than a given threshold
    uint64_t        size_threshold;

    /* The set of known values; typically read from the audit file */
    bool            hashes_loaded;	// true if hash values have been loaded.
    hashlist	    known;		// hashes read from the -k file

    hashlist	    seen;		// hashes seen on this hashing run; from the command line
    uint64_t        hash_round;
    hashid_t        hash_order[NUM_ALGORITHMS];

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
};

/* GENERIC ROUTINES */
void clear_algorithms_inuse(state *s);

/* HASH TABLE */
#if 0
void hashtable_init(hashtable_t *t);
status_t hashtable_add(state *s, hashid_t alg, file_data_t *f);
hashtable_entry_t * hashtable_contains(state *s, hashid_t alg);
void hashtable_destroy(hashtable_entry_t *e);
#endif

/* MULTIHASHING */
void multihash_initialize(state *s);
void multihash_update(state *s, unsigned char *buf, uint64_t len);
void multihash_finalize(state *s);


/* MATCHING MODES */
status_t load_match_file(state *s, char *fn);
status_t display_match_result(state *s,file_data_hasher_t *fdht);

int md5deep_display_hash(state *s,file_data_hasher_t *fdt);
int display_hash_simple(state *s,file_data_t *fdt);

/* AUDIT MODE */

//void setup_audit(state *s);
int audit_check(state *s);		// performs an audit; return 0 if pass, -1 if fail
int display_audit_results(state *s);
int audit_update(state *s,file_data_t *fdt);

/* HASHING CODE */

int hash_file(state *s, TCHAR *file_name);
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
void print_error(const state *s, const char *fmt, ...);

// Display an error message if not in silent mode with a Unicode filename
void print_error_unicode(const state *s, const std::string &fn, const char *fmt, ...);

// Display an error message, if not in silent mode,  
// and exit with EXIT_FAILURE
void fatal_error(const state *s, const char *fmt, ...);

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
//int was_input_not_matched(void);
int md5deep_finalize_matching(state *s);

// Add a single hash to the matching set
void md5deep_add_hash(state *s, char *h, char *fn);

// Functions for file evaluation (files.c) 
int valid_hash(state *s, const char *buf);
int hash_file_type(state *s, FILE *f);
int find_hash_in_line(state *s, char *buf, int fileType, char *filename);

#endif /* ifndef __MAIN_H */
