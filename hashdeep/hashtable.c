
#include "main.h"

/* $Id$ */

status_t file_data_compare(state *s, file_data_t *a, file_data_t *b)
{
  int partial_null = FALSE, partial_match = FALSE, partial_failure = FALSE;
  hashname_t i;
  
  if (NULL == a || NULL == b || NULL == s)
    return status_unknown_error;  
  
  /* We first compare the hashes because they should tell us the fastest if we're
     looking at different files. Then the file size and finally the file name. */
  for (i = 0 ; i < NUM_ALGORITHMS ; ++i)
  {
    if (s->hashes[i]->inuse)
    {
      /* We have to avoid calling STRINGS_EQUAL on NULL values, but
         don't have to worry about capitalization */
      if (NULL == a->hash[i] || NULL == b->hash[i])
	partial_null = TRUE;
      else if (STRINGS_CASE_EQUAL(a->hash[i],b->hash[i]))
	partial_match = TRUE;
      else
	partial_failure = TRUE;
    }
  }

  /* Check for when there are no intersecting hashes */
  if (!partial_match && !partial_failure)
    return status_no_match;

  if (partial_failure)
  {
    if (partial_match)
      return status_partial_match;
    else
      return status_no_match;
  }
  
  if (a->file_size != b->file_size)
    return status_file_size_mismatch;

  /* We can't compare something that's NULL */
  if (NULL == a->file_name || NULL == b->file_name)
    return status_file_name_mismatch;
  if (!(WSTRINGS_EQUAL(a->file_name,b->file_name)))
    return status_file_name_mismatch;
  
  return status_match;
}


/* These two functions are the "hash" functions for the hash table. 
   Because the original implementation of this code was for storing
   md5 hashes, I used the name "translate" to avoid confusion. */


/* Convert a single hexadecimal character to decimal. If c is not a valid
   hex character, returns 0. */
static int translate_char(char c) 
{
  /* If this is a digit */
  if (c > 47 && c < 58) 
    return (c - 48);
  
  c = toupper(c);
  /* If this is a letter... 'A' should be equal to 10 */
  if (c > 64 && c < 71) 
    return (c - 55);

  return 0;
}
    
/* Translates a hex value into it's appropriate index in the array.
   In reality, this just turns the first HASH_SIG_FIGS into decimal */
static uint64_t translate(char *n) 
{ 
  int count;
  uint64_t total = 0, power = 1;

  if (NULL == n)
    internal_error("%s: translate called on NULL string", __progname);

  for (count = HASH_TABLE_SIG_FIGS - 1 ; count >= 0 ; count--) 
  {
    total += translate_char(n[count]) * power;
    power *= 16;
  }

  return total;
}



void hashtable_init(hashtable_t *t)
{
  uint64_t i;
  for (i = 0 ; i < HASH_TABLE_SIZE ; ++i)
    t->member[i] = NULL;
}



status_t hashtable_add(state *s, hashname_t alg, file_data_t *f)
{
  hashtable_entry_t *new, *temp;
  hashtable_t *t = s->hashes[alg]->known;

  if (NULL == t || NULL == f)
    return status_unknown_error;
  
  uint64_t key = translate(f->hash[alg]);
  
  if (NULL == t->member[key])
  {
    new = (hashtable_entry_t *)malloc(sizeof(hashtable_entry_t));
    if (NULL == new)
      return status_out_of_memory;
    
    new->next = NULL;
    new->data = f;
    
    t->member[key] = new;
      return status_ok;
  }

  temp = t->member[key];

  /* If this value is already in the table, we don't need to add it again */
  if (file_data_compare(s,temp->data,f) == status_match)
    return status_ok;

  while (temp->next != NULL)
  {
    temp = temp->next;
    
    if (file_data_compare(s,temp->data,f) == status_match)
      return status_ok;
  }

  new = (hashtable_entry_t *)malloc(sizeof(hashtable_entry_t));
  if (NULL == new)
    return status_out_of_memory;
  
  new->next = NULL;
  new->data = f;
  
  temp->next = new;
  return status_ok;
}



void hashtable_destroy(hashtable_entry_t *e)
{
  hashtable_entry_t *tmp;

  if (NULL == e)
    return;
  
 while (e != NULL)
  {
    tmp = e->next;
    free(e);
    e = tmp;
  }
}


hashtable_entry_t * 
hashtable_contains(state *s, hashname_t alg)
{
  hashtable_entry_t *ret = NULL, *new, *temp, *last = NULL;
  hashtable_t *t; 
  uint64_t key;
  file_data_t * f;
  status_t status;

  if (NULL == s)
    internal_error("%s: state is NULL in hashtable_contains", __progname);
  
  f = s->current_file;
  if (NULL == f)
    internal_error("%s: current_file is in hashtable_contains", __progname);

  key = translate(f->hash[alg]);
  t   = s->hashes[alg]->known;

  //  print_status("key: %"PRIx64, key);

  if (NULL == t->member[key])
    return NULL;

  //  print_status("Found one or more possible hits.");
  
  temp = t->member[key];

  status = file_data_compare(s,temp->data,f);
  //  print_status("First entry %d", status);
  if (status != status_no_match)
    {
      //  print_status("hit on first entry %d", status);
      ret = (hashtable_entry_t *)malloc(sizeof(hashtable_entry_t));
      ret->next = NULL;
      ret->status = status;
      ret->data = temp->data;
      last = ret;
    }

  while (temp->next != NULL)
    {
      temp = temp->next;
    
      status = file_data_compare(s,temp->data,f);
      
      if (status != status_no_match)
	{
	  if (NULL == ret)
	    {
	      ret = (hashtable_entry_t *)malloc(sizeof(hashtable_entry_t));
	      ret->next = NULL;
	      ret->data = temp->data;
	      ret->status = status;
	      last = ret;
	    }
	  else
	    {
	      new = (hashtable_entry_t *)malloc(sizeof(hashtable_entry_t));
	      new->next = NULL;
	      new->data = temp->data;
	      new->status = status;
	      last->next = new;
	      last = new;
	    }
	}
    }
  
  return ret;
}
