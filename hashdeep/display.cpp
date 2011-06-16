#include "main.h"
#include "xml.h"

#include "utf8.h"
#include <string>
using namespace std;

// $Id$

/****************************************************************
 ** These are from the original hashdeep/display.c
 ****************************************************************/

std::string itos(uint64_t i)
{
    char buf[256];
    snprintf(buf,sizeof(buf),"%"PRIu64,i);
    return string(buf);
}

static void display_size(state *s)
{
  if (NULL == s) 
    return;

  if (s->mode & mode_display_size)
  {
    // Under CSV mode we have to include a comma, otherwise two spaces
    if (s->mode & mode_csv)
      printf ("%"PRIu64",", s->current_file->actual_bytes);
    else
      printf ("%10"PRIu64"  ", s->current_file->actual_bytes);   
  }
}


static std::string shorten_filename(const std::string &fn)
{
    if (fn.size() < MAX_FILENAME_LENGTH){
	return fn;
    }
    return(fn.substr(0,MAX_FILENAME_LENGTH-3) + "...");
}

static char display_asterisk(state *s)
{
    return (s->mode & mode_asterisk) ? '*' : ' ';
}


/**
 * output the string, typically a fn, optionally performing unicode escaping
 */

void output_unicode(FILE *out,const std::string &utf8)
{
    fwrite(utf8.c_str(),utf8.size(),1,out);
}

/* By default, we display in UTF-8.
 * We escape UTF-8 if requested.
 */
void display_filename(FILE *out, const file_data_t &fdt)
{
#if 0
    /* old windows code */
  size_t pos,len;

  len = _tcslen(fn);

  for (pos = 0 ; pos < len ; ++pos)
  {
    // We can only display the English (00) code page
    if (0 == (fn[pos] & 0xff00))
      fprintf (out,"%c", (char)(fn[pos]));
    else
      fprintf (out,"?");
  }
  
#else
  if(fdt.print_short_name){
      output_unicode(out,shorten_filename(fdt.file_name));
  } else {
      output_unicode(out,fdt.file_name);
  }
  if(fdt.file_name_annotation.size()>0){
      output_unicode(out,fdt.file_name_annotation);
  }
#endif
}
void display_filename(FILE *out, const file_data_t *fdt)
{
    display_filename(out,*fdt);
}





static void display_banner(state *s)
{
  print_status("%s", HASHDEEP_HEADER_10);

  fprintf (stdout,"%ssize,",HASHDEEP_PREFIX);  
  for (int i = 0 ; i < NUM_ALGORITHMS ; ++i) {
      if (s->hashes[i].inuse){
	  printf ("%s,", s->hashes[i].name.c_str());
      }
  }  
  print_status("filename");

  fprintf(stdout,"## Invoked from: ");
  output_unicode(stdout,s->cwd);
  fprintf(stdout,"%s",NEWLINE);
  
  // Display the command prompt as the user saw it
  fprintf(stdout,"## ");
#ifdef _WIN32
  fprintf(stdout,"%c:\\>", s->cwd[0]);
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

    output_unicode(stdout,s->argv[argc]);
    bytes_written += current_bytes;
  }

  fprintf(stdout,"%s## %s",NEWLINE, NEWLINE);
}


static void compute_dfxml(state *s,int known_hash)
{
    if(s->mode & mode_piecewise){
	uint64_t bytes = s->current_file->read_end - s->current_file->read_start;
	s->current_file->dfxml_hash += string("<byte_run file_offset='") + itos(s->current_file->read_start)
	    + string("' bytes='") + itos(bytes) + string("'>\n   ");
    }
    for(int i=0;i<NUM_ALGORITHMS;i++){
	if(s->hashes[i].inuse){
	    s->current_file->dfxml_hash += "<hashdigest type='";
	    s->current_file->dfxml_hash += makeupper(s->current_file->hash_hex[i]);
	    s->current_file->dfxml_hash += string("'>") + s->current_file->hash_hex[i] + string("</hashdigest>\n");
	}
    }
    if(s->mode & mode_which || known_hash){
	s->current_file->dfxml_hash += string("<matched>1</matched>");
    }
    if(s->mode & mode_piecewise){
	s->current_file->dfxml_hash += "</byte_run>\n";
    }
}

/*
 * Externally called to display a simple hash
 */
int display_hash_simple(state *s)
{
    if ( s->dfxml==0 && s->banner_displayed==0){
	display_banner(s);
	s->banner_displayed = 1;
    }

    if(s->dfxml){
	compute_dfxml(s,0);
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

  for (int i = 0 ; i < NUM_ALGORITHMS ; ++i)  {
    if (s->hashes[i].inuse)
	printf("%s,", s->current_file->hash_hex[i].c_str());
  }
  
  display_filename(stdout,s->current_file);
  fprintf(stdout,"%s",NEWLINE);

  return FALSE;
}

/* The old display_match_result from md5deep */
static int md5deep_display_match_result(state *s)
{  
    int known_hash = md5deep_is_known_hash(s->current_file->hash_hex[s->md5deep_mode_algorithm].c_str(),&s->current_file->known_fn);
  if ((known_hash && (s->mode & mode_match)) ||
      (!known_hash && (s->mode & mode_match_neg)))
  {
      if(s->dfxml){
	  compute_dfxml(s,known_hash);
	  return FALSE;
      }

    display_size(s);

    if (s->mode & mode_display_hash)
    {
	printf ("%s", s->current_file->hash_hex[s->md5deep_mode_algorithm].c_str());
      if (s->mode & mode_csv)
	printf (",");
      else
	printf (" %c", display_asterisk(s));
    }

    if (s->mode & mode_which)
    {
      if (known_hash && (s->mode & mode_match))
      {
	display_filename(stdout,s->current_file);
	printf (" matched ");
	output_unicode(stdout,s->current_file->known_fn);
      }
      else
      {
	display_filename(stdout,s->current_file);
	printf (" does NOT match");
      }
    }
    else
      display_filename(stdout,s->current_file);

    make_newline(s);
  }
  return FALSE;
}


/* The old display_hash from the md5deep program, with a few modifications */
int md5deep_display_hash(state *s)
{
    if (s->mode & mode_triage) {
	if(s->dfxml){
	    compute_dfxml(s,1);
	    return FALSE;
	}
	printf ("\t%s\t", s->current_file->hash_hex[s->md5deep_mode_algorithm].c_str());
	display_filename(stdout,s->current_file);
	make_newline(s);
	return FALSE;
    }

  // We can't call display_size here because we don't know if we're
  // going to display *anything* yet. If we're in matching mode, we
  // have to evaluate if there was a match first. 
    if ((s->mode & mode_match) || (s->mode & mode_match_neg)){
	return md5deep_display_match_result(s);
    }

  if(s->dfxml){
      compute_dfxml(s,0);
      return FALSE;
  }

  display_size(s);

  printf ("%s", s->current_file->hash_hex[s->md5deep_mode_algorithm].c_str());

  if (s->mode & mode_quiet)
    printf ("  ");
  else  {
    if ((s->mode & mode_piecewise) ||
	!(s->current_file->is_stdin))
    {
      if (s->mode & mode_timestamp)
      {
	struct tm * my_time = _gmtime64(&(s->current_file->timestamp));
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

      display_filename(stdout,s->current_file);
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
