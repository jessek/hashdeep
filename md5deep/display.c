
#include "main.h"

/* $Id: display.c,v 1.1 2007/12/09 18:58:06 jessekornblum Exp $ */

static void display_size(state *s)
{
  if (s->mode & mode_display_size)
  {
    // We reserve ten characters for digits followed by two spaces
    if (s->bytes_read > 9999999999LL)
      printf ("9999999999  ");
    else
      printf ("%10"PRIu64"  ", s->bytes_read);      
  }	
}


static char display_asterisk(state *s)
{
  if (s->mode & mode_asterisk)
    return '*';
  return ' ';
}


static int display_match_result(state *s)
{  
  int known_hash;

  known_hash = is_known_hash(s->hash_result,s->known_fn);
  if ((known_hash && (s->mode & mode_match)) ||
      (!known_hash && (s->mode & mode_match_neg)))
  {
    display_size(s);

    if (s->mode & mode_display_hash)
      printf ("%s %c",s->hash_result,display_asterisk(s));

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


int display_hash(state *s)
{
  /* We can't call display_size here because we don't know if we're
     going to display *anything* yet. If we're in matching mode, we
     have to evaluate if there was a match first. */
  if ((s->mode & mode_match) || (s->mode & mode_match_neg))
    return display_match_result(s);

  display_size(s);
  _tprintf (_TEXT("%s"), s->hash_result);

  if (s->mode & mode_quiet)
    _tprintf (_TEXT("  "));
  else
  {
    if ((s->mode & mode_piecewise) ||
	!(s->is_stdin))
    {
      printf(" %c", display_asterisk(s));      
      display_filename(stdout,s->full_name);
    }
  }

  make_newline(s);
  return FALSE;
}
