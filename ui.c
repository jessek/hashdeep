
/* MD5DEEP - ui.c
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

#include "main.h"
#include <stdarg.h>

#define MD5DEEP_PRINT_MSG(A,B) \
va_list(ap);  \
va_start(ap,B); vfprintf(A,B,ap); va_end(ap); fprintf (A,"%s", NEWLINE);


void print_debug(char *fmt, ... )
{
#ifdef __DEBUG
  printf ("DEBUG: ");
  MD5DEEP_PRINT_MSG(stderr,fmt)
#endif
}

void print_status(char *fmt, ...)
{
  MD5DEEP_PRINT_MSG(stdout,fmt)
}

void print_error(state *s, char *fmt, ...)
{
  if (!(s->mode & mode_silent))
  {
    MD5DEEP_PRINT_MSG(stderr,fmt);
  }
}

void fatal_error(state *s, char *fmt, ...)
{
  if (!(s->mode & mode_silent))
  {
    MD5DEEP_PRINT_MSG(stderr,fmt);
  }

  exit (STATUS_USER_ERROR);
}


/* Internal errors are so serious that we ignore the user's wishes 
   about silent mode. Our need to debug the program outweighs their
   preferences. Besides, the program is probably crashing anyway... */
void internal_error(char *fmt, ... )
{
  MD5DEEP_PRINT_MSG(stderr,fmt);  
  print_status ("%s: Internal error. Contact developer!", __progname);  
  exit (STATUS_INTERNAL_ERROR);
}
