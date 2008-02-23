
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

#define MD5DEEP_VERSION     "0.15"
#define MD5DEEP_AUTHOR      "Jesse Kornblum"
#define MD5DEEP_COPYRIGHT   "This program is a work of the US Govenment. "\
"In accordance with 17 USC 105,\n"\
"copyright protection is not available for any work of the US Government.\n"\
"This is free software; see the source for copying conditions. There is NO\n"\
"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"


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


#define TRUE   1
#define FALSE  0
#define ONE_MEGABYTE  1048576

/* Strings have to be long enough to handle inputs from matched hashing files.
   The NSRL is already larger than 256 bytes. We go longer to be safer. */
#define MAX_STRING_LENGTH    768
#define HASH_STRING_LENGTH    32


/* These are the types of files that we can match against */
#define TYPE_PLAIN        0
#define TYPE_HASHKEEPER   1
#define TYPE_NSRL         2
#define TYPE_ILOOK        3
#define TYPE_UNKNOWN    254

/* These are the types of files we can encounter while hashing */
#define FILE_UNKNOWN     0
#define FILE_REGULAR     1
#define FILE_SYMLINK     2
#define FILE_DIRECTORY   3
#define FILE_DEVICE      4
#define FILE_ERROR     254


typedef short bool;


#ifdef __LINUX

#ifndef __UNIX
#define __UNIX
#endif

#include <sys/ioctl.h>
#include <sys/mount.h>

#endif /* ifdef __LINUX */



#ifdef __UNIX

/* This avoids compiler warnings on older systems */
int fseeko(FILE *stream, off_t offset, int whence);
off_t ftello(FILE *stream);

/* This enables 64-bit file offsets with ftello and fseeko */
#define _FILE_OFFSET_BITS   64
#define  DIR_TRAIL_CHAR   '/'
extern char *__progname;

#endif /* #ifdef __UNIX */



/* Code specific to Microsoft Windows */
#ifdef __WIN32

/* By default BUFSIZ is 512 on Windows. We make it 8192 so that it's 
   the same as UNIX. While that shouldn't mean anything in terms of
   computing the hash values, it should speed us up a little bit. */
#ifdef BUFSIZ
#undef BUFSIZ
#endif
#define BUFSIZ 8192

/* It would be nice to have a way to use 64-bit file lengths in Windows */
#define ftello   ftell
#define fseeko   fseek

#define  snprintf         _snprintf

#define  DIR_TRAIL_CHAR   '\\'
#define  u_int32_t        unsigned long

/* We create macros for the Windows equivalent UNIX functions.
   No worries about lstat to stat; Windows doesn't have symbolic links */
#define lstat(A,B)      stat(A,B)
#define realpath(A,B)   _fullpath(B,A,PATH_MAX)

/* This handy UNIX variable doesn't exist on UNIX. We manually
   populate this substitute in setProgramName */
char *__progname;


extern char *optarg;
extern int optind;
int getopt(int argc, char *const argv[], const char *optstring);
int asprintf(char **strp, const char *fmt, ...);

#endif   /* ifdef _WIN32 */



/* Functions from md5deep.c */
void printError(char *fn);


/* Functions from matching (match.c) */
int loadMatchFile(char *fn);
int isKnownHash(char *h);


/* Functions for file evaluation (files.c) */
int determineFileType(FILE *f);
bool findHashValueinLine(char *buf, int fileType);





/* ----------------------------------------------------------------- 
   Everything below this line came from the MD5 source code         */


#define MD5_HASH_LENGTH  16

struct MD5Context {
	u_int32_t buf[4];
	u_int32_t bits[2];
	unsigned char in[64];
};

void MD5Init(struct MD5Context *context);
void MD5Update(struct MD5Context *context, unsigned char const *buf,
	       unsigned len);
void MD5Final(unsigned char digest[16], struct MD5Context *context);
void MD5Transform(u_int32_t buf[4], u_int32_t const in[16]);

/*
 * This is needed to make RSAREF happy on some MS-DOS compilers.
 */
typedef struct MD5Context MD5_CTX;



#endif /* __MD5DEEP_H */



