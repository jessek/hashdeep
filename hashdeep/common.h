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
# define __MSVCRT_VERSION__ 0x0601
#endif
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <wchar.h>
#include <vector>

#if defined(__cplusplus)
/*
 * C++ implementation of a wchar_t string.
 * It works like the regular string, except that it works with wide chars.
 * Notice that .c_str() returns a wchar_t * pointer.
 *
 * You will find a nice discussion about allocating arrays in C++ at:
 * http://www.fredosaurus.com/notes-cpp/newdelete/50dynamalloc.html
 */
class wstring {
private:
    wchar_t *buf;
    size_t  len;			// does not include NULL character
    
public:
#if 0
    class const_iterator {
    public:
	const_iterator(const wstring &base_):base(base_),pos(0){}
	const wstring &base;
	size_t  pos;
	bool operator== (const const_iterator &i2) const {
	    return (this->base.buf == i2.base.buf) && (this->pos == i2.pos);
	}
	bool operator!= (const const_iterator &i2) const {
	    return (this->base.buf != i2.base.buf) || (this->pos != i2.pos);
	}
	const_iterator &operator++() {	// prefix increment operator
	    this->pos++;
	    return *this;
	}
	const_iterator &operator++(int) {// postfix increment operator
	    ++this->pos;
	    return *this;
	}
	const wstring &operator*(){
	    return base;
	}
    };
#endif

    static const ssize_t npos=-1;
    ~wstring(){
	free(buf);
    }
    wstring():buf((wchar_t *)malloc(0)){}
    wstring(const wchar_t *buf_){
	len = wcslen(buf)+1;
	buf = (wchar_t *)calloc(len+1,sizeof(wchar_t));
	wcscpy(buf,buf_);
    }
    wstring(const wstring &w2){
	len = w2.len;
	buf = (wchar_t *)calloc(len+1,sizeof(wchar_t));
	wcscpy(buf,w2.buf);
    }
    size_t size() const { return len;}		// in multi-bytes, not bytes.
    wchar_t operator[] (size_t i) const{
	if(i<0 || i>=len) return 0;
	return buf[i];
    }
    wchar_t *utf16() const{		// allocate wstr if it doesn't exist and return it
	return buf;
    }
    wchar_t *c_str() const{
	return buf;			// wide c strings *are* utf16
    }
    std::string utf8() const;			// returns a UTF-8 representation as a string
    ssize_t find(wchar_t ch,size_t start=0) const{
	for(size_t i=start;i<len;i++){
	    if((*this)[i]==ch) return i;
	}
	return npos;
    }
    ssize_t rfind(wchar_t ch,size_t start=0) const{
	if(this->len==0) return npos;
	if(start==0) start=this->len-1;
	if(start>this->len) start=this->len-1;
	for(size_t i=start;i>0;i--){
	    if((*this)[i]==ch) return i;
	    if(i==0) return npos;	// not found
	}
	return npos;			// should never reach here
    }
    ssize_t find(const wstring &s2,ssize_t start=0) const{
	if(s2.len > this->len) return npos;
	for(size_t i=start;i<len-s2.len;i++){
	    for(size_t j=0;j<s2.len;j++){
		if((*this)[i+j]!=s2[j]) break;
		if(j==len-1) return i;	// found it
	    }
	}
	return npos;
    }
    ssize_t rfind(const wstring &s2) const{
	if((s2.len > this->len) || (this->len==0)) return npos;
	for(size_t i=len-s2.len-1;i>=0;i--){
	    for(size_t j=0;j<s2.len;j++){
		if((*this)[i+j]!=s2[j]) break;
		if(j==len-1) return i;	// found it
	    }
	    if(i==0)return npos;	// got to 0 and couldn't find the end
	}
	return npos;			// should never get here.
    }
    void erase(size_t  start,size_t count){
	if(start>=len) return;		// nothing to erase
	if(start+count > len) count = len-start; // can only erase this many
	memmove(buf+start*2,buf+(start+count)*2,count*2);
	len -= count;
	buf[len*2] = 0;			// new termination
	buf = (wchar_t *)realloc(buf,(len+1)*2);  // removed that many characters
    }
    void erase(size_t start){
	erase(start,start);
    }
    void push_back(wchar_t ch){
	len++;
	buf = (wchar_t *)realloc(buf,(len+1)*2);  // removed that many characters
	buf[len] = ch;
    }
    /* Concatenate a wstring */
    wstring operator+(const wstring &s2) const {
	wstring s1(*this);
	s1.append(s2);
	return s1;
    }
    /* Concatenate a TCHAR * string */
    wstring operator+(const TCHAR *s2) const {
	wstring s1(*this);
	s1.append(s2);
	return s1;
    }
    wstring operator+(TCHAR c2) const {
	wstring s1(*this);
	s1.push_back(c2);
	return s1;
    }
    void append(const wstring &s2){
	buf = (wchar_t *)realloc(buf,(len+s2.len+1)); // enough space for the null
	wcscat(buf,s2.buf);
	len += s2.len;
    }
    void append(const TCHAR *s2){
	size_t l2 = wcslen(s2);
	buf = (wchar_t *)realloc(buf,(len+l2+1)); // enough space for the null
	wcscat(buf,s2);
	len += l2;
    }
#if 0
    const_iterator begin() const{
	return const_iterator(*this);
    }
    const_iterator end() const{
	const_iterator it(*this);
	it.pos = len;
	return it;
    }
#endif
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
#define  _tstat64   struct stat


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
