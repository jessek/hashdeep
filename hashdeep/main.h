
/* $Id: main.h,v 1.14 2007/12/28 01:49:36 jessekornblum Exp $ */

#ifndef __MAIN_H
#define __MAIN_H

#include "common.h"
#include "md5deep_hashtable.h"

#ifdef __cplusplus
#include "xml.h"
#else
#define XML void
#endif

// The default size for hashing 
#define MD5DEEP_IDEAL_BLOCK_SIZE 8192


/* HOW TO ADD A NEW HASHING ALGORITHM

  * Add a value for the algorithm to the hashname_t enumeration

  * Add the functions to compute the hashes. There should be three functions,
    an initialization route, an update routine, and a finalize routine.
    The convention, for an algorithm "foo", is 
    foo_init, foo_update, and foo_final. 

  * Add your new code to Makefile.am under hashdeep_SOURCES

  * Add a call to insert the algorithm in setup_hashing_algorithms

  * Update parse_algorithm_name in main.c and parse_hashing_algorithms
    in match.c to handle your algorithm. 

  * See if you need to increase ALGORITHM_NAME_LENGTH or
    ALGORITHM_CONTEXT_SIZE for your algorithm.

  * Update the usage function and man page to include the function     */


#include "md5.h"
#include "sha1.h"
#include "sha256.h"
#include "tiger.h"
#include "whirlpool.h"

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
} hashname_t;

#define NUM_ALGORITHMS  alg_unknown

/* Which ones are enabled by default */
#define DEFAULT_ENABLE_MD5         TRUE
#define DEFAULT_ENABLE_SHA1        FALSE
#define DEFAULT_ENABLE_SHA256      TRUE
#define DEFAULT_ENABLE_TIGER       FALSE
#define DEFAULT_ENABLE_WHIRLPOOL   FALSE


/* When parsing algorithm names supplied by the user, they must be 
   fewer than ALGORITHM_NAME_LENGTH characters. */
#define ALGORITHM_NAME_LENGTH  15 

/* The largest number of bytes needed for a hash algorithm's context
   variable. They all get initialized to this size. */
#define ALGORITHM_CONTEXT_SIZE 256

/* The largest number of columns we can expect in a file of knowns.
   Normally this should be the number of hash algorithms plus a column
   for file size, file name, and, well, some fudge factors. Any values
   after this number will be ignored. For example, if the user invokes
   the program as:

   hashdeep -c md5,md5,md5,md5,...,md5,md5,md5,md5,md5,md5,md5,whirlpool

   the whirlpool will not be registered. */
#define MAX_KNOWN_COLUMNS  (NUM_ALGORITHMS + 6)
   

