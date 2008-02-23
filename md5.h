
/* MD5DEEP - md5.h
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

/* $Id: md5.h,v 1.7 2007/09/23 01:54:26 jessekornblum Exp $ */

#include "main.h"

/*
#ifdef HASH_ALGORITHM
#error Hash algorithm already defined!
#else
#define HASH_ALGORITHM     "MD5"
#endif
*/

/* Bytes in hash */
//#define HASH_LENGTH        16

/* Characters needed to display hash. This is *usually* twice the
   HASH_LENGTH defined above. This number is used to find and compare
   hashes as part of the matching process. */
//#define HASH_STRING_LENGTH 32

/* These supply the hashing code, hash.c, with the names of the 
   functions used in the algorithm to do the real computation. 
   HASH_Init takes a HASH_CONTEXT only
   HASH_Update takes a hash context, the buffer, and its size in bytes
   HASH_Final takes a char to put the sum in and then a context */
/*
#define HASH_CONTEXT       MD5_CTX
#define HASH_Init(A)       MD5Init(A)
#define HASH_Update(A,B,C) MD5Update(A,B,C)
#define HASH_Final(A,B)    MD5Final(A,B)
*/

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

/*
#define SUPPORT_PLAIN
#define SUPPORT_BSD
#define SUPPORT_HASHKEEPER   4
#define SUPPORT_ILOOK
#define SUPPORT_NSRL_15      6
#define SUPPORT_NSRL_20      1
#define SUPPORT_MD5DEEP_SIZE
*/
/* -------------------------------------------------------------- */
/* After this is the algorithm itself. You shouldn't change these */

typedef struct _context_md5_t {
  uint32_t buf[4];
  uint32_t bits[2];
  unsigned char in[64];
} context_md5_t;

/* This is needed to make RSAREF happy on some MS-DOS compilers */
typedef context_md5_t MD5_CTX;

int hash_init_md5(state *s);
int hash_update_md5(state *s, unsigned char *buf, uint64_t len);
int hash_final_md5(state *s, unsigned char *digest);

void MD5Init(context_md5_t *ctx);
void MD5Update(context_md5_t *context, unsigned char *buf, unsigned len);
void MD5Final(unsigned char digest[16], context_md5_t *context);
void MD5Transform(uint32_t buf[4], uint32_t const in[16]);

