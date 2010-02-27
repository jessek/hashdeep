
// $Id$ 

#ifndef __COMMON_H
#define __COMMON_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef _WIN32
// Required to enable 64-bit stat functions
# define __MSVCRT_VERSION__ 0x0601
#endif

// The version information, VERSION, is defined in config.h 

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

// This allows us to open standard input in binary mode by default 
// See http://gnuwin32.sourceforge.net/compile.html for more 
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#else
// This should really have been caught during configuration, but just in case...
# error Unable to work without inttypes.h!
#endif

// A few operating systems (e.g. versions of OpenBSD) don't meet the 
// C99 standard and don't define the PRI??? macros we use to display 
// large numbers. We have to do something to help those systems, so 
// we guess. This snippet was copied from the FreeBSD source tree, 
// so hopefully it should work on the other BSDs too. 
#ifndef PRIu64 
#define PRIu64 "llu"
#endif

#include "tchar-local.h"

#include "md5.h"
#include "sha1.h"
#include "sha256.h"
#include "tiger.h"
#include "whirlpool.h"


#define TRUE   1
#define FALSE  0

#define ONE_MEGABYTE  1048576

// Strings have to be long enough to handle inputs from matched hashing files.
// The NSRL is already larger than 256 bytes. We go longer to be safer. 
#define MAX_STRING_LENGTH    2048

#define MAX_TIME_STRING_LENGTH  31

// LINE_LENGTH is different between UNIX and WIN32 and is defined below 
#define MAX_FILENAME_LENGTH   LINE_LENGTH - 41

// Note that STRINGS_EQUAL does not check if either A or B is NULL 
#define _MAX(A,B)             (A>B)?A:B
#define STRINGS_CASE_EQUAL(A,B)   (!strncasecmp(A,B,_MAX(strlen(A),strlen(B))))
#define STRINGS_EQUAL(A,B)        (!strncmp(A,B,_MAX(strlen(A),strlen(B))))
#define WSTRINGS_EQUAL(A,B)       (!_tcsncmp(A,B,_MAX(_tcslen(A),_tcslen(B))))




// On non-glibc systems we have to manually set the __progname variable 
#ifdef __GLIBC__
extern char *__progname;
#else
char *__progname;
#endif // ifdef __GLIBC__ 

// Set up the environment for the *nix operating systems (Mac, Linux, 
// BSD, Solaris, and really everybody except Microsoft Windows) 
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


// The current cross compiler for OS X->Windows does not support a few
// critical error codes normally defined in errno.h. Because we need 
// these to detect fatal errors while reading files, we have them here. 
// These will hopefully get wrapped into the Windows API sometime soon. 
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


#define CMD_PROMPT "C:\\>"
#define DIR_SEPARATOR   '\\'

// RBF - Resolve NEWLINE variable
// Testing on Vista shows it needs to be \r\n, but on other
// system \n seems to work ok. Which is it? 
// If we end up using \n, we should move it out of the conditional defines

#define NEWLINE "\n"
#define LINE_LENGTH 72
#define BLANK_LINE \
"                                                                        "

#define ftello   ftell
#define fseeko   fseek

//  We create macros for the Windows equivalent UNIX functions.
// No worries about lstat to stat; Windows doesn't have symbolic links 
// This function has been replaced with _lstat in the code. See tchar-local.h 
//#define lstat(A,B)      stat(A,B)
#define realpath(A,B)   _fullpath(B,A,PATH_MAX) 

char *basename(char *a);
extern char *optarg;
extern int optind;
int getopt(int argc, char *const argv[], const char *optstring);

#endif   /* ifndef _WIN32,#else */

#define MD5DEEP_ALLOC(TYPE,VAR,SIZE)     \
VAR = (TYPE *)malloc(sizeof(TYPE) * SIZE);  \
if (NULL == VAR)  \
   return STATUS_INTERNAL_ERROR; \
memset(VAR,0,SIZE * sizeof(TYPE));

// The default size for hashing 
#define MD5DEEP_IDEAL_BLOCK_SIZE 8192

#define HASH_STRING_LENGTH   (s->hash_length * 2)

// Return values for the program 
// RBF - Document these return values for hashdeep 
#define STATUS_OK                      0
#define STATUS_UNUSED_HASHES           1
#define STATUS_INPUT_DID_NOT_MATCH     2
#define STATUS_USER_ERROR             64
#define STATUS_INTERNAL_ERROR        128 


#define mode_none              0
#define mode_recursive         1<<0
#define mode_estimate          1<<1
#define mode_silent            1<<2
#define mode_warn_only         1<<3
#define mode_match             1<<4
#define mode_match_neg         1<<5
#define mode_display_hash      1<<6
#define mode_display_size      1<<7
#define mode_zero              1<<8
#define mode_relative          1<<9
#define mode_which             1<<10
#define mode_barename          1<<11
#define mode_asterisk          1<<12
#define mode_not_matched       1<<13
#define mode_quiet             1<<14
#define mode_piecewise         1<<15
#define mode_verbose           1<<16
#define mode_more_verbose      1<<17
#define mode_insanely_verbose  1<<18
#define mode_size              1<<19
#define mode_size_all          1<<20
#define mode_timestamp         1<<21
#define mode_csv               1<<22

#define mode_read_from_file    1<<25

// Modes 26-48 are reserved for future use.
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

// Modes 58-62 are reserved for future use in expert mode


// These are the types of files we can encounter while hashing 

#define stat_regular    0
#define stat_directory  1
#define stat_door       2
#define stat_block      3
#define stat_character  4
#define stat_pipe       5
#define stat_socket     6
#define stat_symlink    7
#define stat_unknown  254

// Types of files that contain known hashes 
typedef enum 
  {
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




typedef struct _state state;

// ----------------------------------------------------------------
// CYCLE CHECKING
// ----------------------------------------------------------------
int have_processed_dir(TCHAR *fn);
int processing_dir(TCHAR *fn);
int done_processing_dir(TCHAR *fn);

// ------------------------------------------------------------------
// HELPER FUNCTIONS
// ------------------------------------------------------------------ 
void setup_expert_mode(state *s, char *arg);

void generate_filename(state *s, TCHAR *fn, TCHAR *cwd, TCHAR *input);

uint64_t find_block_size(state *s, char *input_str);

void chop_line(char *s);


void shift_string(char *fn, size_t start, size_t new_start);

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






#endif /* ifndef __COMMON_H */
