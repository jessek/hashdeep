// $Id$ 

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
  file_data_t *tmp_fdt = s->known;

  s->match_unused = 0;

  while (tmp_fdt != NULL)
  {
    if (0 == tmp_fdt->used)
    {
      s->match_unused++;
      if (s->mode & mode_more_verbose) {
	display_filename(stdout,tmp_fdt);
	print_status(": Known file not used");
      }
    }
    tmp_fdt = tmp_fdt->next;
  }
    
  return (0 == s->match_unused  && 
	  0 == s->match_unknown && 
	  0 == s->match_moved);
}


int display_audit_results(state *s)
{
  int status = EXIT_SUCCESS;

  if (NULL == s)
    return EXIT_FAILURE;
  
  if (!audit_status(s))
    {
      print_status("%s: Audit failed", __progname);
      status = EXIT_FAILURE;
    }
  else
    print_status("%s: Audit passed", __progname);
  
  if (s->mode & mode_verbose)
    {
      /*
      print_status("   Input files examined: %"PRIu64, s->match_total);
      print_status("  Known files expecting: %"PRIu64, s->match_expect);
      print_status(" ");
      */
      print_status("          Files matched: %"PRIu64, s->match_exact);
      print_status("Files partially matched: %"PRIu64, s->match_partial);
      print_status("            Files moved: %"PRIu64, s->match_moved);
      print_status("        New files found: %"PRIu64, s->match_unknown);
      print_status("  Known files not found: %"PRIu64, s->match_unused);

    }
  
  return status;
}


int audit_update(state *s)
{
  int no_match = FALSE, exact_match = FALSE, moved = FALSE, partial = FALSE;
  file_data_t * moved_file = NULL, * partial_file = NULL;
  uint64_t my_round;
  
  if (NULL == s)
    return TRUE;

  my_round = s->hash_round;
  s->hash_round++;
  if (my_round > s->hash_round)
    fatal_error(s,"%s: Too many input files", __progname);

  // Although nobody uses match_total right now, we may in the future 
  //  s->match_total++;

  for (int i = 0 ; i < NUM_ALGORITHMS; i++)
  {
    if (s->hashes[i].inuse)
    {
	hashtable_entry_t *matches = hashtable_contains(s,(hashid_t)i);
	hashtable_entry_t *tmp = matches;
      if (NULL == tmp)
      {
	no_match = TRUE;
      }
      else while (tmp != NULL)
      {
	// print_status("Got status %d for %s", tmp->status, tmp->data->file_name);

	if (tmp->data->used != s->hash_round)
	{

	  switch (tmp->status) {
	  case status_match:
	    tmp->data->used = s->hash_round;
	    exact_match = TRUE;
	    break;
    
	    // This shouldn't happen 
	  case status_no_match:
	    no_match = TRUE;
	    print_error_unicode(s,s->current_file->file_name,
				"Internal error: Got match for \"no match\"");
	    break;
    
	  case status_file_name_mismatch:
	    moved = TRUE;
	    moved_file = tmp->data;
	    break;
    
	    // Hash collision 
	  case status_partial_match:
	  case status_file_size_mismatch:
	    partial = TRUE;
	    partial_file = tmp->data;
	    break;
	    
	  case status_unknown_error:
	    return status_unknown_error;
	    
	  default:
	    break;
	  }
	}

	tmp = tmp->next;

      }
      hashtable_destroy(matches);
    }
  }

  // If there was an exact match, that overrides any other matching
  // 'moved' file. Usually this happens when the same file exists
  // in two places. In this case we find an exact match and a case
  // where the filenames don't match. 

  if (exact_match)
  {
    s->match_exact++;
    if (s->mode & mode_insanely_verbose)
    {
	display_filename(stdout,s->current_file);
      print_status(": Ok");
    }
  }

  else if (no_match)
  {
    s->match_unknown++;
    if (s->mode & mode_more_verbose)
    {
      display_filename(stdout,s->current_file);
      print_status(": No match");
    }
  } 

  else if (moved)
  {
    s->match_moved++;
    if (s->mode & mode_more_verbose)
    {
	display_filename(stdout,s->current_file);
	fprintf(stdout,": Moved from ");
	display_filename(stdout,moved_file);
	print_status("");
    }

  }
  
  // A file can have a hash collision even if it matches an otherwise
  // known good. For example, an evil version of ntoskrnl.exe should
  // have an exact match to EVILEVIL.EXE but still have a collision 
  // with the real ntoskrnl.exe. When this happens we should report
  // both results. 
  if (partial)  {
    // We only record the hash collision if it wasn't anything else.
    // At the same time, however, a collision is such a significant
    // event that we print it no matter what. 
    if (!exact_match && !moved && !no_match)
      s->match_partial++;
    display_filename(stdout,s->current_file);
    fprintf(stdout,": Hash collision with ");
    display_filename(stdout,partial_file);
    print_status("");
  }
  
  return FALSE;
}
