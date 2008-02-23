/* MD5DEEP
 *
 * By Jesse Kornblum
 *
 * This is a work of the US Government. In accordance with 17 USC 105,
 * copyright protection is not available for any work of the US Government.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

// $Id: main.h,v 1.12 2007/09/26 20:39:30 jessekornblum Exp $
   
#ifndef __MD5DEEP_H
#define __MD5DEEP_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* The version information, VERSION, is defined in config.h */

#define AUTHOR      "Jesse Kornblum"
#define COPYRIGHT   "This program is a work of the US Government. "\
"In accordance with 17 USC 105,%s"\
"copyright protection is not available for any work of the US Government.%s"\
"This is free software; see the source for copying conditions. There is NO%s"\
"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.",\
NEWLINE, NEWLINE, NEWLINE


#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

#ifdef HAVE_LIMITS_H
# include <limits.h>
#endif

#ifdef HAVE_DIRENT_H
# include <dirent.h>
#endif

#ifdef HAVE_STRING_H
# include <string.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif 

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif

#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif

#ifdef HAVE_SYS_MOUNT_H
# include <sys/mount.h>
#endif 

#ifdef HAVE_SYS_DISK_H
# include <sys/disk.h>
#endif

#ifdef HAVE_LIBGEN_H
# include <libgen.h>
#endif

/* This allows us to open standard input in binary mode by default 
   See http://gnuwin32.sourceforge.net/compile.html for more */
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#else
// This should really have been caught during configuration, but just in case...
# error Unable to work without inttypes.h!
#endif

/* A few operating systems (e.g. versions of OpenBSD) don't meet the 
   C99 standard and don't define the PRI??? macros we use to display 
   large numbers. We have to do something to help those systems, so 
   we guess. This snippet was copied from the FreeBSD source tree, 
   so hopefully it should work on the other BSDs too. */
#ifndef PRIu64 
#define PRIu64 "llu"
#endif

#include "tchar-local.h"
#include "hashTable.h"



#define TRUE   1
#define FALSE  0

#define ONE_MEGABYTE  1048576

/* Strings have to be long enough to handle inputs from matched hashing files.
   The NSRL is already larger than 256 bytes. We go longer to be safer. */
#define MAX_STRING_LENGTH    2048

/* LINE_LENGTH is different between UNIX and WIN32 and is defined below */
#define MAX_FILENAME_LENGTH   LINE_LENGTH - 41


/* These are the types of files that we can match against */
#define TYPE_PLAIN        0
#define TYPE_BSD          1
#define TYPE_HASHKEEPER   2
#define TYPE_NSRL_15      3
#define TYPE_NSRL_20      4
#define TYPE_ILOOK        5
#define TYPE_MD5DEEP_SIZE 6
#define TYPE_ENCASE       7
#define TYPE_UNKNOWN    254


#define mode_none             0
#define mode_recursive     1<<0
#define mode_estimate      1<<1
#define mode_silent        1<<2
#define mode_warn_only     1<<3
#define mode_match         1<<4
#define mode_match_neg     1<<5
#define mode_display_hash  1<<6
#define mode_display_size  1<<7
#define mode_zero          1<<8
#define mode_relative      1<<9
#define mode_which        1<<10
#define mode_barename     1<<11
#define mode_asterisk     1<<12
#define mode_not_matched  1<<13
#define mode_quiet        1<<14
#define mode_piecewise    1<<15

/* Modes 16-48 are reserved for future use.

   Note that the LL is required to avoid overflows of 32-bit words.
   LL must be used for any value equal to or above 1<<31. 
   Also note that these values can't be returned as an int type. 
   For example, any function that returns an int can't use

   return (s->mode & mode_regular);   

   That value is 64-bits wide and may not be returned correctly. */

#define mode_expert        (1LL)<<49
#define mode_regular       (1LL)<<50
#define mode_directory     (1LL)<<51
#define mode_door          (1LL)<<52
#define mode_block         (1LL)<<53
#define mode_character     (1LL)<<54
#define mode_pipe          (1LL)<<55
#define mode_socket        (1LL)<<56
#define mode_symlink       (1LL)<<57

// Modes 58-62 are reserved for future use in expert mode


/* These are the types of files we can encounter while hashing */
#define file_regular    0
#define file_directory  1
#define file_door       2
#define file_block      3
#define file_character  4
#define file_pipe       5
#define file_socket     6
#define file_symlink    7
#define file_unknown  254

