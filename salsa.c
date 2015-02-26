/* 
 * This program implements the SALSA20 algorithm.
 * Salsa20 author Daniel J. Bernstein. Winner the eSTREAM.
 * The SALSA20 home page - http://www.ecrypt.eu.org/stream/.
 * ----------------------
 * Developed: Rostislav Gashin (rost1993). The State University of Syktyvkar (Amplab).
 * Assistant project manager: Lipin Boris (dzruyk).
 * Project manager: Grisha Sitkarev.
 * ----------------------
 * Salsa20 operations based on a 32-bit summation bitwise (XOR) and shift operations.
 * The algorithm uses a hash function with 20 cycles.
 * ----------------------
 * Russia, Komi Republic, Syktyvkar - 15.02.2015, version 3.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "salsa.h"

#define SALSA16		16
#define	SALSA32		32

#define	ROTL32(v, n)	((v << n) | (v >> (32 - n)))

// Selecting the byte order
#if __BYTE_ORDER == __BIG_ENDIAN
#define U32TO32(x)								\
	((x << 24) | ((x << 8) & 0xFF0000) | ((x >> 8) & 0xFF00) | (x >> 24))
#elif __BYTE_ORDER == __LITTLE_ENDIAN
#define U32TO32(x)	(x)
#else
#error unsupported byte order
#endif

// Little-endian 4 uint8_t in the uint32_t
#define U8TO32_LITTLE(p)						\
	(((uint32_t)((p)[0])      ) | ((uint32_t)((p)[1]) << 8) | 	\
	 ((uint32_t)((p)[2]) << 16) | ((uint32_t)((p)[3]) << 24))

/* 
 * Salsa context
 * keylen - chiper key length
 * key - chiper key
 * iv - 16-byte array with a unique number. 8 bytes are filled by the user
 * x - intermediate array
*/
struct salsa_context {
	int keylen;
	uint8_t key[SALSA32];
	uint8_t iv[16];
	uint32_t x[16];
};

// Allocates memory for the salsa context
struct salsa_context * 
salsa_context_new(void)
{
	struct salsa_context *ctx;
	ctx = malloc(sizeof(*ctx));

	if(ctx == NULL)
		return NULL;

	memset(ctx, 0, sizeof(*ctx));
	
	return ctx;
}

// Delete salsa context
void
salsa_context_free(struct salsa_context **ctx)
{
	free(*ctx);
	*ctx = NULL;
}

// Fill the salsa context (key and iv)
// Return value: 0 (if all is well), -1 (if all bad)
int
salsa_set_key_and_iv(struct salsa_context *ctx, const uint8_t *key, const int keylen, const uint8_t iv[8])
{
	int i, j;
	uint8_t *expand;

	uint8_t key_expand_16 [] = {
		'e', 'x', 'p', 'a',
		'n', 'd', ' ', '1',
		'6', '-', 'b', 'y',
		't', 'e', ' ', 'k'
	};

	uint8_t key_expand_32 [] = {
		'e', 'x', 'p', 'a',
		'n', 'd', ' ', '3',
		'2', '-', 'b', 'y',
		't', 'e', ' ', 'k'
	};

	if(keylen == SALSA32) {
		ctx->keylen = SALSA32;
		expand = (uint8_t *)key_expand_32;
		j = 4;
	}
	else if((keylen < SALSA32) && (keylen > 0)){
		ctx->keylen = SALSA16;
		expand = (uint8_t *)key_expand_16;
		j = 0;
	}
	else
	     	return -1;

	memcpy(ctx->key, key, keylen);

	// Fill the iv user data: iv[0] - iv[7].
	memcpy(ctx->iv, iv, 8);
	
	// Fill the iv: iv[8] - iv[15].
	for(i = 8; i < 16; i++)
		ctx->iv[i] = 0;
	
	for(i = 0; i < 4; i++) {
		ctx->x[i *  5] = U8TO32_LITTLE((expand + (i * 4)));
		ctx->x[i +  1] = U8TO32_LITTLE((ctx->key + (i * 4)));
		ctx->x[i +  6] = U8TO32_LITTLE((ctx->iv + (i * 4)));
		ctx->x[i + 11] = U8TO32_LITTLE((ctx->key + ((j + i) * 4)));
	}

	return 0;
}

