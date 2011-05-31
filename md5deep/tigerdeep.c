
/* $Id$ */

#include "md5deep.h"
#include "tiger.h"

int setup_hashing_algorithm(state *s)
{
  s->hash_length        = 24;
  s->hash_init          = hash_init_tiger;
  s->hash_update        = hash_update_tiger;
  s->hash_finalize      = hash_final_tiger;
  
  s->h_plain = s->h_bsd = s->h_md5deep_size = 1;
  s->h_ilook = 0;
  s->h_hashkeeper = 0;
  s->h_nsrl15 = 0;
  s->h_nsrl20 = 0;
  s->h_encase = 0;
  
  s->hash_context = (context_tiger_t *)malloc(sizeof(context_tiger_t));
  if (NULL == s->hash_context)
    return TRUE;
  
  return FALSE;
}
