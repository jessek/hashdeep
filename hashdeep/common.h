/**
 * $Id$
 * 
 * This file provides common include files but no specifics for the hashdeep/md5deep system.
 *
 * The version information, VERSION, is defined in config.h 
 * AUTHOR and COPYRIGHT moved to main.cpp
 *
 */

#ifndef __COMMON_H
#define __COMMON_H

#define TRUE   1
#define FALSE  0
#define ONE_MEGABYTE  1048576

#include "config.h"

#include <assert.h>
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

#ifdef HAVE_SYS_CDEFS_H
# include <sys/cdefs.h>
#endif

// This allows us to open standard input in binary mode by default 
// See http://gnuwin32.sourceforge.net/compile.html for more 
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

/* If we are including inttypes.h, mmake sure __STDC_FORMAT_MACROS is defined */
#ifdef HAVE_INTTYPES_H
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
# include <inttypes.h>
#else
// This should really have been caught during configuration, but just in case...
# error Unable to work without inttypes.h!
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

// A few operating systems (e.g. versions of OpenBSD) don't meet the 
// C99 standard and don't define the PRI??? macros we use to display 
// large numbers. We have to do something to help those systems, so 
// we guess. This snippet was copied from the FreeBSD source tree, 
// so hopefully it should work on the other BSDs too. 
#ifndef PRIu64 
#define PRIu64 "llu"
#endif

//#include "md5.h"
//#include "sha1.h"
//#include "sha256.h"
//#include "tiger.h"
//#include "whirlpool.h"


// Strings have to be long enough to handle inputs from matched hashing files.
// The NSRL is already larger than 256 bytes. We go longer to be safer. 
#define MAX_STRING_LENGTH    2048
#define MAX_TIME_STRING_LENGTH  31

// This denotes when we don't know the file size.
#define UNKNOWN_FILE_SIZE  0xfffffffffffffffeLL

// LINE_LENGTH is different between UNIX and WIN32 and is defined below 
#define MAX_FILENAME_LENGTH   LINE_LENGTH - 41

#if defined(__cplusplus)
#include <string>
#include <algorithm>
#include <iostream>
#include <ctype.h>
#include <vector>

/* Some nice C++ manipulation routines */

inline std::string makelower(const std::string &a)
{
    std::string ret(a);
    std::transform(ret.begin(), ret.end(), ret.begin(), ::tolower);
    return ret;
}

inline std::string makeupper(const std::string &a)
{
    std::string ret(a);
    std::transform(ret.begin(), ret.end(), ret.begin(), ::toupper);
    return ret;
}

inline bool STRINGS_CASE_EQUAL(const char *a,const char *b)
{
    return strcasecmp(a,b)==0;
}

inline bool STRINGS_CASE_EQUAL(const std::string &a,const std::string &b)
{
    return makelower(a)==makelower(b);
}

inline bool STRINGS_EQUAL(const char *a,const char *b)
{
    return strcmp(a,b)==0;
}

inline bool STRINGS_EQUAL(const std::string &a,const std::string &b)
{
    return a==b;
}
#endif


#ifdef _WIN32
/*****************************************************************
 *** Windows support.
 *** Previously in tchar-local.h.
 *** Moved here for simplicity
 * TCHAR:
 *
 * On POSIX systems, TCHAR is defined to be char.
 * On WIN32 systems, TCHAR is wchar_t.
 *
 * TCHAR is used for filenames of files to hash.
 *
 * We have a wstring derrived type which we use internally as wide-char strings.
 *
 * We can convert this to UTF-8 using the GNU utf8 package from sourceforce.
 * 
 *
 */



// Required to enable 64-bit stat functions, but defined by default on mingw
#ifndef __MSVCRT_VERSION__ 
# define __MSVCRT_VERSION__ 0x0601
#endif
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <wchar.h>

#if defined(__cplusplus)
/*
 * C++ implementation of a wchar_t string.
 * It works like the regular string, except that it works with wide chars.
 * Notice that .c_str() returns a wchar_t * pointer.
 */
class wstring : public vector<wchar> {
    wstring(wchar_t *buf){
	size_t len = wcslen(buf);
	for(size_t i = 0;i<len;i++){
	    this->push_back(buf[i]);
	}
    }
};
/* 
 * Internally we use tstring.
 * ON WIN32: we get a wstring.
 * ON POSIX: we get a std::string.
 */ 

typedef wstring tstring; 
#endif

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
#endif

// Set up the environment for the *nix operating systems (Mac, Linux, 
// BSD, Solaris, and really everybody except Microsoft Windows) 
//
// Do this by faking the wide-character functions

#ifndef _WIN32
/* The next few paragraphs are similar to tchar.h when UNICODE
 * is not defined---that is, in the absence of a wide-char mode,
 * all become the standard char * functions.
 * tstring is then typedef'ed to be a std::string.
 * This works just fine on Linux and OS X
 */

typedef char TCHAR;
#define  _TDIR      DIR
#define  _TEXT(A)   A
#define  _tfopen    fopen

#define  _topendir  opendir
#define  _treaddir  readdir
#define  _tdirent   dirent
#define  _tclosedir closedir

#define  _lstat     lstat
#define  _sstat     stat
#define  _tstat_t   struct stat


#if defined(__cplusplus)
typedef std::string tstring;
#endif

#  define CMD_PROMPT	"$"
#  define DIR_SEPARATOR   '/'
#  define NEWLINE		"\n"
#  define LINE_LENGTH	74
#  define BLANK_LINE \
"                                                                          "

#  ifndef HAVE_FSEEKO
#    define fseeko fseek
#    define ftello ftell
#  endif
#endif

#endif /* ifndef __COMMON_H */
