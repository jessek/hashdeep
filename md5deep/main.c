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

// $Id$

#include "main.h"


#ifdef _WIN32 
// This can't go in main.h or we get multiple definitions of it
/* Allows us to open standard input in binary mode by default 
   See http://gnuwin32.sourceforge.net/compile.html for more */
int _CRT_fmode = _O_BINARY;
#endif




/* So that the usage message fits in a standard DOS window, this
   function should produce no more than 22 lines of text. */
static void usage(void) 
{
  print_status("%s version %s by %s.",__progname,VERSION,AUTHOR);
  print_status("%s %s [OPTION]... [FILE]...",CMD_PROMPT,__progname);

  print_status("See the man page or README.txt file for the full list of options");
  print_status("-p  - piecewise mode. Files are broken into blocks for hashing");
  print_status("-r  - recursive mode. All subdirectories are traversed");
  print_status("-e  - compute estimated time remaining for each file");
  print_status("-s  - silent mode. Suppress all error messages");
  print_status("-S  - displays warnings on bad hashes only");
  print_status("-z  - display file size before hash");
  print_status("-m <file> - enables matching mode. See README/man page");
  print_status("-x <file> - enables negative matching mode. See README/man page");
	 
  print_status("-M and -X are the same as -m and -x but also print hashes of each file");
  print_status("-w  - displays which known file generated a match");
  print_status("-n  - displays known hashes that did not match any input files");
  print_status("-a and -A add a single hash to the positive or negative matching set");
  print_status("-b  - prints only the bare name of files; all path information is omitted");
  print_status("-l  - print relative paths for filenames");
  print_status("-k  - print asterisk before filename");
  print_status("-t  - print GMT timestamp");
  print_status("-i/I- only process files smaller than the given threshold");
  print_status("-o  - only process certain types of files. See README/manpage");
  print_status("-v  - display version number and exit");
}


static void setup_expert_mode(state *s, char *arg)
{
  unsigned int i = 0;

  while (i < strlen(arg)) {
    switch (*(arg+i)) {
    case 'b': // Block Device
      s->mode |= mode_block;     break;
    case 'c': // Character Device
      s->mode |= mode_character; break;
    case 'p': // Named Pipe
      s->mode |= mode_pipe;      break;
    case 'f': // Regular File
      s->mode |= mode_regular;   break;
    case 'l': // Symbolic Link
      s->mode |= mode_symlink;   break;
    case 's': // Socket
      s->mode |= mode_socket;    break;
    case 'd': // Door (Solaris)
      s->mode |= mode_door;      break;
    default:
      print_error(s,"%s: Unrecognized file type: %c",__progname,*(arg+i));
    }
    ++i;
  }
}


static void check_flags_okay(state *s)
{
  sanity_check(s,
	       ((s->mode & mode_match) || (s->mode & mode_match_neg)) &&
	       !s->hashes_loaded,
	       "Unable to load any matching files");

  sanity_check(s,
	       (s->mode & mode_relative) && (s->mode & mode_barename),
	       "Relative paths and bare filenames are mutally exclusive");
  
  sanity_check(s,
	       (s->mode & mode_piecewise) && (s->mode & mode_display_size),
	       "Piecewise mode and file size display is just plain silly");


  /* If we try to display non-matching files but haven't initialized the
     list of matching files in the first place, bad things will happen. */
  sanity_check(s,
	       (s->mode & mode_not_matched) && 
	       ! ((s->mode & mode_match) || (s->mode & mode_match_neg)),
	       "Matching or negative matching must be enabled to display non-matching files");

  sanity_check(s,
	       (s->mode & mode_which) && 
	       ! ((s->mode & mode_match) || (s->mode & mode_match_neg)), 
	       "Matching or negative matching must be enabled to display which file matched");
  

  // Additional sanity checks will go here as needed... 
}


static void check_matching_modes(state *s)
{
  sanity_check(s,
	       (s->mode & mode_match) && (s->mode & mode_match_neg),
	       "Regular and negative matching are mutually exclusive.");
}


