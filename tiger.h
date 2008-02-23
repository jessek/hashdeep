
/* MD5DEEP - tiger.h
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

/* $Id: tiger.h,v 1.3 2007/09/23 01:54:29 jessekornblum Exp $ */

#include "main.h"

/*
#ifdef HASH_ALGORITHM
#error Hash algorithm already defined!
#else
#define HASH_ALGORITHM     "Tiger"
#endif

 Bytes in hash 
#define HASH_LENGTH        24

 Characters needed to display hash. This is *usually* twice the
   HASH_LENGTH defined above. This number is used to find and compare
   hashes as part of the matching process. 
#define HASH_STRING_LENGTH 48

 These supply the hashing code, hash.c, with the names of the 
   functions used in the algorithm to do the real computation. 
   HASH_Init takes a HASH_CONTEXT only
   HASH_Update takes a hash context, the buffer, and its size in bytes
  HASH_Final takes a char to put the sum in and then a context 
#define HASH_CONTEXT         TIGER_CONTEXT
#define HASH_Init(A)         tiger_init(A);
#define HASH_Update(A,B,C)   tiger_update(A,B,C);
#define HASH_Final(A,B)      tiger_final(A,B);


 Define which types of files of known hashes that support this type
   of hash. The values, if given, are the location of the hash value
   in terms of number of commas that preceed the hash. For example,
   if the file format is:
   hash,stuff,junk
   the define should be SUPPORT_FORMAT 0 
   if the file format is
   stuff,junk,hash
   the define should be SUPPORT_FORMAT 2

   Remember that numbers are not necessary for all file types! 
#define SUPPORT_PLAIN
#define SUPPORT_BSD
#define SUPPORT_MD5DEEP_SIZE

#undef SUPPORT_HASHKEEPER 
#undef SUPPORT_ILOOK
#undef SUPPORT_NSRL_15
#undef SUPPORT_NSRL_20
*/

/* -------------------------------------------------------------- */
/* After this is the algorithm itself. You shouldn't change these */

//typedef uint64_t u64;
//typedef uint32_t u32;
//typedef unsigned char byte;

#define TIGER_BLOCKSIZE 64
#define TIGER_HASHSIZE 24

typedef struct {
    uint64_t  a, b, c;
    unsigned char buf[64];
    int  count;
    uint32_t  nblocks;
} TIGER_CONTEXT;

extern void tiger_init(TIGER_CONTEXT *hd);
extern void tiger_update(TIGER_CONTEXT *hd, unsigned char *inbuf, size_t inlen);
extern void tiger_final(unsigned char hash[24], TIGER_CONTEXT *hd);

int hash_init_tiger(state *s);
int hash_update_tiger(state *s, unsigned char  *buf, uint64_t len);
int hash_final_tiger(state *s, unsigned char *sum);

#define context_tiger_t TIGER_CONTEXT
