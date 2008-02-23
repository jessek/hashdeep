
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

   
#ifndef __MD5DEEP_H
#define __MD5DEEP_H

/* Version information is defined in the Makefile */

#define AUTHOR      "Jesse Kornblum"

#define COPYRIGHT   "This program is a work of the US Government. "\
"In accordance with 17 USC 105,%s"\
"copyright protection is not available for any work of the US Government.%s"\
"This is free software; see the source for copying conditions. There is NO%s"\
"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.",\
NEWLINE, NEWLINE, NEWLINE

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <inttypes.h>

#ifdef __LINUX
#include <sys/ioctl.h>
#include <sys/mount.h>
#endif 

#include "hashTable.h"

/* This allows us to open standard input in binary mode by default 
   See http://gnuwin32.sourceforge.net/compile.html for more */
#include <fcntl.h>

/* A few operating systems (e.g. versions of OpenBSD) don't meet the 
   C99 standard and don't define the PRI??? macros we use to display 
   large numbers. We have to do something to help those systems, so 
   we guess. This snippet was copied from the FreeBSD source tree, 
   so hopefully it should work on the other BSDs too. */
#ifndef PRIu64 
#define PRIu64 "llu"
#endif

// Note that STRINGS_EQUAL does not check if either A or B is NULL
#define _MAX(A,B)             (A>B)?A:B
#define STRINGS_EQUAL(A,B)   (!strncasecmp(A,B,_MAX(strlen(A),strlen(B))))


/* MD5 and SHA-1 setup require knowing if we're big or little endian */
#ifdef __LINUX

#ifndef __USE_BSD
#define __USE_BSD
#endif
#include <endian.h>

#elif defined (__SOLARIS)

#define BIG_ENDIAN    4321
#define LITTLE_ENDIAN 1234

#include <sys/isa_defs.h>
#ifdef _BIG_ENDIAN       
#define BYTE_ORDER BIG_ENDIAN
#else
#define BYTE_ORDER LITTLE_ENDIAN
#endif

#elif defined (_WIN32)
#include <sys/param.h>

#elif defined (__APPLE__)
#include <machine/endian.h>

#elif defined (__HPUX)
#ifndef BIG_ENDIAN
#define BIG_ENDIAN 4321
#endif

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1234
#endif

#ifndef BYTE_ORDER
#define BYTE_ORDER BIG_ENDIAN
#endif

#endif


// Some algorithms need to know if this is a big endian host
#if BYTE_ORDER == BIG_ENDIAN
// For MD5
#define HIGHFIRST 1
// For Tiger
#define BIG_ENDIAN_HOST
#endif //   #if BYTE_ORDER == BIG_ENDIAN


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

// Modes 16 to 22 are reserved for future use. 

#define mode_regular       1<<23
#define mode_directory     1<<24
#define mode_door          1<<25
#define mode_block         1<<26
#define mode_character     1<<27
#define mode_pipe          1<<28
#define mode_socket        1<<29
#define mode_symlink       1<<30
#define mode_expert        1<<31


/* These are the types of files we can encounter while hashing */
#define file_regular    0
#define file_directory  1
#define file_door       3
#define file_block      4
#define file_character  5
#define file_pipe       6
#define file_socket     7
#define file_symlink    8
#define file_unknown  254




typedef struct _state {
  
  // Basic program state
  uint64_t   mode, piecewise_block;
  int        hashes_loaded, return_value;
  hashTable  known_hashes;
  char       *msg;

  // Used exclusively during hashing
  // Note that we can't put the hash context in here as it depends
  // on which algorithm we are executing
  char *full_name;
  char *short_name;

  // The size of the input file, in megabytes
  uint64_t total_megs;

  uint64_t total_bytes;
  uint64_t bytes_read;

  uint64_t block_size;

  char *result;

  FILE *handle;
  int is_stdin;
} state;

#define MD5DEEP_IDEAL_BLOCK_SIZE 8192


