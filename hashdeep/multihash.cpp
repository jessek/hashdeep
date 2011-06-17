
/* $Id$ */

#include "main.h"

void multihash_initialize(state *s)
{
  for (int i = 0 ; i < NUM_ALGORITHMS ; ++i)    {
      if (s->hashes[i].inuse)	{	  
	  s->current_file->hash_hex[i]="";
	  s->hashes[i].f_init(s->current_file->hash_context[i]);
	}
    }
}


void multihash_update(state *s, unsigned char *buf, uint64_t len)
{
  // We have to copy the data being hashed from the buffer we were
  // passed into another structure because the SHA-1 update 
  // routine modifies it.
  for (int i = 0 ; i < NUM_ALGORITHMS ; ++i)  {
    if (s->hashes[i].inuse)    {
      memcpy(s->current_file->buffer,buf,len);
      s->hashes[i].f_update(s->current_file->hash_context[i],s->current_file->buffer,len);
    }
  }
}


/**
 * multihash_finalizes finalizes each algorithm and converts to hex.
 * Only the hex is preserved.
 */
void multihash_finalize(state *s)
{
    uint16_t j;
    static char hex[] = "0123456789abcdef";
    
    for (int i = 0 ; i < NUM_ALGORITHMS ; ++i) {
	if (s->hashes[i].inuse) {

	    /* Calculate the residue and convert to hex */
	    uint8_t residue[file_data_hasher_t::MAX_ALGORITHM_RESIDUE_SIZE];
	    s->hashes[i].f_finalize(s->current_file->hash_context[i], residue);
	    for (j = 0; j < s->hashes[i].bit_length/8 ; j++) {
		s->current_file->hash_hex[i].push_back(hex[(residue[j] >> 4) & 0xf]);
		s->current_file->hash_hex[i].push_back(hex[residue[j] & 0xf]);
	    }
	}
    }

    if (s->mode & mode_piecewise)
	s->current_file->file_size = s->current_file->bytes_read;
    else
	s->current_file->file_size = s->current_file->actual_bytes;
}



