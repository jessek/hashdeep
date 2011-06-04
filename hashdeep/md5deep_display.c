//#include "md5deep.h"
#include "common.h"
#include "main.h"


// $Id$ 

static int md5deep_display_match_result(state *s,algorithm_t *a)
{  
#if 0
  int known_hash;

  if (NULL == s || a==NULL)    return TRUE;

  known_hash = is_known_hash(s->hash_result,s->known_fn);
  if ((known_hash && (s->mode & mode_match)) ||
      (!known_hash && (s->mode & mode_match_neg)))
  {
    display_size(s);

    if (s->mode & mode_display_hash)
    {
      printf ("%s", s->hash_result);
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
#endif  
  return FALSE;
}


int md5deep_display_hash(state *s,algorithm_t *a)
{
#if 0
  if (NULL == s)
    return TRUE;
  
  if (s->mode & mode_triage)
  {
    printf ("\t%s\t", s->hash_result);
    display_filename(stdout,s->full_name);
    make_newline(s);
    return FALSE;
  }    

  // We can't call display_size here because we don't know if we're
  // going to display *anything* yet. If we're in matching mode, we
  // have to evaluate if there was a match first. 
  if ((s->mode & mode_match) || (s->mode & mode_match_neg))
    return md5deep_display_match_result(s);

  display_size(s);

  printf ("%s", s->hash_result);

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

	// The format is four digit year, two digit month, 
	// two digit hour, two digit minute, two digit second
	strftime(s->time_str, 
		 MAX_TIME_STRING_LENGTH, 
		 "%Y:%m:%d:%H:%M:%S", 
		 my_time);

	printf ("%c%s", (s->mode & mode_csv?',':' '), s->time_str);
      }

      
      if (s->mode & mode_csv)
	printf(",");
      else
	printf(" %c", display_asterisk(s));      

      display_filename(stdout,s->full_name);
    }
  }

  make_newline(s);
#endif
  return FALSE;
}
