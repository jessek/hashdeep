
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

#include "md5deep.h"

#ifdef __WIN32 
/* Allows us to open standard input in binary mode by default 
   See http://gnuwin32.sourceforge.net/compile.html for more */
int _CRT_fmode = _O_BINARY;
#endif

void try_msg()
{
  fprintf(stderr,"Try `%s -h` for more information.%s", __progname,NEWLINE);
}

/* The usage function should, at most, display 22 lines of text to fit
   on a single screen */
void usage() 
{
  fprintf(stderr,"%s version %s by %s.%s",__progname,VERSION,AUTHOR,NEWLINE);
  fprintf(stderr,"Usage:%s%s %s [-v|-V|-h] [-m|-M|-x|-X <file>] ",
	  NEWLINE,CMD_PROMPT,__progname);
  fprintf(stderr,"[-zres0bt] [-o fbcplsd] FILES%s%s",NEWLINE,NEWLINE);

  fprintf(stderr,"-v  - display version number and exit%s",NEWLINE);
  fprintf(stderr,"-V  - display copyright information and exit%s",NEWLINE);
  fprintf(stderr,"-h  - display this help message and exit%s",NEWLINE);
  fprintf(stderr,"-m  - enables matching mode. See README/man page%s",NEWLINE);
  fprintf(stderr,"-x  - enables negative matching mode. See README/man page%s",
	  NEWLINE);
  fprintf(stderr,"-M and -X are the same as -m and -x but also print hashes of each file%s",NEWLINE);
  fprintf(stderr,"-r  - enables recursive mode. All subdirectories are traversed%s",NEWLINE);
  fprintf(stderr,"-e  - compute estimated time remaining for each file%s",
	  NEWLINE);
  fprintf(stderr,"-s  - enables silent mode. Suppress all error messages%s",
	  NEWLINE);
  fprintf(stderr,"-z  - display file size before hash%s", NEWLINE);
  fprintf(stderr,"-0  - Use /0 as line terminator%s", NEWLINE);
  fprintf(stderr,"-o  - Only process certain types of files:%s",NEWLINE);
  fprintf(stderr,"      f - Regular File%s",NEWLINE);
  fprintf(stderr,"      b - Block Device%s",NEWLINE);
  fprintf(stderr,"      c - Character Device%s",NEWLINE);
  fprintf(stderr,"      p - Named Pipe (FIFO)%s",NEWLINE);
  fprintf(stderr,"      l - Symbolic Link%s",NEWLINE);
  fprintf(stderr,"      s - Socket%s",NEWLINE);
  fprintf(stderr,"      d - Solaris Door%s",NEWLINE);
}



void setup_expert_mode(char *arg, off_t *mode) 
{
  int i = 0;

  while (i < strlen(arg)) {
    switch (*(arg+i)) {
    case 'b': // Block Device
      *mode |= mode_block;
      break;
    case 'c': // Character Device
      *mode |= mode_character;
      break;
    case 'p': // Named Pipe
      *mode |= mode_pipe;
      break;
    case 'f': // Regular File
      *mode |= mode_regular;
      break;
    case 'l': // Symbolic Link
      *mode |= mode_symlink;
      break;
    case 's': // Socket
      *mode |= mode_socket;
      break;
    case 'd': // Door (Solaris)
      *mode |= mode_door;
      break;
    default:
      if (!(M_SILENT(*mode)))
	fprintf(stderr,
		"%s: Unrecognized file type: %c%s"
		, __progname,*(arg+i),NEWLINE);
    }
    ++i;
  }
}

void check_matching_okay(off_t mode, int hashes_loaded)
{

  if ((M_MATCH(mode) || M_MATCHNEG(mode)) && !hashes_loaded)
  {
    if (!(M_SILENT(mode)))
    {  
      fprintf(stderr,"%s: Unable to load any matching files.%s",
	      __progname,NEWLINE);
      try_msg();
    }
    exit (1);
  }

  if (M_MATCH(mode) && M_MATCHNEG(mode))
  {
    if (!(M_SILENT(mode)))
    {
      fprintf(stderr,
	      "%s: Regular and negative matching are mutually exclusive.%s",
	      __progname,NEWLINE);
      try_msg();
    }
    exit (1);
  }
}

void process_command_line(int argc, char **argv, off_t *mode) {

  int i, hashes_loaded = FALSE;
  
  while ((i=getopt(argc,argv,"M:X:x:m:u:o:zserhvV0bt")) != -1) { 
    switch (i) {

    case 'o':
      *mode |= mode_expert;
      setup_expert_mode(optarg,mode);
      break;
      
    case 'M':
      *mode |= mode_display_hash;
    case 'm':
      *mode |= mode_match;
      if (load_match_file(*mode,optarg))
	hashes_loaded = TRUE;
      break;

    case 'X':
      *mode |= mode_display_hash;
    case 'x':
      *mode |= mode_match_neg;
      if (load_match_file(*mode,optarg))
	hashes_loaded = TRUE;
      break;

    case 'z':
      *mode |= mode_display_size;
      break;

    case '0':
      *mode |= mode_zero;
      break;

    case 's':
      *mode |= mode_silent;
      break;

    case 'e':
      *mode |= mode_estimate;
      break;

    case 'r':
      *mode |= mode_recursive;
      break;

    case 'h':
      usage();
      exit (1);

    case 'v':
      printf ("%s%s",VERSION,NEWLINE);
      exit (1);

    case 'V':
      printf (COPYRIGHT);
      exit (1);

      /* These are only for compatibility with md5sum */
    case 'b':
    case 't':
      break;

    default:
      try_msg();
      exit (1);

    }
  }

  check_matching_okay(*mode, hashes_loaded);
}


int is_absolute_path(char *fn)
{
  return (fn[0] == DIR_SEPARATOR);
}


int main(int argc, char **argv) 
{
  char *fn  = (char *)malloc(sizeof(char)* PATH_MAX);
  char *cwd = (char *)malloc(sizeof(char)* PATH_MAX);
  off_t mode = mode_none;

#ifndef __GLIBC__
  __progname = basename(argv[0]);
#endif

  process_command_line(argc,argv,&mode);

  /* Anything left on the command line at this point is a file
     or directory we're supposed to process. If there's nothing
     specified, we should tackle standard input */

  argv += optind;
  if (*argv == NULL)
    hash_stdin(mode);
  else
  {
    cwd = getcwd(cwd,PATH_MAX);

    while (*argv != NULL)
    {  

      /* Windows systems don't have symbolic links, so we don't
	 have to worry about carefully preserving the paths 
	 they follow. Just use the system command to resolve the paths */

#ifdef __WIN32
      realpath(*argv,fn);
#else
      
      if (is_absolute_path(*argv))
      {
	strncpy(fn,*argv,PATH_MAX);
      }
      else
      {
	if (cwd == NULL)
	{  
	  print_error(mode,*argv,"Unable to find path to file");
	  ++argv;
	  continue;
	}
	snprintf(fn,PATH_MAX,"%s%c%s",cwd,DIR_SEPARATOR,*argv);
      }
#endif      

      process(mode,fn);
      ++argv;
    }
  }

  free(fn);
  free(cwd);
  return 0;
}
