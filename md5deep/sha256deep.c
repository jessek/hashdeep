
/* $Id$ */

#include "main.h"
#include "sha256.h"

int setup_hashing_algorithm(state *s)
{
  s->hash_length        = 32;
  s->hash_init          = hash_init_sha256;
  s->hash_update        = hash_update_sha256;
  s->hash_finalize      = hash_final_sha256;
  
  s->h_plain = s->h_bsd = s->h_md5deep_size = 1;
  s->h_ilook3 = 6;
  s->h_hashkeeper = 0;
  s->h_nsrl15 = 0;
  s->h_nsrl20 = 0;
  s->h_ilook = 0;
  s->h_encase = 0;
    
  s->hash_context = (context_sha256_t *)malloc(sizeof(context_sha256_t));
  if (NULL == s->hash_context)
    return TRUE;
  
  return FALSE;
}
