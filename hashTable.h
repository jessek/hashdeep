
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
  char *data, *filename;
  struct hashNode *next;
} hashNode;

typedef hashNode *hashTable[HASH_TABLE_SIZE + 1];


/* --- Everything below this line is public --- */

void hashTableInit(hashTable *knownHashes);

/* Adds the string n to the hashTable, along with the filename fn.
   Returns TRUE if an error occured (i.e. Out of memory) */
int hashTableAdd(hashTable *knownHashes, char *n, char *fn);

/* Returns TRUE if the hashTable contains the hash n and stores the
   filename of the known hash in known. Returns FALSE and does not
   alter known if the hashTable does not contain n. This function
   assumes that fn has already been malloc'ed to hold at least 
   PATH_MAX characters */
int hashTableContains(hashTable *knownHashes, char *n, char *known);

/* This function is for debugging */
void hashTableEvaluate(hashTable *knownHashes);




#endif  /* #ifdef __HASHTABLE */
