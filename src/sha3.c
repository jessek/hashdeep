// This code was written by Dr. Markku-Juhani O. Saarinen
// who graciously donated it to the public domain.
// See http://www.mjos.fi/dist/readable_keccak.tgz for the 
// original source code.

// Modifications made by Jesse Kornblum.
// $Id$

#include "sha3.h"

#define KECCAK_ROUNDS 24

#define ROTL64(x, y) (((x) << (y)) | ((x) >> (64 - (y))))

static const uint64_t keccakf_rndc[24] = 
  {
    0x0000000000000001, 0x0000000000008082, 0x800000000000808a,
    0x8000000080008000, 0x000000000000808b, 0x0000000080000001,
    0x8000000080008081, 0x8000000000008009, 0x000000000000008a,
    0x0000000000000088, 0x0000000080008009, 0x000000008000000a,
    0x000000008000808b, 0x800000000000008b, 0x8000000000008089,
    0x8000000000008003, 0x8000000000008002, 0x8000000000000080, 
    0x000000000000800a, 0x800000008000000a, 0x8000000080008081,
    0x8000000000008080, 0x0000000080000001, 0x8000000080008008
  };

static const int keccakf_rotc[24] = 
  {
    1,  3,  6,  10, 15, 21, 28, 36, 45, 55, 2,  14, 
    27, 41, 56, 8,  25, 43, 62, 18, 39, 61, 20, 44
  };

static const int keccakf_piln[24] = 
  {
    10, 7,  11, 17, 18, 3, 5,  16, 8,  21, 24, 4, 
    15, 23, 19, 13, 12, 2, 20, 14, 22, 9,  6,  1 
  };

void hash_init_sha3(void * ctx)
{
  sha3_init((sha3_state *)ctx);
}

void hash_update_sha3(void * ctx, const uint8_t * input, size_t length)
{
  sha3_update((sha3_state *)ctx, input, length);
}

void hash_final_sha3(void * ctx, unsigned char * digest)
{
  sha3_final((sha3_state *)ctx, digest);
}


// update the state with given number of rounds
static void keccakf(uint64_t st[25], int rounds)
{
  int i, j, round_num;
  uint64_t t, bc[5];

  for (round_num = 0; round_num < rounds; round_num++) 
  {

    // Theta
    for (i = 0; i < 5; i++) 
      bc[i] = st[i] ^ st[i + 5] ^ st[i + 10] ^ st[i + 15] ^ st[i + 20];
    
    for (i = 0; i < 5; i++) 
    {
      t = bc[(i + 4) % 5] ^ ROTL64(bc[(i + 1) % 5], 1);
      for (j = 0; j < 25; j += 5)
	st[j + i] ^= t;
      }

    // Rho Pi
    t = st[1];
    for (i = 0; i < 24; i++) 
    {
      j = keccakf_piln[i];
      bc[0] = st[j];
      st[j] = ROTL64(t, keccakf_rotc[i]);
      t = bc[0];
    }

    //  Chi
    for (j = 0; j < 25; j += 5) 
    {
      for (i = 0; i < 5; i++)
	bc[i] = st[j + i];
      for (i = 0; i < 5; i++)
	st[j + i] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
    }

    //  Iota
    st[0] ^= keccakf_rndc[round_num];
  }
}

//static void sha3_init(sha3_state *sctx, unsigned int digest_sz)
void sha3_init(sha3_state *sctx)
{
  memset(sctx, 0, sizeof(*sctx));
  sctx->md_len = SHA3_DEFAULT_DIGEST_SIZE;
  sctx->rsiz = 200 - 2 * SHA3_DEFAULT_DIGEST_SIZE;
  sctx->rsizw = sctx->rsiz / 8;
}

/*
static int sha3_224_init(struct shash_desc *desc)
{
  sha3_state *sctx = shash_desc_ctx(desc);
  sha3_init(sctx, SHA3_224_DIGEST_SIZE);
  return 0;
  }

static int sha3_256_init(struct shash_desc *desc)
{
  sha3_state *sctx = shash_desc_ctx(desc);
  sha3_init(sctx, SHA3_256_DIGEST_SIZE);
  return 0;
  }

static int sha3_384_init(struct shash_desc *desc)
{
  sha3_state *sctx = shash_desc_ctx(desc);
  sha3_init(sctx, SHA3_384_DIGEST_SIZE);
  return 0;
  }

static int sha3_512_init(struct shash_desc *desc)
{
  sha3_state *sctx = shash_desc_ctx(desc);
  sha3_init(sctx, SHA3_512_DIGEST_SIZE);
  return 0;
  }
*/


void sha3_update(sha3_state *sctx, const uint8_t *data, unsigned int len)
{
  unsigned int done;
  const uint8_t *src;

  done = 0;
  src = data;

  if ((sctx->partial + len) > (sctx->rsiz - 1)) 
  {
    if (sctx->partial) 
    {
      done = -sctx->partial;
      memcpy(sctx->buf + sctx->partial, data,
	     done + sctx->rsiz);
      src = sctx->buf;
    }

    do {
      unsigned int i;

      for (i = 0; i < sctx->rsizw; i++)
	sctx->st[i] ^= ((uint64_t *) src)[i];
      keccakf(sctx->st, KECCAK_ROUNDS);

      done += sctx->rsiz;
      src = data + done;
    } while (done + (sctx->rsiz - 1) < len);

    sctx->partial = 0;
  }
  memcpy(sctx->buf + sctx->partial, src, len - done);
  sctx->partial += (len - done);
}


void sha3_final(sha3_state *sctx, uint8_t *out)
{
  unsigned int i, inlen = sctx->partial;

  sctx->buf[inlen++] = 1;
  memset(sctx->buf + inlen, 0, sctx->rsiz - inlen);
  sctx->buf[sctx->rsiz - 1] |= 0x80;

  for (i = 0; i < sctx->rsizw; i++)
  sctx->st[i] ^= ((uint64_t *) sctx->buf)[i];

  keccakf(sctx->st, KECCAK_ROUNDS);

  // RBF - On big endian systems, we may need to reverse the bit order here
  // RBF - CONVERT FROM CPU TO LE64
  /*
  for (i = 0; i < sctx->rsizw; i++)
    sctx->st[i] = cpu_to_le64(sctx->st[i]);
  */
  memcpy(out, sctx->st, sctx->md_len);

  memset(sctx, 0, sizeof(*sctx));
}
