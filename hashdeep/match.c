
/* $Id$ */

#include "main.h"

static filetype_t 
identify_file(state *s, char *fn, FILE *handle)
{
  if (NULL == s || NULL == fn || NULL == handle)
    internal_error("%s: NULL values in identify_file", __progname);
  
  
  return file_hashdeep_10;
}



status_t add_hash_to_algorithm(state *s,
			       hashname_t alg,
			       file_data_t *f)
{
  if (NULL == s || NULL == f)
    return status_unknown_error;
  
  return (hashtable_add(s,alg,f));
}



status_t add_hash(state *s, file_data_t *f)
{
  hashname_t i;

  if (NULL == s || NULL == s)
    return status_unknown_error;

  for (i = 0 ; i < NUM_ALGORITHMS ; ++i)
    {
      /* RBF - Error check this function */
      if (s->hashes[i]->inuse)
	add_hash_to_algorithm(s,i,f);
    }

  return status_ok;

}


status_t load_match_file(state *s, char *fn)
{
  FILE * handle;
  filetype_t type;

  if (NULL == s || NULL == fn)
    return status_omg_ponies;

  handle = fopen(fn,"rb");
  if (NULL == handle)
  {
    print_error(s,"%s: %s: %s", __progname, fn, strerror(errno));
    return status_file_error;
  }
  
  type = identify_file(s,fn,handle);
  if (file_unknown == type)
  {
    print_error(s,"%s: %s: Unable to identify file format", __progname, fn);
    fclose(handle);
    return status_file_error;
  }
  
  /* RBF - Adding hash for /dev/null */
  file_data_t * rbf = (file_data_t *)malloc(sizeof(file_data_t));
  rbf->hash[alg_md5] = strdup("d41d8cd98f00b204e9800998ecf8427e");
  rbf->hash[alg_sha256] = strdup("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
  rbf->file_size = 0;
  rbf->file_name = _TEXT("/dev/null");
  rbf->used = FALSE;
  rbf->id = 0;
  rbf->next = NULL;
  s->next_known_id++;

  add_hash(s,rbf);

  fclose(handle);
  return status_ok;
}

static char * 
status_to_str(status_t s)
{
  switch (s) {
  case status_ok: return "ok";
  case status_match: return "complete match";
  case status_partial_match: return "partial match";
  case status_file_size_mismatch: return "file size mismatch";
  case status_file_name_mismatch: return "file name mismatch";
  case status_no_match: return "no match"; 

  default:
    return "unknown";
  }      
}


status_t is_known_file(state *s)
{
  hashtable_entry_t * ret , * tmp;
  hashname_t i;

  for (i = 0 ; i < NUM_ALGORITHMS; ++i)
  {
    if (s->hashes[i]->inuse)
    {

      ret = hashtable_contains(s,i);
      if (NULL != ret)
	{
	  tmp = ret;
	  while (tmp != NULL)
	    {
	      display_filename(stdout,s->current_file->file_name);
	      fprintf(stdout," matches ");
	      display_filename(stdout,tmp->data->file_name);
	      print_status(": %s", status_to_str(tmp->status));
			   
	      if (tmp->data->used)
		print_status("This file has been previously matched");
	      else
		{
		  tmp->data->used = TRUE;
		  print_status("This file has NOT been previously matched");
		}
		  
	      tmp = tmp->next;
	    }
	}
      else
	{
	  if (s->mode & mode_verbose)
	    {
	      display_filename(stdout,s->current_file->file_name);
	      print_status(": No match");
	    }
	}
    }
  }

  return status_no_match;
}
