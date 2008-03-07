
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

  f->id = s->next_known_id;
  s->next_known_id++;
  f->next = NULL;

  if (NULL == s->known)
    s->known = f;
  else
    s->last->next = f;

  s->last = f;

  for (i = 0 ; i < NUM_ALGORITHMS ; ++i)
    {
      /* RBF - Error check this function */
      if (s->hashes[i]->inuse)
	add_hash_to_algorithm(s,i,f);
    }

  return status_ok;

}

int initialize_file_data(file_data_t *f)
{
  if (NULL == f)
    return TRUE;

  f->next = NULL;
  f->id   = 0;
  f->used = FALSE;

  return FALSE;
}


int add_RBF_hash(state *s, TCHAR *fn, char *md5, char *sha256, uint64_t size)
{
  file_data_t * tmp;

  tmp = (file_data_t *)malloc(sizeof(file_data_t));
  if (NULL == tmp)
    return TRUE;

  if (initialize_file_data(tmp))
    return TRUE;

  tmp->file_name = _tcsdup(fn);
  tmp->hash[alg_md5] = strdup(md5);
  tmp->hash[alg_sha256] = strdup(sha256);
  tmp->file_size = size;

  //  print_status("%s %s", tmp->hash[alg_md5], tmp->hash[alg_sha256]);

  add_hash(s,tmp);

  return FALSE;
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
  
  add_RBF_hash(s,
	       _TEXT("/dev/null"),
	       "d41d8cd98f00b204e9800998ecf8427e",
	       "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
	       0);

  add_RBF_hash(s,
	       _TEXT("/home/jdoe/md5deep/svn/trunk/hashdeep/bar"),
	       "d41d8cd98f00b204e9800998ecf8427e",
	       "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
	       0);

  add_RBF_hash(s,
#ifdef __LINUX__
	       _TEXT("/home/jdoe/md5deep/svn/trunk/hashdeep/abc"),
#else
	       _TEXT("/Users/jessekornblum/Documents/research/md5deep/svn/trunk/hashdeep/abc"),
#endif
	       "900150983cd24fb0d6963f7d28e17f72",
	       "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
	       3);


  fclose(handle);
  return status_ok;
}

char * status_to_str(status_t s)
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

static void
display_match(state *s, hashtable_entry_t * match)
{
  /* RBF - Update this to display partial hash warnings */
	  
  if (s->mode & mode_display_hash)
    display_hash_simple(s);
  else
  {
    display_filename(stdout,s->current_file->file_name);
    if (s->mode & mode_which)
    {
      fprintf(stdout," matches ");
      display_filename(stdout,match->data->file_name);
    }

    print_status("");

    switch (match->status) {
    case status_partial_match:
      print_error_unicode(s,s->current_file->file_name,"Only some hashes matched. Hash collision!");
      break;
      
    case status_file_size_mismatch:
      print_error_unicode(s,s->current_file->file_name,"File size of input does not match known file. Hash collision!");
      break;
      
      /* We don't need to print warnings on file name changes */
    default: break;
    }
  }
}

status_t is_known_file(state *s)
{
  status_t status = status_no_match;
  int should_display = FALSE;
  hashtable_entry_t * ret , * tmp;
  hashname_t i;
  uint64_t my_round = s->hash_round;

  s->hash_round++;
  if (my_round > s->hash_round)
      fatal_error(s,"%s: Too many input files", __progname);

  for (i = 0 ; i < NUM_ALGORITHMS; ++i)
  {
    /* RBF - Do we need to do this for each hash algorithm? */
    if (s->hashes[i]->inuse)
    {
      ret = hashtable_contains(s,i);
      if (ret != NULL && primary_match == s->primary_function)
	{
	  tmp = ret;
	  while (tmp != NULL)
	    {
	      if (tmp->data->used != s->hash_round)
		{
		  tmp->data->used = s->hash_round;
	
		  display_match(s,tmp);
	  	
		
		  status = tmp->status;
		}
	      
	      tmp = tmp->next;
	    }
	  hashtable_destroy(ret);
	}
      
      else if (primary_match_neg == s->primary_function)
	should_display = (NULL == ret);
    }
  }

  if (should_display)
    {
      if (s->mode & mode_display_hash)
	display_hash_simple(s);
      else
	{
	  display_filename(stdout,s->current_file->file_name);
	  print_status("");
	}
    }
  
  return status;
}
