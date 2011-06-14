// MD5DEEP
//
// By Jesse Kornblum
//
// This is a work of the US Government. In accordance with 17 USC 105,
// copyright protection is not available for any work of the US Government.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
//

// $Id: main.h,v 1.5 2007/12/08 16:34:50 jessekornblum Exp $
   
#ifndef __MD5DEEP_H
#define __MD5DEEP_H

#include "common.h"
#include "md5deep_hashtable.h"

/* We no longer use this */
void sanity_check(state *s, int condition, const char *msg);

// ----------------------------------------------------------------
// PROGRAM ENGINE
// ---------------------------------------------------------------- 

// Hashing functions 
int hash_file(state *s, TCHAR *file_name);
int hash_stdin(state *s);

// Sets up hashing algorithm and allocates memory 
int setup_hashing_algorithm(state *s);


// ------------------------------------------------------------------
// HASH TABLE
// ------------------------------------------------------------------ 

void hashTableInit(hashTable *knownHashes);

// Adds the string n to the hashTable, along with the filename fn.
// Returns TRUE if an error occured (i.e. Out of memory) 
int hashTableAdd(state *s, hashTable *knownHashes, char *n, char *fn);

// Returns TRUE if the hashTable contains the hash n and stores the
// filename of the known hash in known. Returns FALSE and does not
// alter known if the hashTable does not contain n. This function
// assumes that fn has already been malloc'ed to hold at least 
// PATH_MAX characters 
int hashTableContains(hashTable *knownHashes, char *n, std::string *known);

// Find any hashes that have not been used. If there are any, and display
// is TRUE, prints them to stdout. Regardless of display, then returns
// TRUE. If there are no unused hashes, returns FALSE. 
int hashTableDisplayNotMatched(hashTable *t, int display);

// This function is for debugging 
void hashTableEvaluate(hashTable *knownHashes);




#endif //  ifndef __MD5DEEP_H 



