
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

__BEGIN_DECLS
typedef struct {
  uint32_t total[2];
  uint32_t state[8];
  uint8_t buffer[64];
} context_sha256_t;

void sha256_starts( context_sha256_t *ctx );

void sha256_update( context_sha256_t *ctx, const uint8_t *input, uint32_t length );

void sha256_finish( context_sha256_t *ctx, uint8_t digest[32] );

void hash_init_sha256(void * ctx);
void hash_update_sha256(void * ctx, const unsigned char *buf, size_t len);
void hash_final_sha256(void * ctx, unsigned char *digest);

__END_DECLS

#endif /* sha256.h */
