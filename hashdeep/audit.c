
/* $Id$ */

#include "main.h"

void setup_audit(state *s)
{
  s->match_exact   = 0;
  s->match_partial = 0;
  s->match_moved   = 0;
  s->match_unknown = 0;
  s->match_total   = 0;
  s->match_expect  = 0;
}


int audit_status(state *s)
{
  file_data_t * tmp = s->known;

  s->match_unused = 0;

  while (tmp != NULL)
    {
      if (0 == tmp->used)
	s->match_unused++;
      tmp = tmp->next;
    }

  return (0 == s->match_unused  && 
	  0 == s->match_unknown && 
	  0 == s->match_moved);
}


int display_audit_results(state *s)
{
  if (NULL == s)
    return TRUE;

  int status = FALSE;
  
  if (!audit_status(s))
    {
      print_status("%s: Audit failed", __progname);
      status = TRUE;
    }
  else
    print_status("%s: Audit passed", __progname);
  
  /* RBF - How should we display the audit results? */
  if (s->mode & mode_verbose || status)
    {
      /*
      print_status("   Input files examined: %"PRIu64, s->match_total);
      print_status("  Known files expecting: %"PRIu64, s->match_expect);
      print_status(" ");
      */
      print_status("          Files matched: %"PRIu64, s->match_exact);
      print_status("Files paritally matched: %"PRIu64, s->match_partial);
      print_status("            Files moved: %"PRIu64, s->match_moved);
      print_status("        New files found: %"PRIu64, s->match_unknown);
      print_status("  Known files not found: %"PRIu64, s->match_unused);

    }
  
  return status;
}

static void update_audit_match(state *s, hashtable_entry_t * matches)
{
  switch(matches->status)
  {
  case status_match:
    display_filename(stdout,matches->data->file_name);
    print_status(" ok");
    s->match_exact++;
    break;
    
    /* RBF - This shouldn't happen? */
  case status_no_match:
    s->match_unknown++;
    break;
    
  case status_file_name_mismatch:
    /* Implies all hashes and file size match */   
    if (s->mode & mode_more_verbose)
      {
	display_filename(stdout,matches->data->file_name);
	fprintf(stdout," moved to ");
	display_filename(stdout,s->current_file->file_name);
	print_status("");
      }
    s->match_moved++;
    break;
    
    /* Hash collision */
  case status_partial_match:
    s->match_partial++;
    break;
    
  case status_file_size_mismatch:
    /* Implies all hashes match, sizes different */
    s->match_partial++;
    break;
    
  case status_unknown_error:
    // RBF - What do we do here?
    
  default:
    break;
  }
}


int audit_update(state *s)
{
  hashname_t i;
  hashtable_entry_t * matches, * tmp;
  uint64_t my_round = s->hash_round;
  
  if (NULL == s)
    return TRUE;

  s->hash_round++;
  if (my_round > s->hash_round)
    fatal_error(s,"%s: Too many input files", __progname);

  // RBF - Do we care?
  /* In an audit, each file should be matched exactly once. 
     As such, their hash_round value should be zero when we
     get here. */

  // RBF - Does anybody ever use match total? 
  s->match_total++;

  for (i = 0 ; i < NUM_ALGORITHMS; ++i)
  {
    if (s->hashes[i]->inuse)
    {
      matches = hashtable_contains(s,i);

      if (NULL == matches)
      {
	/* No matches found */
	if (s->mode & mode_more_verbose)
	{
	  display_filename(stdout,s->current_file->file_name);
	  print_status(" did not match");
	}
	
	s->match_unknown++;
	return FALSE;
      }
      else if (NULL == matches->next)
      {
	if (matches->data->used != 0)
	  

	update_audit_match(s,matches);
	hashtable_destroy(matches);
      }
      else
      {
	print_status("Found multiple matches for %s", s->current_file->file_name);
	hashtable_destroy(matches);
      }
    }
  }

  return FALSE;
}
