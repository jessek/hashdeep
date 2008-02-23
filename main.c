
/* MD5DEEP - main.c
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

#ifdef _WIN32 
/* Allows us to open standard input in binary mode by default 
   See http://gnuwin32.sourceforge.net/compile.html for more */
int _CRT_fmode = _O_BINARY;
#endif


void try_msg(void)
{
  fprintf(stderr,"Try `%s -h` for more information.%s", __progname,NEWLINE);
}


/* The usage function must not produce more than 22 lines of text 
   in order to fit on a single screen */
void usage(void) 
{
  fprintf(stderr,"%s version %s by %s.%s",__progname,VERSION,AUTHOR,NEWLINE);
  fprintf(stderr,"%s %s [OPTION]... [FILES]%s%s",
	  CMD_PROMPT,__progname,NEWLINE,NEWLINE);

  fprintf (stderr,"See the man page or README.txt file for the full list of options%s", NEWLINE);
  fprintf(stderr,"-r  - recursive mode. All subdirectories are traversed%s",NEWLINE);
  fprintf(stderr,"-e  - compute estimated time remaining for each file%s",
	  NEWLINE);
  fprintf(stderr,"-s  - silent mode. Suppress all error messages%s",
	  NEWLINE);
  fprintf(stderr,"-z  - display file size before hash%s", NEWLINE);
  fprintf(stderr,"-m <file> - enables matching mode. See README/man page%s",NEWLINE);
  fprintf(stderr,"-x <file> - enables negative matching mode. See README/man page%s",
	  NEWLINE);
  fprintf(stderr,"-M and -X are the same as -m and -x but also print hashes of each file%s",NEWLINE);
  fprintf(stderr,"-w  - displays which known file generated a match%s", NEWLINE);
  fprintf(stderr,"-n  - displays known hashes that did not match any input files%s", NEWLINE);
  fprintf(stderr,"-a and -A add a single hash to the positive or negative matching set%s", NEWLINE);
  fprintf(stderr,"-b  - prints only the bare name of files; all path information is omitted%s", NEWLINE);
  fprintf(stderr,"-l  - print relative paths for filenames%s", NEWLINE);
  fprintf(stderr,"-k  - print asterisk before hash%s", NEWLINE);
  fprintf(stderr,"-o  - Only process certain types of files. See README/manpage%s",NEWLINE);
  fprintf(stderr,"-v  - display version number and exit%s",NEWLINE);
}


void setup_expert_mode(char *arg, uint64_t *mode) 
{
  unsigned int i = 0;

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
      if (!M_SILENT(*mode))
	fprintf(stderr,
		"%s: Unrecognized file type: %c%s"
		, __progname,*(arg+i),NEWLINE);
    }
    ++i;
  }
}


void sanity_check(uint64_t mode, int condition, char *msg)
{
  if (condition)
  {
    if (!M_SILENT(mode))
    {
      fprintf(stderr,"%s: %s%s", __progname, msg, NEWLINE);
      try_msg();
    }
    exit (STATUS_USER_ERROR);
  }
}
      

void check_flags_okay(uint64_t mode, int hashes_loaded)
{
  sanity_check(mode,
	       ((M_MATCH(mode) || M_MATCHNEG(mode)) && !hashes_loaded),
	       "Unable to load any matching files");

  sanity_check(mode,
	       (M_RELATIVE(mode) && (M_BARENAME(mode))),
	       "Relative paths and bare filenames are mutally exclusive");


  /* If we try to display non-matching files but haven't initialized the
     list of matching files in the first place, bad things will happen. */
  sanity_check(mode,
	       (M_NOT_MATCHED(mode) && ! (M_MATCH(mode) || M_MATCHNEG(mode))),
	       "Matching or negative matching must be enabled to display non-matching files");

  sanity_check(mode,
	       (M_WHICH(mode) && ! (M_MATCH(mode) || M_MATCHNEG(mode))),
	       "Matching or negative matching must be enabled to display which file matched");
  
  // Additional sanity checks will go here as needed... 
}


void check_matching_modes(uint64_t mode)
{
  sanity_check(mode,
	       (M_MATCH(mode) && M_MATCHNEG(mode)),
	       "Regular and negative matching are mutually exclusive.");
}


