
/* MD5DEEP - sha1.h
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

#ifndef __SHA1_H
#define __SHA1_H

#include "common.h"

__BEGIN_DECLS

struct SHA1_Context{
  uint32_t state[5];
  uint32_t count[2];
  unsigned char buffer[64];
};

typedef struct SHA1_Context SHA1_CTX;

typedef SHA1_CTX context_sha1_t;

void SHA1Transform(uint32_t state[5], const unsigned char buffer[64]); 
void SHA1Init(SHA1_CTX* context);

void SHA1Update(SHA1_CTX* context, const unsigned char * data, size_t len);
void SHA1Final(unsigned char digest[20], SHA1_CTX* context);

int hash_init_sha1(void *ctx);
int hash_update_sha1(void *ctx, const unsigned char *buf, size_t len);
int hash_final_sha1(void *ctx, unsigned char *digest);


__END_DECLS

#endif   /* ifndef __SHA1_H */
