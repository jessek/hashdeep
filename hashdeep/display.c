
#include "main.h"

/* $Id: display.c,v 1.11 2007/12/27 20:52:03 jessekornblum Exp $ */

void display_banner(state *s)
{
  hashname_t i;
  int argc;
  
  print_status("%sHASHDEEP-%s", HASHDEEP_PREFIX, HASHDEEP_VERSION);
  printf ("%ssize,",HASHDEEP_PREFIX);
  
  for (i = 0 ; i < NUM_ALGORITHMS ; ++i)
  {
    if (s->hashes[i]->inuse)
      printf ("%s,", s->hashes[i]->name);
  }
  
  print_status("filename");

  print_status("## Invoked from: %s", s->cwd);
  printf ("## %s ", CMD_PROMPT);  
  for (argc = 0 ; argc < s->argc ; ++argc)
    display_filename(stdout,s->argv[argc]);

  printf("%s", NEWLINE);

  s->banner_displayed = TRUE;
}


static int display_hash_simple(state *s)
{
  hashname_t i;

  if ( ! (s->banner_displayed))
    display_banner(s);

  printf ("%"PRIu64",", s->bytes_read);

  for (i = 0 ; i < NUM_ALGORITHMS ; ++i)
  {
    if (s->hashes[i]->inuse)
      printf("%s,", s->hashes[i]->result);
  }
  
  display_filename(stdout,s->full_name);
  fprintf(stdout,"%s", NEWLINE);

  return FALSE;
}


static int 
display_match(state *s)
{
  if (NULL == s)
    return TRUE;

  switch (is_known_file(s))
    {
    case status_match:
      display_filename(stdout,s->full_name);
      return FALSE;
      
    case status_partial_match:
      print_status("%s: WARNING: Partial file match", s->full_name);
      return FALSE;

    case status_omg_ponies:
      fatal_error(s,"OMG! PONIES!1!!");
      
    default:
      return TRUE;
    }
  


  return FALSE;
}

static int 
display_match_neg(state *s)
{
  if (NULL == s)
    return TRUE;

  switch (is_known_file(s))
    {
    case status_match:
      return FALSE;
      
    case status_no_match:
      display_filename(stdout,s->full_name);
      print_status("");
      return FALSE;

    default:
      return FALSE;
    }


  return FALSE;
}


int display_hash(state *s)
{
  if (NULL == s)
    return TRUE;

  switch (s->primary_function)
    {
    case primary_compute: return display_hash_simple(s);
    case primary_match: return display_match(s);
    case primary_match_neg: return display_match_neg(s);
    case primary_audit: return audit_update(s);
    default:
      return FALSE;
    }
}
  

