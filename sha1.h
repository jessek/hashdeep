
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

#include "md5deep.h"

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

#define HASH_CONTEXT       SHA1_CTX
#define HASH_Init(A)       SHA1Init(A)
#define HASH_Update(A,B,C) SHA1Update(A,B,C)
#define HASH_Final(A,B)    SHA1Final(A,B)

/* Options for reading this file. The values, if given, are the offset
   used to find the hash in the file type. They are not necessary for 
   all file types. */
#define SUPPORT_PLAIN

/* Hashkeeper and iLook only have MD5 hashes */
#undef SUPPORT_HASHKEEPER
#undef SUPPORT_ILOOK  

#define SUPPORT_NSRL_15    1
#define SUPPORT_NSRL_20    1 

/* -------------------------------------------------------------- */
/* After this is the algorithm itself. You shouldn't change these */

struct SHA1_Context{
  unsigned long state[5];
  unsigned long count[2];
  unsigned char buffer[64];
};

typedef struct SHA1_Context SHA1_CTX;


void SHA1Transform(unsigned long state[5], unsigned char buffer[64]);
void SHA1Init(SHA1_CTX* context);
void SHA1Update(SHA1_CTX* context, unsigned char* data, unsigned int len);
void SHA1Final(unsigned char digest[20], SHA1_CTX* context);



