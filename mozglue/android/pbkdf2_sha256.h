



























#ifndef _SHA256_H_
#define _SHA256_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

#include <stdint.h>

typedef struct SHA256Context {
	uint32_t state[8];
	uint32_t count[2];
	unsigned char buf[64];
} SHA256_CTX;

typedef struct HMAC_SHA256Context {
	SHA256_CTX ictx;
	SHA256_CTX octx;
} HMAC_SHA256_CTX;

void	SHA256_Init(SHA256_CTX *);
void	SHA256_Update(SHA256_CTX *, const void *, size_t);
void	SHA256_Final(unsigned char [32], SHA256_CTX *);
void	HMAC_SHA256_Init(HMAC_SHA256_CTX *, const void *, size_t);
void	HMAC_SHA256_Update(HMAC_SHA256_CTX *, const void *, size_t);
void	HMAC_SHA256_Final(unsigned char [32], HMAC_SHA256_CTX *);






void	PBKDF2_SHA256(const uint8_t *, size_t, const uint8_t *, size_t,
    uint64_t, uint8_t *, size_t);

#ifdef __cplusplus
}
#endif

#endif
