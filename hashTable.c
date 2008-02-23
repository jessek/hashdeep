
/* MD5DEEP - hashTable.c
 *
 * By Jesse Kornblum
 *
 * This is a work of the US Government. In accordance with 17 USC 105,
 * copyright protection is not available for any work of the US Government.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

// $Id: hashTable.c,v 1.10 2007/09/23 01:54:23 jessekornblum Exp $

#include "main.h"
#include "hashTable.h"


/* These two functions are the "hash" functions for the hash table. 
   Because the original implementation of this code was for storing
   md5 hashes, I used the name "translate" to avoid confusion. */


/* Convert a single hexadecimal character to decimal. If c is not a valid
   hex character, returns 0. */
int translateChar(char c) 
{
  // If this is a digit
  if (c > 47 && c < 58) 
    return (c - 48);
  
  c = toupper(c);
  // If this is a letter... 'A' should be equal to 10
  if (c > 64 && c < 71) 
    return (c - 55);

  return 0;
}
    
/* Translates a hex value into it's appropriate index in the array.
   In reality, this just turns the first HASH_SIG_FIGS into decimal */
uint64_t translate(char *n) 
{ 
  int count;
  uint64_t total = 0, power = 1;
  for (count = HASH_SIG_FIGS - 1 ; count >= 0 ; count--) {
    total += translateChar(n[count]) * power;
    power *= 16;
  }

  return total;
}


/* ---------------------------------------------------------------------- */


void hashTableInit(hashTable *knownHashes) 
{
  uint64_t count;
  for (count = 0 ; count < HASH_TABLE_SIZE ; ++count) 
    (*knownHashes)[count] = NULL;
}

int initialize_node(hashNode *new, char *n, char *fn)
{
  new->been_matched = FALSE;
  new->data         = strdup(n);
  new->filename     = strdup(fn);
  new->next         = NULL;

  if (new->data == NULL || new->filename == NULL)
    return TRUE;
  return FALSE;
}


int hashTableAdd(state *s, hashTable *knownHashes, char *n, char *fn) 
{
  uint64_t key = translate(n);
  hashNode *new, *temp;

  if (!valid_hash(s,n))
    return HASHTABLE_INVALID_HASH;

  if ((*knownHashes)[key] == NULL) 
  {

    if ((new = (hashNode*)malloc(sizeof(hashNode))) == NULL)
      return HASHTABLE_OUT_OF_MEMORY;

    if (initialize_node(new,n,fn))
      return HASHTABLE_OUT_OF_MEMORY;

    (*knownHashes)[key] = new;
    return HASHTABLE_OK;
  }

  temp = (*knownHashes)[key];

  // If this value is already in the table, we don't need to add it again
  if (STRINGS_EQUAL(temp->data,n))
    return HASHTABLE_OK;;
  
  while (temp->next != NULL)
  {
    temp = temp->next;
    
    if (STRINGS_EQUAL(temp->data,n))
      return HASHTABLE_OK;
  }
  
  if ((new = (hashNode*)malloc(sizeof(hashNode))) == NULL)
    return HASHTABLE_OUT_OF_MEMORY;

  if (initialize_node(new,n,fn))
    return HASHTABLE_OUT_OF_MEMORY;

  temp->next = new;
  return FALSE;
}


int hashTableContains(hashTable *knownHashes, char *n, char *known) 
{
  uint64_t key = translate(n);
  hashNode *temp;

  if ((*knownHashes)[key] == NULL)
    return FALSE;

  /* Just because we matched keys doesn't mean we've found a hit yet.
     We still have to verify that we've found the real key. */
  temp = (*knownHashes)[key];

  do {
    if (STRINGS_EQUAL(temp->data,n))
    {
      temp->been_matched = TRUE;
      if (known != NULL)
	strncpy(known,temp->filename,PATH_MAX);
      return TRUE;
    }

    temp = temp->next;
  }  while (temp != NULL);

  return FALSE;
}


int hashTableDisplayNotMatched(hashTable *t, int display)
{
  int status = FALSE;
  uint64_t key;
  hashNode *temp;

  for (key = 0; key < HASH_TABLE_SIZE ; ++key)
  {
    if ((*t)[key] == NULL)
      continue;

    temp = (*t)[key];
    while (temp != NULL)
    {
      if (!(temp->been_matched))
      {
	if (!display)
	  return TRUE;

	// The 'return' above allows us to disregard the if statement.
	status = TRUE;
	
	printf ("%s%s", temp->filename, NEWLINE);
      }
      temp = temp->next;
    }
  } 

  return status;
}  



#ifdef __DEBUG
/* Displays the number of nodes at each depth of the hash table. */
void hashTableEvaluate(hashTable *knownHashes) {

  hashNode *ptr;
  int temp = 0;
  uint64_t count, depth[10];

  for (count = 0 ; count < 10 ; count++) {
    depth[count] = 0;
  }

  for (count = 0 ; count < HASH_TABLE_SIZE ; count++) {

    if (knownHashes[count] == NULL) {
      depth[0]++;
      continue;
    }

    temp = 0;
    ptr = (*knownHashes)[count];
    while (ptr != NULL) {
      temp++;
      ptr = ptr->next;
    }

    depth[temp]++;
  }
  
  if (temp > 10) {
    print_status ("Depth: %d  count: %"PRIu64, temp,count);
    return;
  }

  print_status ("Hash table depth chart:");
  for (count = 0; count < 10 ; count++) {
    print_status ("%"PRIu64": %"PRIu64, count,depth[count]);
  }
}
#endif    
  
