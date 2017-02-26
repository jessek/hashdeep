/* -------------------------------------------------------------------------
 * Works when compiled for either 32-bit or 64-bit targets, optimized for
 * 64 bit.
 *
 * Canonical implementation of Init/Update/Finalize for SHA-3 byte input.
 *
 * SHA3-256, SHA3-384, SHA-512 are implemented. SHA-224 can easily be added.
 *
 * Based on code from http://keccak.noekeon.org/ .
 *
 * I place the code that I wrote into public domain, free to use.
 *
 * I would appreciate if you give credits to this work if you used it to
 * write or test * your code.
 *
 * Aug 2015. Andrey Jivsov. crypto@brainhub.org
 * ---------------------------------------------------------------------- */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "sha3.h"

#define SHA3_ASSERT( x )
#if defined(_MSC_VER)
#define SHA3_TRACE( format, ...)
#define SHA3_TRACE_BUF( format, buf, l, ...)
#else
#define SHA3_TRACE(format, args...)
#define SHA3_TRACE_BUF(format, buf, l, args...)
#endif

//#define SHA3_USE_KECCAK
/*
 * Define SHA3_USE_KECCAK to run "pure" Keccak, as opposed to SHA3.
 * The tests that this macro enables use the input and output from [Keccak]
 * (see the reference below). The used test vectors aren't correct for SHA3,
 * however, they are helpful to verify the implementation.
 * SHA3_USE_KECCAK only changes one line of code in Finalize.
 */

#if defined(_MSC_VER)
#define SHA3_CONST(x) x
#else
#define SHA3_CONST(x) x##L
#endif

/* The following state definition should normally be in a separate
 * header file
 */

/* 'Words' here refers to uint64_t */
#define SHA3_KECCAK_SPONGE_WORDS \
	(((1600)/8/*bits to byte*/)/sizeof(uint64_t))
typedef struct sha3_context_ {
    uint64_t saved;             /* the portion of the input message that we
                                 * didn't consume yet */
    union {                     /* Keccak's state */
        uint64_t s[SHA3_KECCAK_SPONGE_WORDS];
        uint8_t sb[SHA3_KECCAK_SPONGE_WORDS * 8];
    };
    unsigned byteIndex;         /* 0..7--the next byte after the set one
                                 * (starts from 0; 0--none are buffered) */
    unsigned wordIndex;         /* 0..24--the next word to integrate input
                                 * (starts from 0) */
    unsigned capacityWords;     /* the double size of the hash output in
                                 * words (e.g. 16 for Keccak 512) */
} sha3_context;

#ifndef SHA3_ROTL64
#define SHA3_ROTL64(x, y) \
	(((x) << (y)) | ((x) >> ((sizeof(uint64_t)*8) - (y))))
#endif

static const uint64_t keccakf_rndc[24] = {
    SHA3_CONST(0x0000000000000001UL), SHA3_CONST(0x0000000000008082UL),
    SHA3_CONST(0x800000000000808aUL), SHA3_CONST(0x8000000080008000UL),
    SHA3_CONST(0x000000000000808bUL), SHA3_CONST(0x0000000080000001UL),
    SHA3_CONST(0x8000000080008081UL), SHA3_CONST(0x8000000000008009UL),
    SHA3_CONST(0x000000000000008aUL), SHA3_CONST(0x0000000000000088UL),
    SHA3_CONST(0x0000000080008009UL), SHA3_CONST(0x000000008000000aUL),
    SHA3_CONST(0x000000008000808bUL), SHA3_CONST(0x800000000000008bUL),
    SHA3_CONST(0x8000000000008089UL), SHA3_CONST(0x8000000000008003UL),
    SHA3_CONST(0x8000000000008002UL), SHA3_CONST(0x8000000000000080UL),
    SHA3_CONST(0x000000000000800aUL), SHA3_CONST(0x800000008000000aUL),
    SHA3_CONST(0x8000000080008081UL), SHA3_CONST(0x8000000000008080UL),
    SHA3_CONST(0x0000000080000001UL), SHA3_CONST(0x8000000080008008UL)
};

