
/* MD5DEEP - hashTable.h
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


/* We keep all of the hash table manipulation functions here as an 
   abstraction barrier. They're not in md5deep.h so that the main 
   program can't call them. */

#ifndef __HASHTABLE
#define __HASHTABLE

#include <ctype.h>

/* The HASH_TABLE_SIZE must be more than 16 to the power of HASH_SIG_FIGS */
#define HASH_SIG_FIGS           5
#define HASH_TABLE_SIZE   1048577

typedef struct hashNode {
  char *data;
  struct hashNode *next;
} hashNode;

typedef hashNode *hashTable[HASH_TABLE_SIZE + 1];


/* --- Everything below this line is public --- */

void hashTableInit(hashTable *knownHashes);
void hashTableAdd(hashTable *knownHashes, char *n);
int hashTableContains(hashTable *knownHashes, char *n);

/* This function is for debugging */
void hashTableEvaluate(hashTable *knownHashes);




#endif  /* #ifdef __HASHTABLE */
