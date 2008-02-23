
/* $Id: whirlpooldeep.c,v 1.1 2007/12/07 04:20:10 jessekornblum Exp $ */

#include "main.h"
#include "whirlpool.h"

int setup_hashing_algorithm(state *s)
{
    s->hash_length        = 64;
    s->hash_init          = hash_init_whirlpool;
    s->hash_update        = hash_update_whirlpool;
    s->hash_finalize      = hash_final_whirlpool;
    
    s->h_plain = s->h_bsd = s->h_md5deep_size = 1;
    s->h_ilook = 0;
    s->h_hashkeeper = 0;
    s->h_nsrl15 = 0;
    s->h_nsrl20 = 0;
    s->h_encase = 0;

    s->hash_context = (context_whirlpool_t *)malloc(sizeof(context_whirlpool_t));
    if (NULL == s->hash_context)
      return TRUE;
    
    return FALSE;
}
