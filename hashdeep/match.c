
/* $Id$ */

#include "main.h"

static int parse_hashing_algorithms(state *s, char *fn, char *val)
{
  int done = FALSE;
  size_t len = strlen(val), pos = 0, local_pos;
  char buf[ALGORITHM_NAME_LENGTH];
  /* The first position is always the file size */
  uint8_t order = 1;
  
  if (0 == len || NULL == s)
    return TRUE;

  /* RBF - Replace homebrew crap with strtok */
  
  while (pos < len && !done)
  {
    local_pos = 0;
    while (val[pos] != ',' && 
	   pos < len && 
	   local_pos < ALGORITHM_NAME_LENGTH)
    {    
      buf[local_pos] = val[pos];
      
      ++pos;
      ++local_pos;
    }


    if (',' == val[pos] || pos == len)
    {
      buf[local_pos] = 0; 
      
      if (STRINGS_EQUAL(buf,"md5"))
	{
	  s->hashes[alg_md5]->inuse = TRUE;
	  s->hash_order[order] = alg_md5;
	}
      
      else if (STRINGS_EQUAL(buf,"sha1") || 
	       STRINGS_EQUAL(buf,"sha-1"))
	{
	  s->hashes[alg_sha1]->inuse = TRUE;
	  s->hash_order[order] = alg_sha1;
	}
      
      else if (STRINGS_EQUAL(buf,"sha256") || 
	       STRINGS_EQUAL(buf,"sha-256"))
	{
	  s->hashes[alg_sha256]->inuse = TRUE;
	  s->hash_order[order] = alg_sha256;
	}
      
      else if (STRINGS_EQUAL(buf,"tiger"))
	{
	  s->hashes[alg_tiger]->inuse = TRUE;
	  s->hash_order[order] = alg_tiger;
	}
      
      else if (STRINGS_EQUAL(buf,"whirlpool"))
	{
	  s->hashes[alg_whirlpool]->inuse = TRUE;
	  s->hash_order[order] = alg_whirlpool;
	}

      else if (STRINGS_EQUAL(buf,"filename"))
      {
	if (pos != len)
	  {
	    print_error(s,"%s: %s: Badly formatted file", __progname, fn);
	    try_msg();
	    exit(EXIT_FAILURE);
	  }
	done = TRUE;
      }
      
      else
      {
	print_error(s,"%s: %s: Unknown algorithm", __progname, buf);
	try_msg();
	exit(EXIT_FAILURE);
      }

      /* Skip over the comma that's separating this value from the next.
	 It's okay if we're working with the last value in the string.
	 The loop invariant is that pos < len (where len is the length
	 of the string). We won't reference anything out of bounds. */
      ++pos;
      ++order;
    }
    
    else
      return TRUE;
  }

  s->hash_order[order] = alg_unknown;

  if (done)
    return FALSE;
  return TRUE;
}



