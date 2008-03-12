
/* $Id$ */

#include "main.h"

static int parse_hashing_algorithms(state *s, char *fn, char *val)
{
  uint8_t i;
  int done = FALSE;
  char **ap, *buf[MAX_KNOWN_COLUMNS];
  /* The first position is always the file size, so we start with an 
     the first position of one. */
  uint8_t order = 1;
  
  if (NULL == s || NULL == fn || NULL == val)
    return TRUE;

  for (ap = buf ; (*ap = strsep(&val,",")) != NULL ; )
    if (*ap != '\0')
      if (++ap >= &buf[MAX_KNOWN_COLUMNS])
	break;

  for (i = 0 ; i < MAX_KNOWN_COLUMNS && buf[i] != NULL ; i++)
  {
    if (STRINGS_EQUAL(buf[i],"md5"))
    {
      s->hashes[alg_md5]->inuse = TRUE;
      s->hash_order[order] = alg_md5;
    }
    
    else if (STRINGS_EQUAL(buf[i],"sha1") || 
	     STRINGS_EQUAL(buf[i],"sha-1"))
    {
      s->hashes[alg_sha1]->inuse = TRUE;
      s->hash_order[order] = alg_sha1;
    }
    
    else if (STRINGS_EQUAL(buf[i],"sha256") || 
	     STRINGS_EQUAL(buf[i],"sha-256"))
    {
      s->hashes[alg_sha256]->inuse = TRUE;
      s->hash_order[order] = alg_sha256;
    }
      
    else if (STRINGS_EQUAL(buf[i],"tiger"))
    {
      s->hashes[alg_tiger]->inuse = TRUE;
      s->hash_order[order] = alg_tiger;
    }
      
    else if (STRINGS_EQUAL(buf[i],"whirlpool"))
    {
      s->hashes[alg_whirlpool]->inuse = TRUE;
      s->hash_order[order] = alg_whirlpool;
    }

    else if (STRINGS_EQUAL(buf[i],"filename"))
    {
      if (buf[i+1] != NULL)
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
    
    ++order;
  }

  s->hash_order[order] = alg_unknown;

  if (done)
    return FALSE;
  return TRUE;
}



static filetype_t 
identify_file(state *s, char *fn, FILE *handle)
{
  hashname_t i;
  hashname_t current_order[NUM_ALGORITHMS];
 
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

  /* We don't use STRINGS_EQUAL here because we only care about
     the first ten characters for right now. */
  if (strncasecmp("%%%% size,",buf,10))
  {
    free(buf);
    return file_unknown;
  }

  /* If this is the first file of hashes being loaded, clear out the 
     list of known values. Otherwise, record the current values to
     let the user know if they have changed when we load the new file. */
  if ( ! s->hashes_loaded )
  {
    clear_algorithms_inuse(s);
  }
  else
  {
    for (i = 0 ; i < NUM_ALGORITHMS ; ++i)
      current_order[i] = s->hash_order[i];
  }

  /* We have to clear out the algorithm order to remove the values
     from the previous file. This file may have different ones */
  for (i = 0 ; i < NUM_ALGORITHMS ; ++i)
    s->hash_order[i] = alg_unknown;

  /* Skip the "%%%% size," when parsing the list of hashes */
  parse_hashing_algorithms(s,fn,buf + 10);

  if (s->hashes_loaded)
  {
    i = 0;
    while (i < NUM_ALGORITHMS && 
	   s->hash_order[i] == current_order[i])
      i++;
    if (i < NUM_ALGORITHMS)
      print_error(s,"%s: %s: Hashes not in same format as previously loaded",
		  __progname, fn);
  }

  free(buf);

  return file_hashdeep_10;
}



static status_t add_file(state *s, file_data_t *f)
{
  status_t st;
  hashname_t i;

  if (NULL == s || NULL == f)
    return status_unknown_error;

  f->next = NULL;

  if (NULL == s->known)
    s->known = f;
  else
    s->last->next = f;

  s->last = f;

  i = 1;
  while (s->hash_order[i] != alg_unknown)
  {
    st = hashtable_add(s,s->hash_order[i],f);
    if (st != status_ok)
      return st;
    ++i;
  }

  return status_ok;

}

static int initialize_file_data(file_data_t *f)
{
  hashname_t i;

  if (NULL == f)
    return TRUE;

  f->next = NULL;
  f->used = 0;

  for (i = 0 ; i < NUM_ALGORITHMS ; ++i)
    f->hash[i] = NULL;

  return FALSE;
}


#ifdef _DEBUG
static void display_file_data(state *s, file_data_t * t)
{
  int i;

  fprintf(stdout,"  Filename: ");
  display_filename(stdout,t->file_name);
  fprintf(stdout,"%s",NEWLINE);

  print_status("      Size: %"PRIu64, t->file_size);

  for (i = 0 ; i < NUM_ALGORITHMS ; ++i)
    print_status("%10s: %s", s->hashes[i]->name, t->hash[i]);
  print_status("");
}

static void display_all_known_files(state *s)
{
  if (NULL == s)
    return;

  if (NULL == s->known)
  {
    print_status("No known hashes");
    return;
  }

  file_data_t * tmp = s->known;
  while (tmp != NULL)
  {
    display_file_data(s,tmp);
    tmp = tmp->next;
  }
}
#endif


int read_file(state *s, char *fn, FILE *handle)
{
  uint8_t i;
  char * buf;
  file_data_t * t;
  uint64_t line_number = 0;
  char **ap, *argv[MAX_KNOWN_COLUMNS];

  if (NULL == s || NULL == fn || NULL == handle)
    return TRUE;

  buf = (char *)malloc(sizeof(char) * MAX_STRING_LENGTH);
  if (NULL == buf)
    fatal_error(s,"%s: Out of memory while trying to read %s", __progname,fn);

  while (fgets(buf,MAX_STRING_LENGTH,handle))
  {
    line_number++;
    chop_line(buf);

    /* Ignore comments in the input file */
    if ('#' == buf[0])
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

    for (ap = argv ; (*ap = strsep(&tmp,",")) != NULL ; )
      if (**ap != '\0')
	if (++ap >= &argv[MAX_KNOWN_COLUMNS])
	  break;

    /* The first value is always the file size */
    t->file_size = (uint64_t)strtoll(argv[0],NULL,10);

    i = 1;
    while (argv[i] != NULL && 
	   s->hash_order[i] != alg_unknown && 
	   i <= NUM_ALGORITHMS)
    {
      t->hash[s->hash_order[i]] = strdup(argv[i]);
      i++;
    }

    /* The last value is always the filename. */
#ifdef _WIN32
    size_t sz = strlen(argv[i]);
    t->file_name = (TCHAR *)malloc(sizeof(TCHAR) * (sz+1));
    if (NULL == t->file_name)
      fatal_error(s,"%s: Out of memory", __progname);

    /* On Windows we must convert the filename from ANSI to Unicode.
       The -1 parameter for the input length asserts that the input
       string, argv[i] is NULL terminated and that the function should
       process the whole thing. The full definition of this function:
       http://msdn2.microsoft.com/en-us/library/ms776413(VS.85).aspx */
    if ( ! MultiByteToWideChar(CP_ACP,
			       MB_PRECOMPOSED,
			       argv[i],
			       -1,   
			       t->file_name,
			       sz+1))
      fatal_error(s,"%s: MultiByteToWideChar failed (%d)", 
		  __progname, 
		  GetLastError()); 
#else
    t->file_name = strdup(argv[i]);
    if (NULL == t->file_name)
      fatal_error(s,"%s: Out of memory while allocating filename %s", 
		  __progname, argv[i]);
#endif
    
    
    if (add_file(s,t))
      return TRUE;
    
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
    return status_unknown_filetype;
  }

  if (read_file(s,fn,handle))
    status = status_file_error;
  
  fclose(handle);


  //  display_all_known_files(s);


  return status;
}


/* We don't use this function anymore, but it's handy to have just in case
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
*/


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
	    fprintf(stderr,": Hash collision with ");
	    display_filename(stderr,tmp->data->file_name);
	    fprintf(stderr,"%s", NEWLINE);

	    /* Technically this wasn't a match, so we're still ok
	       with the match result we already have */

	  }
	  break;
	
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