static const unsigned keccakf_rotc[24] = {
    1, 3, 6, 10, 15, 21, 28, 36, 45, 55, 2, 14, 27, 41, 56, 8, 25, 43, 62,
    18, 39, 61, 20, 44
};

static const unsigned keccakf_piln[24] = {
    10, 7, 11, 17, 18, 3, 5, 16, 8, 21, 24, 4, 15, 23, 19, 13, 12, 2, 20,
    14, 22, 9, 6, 1
};

/* generally called after SHA3_KECCAK_SPONGE_WORDS-ctx->capacityWords words
 * are XORed into the state s
 */
static void
keccakf(uint64_t s[25])
{
    int i, j, round;
    uint64_t t, bc[5];
#define KECCAK_ROUNDS 24

    for(round = 0; round < KECCAK_ROUNDS; round++) {

        /* Theta */
        for(i = 0; i < 5; i++)
            bc[i] = s[i] ^ s[i + 5] ^ s[i + 10] ^ s[i + 15] ^ s[i + 20];

        for(i = 0; i < 5; i++) {
            t = bc[(i + 4) % 5] ^ SHA3_ROTL64(bc[(i + 1) % 5], 1);
            for(j = 0; j < 25; j += 5)
                s[j + i] ^= t;
        }

        /* Rho Pi */
        t = s[1];
        for(i = 0; i < 24; i++) {
            j = keccakf_piln[i];
            bc[0] = s[j];
            s[j] = SHA3_ROTL64(t, keccakf_rotc[i]);
            t = bc[0];
        }

        /* Chi */
        for(j = 0; j < 25; j += 5) {
            for(i = 0; i < 5; i++)
                bc[i] = s[j + i];
            for(i = 0; i < 5; i++)
                s[j + i] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
        }

        /* Iota */
        s[0] ^= keccakf_rndc[round];
    }
}

/* *************************** Public Inteface ************************ */

/* For Init or Reset call these: */
static void
sha3_Init256(void *priv)
{
    sha3_context *ctx = (sha3_context *) priv;
    memset(ctx, 0, sizeof(*ctx));
    ctx->capacityWords = 2 * 256 / (8 * sizeof(uint64_t));
}

static void
sha3_Init384(void *priv)
{
    sha3_context *ctx = (sha3_context *) priv;
    memset(ctx, 0, sizeof(*ctx));
    ctx->capacityWords = 2 * 384 / (8 * sizeof(uint64_t));
}

static void
sha3_Init512(void *priv)
{
    sha3_context *ctx = (sha3_context *) priv;
    memset(ctx, 0, sizeof(*ctx));
    ctx->capacityWords = 2 * 512 / (8 * sizeof(uint64_t));
}