static filetype_t 
identify_file(state *s, char *fn, FILE *handle)
{
  if (NULL == s || NULL == fn || NULL == handle)
    internal_error("%s: NULL values in identify_file", __progname);

  char * buf = (char *)malloc(MAX_STRING_LENGTH);
  if (NULL == buf)
    return file_unknown;

  /* Find the header */
  if ((fgets(buf,MAX_STRING_LENGTH,handle)) == NULL) 
    {
      free(buf);
      return file_unknown;
    }

  chop_line(buf);

  if ( ! STRINGS_EQUAL(buf,HASHDEEP_HEADER_10))
    {
      free(buf);
      return file_unknown;
    }

  /* Find which hashes are in this file */
  if ((fgets(buf,MAX_STRING_LENGTH,handle)) == NULL) 
    {
      free(buf);
      return file_unknown;
    }

  chop_line(buf);

  if (strncasecmp("%%%% size,",buf,10))
    {
      free(buf);
      return file_unknown;
    }

  /* RBF - Save the list of currently enabled algorithms if this is
     not the first file being loaded. If changes are made, error! */
  hashname_t i;
  for (i = 0 ; i < NUM_ALGORITHMS ; ++i)
    {
      s->hashes[i]->inuse = FALSE;
      s->hash_order[i] = alg_unknown;
    }

  parse_hashing_algorithms(s,fn,buf + 10);

  /*
  for (i = 0 ; i < NUM_ALGORITHMS ; ++i)
    print_status("%s: %s", s->hashes[i]->name, s->hashes[i]->inuse?"ON":"-");

  print_status("File in order:%ssize", NEWLINE); 
  i = 1;
  while (s->hash_order[i] != alg_unknown)
    {
      print_status("%s", s->hashes[s->hash_order[i]]->name);
      ++i;
    }
  */

  free(buf);

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


static int read_file(state *s, char *fn, FILE *handle)
{
  char * buf;
  file_data_t * t;
  uint64_t line_number = 0;

  if (NULL == s || NULL == fn || NULL == handle)
    return TRUE;

  buf = (char *)malloc(sizeof(char) * MAX_STRING_LENGTH);
  if (NULL == buf)
    fatal_error(s,"%s: Out of memory while trying to read %s", __progname,fn);

  while (fgets(buf,MAX_STRING_LENGTH,handle))
  {
    line_number++;
    chop_line(buf);

    if (buf[0] == '#')
      continue;

    t = (file_data_t *)malloc(sizeof(file_data_t));
    if (NULL == t)
      fatal_error(s,"%s: %s: Out of memory in line %"PRIu64, 
		  __progname, fn, line_number);

    initialize_file_data(t);

    char * tmp = strdup(buf);
    if (NULL == tmp)
      fatal_error(s,"%s: %s: Out of memory in line %"PRIu64, 
		  __progname, fn, line_number);


    char **ap, *argv[10];
    for (ap = argv ; (*ap = strsep(&tmp,",")) != NULL ; )
      if (**ap != '\0')
	if (++ap >= &argv[10])
	  break;

    /* The first value is always the file size */
    t->file_size = (uint64_t)strtoll(argv[0],NULL,10);
    //    print_status("Size: %"PRIu64, sz);

    

    int i = 1;
    while (s->hash_order[i] != alg_unknown)
      {
	t->hash[s->hash_order[i]] = strdup(argv[i]);
	i++;
      }

    /* RBF - We must convert this value to a TCHAR */
    t->file_name = strdup(argv[i]);
    
    /*
    fprintf(stdout,"Filename: ");
    display_filename(stdout,t->file_name);
    fprintf(stdout,"%s",NEWLINE);
    for (i = 0 ; i < NUM_ALGORITHMS ; ++i)
      print_status("%s: %s", s->hashes[i]->name, t->hash[i]);
    print_status("");
    */

    add_hash(s,t);    

    free(tmp);
  }

  free(buf);
  return FALSE;
}



status_t load_match_file(state *s, char *fn)
{
  status_t status = status_ok;
  FILE * handle;
  filetype_t type;

  if (NULL == s || NULL == fn)
    return status_unknown_error;

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

  if (read_file(s,fn,handle))
    status = status_file_error;
  
  fclose(handle);
  return status;
}

  /*
  add_RBF_hash(s,
	       _TEXT("/dev/null"),
	       "d41d8cd98f00b204e9800998ecf8427e",
	       "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
	       0);

  add_RBF_hash(s,
#ifdef __LINUX__
	       _TEXT("/home/jdoe/md5deep/svn/trunk/hashdeep/abc"),
#else
	       _TEXT("/Users/jessek/projects/md5deep/hashdeep/abc"),
#endif
	       "900150983cd24fb0d6963f7d28e17f72",
	       "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
	       3);
  */


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


status_t display_match_neg(state *s)
{
  hashtable_entry_t * ret, * tmp;
  int should_display = TRUE;
  hashname_t i;
  uint64_t my_round;
 
  if (NULL == s)
    return status_unknown_error;

  my_round = s->hash_round;
  s->hash_round++;
  if (my_round > s->hash_round)
    fatal_error(s,"%s: Too many input files", __progname);

  for (i = 0 ; i < NUM_ALGORITHMS; ++i)
  {
    if (s->hashes[i]->inuse)
    {
      ret = hashtable_contains(s,i);
      tmp = ret;
      while (tmp != NULL)
      {
	switch (tmp->status) {

	  /* If only the name is different, it's still really a match
	     as far as we're concerned. */
	case status_file_name_mismatch:
	case status_match:
	  should_display = FALSE;
	  break;
	  
	case status_file_size_mismatch:
	case status_partial_match:

	  if (tmp->data->used != s->hash_round)
	  {
	    tmp->data->used = s->hash_round;
	    display_filename(stderr,s->current_file->file_name);
	    fprintf(stderr,": WARNING: Partial match ");
	    display_filename(stderr,tmp->data->file_name);
	    fprintf(stderr,"%s", NEWLINE);

	    /* Technically this wasn't a match, so we're still ok
	       to display this result at the end as not matching */
	    break;
	  }
	    
	default:
	  break;
	}
      
	tmp = tmp->next;
      }

      hashtable_destroy(ret);
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

  return status_ok;
}



status_t display_match_result(state *s)
{
  hashtable_entry_t *ret , *tmp;
  TCHAR * matched_filename = NULL;
  int should_display; 
  hashname_t i;
  uint64_t my_round;

  if (NULL == s)
    fatal_error(s,"%s: NULL state in display_match_result", __progname);

  my_round = s->hash_round;
  s->hash_round++;
  if (my_round > s->hash_round)
      fatal_error(s,"%s: Too many input files", __progname);

  should_display = (primary_match_neg == s->primary_function);

  for (i = 0 ; i < NUM_ALGORITHMS; ++i)
  {
    if (s->hashes[i]->inuse)
    {
      ret = hashtable_contains(s,i);
      tmp = ret;
      while (tmp != NULL)
      {
	switch (tmp->status) {

	  /* If only the name is different, it's still really a match
	     as far as we're concerned. */
	case status_file_name_mismatch:
	case status_match:
	  matched_filename = tmp->data->file_name;
	  should_display = (primary_match_neg != s->primary_function);
	  break;
	  
	case status_file_size_mismatch:
	case status_partial_match:

	  /* We only want to display a partial hash error once per input file */
	  if (tmp->data->used != s->hash_round)
	  {
	    tmp->data->used = s->hash_round;
	    display_filename(stderr,s->current_file->file_name);
	    fprintf(stderr,": WARNING: Partial match ");
	    display_filename(stderr,tmp->data->file_name);
	    fprintf(stderr,"%s", NEWLINE);

	    /* Technically this wasn't a match, so we're still ok
	       with the match result we already have */
	    break;
	  }
	
	case status_unknown_error:
	  return status_unknown_error;

	default:
	  break;
	}
      
	tmp = tmp->next;
      }

      hashtable_destroy(ret);
    }
  }

  /* RBF - Can this code be abstracted? Should it be? */
  if (should_display)
  {
    if (s->mode & mode_display_hash)
      display_hash_simple(s);
    else
    {
      display_filename(stdout,s->current_file->file_name);
      if (s->mode & mode_which && primary_match == s->primary_function)
      {
	fprintf(stdout," matches ");
	if (NULL == matched_filename)
	  fprintf(stdout,"(unknown file)");
	else
	  display_filename(stdout,matched_filename);
      }
      print_status("");
    }
  }
  
  return status_ok;
}
