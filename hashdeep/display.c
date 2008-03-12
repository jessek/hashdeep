
#include "main.h"

/* $Id$ */

static void display_banner(state *s)
{
  hashname_t i;
  int argc;
  
  print_status("%s", HASHDEEP_HEADER_10);
  fprintf (stdout,"%ssize,",HASHDEEP_PREFIX);
  
  for (i = 0 ; i < NUM_ALGORITHMS ; ++i)
  {
    if (s->hashes[i]->inuse)
      printf ("%s,", s->hashes[i]->name);
  }
  
  print_status("filename");

  fprintf(stdout,"## Invoked from: ");
  display_filename(stdout,s->cwd);
  fprintf(stdout,"%s",NEWLINE);

  
  /* Display the command prompt as the user saw it */
  fprintf(stdout,"## ");
#ifdef _WIN32
  _ftprintf(stdout,_TEXT("%c:\\>"), s->cwd[0]);
#else
  if (0 == geteuid())
    fprintf(stdout,"#");
  else
    fprintf(stdout,"$");
#endif

  for (argc = 0 ; argc < s->argc ; ++argc)
  {
    fprintf(stdout," ");
    display_filename(stdout,s->argv[argc]);
  }

  fprintf(stdout,"%s", NEWLINE);

  s->banner_displayed = TRUE;
}


int display_hash_simple(state *s)
{
  hashname_t i;

  if ( ! (s->banner_displayed))
    display_banner(s);

  printf ("%"PRIu64",", s->bytes_read);

  for (i = 0 ; i < NUM_ALGORITHMS ; ++i)
  {
    if (s->hashes[i]->inuse)
      printf("%s,", s->current_file->hash[i]);
  }
  
  display_filename(stdout,s->current_file->file_name);
  fprintf(stdout,"%s", NEWLINE);

  return FALSE;
}


/* This function is called by hash.c when the hashing operation is complete. */
int display_hash(state *s)
{
  if (NULL == s)
    return TRUE;

  switch (s->primary_function)
    {
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
  

