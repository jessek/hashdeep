
/* $Id: ui.h,v 1.2 2007/12/09 18:58:06 jessekornblum Exp $ */

/* User Interface Functions */

// Display an ordinary message with newline added
void print_status(char *fmt, ...);

// Display an error message if not in silent mode
void print_error(state *s, char *fmt, ...);

// Display an error message if not in silent mode with a Unicode filename
void print_error_unicode(state *s, TCHAR *fn, char *fmt, ...);

// Display an error message, if not in silent mode,  
// and exit with EXIT_FAILURE
void fatal_error(state *s, char *fmt, ...);

// Display an error message, ask user to contact the developer, 
// and exit with EXIT_FAILURE
void internal_error(char *fmt, ... );

// Display a filename, possibly including Unicode characters
void display_filename(FILE *out, TCHAR *fn);

void print_debug(char *fmt, ...);

void make_newline(state *s);

void try_msg(void);

int display_hash(state *s);
