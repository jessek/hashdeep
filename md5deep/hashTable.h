
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

/* $Id: hashTable.h,v 1.1 2007/12/07 04:20:00 jessekornblum Exp $ */

/* We keep all of the hash table manipulation functions here as an 
   abstraction barrier. They're not in main.h so that the main 
   program can't call them. */

#ifndef __HASHTABLE
#define __HASHTABLE


/* The HASH_TABLE_SIZE must be more than 16 to the power of HASH_SIG_FIGS */
#define HASH_SIG_FIGS           5
#define HASH_TABLE_SIZE   1048577

/* We're making the decision NOT to hold known filenames with Unicode
   characters in their names. We can't print them anyway, so it's 
   unlikely that we'll end up with them in files of known hashes. */
typedef struct hashNode {
  int been_matched;
  char  *data;
  char  *filename;
  struct hashNode *next;
} hashNode;

typedef hashNode *hashTable[HASH_TABLE_SIZE + 1];

#define HASHTABLE_OK            0
#define HASHTABLE_INVALID_HASH  1
#define HASHTABLE_OUT_OF_MEMORY 2

#endif  /* #ifdef __HASHTABLE */
