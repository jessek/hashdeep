
/* MD5DEEP - sha256.h
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

/* $Id$ */

#ifndef _SHA256_H
#define _SHA256_H

#include "common.h"

/*
#ifdef HASH_ALGORITHM
#error Hash algorithm already defined!
#else
#define HASH_ALGORITHM     "SHA256"
#endif
*/

/* Bytes in hash */
//#define HASH_LENGTH        32

/* Characters needed to display hash. This is *usually* twice the
   HASH_LENGTH defined above. This number is used to find and compare
   hashes as part of the matching process. */
//#define HASH_STRING_LENGTH 64

/* These supply the hashing code, hash.c, with the names of the 
   functions used in the algorithm to do the real computation. 
   HASH_Init takes a HASH_CONTEXT only
   HASH_Update takes a hash context, the buffer, and its size in bytes
   HASH_Final takes a char to put the sum in and then a context */
//#define HASH_CONTEXT       sha256_context
//#define HASH_Init(A)       sha256_starts(A)
//#define HASH_Update(A,B,C) sha256_update(A,B,C)
#define HASH_Final(A,B)    sha256_finish(B,A)

/* Define which types of files of known hashes that support this type
   of hash. The values, if given, are the location of the hash value
   in terms of number of commas that preceed the hash. For example,
   if the file format is:
   hash,stuff,junk
   the define should be SUPPORT_FORMAT 0 
   if the file format is
   stuff,junk,hash
   the define should be SUPPORT_FORMAT 2

   Remember that numbers are not necessary for all file types! */ 
//#define SUPPORT_PLAIN

/* Although there is no sha256 program on BSD at this time, there's
   no reason why these hashes can't be stored in BSD format. */
//#define SUPPORT_BSD
//#define SUPPORT_MD5DEEP_SIZE

//#undef SUPPORT_HASHKEEPER   
//#undef SUPPORT_ILOOK
//#undef SUPPORT_NSRL_15      
//#undef SUPPORT_NSRL_20      


/* -------------------------------------------------------------- */
/* After this is the algorithm itself. You shouldn't change these */

/*
#ifndef uint8
#define uint8  unsigned char
#endif

#ifndef uint32
#define uint32 unsigned long int
#endif
*/

typedef struct {
  uint32_t total[2];
  uint32_t state[8];
  uint8_t buffer[64];
} context_sha256_t;

void sha256_starts( context_sha256_t *ctx );

void sha256_update( context_sha256_t *ctx, uint8_t *input, uint32_t length );

void sha256_finish( context_sha256_t *ctx, uint8_t digest[32] );

int hash_init_sha256(void * ctx);
int hash_update_sha256(void * ctx, unsigned char *buf, uint64_t len);
int hash_final_sha256(void * ctx, unsigned char *digest);


#endif /* sha256.h */
