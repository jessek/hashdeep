
#include "main.h"
#include "xml.h"

#include <string>
using namespace std;

// $Id$

/****************************************************************
 ** These are from the original hashdeep/display.c
 ****************************************************************/

static void display_size(state *s)
{
  if (NULL == s) return;

  if (s->mode & mode_display_size)
  {
    // When in CSV mode we always display the full size
    if (s->mode & mode_csv)
    {
      printf ("%"PRIu64",", s->current_file->actual_bytes);
    }
    // We reserve ten characters for digits followed by two spaces
    else if (s->current_file->bytes_read > 9999999999LL)
      printf ("9999999999  ");
    else
      printf ("%10"PRIu64"  ", s->current_file->actual_bytes);      
  }	
}


static char display_asterisk(state *s)
{
  if (NULL == s) return ' ';
  return (s->mode & mode_asterisk) ? '*' : ' ';
}



static void display_banner(state *s)
{
  print_status("%s", HASHDEEP_HEADER_10);

  fprintf (stdout,"%ssize,",HASHDEEP_PREFIX);  
  for (int i = 0 ; i < NUM_ALGORITHMS ; ++i) {
    if (s->hashes[i]->inuse)
      printf ("%s,", s->hashes[i]->name);
  }  
  print_status("filename");

  fprintf(stdout,"## Invoked from: ");
  display_filename(stdout,s->cwd);
  fprintf(stdout,"%s",NEWLINE);
  
  // Display the command prompt as the user saw it
  fprintf(stdout,"## ");
#ifdef _WIN32
  _ftprintf(stdout,_TEXT("%c:\\>"), s->cwd[0]);
#else
  if (0 == geteuid())
    fprintf(stdout,"#");
  else
    fprintf(stdout,"$");
#endif

  // Accounts for '## ', command prompt, and space before first argument
  size_t bytes_written = 8;

  for (int argc = 0 ; argc < s->argc ; ++argc)
  {
    fprintf(stdout," ");
    bytes_written++;

    size_t current_bytes = _tcslen(s->argv[argc]);

    // The extra 32 bytes is a fudge factor
    if (current_bytes + bytes_written + 32 > MAX_STRING_LENGTH)
    {
      fprintf(stdout,"%s## ", NEWLINE);
      bytes_written = 3;
    }

    display_filename(stdout,s->argv[argc]);
    bytes_written += current_bytes;
  }

  fprintf(stdout,"%s## %s",NEWLINE, NEWLINE);
}


void display_dfxml(state *s,int known_hash)
{
    s->dfxml->push("fileobject");
    s->dfxml->xmlout("filename",s->current_file->file_name0);
    for(int i=0;i<NUM_ALGORITHMS;i++){
	if(s->hashes[i]->inuse){
	    string attrib="type='";
	    for(const char *cc=s->hashes[i]->name;*cc;cc++){
		attrib.push_back(toupper(*cc)); // add it in uppercase
	    }
	    attrib.append("'");
	    s->dfxml->xmlout("hashdigest",s->current_file->hash[i],attrib,0);
	}
    }
    if(s->mode & mode_which || known_hash){
	s->dfxml->xmlout("matched",known_hash);
    }
    s->dfxml->pop();			// file object
}

int display_hash_simple(state *s)
{
    if ( s->dfxml==0 && s->banner_displayed==0){
	display_banner(s);
	s->banner_displayed = 1;
    }

    if(s->dfxml){
	display_dfxml(s,0);
	return FALSE;
    }

  // In piecewise mode the size of each 'file' is the size
  // of the block it came from. This is important when doing an
  // audit in piecewise mode. In all other cases we use the 
  // total number of bytes from the file we *actually* read
  if (s->mode & mode_piecewise)
    printf ("%"PRIu64",", s->current_file->bytes_read);
  else
    printf ("%"PRIu64",", s->current_file->actual_bytes);

  for (int i = 0 ; i < NUM_ALGORITHMS ; ++i)
  {
    if (s->hashes[i]->inuse)
      printf("%s,", s->current_file->hash[i]);
  }
  
  display_filename(stdout,s->current_file->file_name);
  fprintf(stdout,"%s",NEWLINE);

  return FALSE;
}

/* The old display_match_result from md5deep */
static int md5deep_display_match_result(state *s)
{  
  int known_hash = md5deep_is_known_hash(s->md5deep_mode_hash_result,s->known_fn);
  if ((known_hash && (s->mode & mode_match)) ||
      (!known_hash && (s->mode & mode_match_neg)))
  {
      if(s->dfxml){
	  display_dfxml(s,known_hash);
	  return FALSE;
      }

    display_size(s);

    if (s->mode & mode_display_hash)
    {
      printf ("%s", s->md5deep_mode_hash_result);
      if (s->mode & mode_csv)
	printf (",");
      else
	printf (" %c", display_asterisk(s));
    }

    if (s->mode & mode_which)
    {
      if (known_hash && (s->mode & mode_match))
      {
	display_filename(stdout,s->full_name);
	printf (" matched %s", s->known_fn);
      }
      else
      {
	display_filename(stdout,s->full_name);
	printf (" does NOT match");
      }
    }
    else
      display_filename(stdout,s->full_name);

    make_newline(s);
  }
  return FALSE;
}


/* The old display_hash from the md5deep program, with a few modifications */
int md5deep_display_hash(state *s)
{
    if (s->mode & mode_triage) {
	if(s->dfxml){
	    display_dfxml(s,1);
	    return FALSE;
	}
	printf ("\t%s\t", s->md5deep_mode_hash_result);
	display_filename(stdout,s->full_name);
	make_newline(s);
	return FALSE;
    }

  // We can't call display_size here because we don't know if we're
  // going to display *anything* yet. If we're in matching mode, we
  // have to evaluate if there was a match first. 
  if ((s->mode & mode_match) || (s->mode & mode_match_neg))
    return md5deep_display_match_result(s);

  if(s->dfxml){
      display_dfxml(s,0);
      return FALSE;
  }

  display_size(s);

  printf ("%s", s->md5deep_mode_hash_result);

  if (s->mode & mode_quiet)
    printf ("  ");
  else
  {
    if ((s->mode & mode_piecewise) ||
	!(s->is_stdin))
    {
      if (s->mode & mode_timestamp)
      {
	struct tm * my_time = _gmtime64(&(s->timestamp));
	char time_str[MAX_TIME_STRING_LENGTH];

	// The format is four digit year, two digit month, 
	// two digit hour, two digit minute, two digit second
	strftime(time_str, sizeof(time_str), "%Y:%m:%d:%H:%M:%S", my_time);

	printf ("%c%s", (s->mode & mode_csv?',':' '), time_str);
      }

      
      if (s->mode & mode_csv)
	printf(",");
      else
	printf(" %c", display_asterisk(s));      

      display_filename(stdout,s->full_name);
    }
  }

  make_newline(s);
  return FALSE;
}

// This function is called by hash.c when the hashing operation is complete.
int display_hash(state *s)
{
    assert(s!=NULL);
    
    if(s->md5deep_mode){
	md5deep_display_hash(s);
	return TRUE;
    }

    switch (s->primary_function) {
    case primary_compute: 
      return display_hash_simple(s);

    case primary_match: 
    case primary_match_neg: 
      return display_match_result(s);

    case primary_audit: 
      return audit_update(s);
    }

  return FALSE;
}
