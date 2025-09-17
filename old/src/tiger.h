
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



__BEGIN_DECLS

void hash_init_tiger(void * ctx);
void hash_update_tiger(void * ctx, const unsigned char  *buf, size_t len);
void hash_final_tiger(void * ctx, unsigned char *sum);

#define context_tiger_t TIGER_CONTEXT

__END_DECLS

#endif /* ifndef __TIGER_H */
