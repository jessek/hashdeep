
/* $Id: multihash.c,v 1.6 2007/12/14 11:50:28 jessekornblum Exp $ */

#include "main.h"

void multihash_initialize(state *s)
{
  hashname_t i;

  for (i = 0 ; i < NUM_ALGORITHMS ; ++i)
  {
    if (s->hashes[i]->inuse)
    {
      memset(s->hashes[i]->result,0,s->hashes[i]->byte_length);
      s->hashes[i]->f_init(s->hashes[i]->hash_context);
    }
  }
}


void multihash_update(state *s, unsigned char *buf, uint64_t len)
{
  hashname_t i;

  /* We have to copy the data being hashed from the buffer we were
     passed into another structure because the SHA-1 update 
     routine modifies it. */
  /* RBF - Can we optimize multihash_update by only recopying after SHA-1? */

  for (i = 0 ; i < NUM_ALGORITHMS ; ++i)
  {
    if (s->hashes[i]->inuse)
    {
      memcpy(s->buffer,buf,len);
      s->hashes[i]->f_update(s->hashes[i]->hash_context,s->buffer,len);
    }
  }
}


void multihash_finalize(state *s)
{
  hashname_t i;
  uint16_t j, len;
  static char hex[] = "0123456789abcdef";

  for (i = 0 ; i < NUM_ALGORITHMS ; ++i)
  {
    if (s->hashes[i]->inuse)
    {
       s->hashes[i]->f_finalize(s->hashes[i]->hash_context,
			       s->hashes[i]->hash_sum);
       
       len = s->hashes[i]->byte_length / 2;       
       for (j = 0; j < len ; ++j) 
       {
	 s->hashes[i]->result[2 * j] = hex[(s->hashes[i]->hash_sum[j] >> 4) & 0xf];
	 s->hashes[i]->result[2 * j + 1] = hex[s->hashes[i]->hash_sum[j] & 0xf];
       }
       s->hashes[i]->result[s->hashes[i]->byte_length] = 0;
    }
  }
  
  /* RBF - Write filename and filesize into the file_data structure */
}



