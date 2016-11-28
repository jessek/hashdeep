
/* $Id$ */

#ifndef __NULL_H
#define __NULL_H

__BEGIN_DECLS

#define context_null_t ""

void hash_init_whirlpool(void * ctx);
void hash_update_whirlpool(void * ctx, const unsigned char *buf, size_t len);
void hash_final_whirlpool(void * ctx, unsigned char *digest);

__END_DECLS

#endif   /* __NULL_H */
