
/* $Id$ */

#include "main.h"
#include "sha1.h"

int setup_hashing_algorithm(state *s)
{
  s->hash_length        = 20;
  s->hash_init          = hash_init_sha1;
  s->hash_update        = hash_update_sha1;
  s->hash_finalize      = hash_final_sha1;
  
  s->h_plain = s->h_bsd = s->h_md5deep_size = 1;      
  s->h_ilook3 = 2;
  s->h_ilook = s->h_hashkeeper = 0;
  s->h_nsrl15 = 1;
  s->h_nsrl20 = 1;
  s->h_encase = 0;
  
  s->hash_context = (context_sha1_t *)malloc(sizeof(context_sha1_t));
  if (NULL == s->hash_context)
    return TRUE;
  
  return FALSE;
}
