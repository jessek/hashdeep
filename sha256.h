
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

#include "md5deep.h"

#ifndef _SHA256_H
#define _SHA256_H

#ifdef HASH_ALGORITHM
#error Hash algorithm already defined!
#else
#define HASH_ALGORITHM     "SHA256"
#endif

/* Bytes in hash */
#define HASH_LENGTH        32

/* Characters needed to display hash. This is *usually* twice the
   HASH_LENGTH defined above. This number is used to find and compare
   hashes as part of the matching process. */
#define HASH_STRING_LENGTH 64

/* Data types used by the hash */
#define HASH_CONTEXT       sha256_context
#define HASH_Init(A)       sha256_starts(A)
#define HASH_Update(A,B,C) sha256_update(A,B,C)
#define HASH_Final(A,B)    sha256_finish(B,A)

/* Define which types of files of known hashes that support this type
   of hash. The values, if given, are the offset,
   in terms of numbers of quotation marks, used to find the hash in the 
   file type. Numbers are not necessary for all file types. */
#define SUPPORT_PLAIN
//#define SUPPORT_HASHKEEPER   
//#define SUPPORT_ILOOK
//#define SUPPORT_NSRL_15      
//#define SUPPORT_NSRL_20      

/* -------------------------------------------------------------- */
/* After this is the algorithm itself. You shouldn't change these */

#ifndef uint8
#define uint8  unsigned char
#endif

#ifndef uint32
#define uint32 unsigned long int
#endif

typedef struct
{
  uint32 total[2];
  uint32 state[8];
  uint8 buffer[64];
}
sha256_context;

void sha256_starts( sha256_context *ctx );
void sha256_update( sha256_context *ctx, uint8 *input, uint32 length );
void sha256_finish( sha256_context *ctx, uint8 digest[32] );

#endif /* sha256.h */
