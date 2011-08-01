
/* $Id$ */

#include "main.h"

void hash_context_obj::multihash_initialize()
{
  for (int i = 0 ; i < NUM_ALGORITHMS ; ++i)    {
      if (hashes[i].inuse) { 
	  hashes[i].f_init(this->hash_context[i]);
	}
    }
}


void hash_context_obj::multihash_update(const unsigned char *buf, size_t len)
{
  // We have to copy the data being hashed from the buffer we were
  // passed into another structure because the SHA-1 update 
  // routine modifies it.
  for (int i = 0 ; i < NUM_ALGORITHMS ; ++i)  {
    if (hashes[i].inuse)    {
	//	memcpy(this->buffer,buf,len);
	//hashes[i].f_update(this->hash_context[i],this->buffer,len);

	// new LTC implementation means no need to copy
	hashes[i].f_update(this->hash_context[i],buf,len);
    }
  }
}


/**
 * multihash_finalizes finalizes each algorithm and converts to hex.
 * Only the hex is preserved.
 */
void hash_context_obj::multihash_finalize(std::string dest[])
{
    uint16_t j;
    static char hex[] = "0123456789abcdef";
    
    for (int i = 0 ; i < NUM_ALGORITHMS ; ++i) {
	dest[i]="";
	if (hashes[i].inuse) {
	    /* Calculate the residue and convert to hex */
	    uint8_t residue[MAX_ALGORITHM_RESIDUE_SIZE];
	    hashes[i].f_finalize(this->hash_context[i], residue);
	    for (j = 0; j < hashes[i].bit_length/8 ; j++) {
		dest[i].push_back(hex[(residue[j] >> 4) & 0xf]);
		dest[i].push_back(hex[residue[j] & 0xf]);
	    }
	}
    }

    // old code
    //this->file_size = this->bytes_read;
}