/* Return codes */
typedef enum 
  {
    status_ok = 0,

    /* Matching hashes */
    status_match,
    status_partial_match,        /* One or more hashes match, but not all */
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



/* These describe the version of the file format being used, not
 *   the version of the program.
 */
#define HASHDEEP_PREFIX     "%%%% "
#define HASHDEEP_HEADER_10  "%%%% HASHDEEP-1.0"


/* This describes the file being hashed.*/

class file_data_t {
public:
  char                * hash[NUM_ALGORITHMS]; // the hex hashes
  uint64_t              file_size;
  TCHAR               * file_name;
  uint64_t              used;
  class file_data_t * next;
};

class hashtable_entry_t {
public:
  status_t                     status; 
  file_data_t                * data;
  struct _hashtable_entry_t  * next;   
};

/* HASH_TABLE_SIZE must be at least 16 to the power of HASH_TABLE_SIG_FIGS */
#define HASH_TABLE_SIG_FIGS   5
#define HASH_TABLE_SIZE       1048577   

typedef struct _hash_table_t {
  hashtable_entry_t * member[HASH_TABLE_SIZE];
} hashtable_t;


/* This structure defines what's known about a hash algorithm */
typedef struct _algorithm_t
{
  char          * name;
  uint16_t        byte_length;
  void          * hash_context; 

  int ( *f_init)(void *);
  int ( *f_update)(void *, unsigned char *, uint64_t );
  int ( *f_finalize)(void *, unsigned char *);


  hashtable_t   * known;	/* The set of known hashes for this algorithm */
  unsigned char * hash_sum;	// printable
  int             inuse;
} algorithm_t;


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

typedef struct _state {

  /* Basic Program State */
  primary_t       primary_function;
  uint64_t        mode;
  int             return_value;
  time_t          start_time, last_time;

  /* Command line arguments */
  TCHAR        ** argv;
  int             argc;
  char          * input_list;
  TCHAR         * cwd;

  /* The file currently being hashed */
  file_data_t   * current_file;
  int             is_stdin;
  FILE          * handle;
  unsigned char * buffer;

#ifdef _WIN32
  __time64_t    timestamp;
#else
  time_t        timestamp;
#endif
  char          * time_str;

  // Lists of known hashes 
  hashTable     known_hashes;
  uint32_t      expected_hashes;

  // The type of file, as report by stat
  uint8_t       input_type;


  // When only hashing files larger/smaller than a given threshold
  uint64_t        size_threshold;

  // How many bytes (and megs) we think are in the file, via stat(2)
  // and how many bytes we've actually read in the file
  uint64_t        stat_bytes;
  uint64_t        stat_megs;
  uint64_t        actual_bytes;

  // Where the last read operation started and ended
  // bytes_read is a shorthand for read_end - read_start
  uint64_t        read_start;
  uint64_t        read_end;
  uint64_t        bytes_read;

  /* We don't want to use s->full_name, but it's required for hash.c */
  TCHAR         * full_name;
  TCHAR         * short_name;
  TCHAR         * msg;

  /* The set of known values */
  int             hashes_loaded;
  algorithm_t   * hashes[NUM_ALGORITHMS];
  uint8_t         expected_columns;
  file_data_t   * known, * last;
  uint64_t        hash_round;
  hashname_t      hash_order[NUM_ALGORITHMS];

  // Hashing algorithms 
  // We don't define hash_string_length, it's just twice this length. 
  // We use a signed value as this gets compared with the output of strlen() */
    //size_t          hash_length;

  // Which filetypes this algorithm supports and their position in the file
  uint8_t      h_plain, h_bsd, h_md5deep_size, h_hashkeeper;
  uint8_t      h_ilook, h_ilook3, h_ilook4, h_nsrl15, h_nsrl20, h_encase;

  int             banner_displayed;

  /* Size of blocks used in normal hashing */
  uint64_t        block_size;

  /* Size of blocks used in piecewise hashing */
  uint64_t        piecewise_size;
  
  /* For audit mode, the number of each type of file */
  uint64_t        match_exact, match_expect,
    match_partial, match_moved, match_unused, match_unknown, match_total;

    /* Legacy 'md5deep', 'sha1deep', etc. mode.  */
    int	md5deep_mode;
    size_t md5deep_mode_hash_length;	// in bytes
    char *md5deep_mode_hash_result;	// printable ASCII; md5deep_mode_hash_length*2+1 bytes long
    char known_fn[PATH_MAX+1];

  /* output in DFXML */
    XML       *dfxml;
} state;

__BEGIN_DECLS
/* GENERIC ROUTINES */
void clear_algorithms_inuse(state *s);

/* HASH TABLE */
void hashtable_init(hashtable_t *t);
status_t hashtable_add(state *s, hashname_t alg, file_data_t *f);
hashtable_entry_t * hashtable_contains(state *s, hashname_t alg);
void hashtable_destroy(hashtable_entry_t *e);

/* MULTIHASHING */
void multihash_initialize(state *s);
void multihash_update(state *s, unsigned char *buf, uint64_t len);
void multihash_finalize(state *s);


/* MATCHING MODES */
status_t load_match_file(state *s, char *fn);
status_t display_match_result(state *s);

int md5deep_display_hash(state *s);
int display_hash_simple(state *s);

/* AUDIT MODE */

void setup_audit(state *s);
int audit_status(state *s);
int display_audit_results(state *s);
int audit_update(state *s);

/* HASHING CODE */

int hash_file(state *s, TCHAR *file_name);
int hash_stdin(state *s);


/* HELPER FUNCTIONS */
void generate_filename(state *s, TCHAR *fn, TCHAR *cwd, TCHAR *input);
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


/* ui.c */
/* User Interface Functions */

// Display an ordinary message with newline added
void print_status(const char *fmt, ...);

// Display an error message if not in silent mode
void print_error(const state *s, const char *fmt, ...);

// Display an error message if not in silent mode with a Unicode filename
void print_error_unicode(const state *s, const TCHAR *fn, const char *fmt, ...);

// Display an error message, if not in silent mode,  
// and exit with EXIT_FAILURE
void fatal_error(const state *s, const char *fmt, ...);

// Display an error message, ask user to contact the developer, 
// and exit with EXIT_FAILURE
void internal_error(const char *fmt, ... );

// Display a filename, possibly including Unicode characters
void display_filename(FILE *out, const TCHAR *fn);

void print_debug(const char *fmt, ...);

void make_newline(const state *s);

void try_msg(void);

int display_hash(state *s);



// ----------------------------------------------------------------
// FILE MATCHING
// ---------------------------------------------------------------- 

// md5deep_match.c
int md5deep_load_match_file(state *s, char *fn);
int md5deep_is_known_hash(char *h, char *known_fn);
//int was_input_not_matched(void);
int md5deep_finalize_matching(state *s);

// Add a single hash to the matching set
void md5deep_add_hash(state *s, char *h, char *fn);

// Functions for file evaluation (files.c) 
int valid_hash(state *s, char *buf);
int hash_file_type(state *s, FILE *f);
int find_hash_in_line(state *s, char *buf, int fileType, char *filename);






__END_DECLS

#endif /* ifndef __MAIN_H */