// Salsa hash function
static void
salsa20(struct salsa_context *ctx, uint32_t *keystream)
{
	uint32_t z[16];
	int i;

	for(i = 0; i < 16; i++)
		z[i] = ctx->x[i];

	for(i = 0; i < 10; i++) {
		z[ 4] ^= ROTL32((z[ 0] + z[12]), 7);
		z[ 8] ^= ROTL32((z[ 4] + z[ 0]), 9);
		z[12] ^= ROTL32((z[ 8] + z[ 4]), 13);
		z[ 0] ^= ROTL32((z[12] + z[ 8]), 18);

		z[ 9] ^= ROTL32((z[ 5] + z[ 1]), 7);
		z[13] ^= ROTL32((z[ 9] + z[ 5]), 9);
		z[ 1] ^= ROTL32((z[13] + z[ 9]), 13);
		z[ 5] ^= ROTL32((z[ 1] + z[13]), 18);

		z[14] ^= ROTL32((z[10] + z[ 6]), 7);
		z[ 2] ^= ROTL32((z[14] + z[10]), 9);
		z[ 6] ^= ROTL32((z[ 2] + z[14]), 13);
		z[10] ^= ROTL32((z[ 6] + z[ 2]), 18);

		z[ 3] ^= ROTL32((z[15] + z[11]), 7);
		z[ 7] ^= ROTL32((z[ 3] + z[15]), 9);
		z[11] ^= ROTL32((z[ 7] + z[ 3]), 13);
		z[15] ^= ROTL32((z[11] + z[ 7]), 18);
	
		z[ 1] ^= ROTL32((z[ 0] + z[ 3]), 7);
		z[ 2] ^= ROTL32((z[ 1] + z[ 0]), 9);
		z[ 3] ^= ROTL32((z[ 2] + z[ 1]), 13);
		z[ 0] ^= ROTL32((z[ 3] + z[ 2]), 18);

		z[ 6] ^= ROTL32((z[ 5] + z[ 4]), 7);
		z[ 7] ^= ROTL32((z[ 6] + z[ 5]), 9);
		z[ 4] ^= ROTL32((z[ 7] + z[ 6]), 13);
		z[ 5] ^= ROTL32((z[ 4] + z[ 7]), 18);
	
		z[11] ^= ROTL32((z[10] + z[ 9]), 7);
		z[ 8] ^= ROTL32((z[11] + z[10]), 9);
		z[ 9] ^= ROTL32((z[ 8] + z[11]), 13);
		z[10] ^= ROTL32((z[ 9] + z[ 8]), 18);

		z[12] ^= ROTL32((z[15] + z[14]), 7);
		z[13] ^= ROTL32((z[12] + z[15]), 9);
		z[14] ^= ROTL32((z[13] + z[12]), 13);
		z[15] ^= ROTL32((z[14] + z[13]), 18);
	}
	
	for(i = 0; i < 16; i++)
		keystream[i] = U32TO32(z[i] + ctx->x[i]);
}

/* 
 * Salsa encrypt algorithm.
 * ctx - pointer on salsa context
 * buf - pointer on buffer data
 * buflen - length the data buffer
*/
void
salsa_encrypt(struct salsa_context *ctx, const uint8_t *buf, uint32_t buflen, uint8_t *out)
{
	uint32_t keystream[16];
	int i;
	
	for(; buflen >= 64; buflen -= 64, buf += 64, out += 64) {
		salsa20(ctx, keystream);
		
		ctx->x[8] += 1;

		if(!ctx->x[8])
			ctx->x[9] += 1;

		*(uint32_t *)(out +  0) = *(uint32_t *)(buf +  0) ^ keystream[ 0];
		*(uint32_t *)(out +  4) = *(uint32_t *)(buf +  4) ^ keystream[ 1];
		*(uint32_t *)(out +  8) = *(uint32_t *)(buf +  8) ^ keystream[ 2];
		*(uint32_t *)(out + 12) = *(uint32_t *)(buf + 12) ^ keystream[ 3];
		*(uint32_t *)(out + 16) = *(uint32_t *)(buf + 16) ^ keystream[ 4];
		*(uint32_t *)(out + 20) = *(uint32_t *)(buf + 20) ^ keystream[ 5];
		*(uint32_t *)(out + 24) = *(uint32_t *)(buf + 24) ^ keystream[ 6];
		*(uint32_t *)(out + 28) = *(uint32_t *)(buf + 28) ^ keystream[ 7];
		*(uint32_t *)(out + 32) = *(uint32_t *)(buf + 32) ^ keystream[ 8];
		*(uint32_t *)(out + 36) = *(uint32_t *)(buf + 36) ^ keystream[ 9];
		*(uint32_t *)(out + 40) = *(uint32_t *)(buf + 40) ^ keystream[10];
		*(uint32_t *)(out + 44) = *(uint32_t *)(buf + 44) ^ keystream[11];
		*(uint32_t *)(out + 48) = *(uint32_t *)(buf + 48) ^ keystream[12];
		*(uint32_t *)(out + 52) = *(uint32_t *)(buf + 52) ^ keystream[13];
		*(uint32_t *)(out + 56) = *(uint32_t *)(buf + 56) ^ keystream[14];
		*(uint32_t *)(out + 60) = *(uint32_t *)(buf + 60) ^ keystream[15];
	}

	if(buflen > 0) {
		salsa20(ctx, keystream);

		ctx->x[8] += 1;

		if(!ctx->x[8])
			ctx->x[9] += 1;

		for(i = 0; i < buflen; i++)
			out[i] = buf[i] ^ ((uint8_t *)keystream)[i];
	}

}

// Salsa decrypt function. See salsa_encrypt
void
salsa_decrypt(struct salsa_context *ctx, const uint8_t *buf, uint32_t buflen, uint8_t *out)
{
	salsa_encrypt(ctx, buf, buflen, out);
}

#if __BYTE_ORDER == __BIG_ENDIAN
#define PRINT_U32TO32(x) \
	(printf("%02x %02x %02x %02x ", (x >> 24), ((x >> 16) & 0xFF), ((x >> 8) & 0xFF), (x & 0xFF)))
#else
#define PRINT_U32TO32(x) \
	(printf("%02x %02x %02x %02x ", (x & 0xFF), ((x >> 8) & 0xFF), ((x >> 16) & 0xFF), (x >> 24)))
#endif

void
salsa_test_vectors(struct salsa_context *ctx)
{
	uint32_t keystream[16];
	int i;

	salsa20(ctx, keystream);

	printf("\nTest vectors for the Salsa20 64 bytes:\n");

	printf("\nKey:       ");

	for(i = 0; i < 32; i++)
		printf("%02x ", ctx->key[i]);
	
	printf("\nIV:        ");
	
	for(i = 0; i < 16; i++)
		printf("%02x ", ctx->iv[i]);
	
	printf("\nKeystream: ");

	for(i = 0; i < 16; i++)
		PRINT_U32TO32(keystream[i]);
	
	printf("\n\n");
}

