// MD5DEEP
//
// By Jesse Kornblum
//
// This is a work of the US Government. In accordance with 17 USC 105,
// copyright protection is not available for any work of the US Government.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
//

// $Id: main.h,v 1.5 2007/12/08 16:34:50 jessekornblum Exp $
   
#ifndef __MD5DEEP_H
#define __MD5DEEP_H

#include "common.h"
#include "hashTable.h"




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

  // Basic program state 
  uint64_t      mode;
  int           return_value;
  time_t        start_time, last_time;

  // Command line arguments 
  TCHAR        **argv;
  int            argc;
  char          *input_list;

  // The input file 
  int           is_stdin;
  FILE          * handle;

  // The type of file, as report by stat
  uint8_t       input_type;

  // How many bytes (and megs) we think are in the file, via stat(2)
  // and how many bytes we've actually read in the file
  uint64_t        stat_bytes;
  uint64_t        stat_megs;
  uint64_t        actual_bytes;

  // Where the last read operation started and ended
  // bytes_read is a shorthand for read_end - read_start
  uint64_t      read_start;
  uint64_t      read_end;
  uint64_t      bytes_read;

  // RBF - Should this conditional define be handled in tchar-local.h?
#ifdef _WIN32
  __time64_t    timestamp;
#else
  time_t        timestamp;
#endif
  char          * time_str;
  
  // Lists of known hashes 
  int           hashes_loaded;
  hashTable     known_hashes;
  uint32_t      expected_hashes;

  // Size of blocks used in normal hashing 
  uint64_t      block_size;

  // Size of blocks used in piecewise hashing
  uint64_t      piecewise_size;

  // Size threshold for hashing small files only
  uint64_t      size_threshold;

  // These strings are used in hash.c to hold the filename
  TCHAR         * full_name;
  TCHAR         * short_name;
  TCHAR         * msg;

  // Hashing algorithms 

  // We don't define hash_string_length, it's just twice this length. 
  // We use a signed value as this gets compared with the output of strlen() */
  size_t       hash_length;
  
  // Which filetypes this algorithm supports and their position in the file
  uint8_t      h_plain, h_bsd, h_md5deep_size, h_hashkeeper;
  uint8_t      h_ilook, h_ilook3, h_ilook4, h_nsrl15, h_nsrl20, h_encase;
  
  // Functions used to do the actual hashing
  int ( *hash_init)(void *);
  int ( *hash_update)(void *, unsigned char *, uint64_t );
  int ( *hash_finalize)(void *, unsigned char *);

  void * hash_context;
  
  unsigned char * hash_sum;
  char          * hash_result;

  // Used in matching operations
  char          * known_fn;

} _state;




void sanity_check(state *s, int condition, char *msg);

// ----------------------------------------------------------------
// PROGRAM ENGINE
// ---------------------------------------------------------------- 

// Hashing functions 
int hash_file(state *s, TCHAR *file_name);
int hash_stdin(state *s);

// Sets up hashing algorithm and allocates memory 
int setup_hashing_algorithm(state *s);




// ----------------------------------------------------------------
// FILE MATCHING
// ---------------------------------------------------------------- 

// Load a file of known hashes from the disk 
int load_match_file(state *s, char *fn);

int is_known_hash(char *h, char *known_fn);
int was_input_not_matched(void);
int finalize_matching(state *s);

// Add a single hash to the matching set
void add_hash(state *s, char *h, char *fn);

// Functions for file evaluation (files.c) 
int valid_hash(state *s, char *buf);
int hash_file_type(state *s, FILE *f);
int find_hash_in_line(state *s, char *buf, int fileType, char *filename);



#include "ui.h"



// ------------------------------------------------------------------
// HASH TABLE
// ------------------------------------------------------------------ 

void hashTableInit(hashTable *knownHashes);

// Adds the string n to the hashTable, along with the filename fn.
// Returns TRUE if an error occured (i.e. Out of memory) 
int hashTableAdd(state *s, hashTable *knownHashes, char *n, char *fn);

// Returns TRUE if the hashTable contains the hash n and stores the
// filename of the known hash in known. Returns FALSE and does not
// alter known if the hashTable does not contain n. This function
// assumes that fn has already been malloc'ed to hold at least 
// PATH_MAX characters 
int hashTableContains(hashTable *knownHashes, char *n, char *known);

// Find any hashes that have not been used. If there are any, and display
// is TRUE, prints them to stdout. Regardless of display, then returns
// TRUE. If there are no unused hashes, returns FALSE. 
int hashTableDisplayNotMatched(hashTable *t, int display);

// This function is for debugging 
void hashTableEvaluate(hashTable *knownHashes);




#endif //  ifndef __MD5DEEP_H 



