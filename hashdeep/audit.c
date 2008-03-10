
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
  if (s->mode & mode_verbose)
    {
      /* RBF - Should we display the known files that we're never matched? */
      
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


int audit_update(state *s)
{
  int no_match = FALSE, exact_match = FALSE, moved = FALSE, partial = FALSE;
  hashname_t i;
  hashtable_entry_t * matches, * tmp;
  uint64_t my_round = s->hash_round;
  TCHAR * matched_file = NULL;
  
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
      tmp = matches;
      if (NULL == tmp)
      {
	no_match = TRUE;
      }
      else while (tmp != NULL)
      {
	if (tmp->data->used != s->hash_round)
	{
	  tmp->data->used = s->hash_round;
	  switch (tmp->status) {
	  case status_match:
	    exact_match = TRUE;
	    break;
    
	    /* RBF - This shouldn't happen? */
	  case status_no_match:
	    break;
    
	  case status_file_name_mismatch:
	    moved = TRUE;
	    break;
    
	    /* Hash collision */
	  case status_partial_match:
	    partial = TRUE;
	    matched_file = tmp->data->file_name;
	    break;
	    
	  case status_file_size_mismatch:
	    /* Implies all hashes match, sizes different */
	    partial = TRUE;
	    break;
	    
	  case status_unknown_error:
	    // RBF - What do we do here?
	    
	    
	  default:
	    break;
	  }
	}

	tmp = tmp->next;
      }
      
      hashtable_destroy(matches);
    }
  }


  if (exact_match)
  {
    if (s->mode & mode_more_verbose)
    {
      display_filename(stdout,matches->data->file_name);
      print_status(" ok");
    }
    s->match_exact++;
  }
  else if (no_match)
  {
    s->match_unknown++;
    if (s->mode & mode_more_verbose)
    {
      display_filename(stdout,s->current_file->file_name);
      print_status(": No match");
    }
  } 
  else if (moved)
  {
    if (s->mode & mode_more_verbose)
    {
	display_filename(stdout,matches->data->file_name);
	fprintf(stdout," moved to ");
	display_filename(stdout,s->current_file->file_name);
	print_status("");
    }
    s->match_moved++;
  }
  else if (partial)
  {
    if (s->mode & mode_more_verbose)
    {
	display_filename(stdout,matches->data->file_name);
	fprintf(stdout," partially matched ");
	display_filename(stdout,matched_file);
	print_status("");
    }
    s->match_partial++;
  }
  
  return status_ok;
}
