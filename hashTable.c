
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

#include "md5deep.h"
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
  unsigned long total = 0, power = 1;
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


int hashTableAdd(hashTable *knownHashes, char *n, char *fn) 
{
  uint64_t key = translate(n);
  hashNode *new, *temp;

  if ((*knownHashes)[key] == NULL) 
  {

    if ((new = (hashNode*)malloc(sizeof(hashNode))) == NULL)
      return TRUE;

    new->data     = strdup(n);
    new->filename = strdup(fn);
    new->next     = NULL;

    (*knownHashes)[key] = new;
    return FALSE;
  }

  temp = (*knownHashes)[key];

  // If this value is already in the table, we don't need to add it again
  if (STRINGS_EQUAL(temp->data,n))
    return FALSE;
  
  while (temp->next != NULL)
  {
    temp = temp->next;
    
    if (STRINGS_EQUAL(temp->data,n))
      return FALSE;
  }
  
  if ((new = (hashNode*)malloc(sizeof(hashNode))) == NULL)
    return TRUE;
  
  new->data     = strdup(n);
  new->filename = strdup(fn);
  new->next     = NULL;

  temp->next = new;
  return FALSE;
}


int hashTableContains(hashTable *knownHashes, char *n, char *known) 
{
  unsigned long key = translate(n);
  hashNode *temp;

  if ((*knownHashes)[key] == NULL)
    return FALSE;

  /* Just because we matched keys doesn't mean we've found a hit yet.
     We still have to verify that we've found the real key. */
  temp = (*knownHashes)[key];

  do {
    if (STRINGS_EQUAL(temp->data,n))
    {
      strncpy(known,temp->filename,PATH_MAX);
      return TRUE;
    }

    temp = temp->next;
  }  while (temp != NULL);

  return FALSE;
}


#ifdef __DEBUG
/* Displays the number of nodes at each depth of the hash table. */
void hashTableEvaluate(hashTable *knownHashes) {

  hashNode *ptr;
  int temp = 0;
  unsigned long count, depth[10];

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
    printf ("Depth: %d  count: %ld\n",temp,count);
    return;
  }

  printf ("Hash table depth chart:\n");
  for (count = 0; count < 10 ; count++) {
    printf ("%ld: %ld\n", count,depth[count]);
  }
}
#endif    
  