// Return values for the program
#define STATUS_OK                      0
#define STATUS_UNUSED_HASHES           1
#define STATUS_INPUT_DID_NOT_MATCH     2

#define STATUS_USER_ERROR             64
#define STATUS_INTERNAL_ERROR        128 


#ifdef __SOLARIS
#define   u_int32_t   unsigned int
#define   u_int64_t   unsigned long
#endif 

#ifdef __HPUX
#define   u_int32_t   unsigned int

#ifdef __LP64__
#define   u_int64_t   unsigned long
#else
#define   u_int64_t   unsigned long long
#endif

#endif

/* Set up the environment for the *nix operating systems (Mac, Linux, 
   BSD, Solaris, and really everybody except Microsoft Windows) */
#ifndef _WIN32

#include <libgen.h>

// These prototypes help us avoid compiler warnings on older systems 

#ifndef __HPUX
int fseeko(FILE *stream, off_t offset, int whence);
off_t ftello(FILE *stream);
#endif

#define CMD_PROMPT "$"
#define DIR_SEPARATOR   '/'
#define NEWLINE "\n"
#define LINE_LENGTH 74
#define BLANK_LINE \
"                                                                          "

#endif // #ifndef _WIN32 


#ifdef _WIN32

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

#define  snprintf         _snprintf
#define  u_int32_t        unsigned long

/* We create macros for the Windows equivalent UNIX functions.
   No worries about lstat to stat; Windows doesn't have symbolic links */
#define lstat(A,B)      stat(A,B)

#define realpath(A,B)   _fullpath(B,A,PATH_MAX) 

/* Not used in md5deep anymore, but left in here in case I 
   ever need it again. Win32 documentation searches are evil.
   int asprintf(char **strp, const char *fmt, ...);  */

char *basename(char *a);
extern char *optarg;
extern int optind;
int getopt(int argc, char *const argv[], const char *optstring);

#endif   /* ifdef _WIN32 */


/* On non-glibc systems we have to manually set the __progname variable */
#ifdef __GLIBC__
extern char *__progname;
#else
char *__progname;
#endif /* ifdef __GLIBC__ */


/* The algorithm headers need all of the operating system specific 
   defines, so we don't add them until down here */
#include "algorithms.h"

/* -----------------------------------------------------------------
   Function definitions
   ----------------------------------------------------------------- */

/* To avoid cycles */
int have_processed_dir(char *fn);
int processing_dir(char *fn);
int done_processing_dir(char *fn);

/* Functions from matching (match.c) */
int load_match_file(state *s, char *fn);

int is_known_hash(char *h, char *known_fn);
int was_input_not_matched(void);
int finalize_matching(state *s);

// Add a single hash to the matching set
void add_hash(state *s, char *h, char *fn);

/* Functions for file evaluation (files.c) */
int valid_hash(char *buf);
int hash_file_type(FILE *f);
int find_hash_in_line(char *buf, int fileType, char *filename);

/* Dig into file hierarchies */
int process(state *s, char *input);

/* Hashing functions */
int hash_file(state *s, char *filename);
int hash_stdin(state *s);

/* Miscellaneous helper functions */
void shift_string(char *fn, size_t start, size_t new_start);

// ----------------------------------------------------------------
// User interface functions
// ----------------------------------------------------------------

// Display an ordinary message with newline added
void print_status(char *fmt, ...);

// Display an error message if not in silent mode
void print_error(state *s, char *fmt, ...);

// Display an error message, if not in silent mode,  
// and exit with EXIT_FAILURE
void fatal_error(state *s, char *fmt, ...);

// Display an error message, ask user to contact the developer, 
// and exit with EXIT_FAILURE
void internal_error(char *fmt, ... );


void print_debug(char *fmt, ...);

void make_newline(state *s);
int find_comma_separated_string(char *s, unsigned int n);
int find_quoted_string(char *buf, unsigned int n);

/* Return the size, in bytes of an open file stream. On error, return -1 */
off_t find_file_size(FILE *f);

#endif /* __MD5DEEP_H */



