
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


/* The usage function should, at most, display 22 lines of text to fit
   on a single screen */

void usage() 
{
  fprintf(stderr,"%s version %s by %s.\n",__progname,VERSION,AUTHOR);
  fprintf(stderr,"\n");
  fprintf(stderr,"Usage:\n$ %s [-v|-V|-h] [-m|-M|-x|-X <file>] ",__progname);
  fprintf(stderr,"[-resbt] [-o fbcplsd] FILES\n\n");

  fprintf(stderr,"-v  - display version number and exit\n");
  fprintf(stderr,"-V  - display copyright information and exit\n");
  fprintf(stderr,"-h  - display this help message and exit\n");
  fprintf(stderr,"-m  - enables matching mode. See README/man page\n");
  fprintf(stderr,"-x  - enables negative matching mode. See README/man page\n");
  fprintf(stderr,"-M and -X are the same as -m and -x but also print hashes of each file\n");
  fprintf(stderr,"-r  - enables recursive mode. All subdirectories are traversed\n");
  fprintf(stderr,"-e  - compute estimated time remaining for each file\n");
  fprintf(stderr,"-s  - enables silent mode. Suppress all error messages\n");
  fprintf(stderr,"-o  - Only process certain types of files:\n");
  fprintf(stderr,"      f - Regular File\n");
  fprintf(stderr,"      b - Block Device\n");
  fprintf(stderr,"      c - Character Device\n");
  fprintf(stderr,"      p - Named Pipe (FIFO)\n");
  fprintf(stderr,"      l - Symbolic Link\n");
  fprintf(stderr,"      s - Socket\n");
  fprintf(stderr,"      d - Solaris Door\n");
  fprintf(stderr,"-b and -t are ignored. They are only present for compatibility with md5sum\n");
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
		"%s: Unrecognized file type: %c\n", __progname,*(arg+i));
    }
    ++i;
  }
}


void process_command_line(int argc, char **argv, off_t *mode) {

  int i;
  
  while ((i=getopt(argc,argv,"M:X:x:m:u:o:serhvVbt")) != -1) { 
    switch (i) {
    
    case 'o':
      *mode |= mode_expert;
      setup_expert_mode(optarg,mode);
      break;
      
    case 'M':
      *mode |= mode_display_hash;
    case 'm':
      *mode |= mode_match;
      load_match_file(*mode,optarg);
      break;

    case 'X':
      *mode |= mode_display_hash;
    case 'x':
      *mode |= mode_match_neg;
      load_match_file(*mode,optarg);
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
      printf ("%s\n",VERSION);
      exit (1);

    case 'V':
      printf (COPYRIGHT);
      exit (1);

      /* These are only for compatibility with md5sum and are ignored */
    case 'b':
    case 't':
      break;

    default:
      usage();
      exit (1);

    }
  }
  
  if (M_MATCH(*mode) && M_MATCHNEG(*mode))
  {
    fprintf(stderr,
            "%s: Regular and negative matching are mutually exclusive!\n\n",
	    __progname);
    usage();
    exit (1);
  }
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
    printf("%s\n",md5_file(mode,stdin,"STDIN"));

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