static int process_command_line(state *s, int argc, char **argv)
{
  int i;
  
  while ((i = getopt(argc,
		     argv,
		     "I:i:M:X:x:m:o:A:a:tnwzsSp:erhvV0lbkqU")) != -1) { 
    switch (i) {

    case 'I':
      s->mode |= mode_size_all;
      // Note that there is no break here
    case 'i':
      s->mode |= mode_size;
      s->size_threshold = find_block_size(s,optarg);
      if (0 == s->size_threshold)
      {
	print_error(s,"%s: Requested size threshold implies not hashing anything",
		    __progname);
	exit(STATUS_USER_ERROR);
      }
      break;

    case 'p':
      s->mode |= mode_piecewise;
      s->piecewise_size = find_block_size(s, optarg);
      if (0 == s->piecewise_size)
      {
	print_error(s,"%s: Piecewise blocks of zero bytes are impossible", 
		    __progname);
	exit(STATUS_USER_ERROR);
      }

      break;

    case 'q': 
      s->mode |= mode_quiet; 
      break;
    case 't':
      s->mode |= mode_timestamp;
      MD5DEEP_ALLOC(char,s->time_str,MAX_TIME_STRING_LENGTH);
      break;
    case 'n': 
      s->mode |= mode_not_matched; 
      break;
    case 'w': 
      s->mode |= mode_which; 
      break;

    case 'a':
      s->mode |= mode_match;
      check_matching_modes(s);
      add_hash(s,optarg,optarg);
      s->hashes_loaded = TRUE;
      break;

    case 'A':
      s->mode |= mode_match_neg;
      check_matching_modes(s);
      add_hash(s,optarg,optarg);
      s->hashes_loaded = TRUE;
      break;
      
    case 'l': 
      s->mode |= mode_relative; 
      break;

    case 'b': 
      s->mode |= mode_barename; 
      break;

    case 'o': 
      s->mode |= mode_expert; 
      setup_expert_mode(s,optarg);
      break;
      
    case 'M':
      s->mode |= mode_display_hash;
    case 'm':
      s->mode |= mode_match;
      check_matching_modes(s);
      if (load_match_file(s,optarg))
	s->hashes_loaded = TRUE;
      break;

    case 'X':
      s->mode |= mode_display_hash;
    case 'x':
      s->mode |= mode_match_neg;
      check_matching_modes(s);
      if (load_match_file(s,optarg))
	s->hashes_loaded = TRUE;
      break;

    case 'z': 
      s->mode |= mode_display_size; 
      break;

    case '0': 
      s->mode |= mode_zero; 
      break;

    case 'S': 
      s->mode |= mode_warn_only;
      s->mode |= mode_silent;
      break;

    case 's':
      s->mode |= mode_silent;
      break;

    case 'e':
      s->mode |= mode_estimate;
      break;

    case 'r':
      s->mode |= mode_recursive;
      break;

    case 'k':
      s->mode |= mode_asterisk;
      break;

    case 'h':
      usage();
      exit (STATUS_OK);

    case 'v':
      print_status("%s",VERSION);
      exit (STATUS_OK);

    case 'V':
      // COPYRIGHT is a format string, complete with newlines
      print_status(COPYRIGHT);
      exit (STATUS_OK);

    default:
      try_msg();
      exit (STATUS_USER_ERROR);

    }
  }

  check_flags_okay(s);

  return STATUS_OK;
}


static int initialize_state(state *s)
{
  /* We use PATH_MAX as a handy constant for working with strings
     It may be overkill, but it keeps us out of trouble */
  MD5DEEP_ALLOC(TCHAR,s->msg,PATH_MAX);
  MD5DEEP_ALLOC(TCHAR,s->full_name,PATH_MAX);
  MD5DEEP_ALLOC(TCHAR,s->short_name,PATH_MAX);

  MD5DEEP_ALLOC(unsigned char,s->hash_sum,PATH_MAX);
  MD5DEEP_ALLOC(char,s->hash_result,PATH_MAX);
  MD5DEEP_ALLOC(char,s->known_fn,PATH_MAX);

  s->mode            = mode_none;
  s->hashes_loaded   = FALSE;
  s->return_value    = STATUS_OK;
  s->piecewise_size  = 0;
  s->size_threshold  = 0;
  s->block_size      = MD5DEEP_IDEAL_BLOCK_SIZE;

  if (setup_hashing_algorithm(s))
    return TRUE;

  return FALSE;
}


#ifdef _WIN32
static int prepare_windows_command_line(state *s)
{
  int argc;
  TCHAR **argv;

  argv = CommandLineToArgvW(GetCommandLineW(),&argc);
  
  s->argc = argc;
  s->argv = argv;

  return FALSE;
}
#endif


int main(int argc, char **argv) 
{
  TCHAR *fn, *cwd;
  state *s;
  int count, status = STATUS_OK;

  /* Because the main() function can handle wchar_t arguments on Win32,
     we need a way to reference those values. Thus we make a duplciate
     of the argc and argv values. */ 

#ifndef __GLIBC__
  __progname  = basename(argv[0]);
#endif

  
  s = (state *)malloc(sizeof(state));
  if (NULL == s)
  {
    // We can't use fatal_error because it requires a valid state
    print_status("%s: Unable to allocate state variable", __progname);
    return STATUS_INTERNAL_ERROR;
  }

  if (initialize_state(s))
  {
    print_status("%s: Unable to initialize state variable", __progname);
    return STATUS_INTERNAL_ERROR;
  }

  if (process_command_line(s,argc,argv))
  {
    print_status("%s: Unable to process command line arguments", __progname);
    return STATUS_INTERNAL_ERROR;
  }

#ifdef _WIN32
  if (prepare_windows_command_line(s))
    fatal_error(s,"%s: Unable to process command line arguments", __progname);
#else
  s->argc = argc;
  s->argv = argv;
#endif

  /* Anything left on the command line at this point is a file
     or directory we're supposed to process. If there's nothing
     specified, we should tackle standard input */
  if (optind == argc)
    hash_stdin(s);
  else
  {
    MD5DEEP_ALLOC(TCHAR,fn,PATH_MAX);
    MD5DEEP_ALLOC(TCHAR,cwd,PATH_MAX);

    cwd = _tgetcwd(cwd,PATH_MAX);
    if (NULL == cwd)
      fatal_error(s,"%s: %s", __progname, strerror(errno));

    count = optind;

    while (count < s->argc)
    {  
      generate_filename(s,fn,cwd,s->argv[count]);

#ifdef _WIN32
      status = process_win32(s,fn);
#else
      status = process_normal(s,fn);
#endif

      //      if (status != STATUS_OK)
      //	return status;

      ++count;
    }

    free(fn);
    free(cwd);
  }

  /* We only have to worry about checking for unused hashes if one 
     of the matching modes was enabled. We let the display_not_matched
     function determine if it needs to display anything. The function
     also sets our return values in terms of inputs not being matched
     or known hashes not being used */
  if ((s->mode & mode_match) || (s->mode & mode_match_neg))
    s->return_value = finalize_matching(s);

  return s->return_value;
}
