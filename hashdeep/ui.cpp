// MD5DEEP - ui.c
//
// By Jesse Kornblum and Simson Garfinkel
//
// This is a work of the US Government. In accordance with 17 USC 105,
// copyright protection is not available for any work of the US Government.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// $Id$ 

#include "main.h"
#include <stdarg.h>

#define MD5DEEP_PRINT_MSG(HANDLE,MSG) \
va_list(ap);  \
va_start(ap,MSG); \
if (vfprintf(HANDLE,MSG,ap) < 0)  \
{ \
   fprintf(stderr, "%s: %s", __progname, strerror(errno)); \
   exit(EXIT_FAILURE);  \
} \
va_end(ap); fprintf (HANDLE,"%s", NEWLINE);


void print_debug(const char *fmt, ... )
{
#ifdef __DEBUG
    printf ("DEBUG: ");
    MD5DEEP_PRINT_MSG(stderr,fmt)
#endif
}

void print_status(const char *fmt, ...)
{
    MD5DEEP_PRINT_MSG(stdout,fmt)
}

void print_error(const char *fmt, ...)
{
    if (opt_silent) return;
    MD5DEEP_PRINT_MSG(stderr,fmt);
}


void print_error_filename(const char *fn, const char *fmt, ...)
{
    if (opt_silent) return;
    output_filename(stderr,fn);
    fprintf(stderr,": ");
    MD5DEEP_PRINT_MSG(stderr,fmt);
}


#ifdef _WIN32
/* This is where the unicode happens */
void print_error_filename(TCHAR *fn, const char *fmt, ...)
{
    if (opt_silent) return;
    output_filename(stderr,fn);
    fprintf(stderr,": ");
    MD5DEEP_PRINT_MSG(stderr,fmt);
}
#endif


void fatal_error(const char *fmt, ...)
{
    if(!opt_silent){
	MD5DEEP_PRINT_MSG(stderr,fmt);
    }
    exit (STATUS_USER_ERROR);
}


// Internal errors are so serious that we ignore the user's wishes 
// about silent mode. Our need to debug the program outweighs their
// preferences. Besides, the program is probably crashing anyway...
void internal_error(const char *fmt, ... )
{
    MD5DEEP_PRINT_MSG(stderr,fmt);  
    print_status ("%s: Internal error. Contact developer!", __progname);  
    exit (STATUS_INTERNAL_ERROR);
}


void try_msg(void)
{
    print_status("Try `%s -h` for more information.", __progname);
}