static void
sha3_Update(void *priv, void const *bufIn, size_t len)
{
    sha3_context *ctx = (sha3_context *) priv;

    /* 0...7 -- how much is needed to have a word */
    unsigned old_tail = (8 - ctx->byteIndex) & 7;

    size_t words;
    unsigned tail;
    size_t i;

    const uint8_t *buf = bufIn;

    SHA3_TRACE_BUF("called to update with:", buf, len);

    SHA3_ASSERT(ctx->byteIndex < 8);
    SHA3_ASSERT(ctx->wordIndex < sizeof(ctx->s) / sizeof(ctx->s[0]));

    if(len < old_tail) {        /* have no complete word or haven't started
                                 * the word yet */
        SHA3_TRACE("because %d<%d, store it and return", (unsigned)len,
                (unsigned)old_tail);
        /* endian-independent code follows: */
        while (len--)
            ctx->saved |= (uint64_t) (*(buf++)) << ((ctx->byteIndex++) * 8);
        SHA3_ASSERT(ctx->byteIndex < 8);
        return;
    }

    if(old_tail) {              /* will have one word to process */
        SHA3_TRACE("completing one word with %d bytes", (unsigned)old_tail);
        /* endian-independent code follows: */
        len -= old_tail;
        while (old_tail--)
            ctx->saved |= (uint64_t) (*(buf++)) << ((ctx->byteIndex++) * 8);

        /* now ready to add saved to the sponge */
        ctx->s[ctx->wordIndex] ^= ctx->saved;
        SHA3_ASSERT(ctx->byteIndex == 8);
        ctx->byteIndex = 0;
        ctx->saved = 0;
        if(++ctx->wordIndex ==
                (SHA3_KECCAK_SPONGE_WORDS - ctx->capacityWords)) {
            keccakf(ctx->s);
            ctx->wordIndex = 0;
        }
    }

    /* now work in full words directly from input */

    SHA3_ASSERT(ctx->byteIndex == 0);

    words = len / sizeof(uint64_t);
    tail = len - words * sizeof(uint64_t);

    SHA3_TRACE("have %d full words to process", (unsigned)words);

    for(i = 0; i < words; i++, buf += sizeof(uint64_t)) {
        const uint64_t t = (uint64_t) (buf[0]) |
                ((uint64_t) (buf[1]) << 8 * 1) |
                ((uint64_t) (buf[2]) << 8 * 2) |
                ((uint64_t) (buf[3]) << 8 * 3) |
                ((uint64_t) (buf[4]) << 8 * 4) |
                ((uint64_t) (buf[5]) << 8 * 5) |
                ((uint64_t) (buf[6]) << 8 * 6) |
                ((uint64_t) (buf[7]) << 8 * 7);
#if defined(__x86_64__ ) || defined(__i386__)
        SHA3_ASSERT(memcmp(&t, buf, 8) == 0);
#endif
        ctx->s[ctx->wordIndex] ^= t;
        if(++ctx->wordIndex ==
                (SHA3_KECCAK_SPONGE_WORDS - ctx->capacityWords)) {
            keccakf(ctx->s);
            ctx->wordIndex = 0;
        }
    }

    SHA3_TRACE("have %d bytes left to process, save them", (unsigned)tail);

    /* finally, save the partial word */
    SHA3_ASSERT(ctx->byteIndex == 0 && tail < 8);
    while (tail--) {
        SHA3_TRACE("Store byte %02x '%c'", *buf, *buf);
        ctx->saved |= (uint64_t) (*(buf++)) << ((ctx->byteIndex++) * 8);
    }
    SHA3_ASSERT(ctx->byteIndex < 8);
    SHA3_TRACE("Have saved=0x%016" PRIx64 " at the end", ctx->saved);
}

/* This is simply the 'update' with the padding block.
 * The padding block is 0x01 || 0x00* || 0x80. First 0x01 and last 0x80
 * bytes are always present, but they can be the same byte.
 */