// Note that STRINGS_EQUAL does not check if either A or B is NULL
#define _MAX(A,B)             (A>B)?A:B
#define STRINGS_EQUAL(A,B)   (!strncasecmp(A,B,_MAX(strlen(A),strlen(B))))


#define MD5DEEP_ALLOC(TYPE,VAR,SIZE)     \
VAR = (TYPE *)malloc(sizeof(TYPE) * SIZE);  \
if (NULL == VAR)  \
   return STATUS_INTERNAL_ERROR; \
memset(VAR,0,SIZE * sizeof(TYPE));


typedef struct _state {

  /* Basic program state */
  uint64_t      mode;
  int           return_value;
  time_t        start_time, last_time;

  /* Command line arguments */
  TCHAR        **argv;
  int            argc;

  /* The input file */
  int           is_stdin;
  FILE          * handle;
  // The size of the input file, in megabytes
  uint64_t      total_megs;
  uint64_t      total_bytes;
  uint64_t      bytes_read;
  
  /* Lists of known hashes */
  int           hashes_loaded;
  hashTable     known_hashes;
  uint32_t      expected_hashes;

  /* Size of blocks used in normal hashing */
  uint64_t      block_size;

  /* Size of blocks used in piecewise hashing */
  uint64_t      piecewise_size;

  // These strings are used in hash.c to hold the filename
  TCHAR         * full_name;
  TCHAR          * short_name;
  TCHAR          * msg;

  /* Hashing algorithms */

  /* We don't define hash_string_length, it's just twice this length. 
     We use a signed value as this gets compared with the output of strlen() */
  size_t       hash_length;
  
  // Which filetypes this algorithm supports and their position in the file
  uint8_t      h_plain, h_bsd, h_md5deep_size, h_hashkeeper;
  uint8_t      h_ilook, h_nsrl15, h_nsrl20, h_encase;
  
  // Function used to do the actual hashing
  int ( *hash_init)(struct _state *);
  int ( *hash_update)(struct _state *, unsigned char *, uint64_t );
  int ( *hash_finalize)(struct _state *, unsigned char *);

  void * hash_context;
  
  unsigned char * hash_sum;
  char          * hash_result;
  char          * known_fn;

} state;



/* The default size for hashing */
#define MD5DEEP_IDEAL_BLOCK_SIZE 8192

/* Return values for the program */
#define STATUS_OK                      0
#define STATUS_UNUSED_HASHES           1
#define STATUS_INPUT_DID_NOT_MATCH     2
#define STATUS_USER_ERROR             64
#define STATUS_INTERNAL_ERROR        128 

#define MM_STR "\x53\x41\x4E\x20\x44\x49\x4D\x41\x53\x20\x48\x49\x47\x48\x20\x53\x43\x48\x4F\x4F\x4C\x20\x46\x4F\x4F\x54\x42\x41\x4C\x4C\x20\x52\x55\x4C\x45\x53\x21"



/* Set up the environment for the *nix operating systems (Mac, Linux, 
   BSD, Solaris, and really everybody except Microsoft Windows) */
#ifndef _WIN32
#define CMD_PROMPT "$"
#define DIR_SEPARATOR   '/'
#define NEWLINE "\n"
#define LINE_LENGTH 74
#define BLANK_LINE \
"                                                                          "

#ifndef HAVE_FSEEKO
#define fseeko fseek
#define ftello ftell
#endif


#else   // ifndef _WIN32


/* The current cross compiler for OS X->Windows does not support a few
   critical error codes normally defined in errno.h. Because we need 
   these to detect fatal errors while reading files, we have them here. 
   These will hopefully get wrapped into the Windows API sometime soon. */
#ifndef ENOTBLK
#define ENOTBLK   15   // Not a block device
#endif

#ifndef ETXTBSY
#define ETXTBSY   26   // Text file busy
#endif

#ifndef EAGAIN
#define EAGAIN    35   // Resource temporarily unavailable
#endif

#ifndef EALREADY
#define EALREADY  37   // Operation already in progress
#endif


#define CMD_PROMPT "c:\\>"
#define DIR_SEPARATOR   '\\'
#define NEWLINE "\r\n"
#define LINE_LENGTH 72
#define BLANK_LINE \
"                                                                        "

#define ftello   ftell
#define fseeko   fseek

