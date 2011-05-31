
/* $Id$ */

//#include "main.h"
//#include "md5.h"

#include "md5deep.h"

int setup_hashing_algorithm(state *s)
{
  s->hash_length        = 16;
  s->hash_init          = hash_init_md5;
  s->hash_update        = hash_update_md5;
  s->hash_finalize      = hash_final_md5;
  
  s->h_plain = s->h_bsd = s->h_ilook = s->h_md5deep_size = 1;
  s->h_ilook3 = s->h_ilook4 = 1;
  s->h_hashkeeper = 5;
  s->h_nsrl15 = 7;
  s->h_nsrl20 = 2;
  s->h_encase = 1;
  
  s->hash_context = (context_md5_t *)malloc(sizeof(context_md5_t));
  if (NULL == s->hash_context)
    return TRUE;
  
  return FALSE;
}