void process_command_line(int argc, char **argv, uint64_t *mode) {

  int i, hashes_loaded = FALSE;
  
  while ((i=getopt(argc,argv,"M:X:x:m:u:o:A:a:nwzserhvV0lbkq")) != -1) { 
    switch (i) {

    case 'q':
      *mode |= mode_quiet;
      break;

    case 'n':
      *mode |= mode_not_matched;
      break;

    case 'w':
      *mode |= mode_which;
      break;

    case 'a':
      *mode |= mode_match;
      check_matching_modes(*mode);
      add_hash(*mode,optarg,optarg);
      hashes_loaded = TRUE;
      break;

    case 'A':
      *mode |= mode_match_neg;
      check_matching_modes(*mode);

      add_hash(*mode,optarg,optarg);
      hashes_loaded = TRUE;
      break;
      
    case 'l':
      *mode |= mode_relative;
      break;

    case 'o':
      *mode |= mode_expert;
      setup_expert_mode(optarg,mode);
      break;
      
    case 'M':
      *mode |= mode_display_hash;
    case 'm':
      *mode |= mode_match;
      check_matching_modes(*mode);
      if (load_match_file(*mode,optarg))
	hashes_loaded = TRUE;
      break;

    case 'X':
      *mode |= mode_display_hash;
    case 'x':
      *mode |= mode_match_neg;
      check_matching_modes(*mode);
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

    case 'k':
      *mode |= mode_asterisk;
      break;

    case 'h':
      usage();
      exit (STATUS_OK);

    case 'v':
      printf ("%s%s",VERSION,NEWLINE);
      exit (STATUS_OK);

    case 'V':
      /* We could just say printf(COPYRIGHT), but that's a good way
	 to introduce a format string vulnerability. Better to always
	 use good programming practice... */
      printf ("%s", COPYRIGHT);
      exit (STATUS_OK);


    case 'b':
      *mode |= mode_barename;
      break;

    default:
      try_msg();
      exit (STATUS_USER_ERROR);

    }
  }
  check_flags_okay(*mode, hashes_loaded);
}


int is_absolute_path(char *fn)
{
#ifdef _WIN32
  /* Windows has so many ways to make absolute paths (UNC, C:\, etc)
     that it's hard to keep track. It doesn't hurt us
     to call realpath as there are no symbolic links to lose. */
  return FALSE;
#else
  return (fn[0] == DIR_SEPARATOR);
#endif
}


void generate_filename(uint64_t mode, char **argv, char *fn, char *cwd)
{
  if (M_RELATIVE(mode) || is_absolute_path(*argv))
    strncpy(fn,*argv,PATH_MAX);	
  else
  {
    /* Windows systems don't have symbolic links, so we don't
       have to worry about carefully preserving the paths 
       they follow. Just use the system command to resolve the paths */   
#ifdef _WIN32
    realpath(*argv,fn);
#else	  
    if (cwd == NULL)
      /* If we can't get the current working directory, we're not
	 going to be able to build the relative path to this file anyway.
	 So we just call realpath and make the best of things */
      realpath(*argv,fn);
    else
      snprintf(fn,PATH_MAX,"%s%c%s",cwd,DIR_SEPARATOR,*argv);
#endif   
  }
}


int main(int argc, char **argv) 
{
  int return_value = STATUS_OK;
  char *fn;
  char *cwd;
  uint64_t mode = mode_none;

#ifndef __GLIBC__
  __progname  = basename(argv[0]);
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
    fn  = (char *)malloc(sizeof(char)* PATH_MAX);
    cwd = (char *)malloc(sizeof(char)* PATH_MAX);
    cwd = getcwd(cwd,PATH_MAX);
    while (*argv != NULL)
    {  
      generate_filename(mode,argv,fn,cwd);
      process(mode,fn);
      ++argv;
    }

    free(fn);
    free(cwd);
  }

  /* We only have to worry about checking for unused hashes if one 
     of the matching modes was enabled. We let the display_not_matched
     function determine if it needs to display anything. The function
     also sets our return values in terms of inputs not being matched
     or known hashes not being used */
  if (M_MATCH(mode) || M_MATCHNEG(mode))
    return_value = finalize_matching(mode);

  return return_value;
}
