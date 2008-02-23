
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

/* We use \r\n for newlines as this has to work on Win32. It's redundant for
   everybody else, but shouldn't cause any harm. */
#define COPYRIGHT   "This program is a work of the US Government. "\
"In accordance with 17 USC 105,\r\n"\
"copyright protection is not available for any work of the US Government.\r\n"\
"This is free software; see the source for copying conditions. There is NO\r\n"\
"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\r\n"

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

#define MAX(A,B)             (A>B)?A:B
#define STRINGS_EQUAL(A,B)   (!strncasecmp(A,B,MAX(strlen(A),strlen(B))))


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
#define MAX_STRING_LENGTH   1024

/* LINE_LENGTH is different between UNIX and WIN32 and is defined below */
#define MAX_FILENAME_LENGTH   LINE_LENGTH - 41

/* These are the types of files that we can match against */
#define TYPE_PLAIN        0
#define TYPE_BSD          1
#define TYPE_HASHKEEPER   2
#define TYPE_NSRL_15      3
#define TYPE_NSRL_20      4
#define TYPE_ILOOK        5
#define TYPE_UNKNOWN    254


#define mode_none             0
#define mode_recursive     1<<0
#define mode_estimate      1<<1
#define mode_silent        1<<2
#define mode_match         1<<3
#define mode_match_neg     1<<4
#define mode_display_hash  1<<5
#define mode_display_size  1<<6
#define mode_zero          1<<7
#define mode_relative      1<<8
#define mode_which         1<<9
#define mode_barename     1<<10
#define mode_asterisk     1<<11
#define mode_not_matched  1<<12

/* Modes 13 to 22 and 32 to 63 are reserved for future use. 
   (Yes, I could move the expert file modes, below, up to the higher
   ranger of numbers, but it's working now, and so why change anything?
   The next person who wants to add a lot of modes can have the fun.) */

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

#define M_MATCH(A)         (A & mode_match)
#define M_MATCHNEG(A)      (A & mode_match_neg)
#define M_RECURSIVE(A)     (A & mode_recursive)
#define M_ESTIMATE(A)      (A & mode_estimate)
#define M_SILENT(A)        (A & mode_silent)
#define M_DISPLAY_HASH(A)  (A & mode_display_hash)
#define M_DISPLAY_SIZE(A)  (A & mode_display_size)
#define M_ZERO(A)          (A & mode_zero)
#define M_RELATIVE(A)      (A & mode_relative)
#define M_WHICH(A)         (A & mode_which)
#define M_BARENAME(A)      (A & mode_barename)
#define M_ASTERISK(A)      (A & mode_asterisk)
#define M_NOT_MATCHED(A)   (A & mode_not_matched)

#define M_EXPERT(A)        (A & mode_expert)
#define M_REGULAR(A)       (A & mode_regular)
#define M_BLOCK(A)         (A & mode_block)
#define M_CHARACTER(A)     (A & mode_character)
#define M_PIPE(A)          (A & mode_pipe)
#define M_SOCKET(A)        (A & mode_socket)
#define M_DOOR(A)          (A & mode_door)
#define M_SYMLINK(A)       (A & mode_symlink)


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


/* Set up the environment for the *nix operating systems (Mac, Linux, 
   BSD, Solaris, and really everybody except Microsoft Windows) */
#ifndef _WIN32

#include <libgen.h>

// These prototypes help us avoid compiler warnings on older systems 
int fseeko(FILE *stream, off_t offset, int whence);
off_t ftello(FILE *stream);

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


/* By default BUFSIZ is 512 on Windows. We make it 8192 so that it's 
   the same as UNIX. While that shouldn't mean anything in terms of
   computing the hash values, it should speed us up a little bit. */
#ifdef BUFSIZ
#undef BUFSIZ
#endif
#define BUFSIZ 8192

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
int have_processed_dir(uint64_t mode, char *fn);
int processing_dir(uint64_t mode, char *fn);
int done_processing_dir(uint64_t mode, char *fn);

/* Functions from matching (match.c) */
int load_match_file(uint64_t mode, char *filename);
int is_known_hash(char *h, char *known_fn);
int was_input_not_matched(void);
int finalize_matching(uint64_t mode);

// Add a single hash to the matching set
void add_hash(uint64_t mode, char *h, char *fn);

/* Functions for file evaluation (files.c) */
int valid_hash(char *buf);
int hash_file_type(FILE *f);
int find_hash_in_line(char *buf, int fileType, char *filename);

/* Dig into file hierarchies */
int process(uint64_t mode, char *input);

/* Hashing functions */
int hash_file(uint64_t mode, char *filename);
int hash_stdin(uint64_t mode);

/* Miscellaneous helper functions */
void shift_string(char *fn, int start, int new_start);
void print_error(uint64_t mode, char *fn, char *msg);
void internal_error(char *fn, char *msg);
void make_newline(uint64_t mode);
int find_comma_separated_string(char *s, unsigned int n);
int find_quoted_string(char *buf, unsigned int n);

/* Return the size, in bytes of an open file stream. On error, return -1 */
off_t find_file_size(FILE *f);

#endif /* __MD5DEEP_H */