static void const *
sha3_Finalize(void *priv)
{
    sha3_context *ctx = (sha3_context *) priv;

    SHA3_TRACE("called with %d bytes in the buffer", ctx->byteIndex);

    /* Append 2-bit suffix 01, per SHA-3 spec. Instead of 1 for padding we
     * use 1<<2 below. The 0x02 below corresponds to the suffix 01.
     * Overall, we feed 0, then 1, and finally 1 to start padding. Without
     * M || 01, we would simply use 1 to start padding. */

#ifndef SHA3_USE_KECCAK
    /* SHA3 version */
    ctx->s[ctx->wordIndex] ^=
            (ctx->saved ^ ((uint64_t) ((uint64_t) (0x02 | (1 << 2)) <<
                            ((ctx->byteIndex) * 8))));
#else
    /* For testing the "pure" Keccak version */
    ctx->s[ctx->wordIndex] ^=
            (ctx->saved ^ ((uint64_t) ((uint64_t) 1 << (ctx->byteIndex *
                                    8))));
#endif

    ctx->s[SHA3_KECCAK_SPONGE_WORDS - ctx->capacityWords - 1] ^=
            SHA3_CONST(0x8000000000000000UL);
    keccakf(ctx->s);

    /* Return first bytes of the ctx->s. This conversion is not needed for
     * little-endian platforms e.g. wrap with #if !defined(__BYTE_ORDER__)
     * || !defined(__ORDER_LITTLE_ENDIAN__) || \
     * __BYTE_ORDER__!=__ORDER_LITTLE_ENDIAN__ ... the conversion below ...
     * #endif */
    {
        unsigned i;
        for(i = 0; i < SHA3_KECCAK_SPONGE_WORDS; i++) {
            const unsigned t1 = (uint32_t) ctx->s[i];
            const unsigned t2 = (uint32_t) ((ctx->s[i] >> 16) >> 16);
            ctx->sb[i * 8 + 0] = (uint8_t) (t1);
            ctx->sb[i * 8 + 1] = (uint8_t) (t1 >> 8);
            ctx->sb[i * 8 + 2] = (uint8_t) (t1 >> 16);
            ctx->sb[i * 8 + 3] = (uint8_t) (t1 >> 24);
            ctx->sb[i * 8 + 4] = (uint8_t) (t2);
            ctx->sb[i * 8 + 5] = (uint8_t) (t2 >> 8);
            ctx->sb[i * 8 + 6] = (uint8_t) (t2 >> 16);
            ctx->sb[i * 8 + 7] = (uint8_t) (t2 >> 24);
        }
    }

    SHA3_TRACE_BUF("Hash: (first 32 bytes)", ctx->sb, 256 / 8);

    return (ctx->sb);
}

/* *************************** Self Tests ************************ */

/*
 * There are two set of mutually exclusive tests, based on SHA3_USE_KECCAK,
 * which is undefined in the production version.
 *
 * Known answer tests are from NIST SHA3 test vectors at
 * http://csrc.nist.gov/groups/ST/toolkit/examples.html
 *
 * SHA3-256:
 *   http://csrc.nist.gov/groups/ST/toolkit/documents/Examples/SHA3-256_Msg0.pdf
 *   http://csrc.nist.gov/groups/ST/toolkit/documents/Examples/SHA3-256_1600.pdf
 * SHA3-384:
 *   http://csrc.nist.gov/groups/ST/toolkit/documents/Examples/SHA3-384_1600.pdf
 * SHA3-512:
 *   http://csrc.nist.gov/groups/ST/toolkit/documents/Examples/SHA3-512_1600.pdf
 *
 * These are refered to as [FIPS 202] tests.
 *
 * -----
 *
 * A few Keccak algorithm tests (when M and not M||01 is hashed) are
 * added here. These are from http://keccak.noekeon.org/KeccakKAT-3.zip,
 * ShortMsgKAT_256.txt for sizes even to 8. There is also one test for
 * ExtremelyLongMsgKAT_256.txt.
 *
 * These will work with this code when SHA3_USE_KECCAK converts Finalize
 * to use "pure" Keccak algorithm.
 *
 *
 * These are referred to as [Keccak] test.
 *
 * -----
 *
 * In one case the input from [Keccak] test was used to test SHA3
 * implementation. In this case the calculated hash was compared with
 * the output of the sha3sum on Fedora Core 20 (which is Perl's based).
 *
 */

