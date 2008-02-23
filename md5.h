
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

#include "md5deep.h"

#ifdef HASH_ALGORITHM
#error Hash algorithm already defined!
#else
#define HASH_ALGORITHM     "MD5"
#endif

/* Bytes in hash */
#define HASH_LENGTH        16

/* Characters needed to display hash. This is *usually* twice the
   HASH_LENGTH defined above. This number is used to find and compare
   hashes as part of the matching process. */
#define HASH_STRING_LENGTH 32

/* Data types used by the hash */
#define HASH_CONTEXT       MD5_CTX
#define HASH_Init(A)       MD5Init(A)
#define HASH_Update(A,B,C) MD5Update(A,B,C)
#define HASH_Final(A,B)    MD5Final(A,B)


/* Define which types of files of known hashes that support this type
   of hash. The values, if given, are the offset,
   in terms of numbers of quotation marks, used to find the hash in the 
   file type. Numbers are not necessary for all file types. */
#define SUPPORT_PLAIN
#define SUPPORT_HASHKEEPER   3
#define SUPPORT_ILOOK
#define SUPPORT_NSRL_15      9
#define SUPPORT_NSRL_20      3

/* -------------------------------------------------------------- */
/* After this is the algorithm itself. You shouldn't change these */

struct MD5Context {
	u_int32_t buf[4];
	u_int32_t bits[2];
	unsigned char in[64];
};

/* This is needed to make RSAREF happy on some MS-DOS compilers */
typedef struct MD5Context MD5_CTX;

void MD5Init(struct MD5Context *context);
void MD5Update(struct MD5Context *context, unsigned char const *buf,
	       unsigned len);
void MD5Final(unsigned char digest[16], struct MD5Context *context);
void MD5Transform(u_int32_t buf[4], u_int32_t const in[16]);

