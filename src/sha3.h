// $Id$

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void hash_init_sha3(void * ctx);
void hash_update_sha3(void * ctx, const unsigned char  *buf, size_t len);
void hash_final_sha3(void * ctx, unsigned char *sum);

#ifdef __cplusplus
}
#endif

