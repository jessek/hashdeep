/**
 *
 * tchar-local.h provides support for tchar under windows, and mapping
 * to standard POSIX calls under Linux and MacOS.
 */

#ifndef __TCHAR_LOCAL_H
#define __TCHAR_LOCAL_H

// $Id$

__BEGIN_DECLS

// Unicode support 
#ifdef _WIN32

// These defines are required to enable Unicode support.
// Some of the build scripts define them for us, some don't.
// We use conditional defines to avoid redefining them.
#ifndef UNICODE
# define UNICODE
#endif
#ifndef _UNICODE
# define _UNICODE
#endif

# include <windows.h>
# include <wchar.h>
# include <tchar.h>

/* The PRINTF_S character is used in situations where we have a string
   with one TCHAR and one char argument. It's impossible to use the
   _TEXT macro because we don't know which will be which.
*/
#define  PRINTF_S   "S"

#define _tmemmove      wmemmove

/* The Win32 API does have lstat, just stat. As such, we don't have to
   worry about the difference between the two. */
#define _lstat         _tstat64
#define _sstat         _tstat64
#define _tstat_t       struct __stat64

#else  // ifdef _WIN32

#define  PRINTF_S   "s"

/* The next few paragraphs are similar to tchar.h when UNICODE
   is not defined. They define all of the _t* functions to use
   the standard char * functions. This works just fine on Linux and OS X */
#define  TCHAR      char

#define  _TDIR      DIR
#define  _TEXT(A)   A

#define  _sntprintf snprintf
#define  _tprintf   printf

#define  _lstat     lstat
#define  _sstat     stat
#define  _tstat_t   struct stat

#define  _tgetcwd   getcwd
#define  _tfopen    fopen

#define  _topendir  opendir
#define  _treaddir  readdir
#define  _tdirent   dirent
#define  _tclosedir closedir

#define  _tcsncpy   strncpy
#define  _tcslen    strlen
#define  _tcsnicmp  strncasecmp
#define  _tcsncmp   strncmp
#define  _tcsrchr   strrchr
#define  _tmemmove  memmove
#define  _tcsdup    strdup
#define  _tcsstr    strstr

#define _gmtime64   gmtime

#endif

__END_DECLS


#endif //   __TCHAR_LOCAL_H
