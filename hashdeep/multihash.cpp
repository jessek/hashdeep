
/* $Id$ */

#include "main.h"

void multihash_initialize(file_data_hasher_t *fdht)
{
  for (int i = 0 ; i < NUM_ALGORITHMS ; ++i)    {
      if (hashes[i].inuse)	{	  
	  fdht->hash_hex[i]="";
	  hashes[i].f_init(fdht->hash_context[i]);
	}
    }
}


void multihash_update(file_data_hasher_t *fdht, unsigned char *buf, uint64_t len)
{
  // We have to copy the data being hashed from the buffer we were
  // passed into another structure because the SHA-1 update 
  // routine modifies it.
  for (int i = 0 ; i < NUM_ALGORITHMS ; ++i)  {
    if (hashes[i].inuse)    {
	memcpy(fdht->buffer,buf,len);
	hashes[i].f_update(fdht->hash_context[i],fdht->buffer,len);
    }
  }
}


/**
 * multihash_finalizes finalizes each algorithm and converts to hex.
 * Only the hex is preserved.
 */
void multihash_finalize(state *s,file_data_hasher_t *fdht)
{
    uint16_t j;
    static char hex[] = "0123456789abcdef";
    
    for (int i = 0 ; i < NUM_ALGORITHMS ; ++i) {
	if (hashes[i].inuse) {

	    /* Calculate the residue and convert to hex */
	    uint8_t residue[file_data_hasher_t::MAX_ALGORITHM_RESIDUE_SIZE];
	    hashes[i].f_finalize(fdht->hash_context[i], residue);
	    for (j = 0; j < hashes[i].bit_length/8 ; j++) {
		fdht->hash_hex[i].push_back(hex[(residue[j] >> 4) & 0xf]);
		fdht->hash_hex[i].push_back(hex[residue[j] & 0xf]);
	    }
	}
    }

    if (s->mode & mode_piecewise)
	fdht->file_size = fdht->bytes_read;
    else
	fdht->file_size = fdht->actual_bytes;
}



