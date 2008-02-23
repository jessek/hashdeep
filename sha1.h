
/* MD5DEEP - sha15.h
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

#include "main.h"

/* See md5.h for a descripton of these fields */

#ifdef HASH_ALGORITHM
#error Hash algorithm already defined!
#else
#define HASH_ALGORITHM     "SHA1"
#endif

#define PROGRAM_NAME       "sha1deep"

/* Bytes in hash */
#define HASH_LENGTH        20

/* Characters needed to display hash. This is *usually* twice the
   HASH_LENGTH defined above. This number is used to find and compare
   hashes as part of the matching process. */
#define HASH_STRING_LENGTH 40

/* These supply the hashing code, hash.c, with the names of the 
   functions used in the algorithm to do the real computation. 
   HASH_Init takes a HASH_CONTEXT only
   HASH_Update takes a hash context, the buffer, and its size in bytes
   HASH_Final takes a char to put the sum in and then a context */
#define HASH_CONTEXT       SHA1_CTX
#define HASH_Init(A)       SHA1Init(A)
#define HASH_Update(A,B,C) SHA1Update(A,B,C)
#define HASH_Final(A,B)    SHA1Final(A,B)

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
#define SUPPORT_PLAIN
#define SUPPORT_BSD
#define SUPPORT_MD5DEEP_SIZE

/* Hashkeeper and iLook only have MD5 hashes */
#undef SUPPORT_HASHKEEPER
#undef SUPPORT_ILOOK  

#define SUPPORT_NSRL_15    0
#define SUPPORT_NSRL_20    0 

/* -------------------------------------------------------------- */
/* After this is the algorithm itself. You shouldn't change these */

struct SHA1_Context{
  uint32_t state[5];
  uint32_t count[2];
  unsigned char buffer[64];
};

typedef struct SHA1_Context SHA1_CTX;


void SHA1Transform(uint32_t state[5], unsigned char buffer[64]);
void SHA1Init(SHA1_CTX* context);
void SHA1Update(SHA1_CTX* context, unsigned char* data, unsigned int len);
void SHA1Final(unsigned char digest[20], SHA1_CTX* context);