/* We create macros for the Windows equivalent UNIX functions.
   No worries about lstat to stat; Windows doesn't have symbolic links 
   This function has been replaced with _lstat in the code. See tchar-local.h */
//#define lstat(A,B)      stat(A,B)
#define realpath(A,B)   _fullpath(B,A,PATH_MAX) 

char *basename(char *a);
extern char *optarg;
extern int optind;
int getopt(int argc, char *const argv[], const char *optstring);

#endif   /* ifndef _WIN32,#else */


/* On non-glibc systems we have to manually set the __progname variable */
#ifdef __GLIBC__
extern char *__progname;
#else
char *__progname;
#endif /* ifdef __GLIBC__ */




/* ----------------------------------------------------------------
   PROGRAM ENGINE
   ---------------------------------------------------------------- */

/* Dig into the file hierachy and hash file accordingly */
int process_normal(state *s, TCHAR *input);
int process_win32(state *s, TCHAR *fn);

/* Hashing functions */
int hash_file(state *s, TCHAR *file_name);
int hash_stdin(state *s);

/* Sets up hashing algorithm and allocates memory */
int setup_hashing_algorithm(state *s);



/* ----------------------------------------------------------------
   CYCLE CHECKING
   ---------------------------------------------------------------- */
int have_processed_dir(TCHAR *fn);
int processing_dir(TCHAR *fn);
int done_processing_dir(TCHAR *fn);



/* ----------------------------------------------------------------
   FILE MATCHING
   ---------------------------------------------------------------- */

/* Load a file of known hashes from the disk */
int load_match_file(state *s, char *fn);

int is_known_hash(char *h, char *known_fn);
int was_input_not_matched(void);
int finalize_matching(state *s);

// Add a single hash to the matching set
void add_hash(state *s, char *h, char *fn);

/* Functions for file evaluation (files.c) */
int valid_hash(state *s, char *buf);
int hash_file_type(state *s, FILE *f);
int find_hash_in_line(state *s, char *buf, int fileType, char *filename);




/* ----------------------------------------------------------------
   USER INTERFACE

   These are all command line functions, but could be replaced
   with GUI functions if desired.
   ---------------------------------------------------------------- */

// Display an ordinary message with newline added
void print_status(char *fmt, ...);

// Display an error message if not in silent mode
void print_error(state *s, char *fmt, ...);

// Display an error message if not in silent mode with a Unicode filename
void print_error_unicode(state *s, TCHAR *fn, char *fmt, ...);

// Display an error message, if not in silent mode,  
// and exit with EXIT_FAILURE
void fatal_error(state *s, char *fmt, ...);

// Display an error message, ask user to contact the developer, 
// and exit with EXIT_FAILURE
void internal_error(char *fmt, ... );

// Display a filename, possibly including Unicode characters
void display_filename(FILE *out, TCHAR *fn);

void print_debug(char *fmt, ...);

void make_newline(state *s);


/* ------------------------------------------------------------------
   HASH TABLE
   ------------------------------------------------------------------ */

void hashTableInit(hashTable *knownHashes);

/* Adds the string n to the hashTable, along with the filename fn.
Returns TRUE if an error occured (i.e. Out of memory) */
int hashTableAdd(state *s, hashTable *knownHashes, char *n, char *fn);

/* Returns TRUE if the hashTable contains the hash n and stores the
filename of the known hash in known. Returns FALSE and does not
alter known if the hashTable does not contain n. This function
assumes that fn has already been malloc'ed to hold at least 
PATH_MAX characters */
int hashTableContains(hashTable *knownHashes, char *n, char *known);

/* Find any hashes that have not been used. If there are any, and display
is TRUE, prints them to stdout. Regardless of display, then returns
TRUE. If there are no unused hashes, returns FALSE. */
int hashTableDisplayNotMatched(hashTable *t, int display);

/* This function is for debugging */
void hashTableEvaluate(hashTable *knownHashes);




/* ------------------------------------------------------------------
   HELPER FUNCTIONS
   ------------------------------------------------------------------ */

void shift_string(char *fn, size_t start, size_t new_start);

// Works like dirname(3), but accepts a TCHAR* instead of char*
int my_dirname(TCHAR *c);
int my_basename(TCHAR *s);

int find_comma_separated_string(char *s, unsigned int n);
int find_quoted_string(char *buf, unsigned int n);

/* Return the size, in bytes of an open file stream. On error, return -1 */
off_t find_file_size(FILE *f);

#endif /* __MD5DEEP_H */



