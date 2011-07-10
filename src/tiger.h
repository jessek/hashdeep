
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

/* $Id$ */

#ifndef __TIGER_H
#define __TIGER_H

#include "common.h"

__BEGIN_DECLS


#define TIGER_BLOCKSIZE 64
#define TIGER_HASHSIZE 24

typedef struct {
    uint64_t  a, b, c;
    unsigned char buf[64];
    int  count;
    uint32_t  nblocks;
} TIGER_CONTEXT;

extern void tiger_init(TIGER_CONTEXT *hd);
extern void tiger_update(TIGER_CONTEXT *hd, const unsigned char *inbuf, size_t inlen);
extern void tiger_final(unsigned char hash[24], TIGER_CONTEXT *hd);

int hash_init_tiger(void * ctx);
int hash_update_tiger(void * ctx, const unsigned char  *buf, size_t len);
int hash_final_tiger(void * ctx, unsigned char *sum);

#define context_tiger_t TIGER_CONTEXT

__END_DECLS

#endif /* ifndef __TIGER_H */
