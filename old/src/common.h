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

#include "config.h"

#define TRUE   1
#define FALSE  0
#define ONE_MEGABYTE  1048576

#define MAX_ALGORITHM_RESIDUE_SIZE 256
// Raised for SHA-3
#define MAX_ALGORITHM_CONTEXT_SIZE 384

#ifdef _WIN32
/* For some reason this doesn't work properly with mingw */
#undef HAVE_EXTERN_PROGNAME
#endif

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

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_TIME_H
#include <time.h>
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

#ifdef HAVE_PTHREAD_H
# include <pthread.h>
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

// Strings have to be long enough to handle inputs from matched hashing files.
// The NSRL is already larger than 256 bytes. We go longer to be safer. 
#define MAX_STRING_LENGTH    2048
#define MAX_TIME_STRING_LENGTH  31

// This denotes when we don't know the file size.
#define UNKNOWN_FILE_SIZE  0xfffffffffffffffeLL

// LINE_LENGTH is different between UNIX and WIN32 and is defined below 
#define MAX_FILENAME_LENGTH   LINE_LENGTH - 41

#ifdef HAVE_MMAP_H
#include <mmap.h>
#endif

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#if defined(__cplusplus)
#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <ctype.h>
#include <vector>
#include <sstream>


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
 * We can convert this to UTF-8 using the GNU utf8 package from sourceforce.
 * 
 *
 */

/*
 * __MSVCRT_VERSION__ specifies which version of Microsoft's DLL we require.
 * Mingw defines this to be 0x0600 by default.
 * 
 * We want version 0x0601 by default to get 64-bit stat functions and _gmtime64.
 * This is defined in configure.ac.
 * 
 * If we aren't compiling under mingw, define it by hand.
 * (Perhaps some poor soul was forced to port this project to VC++.)
 */
#ifndef __MSVCRT_VERSION__ 
#define __MSVCRT_VERSION__ 0x0601
#endif

/* For reasons I don't understand we typedef wchar_t TCHAR doesn't
 * work for some reasons for mingw, so we use this.
 */
#ifndef _TCHAR_DEFINED
//#define TCHAR wchar_t
//#define _TCHAR wchar_t
//#define _TCHAR_DEFINED
#endif

#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <wchar.h>

#if defined(__cplusplus)
#include <string>
/* 
 * Internally we use tstring.
 * ON WIN32: we get a std::wstring.
 * ON POSIX: we get a std::string.
 */ 

#define tstring std::wstring 
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

#define NEWLINE "\n"
#define CMD_PROMPT "C:\\>"
#define DIR_SEPARATOR   '\\'

#define LINE_LENGTH 72
#define ftello   ftello64	/* use the 64-bit version */
#define fseeko   fseeko64	/* use the 64-bit version */
#define off_t    off64_t
#define tlstat64(path,buf)   _wstat64(path,buf) // on windows, use _wstat64
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

typedef char TCHAR;			// no TCHAR on POSIX; use CHAR
#define  _TDIR      DIR			// no _TDIR, use dir...
#define  _TEXT(A)   A
#define  _T(A)      A
#define  _tfopen    fopen
#define  _topen	    open

#define  _topendir  opendir
#define  _treaddir  readdir
#define  _tdirent   dirent
#define  _tclosedir closedir

#define  _lstat     lstat
#define  _wstat64(path,buf)   stat(path,buf)
#define  __stat64   stat


#if defined(__cplusplus)
typedef std::string tstring;
#endif

#  define CMD_PROMPT	"$"
#  define DIR_SEPARATOR   '/'
#  define NEWLINE		"\n"
#  define LINE_LENGTH	74


#  ifndef HAVE_FSEEKO
#    define fseeko fseek
#    define ftello ftell
#  endif
#endif

#ifndef __BEGIN_DECLS
#if defined(__cplusplus)
#define __BEGIN_DECLS   extern "C" {
#define __END_DECLS     }
#else
#define __BEGIN_DECLS
#define __END_DECLS
#endif
#endif
/* End Win32 */
#endif /* ifndef __COMMON_H */
