#ifndef UI_H
#define UI_H

#include <sys/cdefs.h>
__BEGIN_DECLS

/* $Id$ */

/* User Interface Functions */

// Display an ordinary message with newline added
void print_status(const char *fmt, ...);

// Display an error message if not in silent mode
void print_error(const state *s, const char *fmt, ...);

// Display an error message if not in silent mode with a Unicode filename
void print_error_unicode(const state *s, const TCHAR *fn, const char *fmt, ...);

// Display an error message, if not in silent mode,  
// and exit with EXIT_FAILURE
void fatal_error(const state *s, const char *fmt, ...);

// Display an error message, ask user to contact the developer, 
// and exit with EXIT_FAILURE
void internal_error(const char *fmt, ... );

// Display a filename, possibly including Unicode characters
void display_filename(FILE *out, const TCHAR *fn);

void print_debug(const char *fmt, ...);

void make_newline(const state *s);

void try_msg(void);

int display_hash(state *s);

__END_DECLS
#endif