#ifdef __TEST
int
main()
{
    uint8_t buf[200];
    sha3_context c;
    const uint8_t *hash;
    unsigned i;
    const uint8_t c1 = 0xa3;

#ifndef SHA3_USE_KECCAK
    /* [FIPS 202] KAT follow */
    const static uint8_t sha3_256_empty[256 / 8] = {
        0xa7, 0xff, 0xc6, 0xf8, 0xbf, 0x1e, 0xd7, 0x66,
	0x51, 0xc1, 0x47, 0x56, 0xa0, 0x61, 0xd6, 0x62,
	0xf5, 0x80, 0xff, 0x4d, 0xe4, 0x3b, 0x49, 0xfa,
	0x82, 0xd8, 0x0a, 0x4b, 0x80, 0xf8, 0x43, 0x4a
    };
    const static uint8_t sha3_256_0xa3_200_times[256 / 8] = {
        0x79, 0xf3, 0x8a, 0xde, 0xc5, 0xc2, 0x03, 0x07,
        0xa9, 0x8e, 0xf7, 0x6e, 0x83, 0x24, 0xaf, 0xbf,
        0xd4, 0x6c, 0xfd, 0x81, 0xb2, 0x2e, 0x39, 0x73,
        0xc6, 0x5f, 0xa1, 0xbd, 0x9d, 0xe3, 0x17, 0x87
    };
    const static uint8_t sha3_384_0xa3_200_times[384 / 8] = {
        0x18, 0x81, 0xde, 0x2c, 0xa7, 0xe4, 0x1e, 0xf9,
        0x5d, 0xc4, 0x73, 0x2b, 0x8f, 0x5f, 0x00, 0x2b,
        0x18, 0x9c, 0xc1, 0xe4, 0x2b, 0x74, 0x16, 0x8e,
        0xd1, 0x73, 0x26, 0x49, 0xce, 0x1d, 0xbc, 0xdd,
        0x76, 0x19, 0x7a, 0x31, 0xfd, 0x55, 0xee, 0x98,
        0x9f, 0x2d, 0x70, 0x50, 0xdd, 0x47, 0x3e, 0x8f
    };
    const static uint8_t sha3_512_0xa3_200_times[512 / 8] = {
        0xe7, 0x6d, 0xfa, 0xd2, 0x20, 0x84, 0xa8, 0xb1,
        0x46, 0x7f, 0xcf, 0x2f, 0xfa, 0x58, 0x36, 0x1b,
        0xec, 0x76, 0x28, 0xed, 0xf5, 0xf3, 0xfd, 0xc0,
        0xe4, 0x80, 0x5d, 0xc4, 0x8c, 0xae, 0xec, 0xa8,
        0x1b, 0x7c, 0x13, 0xc3, 0x0a, 0xdf, 0x52, 0xa3,
        0x65, 0x95, 0x84, 0x73, 0x9a, 0x2d, 0xf4, 0x6b,
        0xe5, 0x89, 0xc5, 0x1c, 0xa1, 0xa4, 0xa8, 0x41,
        0x6d, 0xf6, 0x54, 0x5a, 0x1c, 0xe8, 0xba, 0x00
    };
#endif

    memset(buf, c1, sizeof(buf));

#ifdef SHA3_USE_KECCAK          /* run tests against "pure" Keccak
                                 * algorithm; from [Keccak] */

    sha3_Init256(&c);
    sha3_Update(&c, "\xcc", 1);
    hash = sha3_Finalize(&c);
    if(memcmp(hash, "\xee\xad\x6d\xbf\xc7\x34\x0a\x56"
                    "\xca\xed\xc0\x44\x69\x6a\x16\x88"
                    "\x70\x54\x9a\x6a\x7f\x6f\x56\x96"
                    "\x1e\x84\xa5\x4b\xd9\x97\x0b\x8a", 256 / 8) != 0) {
        printf("SHA3-256(cc) "
                "doesn't match known answer (single buffer)\n");
        return 11;
    }

    sha3_Init256(&c);
    sha3_Update(&c, "\x41\xfb", 2);
    hash = sha3_Finalize(&c);
    if(memcmp(hash, "\xa8\xea\xce\xda\x4d\x47\xb3\x28"
                    "\x1a\x79\x5a\xd9\xe1\xea\x21\x22"
                    "\xb4\x07\xba\xf9\xaa\xbc\xb9\xe1"
                    "\x8b\x57\x17\xb7\x87\x35\x37\xd2", 256 / 8) != 0) {
        printf("SHA3-256(41fb) "
                "doesn't match known answer (single buffer)\n");
        return 12;
    }

    sha3_Init256(&c);
    sha3_Update(&c,
            "\x52\xa6\x08\xab\x21\xcc\xdd\x8a"
            "\x44\x57\xa5\x7e\xde\x78\x21\x76", 128 / 8);
    hash = sha3_Finalize(&c);
    if(memcmp(hash, "\x0e\x32\xde\xfa\x20\x71\xf0\xb5"
                    "\xac\x0e\x6a\x10\x8b\x84\x2e\xd0"
                    "\xf1\xd3\x24\x97\x12\xf5\x8e\xe0"
                    "\xdd\xf9\x56\xfe\x33\x2a\x5f\x95", 256 / 8) != 0) {
        printf("SHA3-256(52a6...76) "
                "doesn't match known answer (single buffer)\n");
        return 13;
    }

    sha3_Init256(&c);
    sha3_Update(&c,
            "\x43\x3c\x53\x03\x13\x16\x24\xc0"
            "\x02\x1d\x86\x8a\x30\x82\x54\x75"
            "\xe8\xd0\xbd\x30\x52\xa0\x22\x18"
            "\x03\x98\xf4\xca\x44\x23\xb9\x82"
            "\x14\xb6\xbe\xaa\xc2\x1c\x88\x07"
            "\xa2\xc3\x3f\x8c\x93\xbd\x42\xb0"
            "\x92\xcc\x1b\x06\xce\xdf\x32\x24"
            "\xd5\xed\x1e\xc2\x97\x84\x44\x4f"
            "\x22\xe0\x8a\x55\xaa\x58\x54\x2b"
            "\x52\x4b\x02\xcd\x3d\x5d\x5f\x69"
            "\x07\xaf\xe7\x1c\x5d\x74\x62\x22"
            "\x4a\x3f\x9d\x9e\x53\xe7\xe0\x84" "\x6d\xcb\xb4\xce", 800 / 8);
    hash = sha3_Finalize(&c);
    if(memcmp(hash, "\xce\x87\xa5\x17\x3b\xff\xd9\x23"
                    "\x99\x22\x16\x58\xf8\x01\xd4\x5c"
                    "\x29\x4d\x90\x06\xee\x9f\x3f\x9d"
                    "\x41\x9c\x8d\x42\x77\x48\xdc\x41", 256 / 8) != 0) {
        printf("SHA3-256(433C...CE) "
                "doesn't match known answer (single buffer)\n");
        return 14;
    }

    /* SHA3-256 byte-by-byte: 16777216 steps. ExtremelyLongMsgKAT_256
     * [Keccak] */
    i = 16777216;
    sha3_Init256(&c);
    while (i--) {
        sha3_Update(&c,
                "abcdefghbcdefghicdefghijdefghijk"
                "efghijklfghijklmghijklmnhijklmno", 64);
    }
    hash = sha3_Finalize(&c);
    if(memcmp(hash, "\x5f\x31\x3c\x39\x96\x3d\xcf\x79"
                    "\x2b\x54\x70\xd4\xad\xe9\xf3\xa3"
                    "\x56\xa3\xe4\x02\x17\x48\x69\x0a"
                    "\x95\x83\x72\xe2\xb0\x6f\x82\xa4", 256 / 8) != 0) {
        printf("SHA3-256( abcdefgh...[16777216 times] ) "
                "doesn't match known answer\n");
        return 15;
    }
#else                           /* SHA3 testing begins */

    /* SHA-256 on an empty buffer */
    sha3_Init256(&c);
    hash = sha3_Finalize(&c);
    if(memcmp(sha3_256_empty, hash, sizeof(sha3_256_empty)) != 0) {
        printf("SHA3-256() doesn't match known answer\n");
        return 1;
    }

    /* SHA3-256 as a single buffer. [FIPS 202] */
    sha3_Init256(&c);
    sha3_Update(&c, buf, sizeof(buf));
    hash = sha3_Finalize(&c);
    if(memcmp(sha3_256_0xa3_200_times, hash,
                    sizeof(sha3_256_0xa3_200_times)) != 0) {
        printf("SHA3-256( 0xa3 ... [200 times] ) "
                "doesn't match known answer (1 buffer)\n");
        return 1;
    }

    /* SHA3-256 in two steps. [FIPS 202] */
    sha3_Init256(&c);
    sha3_Update(&c, buf, sizeof(buf) / 2);
    sha3_Update(&c, buf + sizeof(buf) / 2, sizeof(buf) / 2);
    hash = sha3_Finalize(&c);
    if(memcmp(sha3_256_0xa3_200_times, hash,
                    sizeof(sha3_256_0xa3_200_times)) != 0) {
        printf("SHA3-256( 0xa3 ... [200 times] ) "
                "doesn't match known answer (2 steps)\n");
        return 2;
    }

    /* SHA3-256 byte-by-byte: 200 steps. [FIPS 202] */
    i = 200;
    sha3_Init256(&c);
    while (i--) {
        sha3_Update(&c, &c1, 1);
    }
    hash = sha3_Finalize(&c);
    if(memcmp(sha3_256_0xa3_200_times, hash,
                    sizeof(sha3_256_0xa3_200_times)) != 0) {
        printf("SHA3-256( 0xa3 ... [200 times] ) "
                "doesn't match known answer (200 steps)\n");
        return 3;
    }

    /* SHA3-256 byte-by-byte: 135 bytes. Input from [Keccak]. Output
     * matched with sha3sum. */
    sha3_Init256(&c);
    sha3_Update(&c,
            "\xb7\x71\xd5\xce\xf5\xd1\xa4\x1a"
            "\x93\xd1\x56\x43\xd7\x18\x1d\x2a"
            "\x2e\xf0\xa8\xe8\x4d\x91\x81\x2f"
            "\x20\xed\x21\xf1\x47\xbe\xf7\x32"
            "\xbf\x3a\x60\xef\x40\x67\xc3\x73"
            "\x4b\x85\xbc\x8c\xd4\x71\x78\x0f"
            "\x10\xdc\x9e\x82\x91\xb5\x83\x39"
            "\xa6\x77\xb9\x60\x21\x8f\x71\xe7"
            "\x93\xf2\x79\x7a\xea\x34\x94\x06"
            "\x51\x28\x29\x06\x5d\x37\xbb\x55"
            "\xea\x79\x6f\xa4\xf5\x6f\xd8\x89"
            "\x6b\x49\xb2\xcd\x19\xb4\x32\x15"
            "\xad\x96\x7c\x71\x2b\x24\xe5\x03"
            "\x2d\x06\x52\x32\xe0\x2c\x12\x74"
            "\x09\xd2\xed\x41\x46\xb9\xd7\x5d"
            "\x76\x3d\x52\xdb\x98\xd9\x49\xd3"
            "\xb0\xfe\xd6\xa8\x05\x2f\xbb", 1080 / 8);
    hash = sha3_Finalize(&c);
    if(memcmp(hash, "\xa1\x9e\xee\x92\xbb\x20\x97\xb6"
                    "\x4e\x82\x3d\x59\x77\x98\xaa\x18"
                    "\xbe\x9b\x7c\x73\x6b\x80\x59\xab"
                    "\xfd\x67\x79\xac\x35\xac\x81\xb5", 256 / 8) != 0) {
        printf("SHA3-256( b771 ... ) doesn't match the known answer\n");
        return 4;
    }

    /* SHA3-384 as a single buffer. [FIPS 202] */
    sha3_Init384(&c);
    sha3_Update(&c, buf, sizeof(buf));
    hash = sha3_Finalize(&c);
    if(memcmp(sha3_384_0xa3_200_times, hash,
                    sizeof(sha3_384_0xa3_200_times)) != 0) {
        printf("SHA3-384( 0xa3 ... [200 times] ) "
                "doesn't match known answer (1 buffer)\n");
        return 5;
    }

    /* SHA3-384 in two steps. [FIPS 202] */
    sha3_Init384(&c);
    sha3_Update(&c, buf, sizeof(buf) / 2);
    sha3_Update(&c, buf + sizeof(buf) / 2, sizeof(buf) / 2);
    hash = sha3_Finalize(&c);
    if(memcmp(sha3_384_0xa3_200_times, hash,
                    sizeof(sha3_384_0xa3_200_times)) != 0) {
        printf("SHA3-384( 0xa3 ... [200 times] ) "
                "doesn't match known answer (2 steps)\n");
        return 6;
    }

    /* SHA3-384 byte-by-byte: 200 steps. [FIPS 202] */
    i = 200;
    sha3_Init384(&c);
    while (i--) {
        sha3_Update(&c, &c1, 1);
    }
    hash = sha3_Finalize(&c);
    if(memcmp(sha3_384_0xa3_200_times, hash,
                    sizeof(sha3_384_0xa3_200_times)) != 0) {
        printf("SHA3-384( 0xa3 ... [200 times] ) "
                "doesn't match known answer (200 steps)\n");
        return 7;
    }

    /* SHA3-512 as a single buffer. [FIPS 202] */
    sha3_Init512(&c);
    sha3_Update(&c, buf, sizeof(buf));
    hash = sha3_Finalize(&c);
    if(memcmp(sha3_512_0xa3_200_times, hash,
                    sizeof(sha3_512_0xa3_200_times)) != 0) {
        printf("SHA3-512( 0xa3 ... [200 times] ) "
                "doesn't match known answer (1 buffer)\n");
        return 8;
    }

    /* SHA3-512 in two steps. [FIPS 202] */
    sha3_Init512(&c);
    sha3_Update(&c, buf, sizeof(buf) / 2);
    sha3_Update(&c, buf + sizeof(buf) / 2, sizeof(buf) / 2);
    hash = sha3_Finalize(&c);
    if(memcmp(sha3_512_0xa3_200_times, hash,
                    sizeof(sha3_512_0xa3_200_times)) != 0) {
        printf("SHA3-512( 0xa3 ... [200 times] ) "
                "doesn't match known answer (2 steps)\n");
        return 9;
    }

    /* SHA3-512 byte-by-byte: 200 steps. [FIPS 202] */
    i = 200;
    sha3_Init512(&c);
    while (i--) {
        sha3_Update(&c, &c1, 1);
    }
    hash = sha3_Finalize(&c);
    if(memcmp(sha3_512_0xa3_200_times, hash,
                    sizeof(sha3_512_0xa3_200_times)) != 0) {
        printf("SHA3-512( 0xa3 ... [200 times] ) "
                "doesn't match known answer (200 steps)\n");
        return 10;
    }
#endif

    printf("SHA3-256, SHA3-384, SHA3-512 tests passed OK\n");

    return 0;
}
#endif // #ifdef __TEST

void hash_init_sha3(void * ctx) {
  sha3_Init384(ctx);
}

void hash_update_sha3(void * ctx, const unsigned char  *buf, size_t len) {
  sha3_Update(ctx, buf, len);
}

void hash_final_sha3(void * ctx, unsigned char *sum) {
  const void * hash = sha3_Finalize(ctx);
  // This magic value is the size of the strcture being returned
  memcpy(sum, hash, SHA3_KECCAK_SPONGE_WORDS * 8);
}

