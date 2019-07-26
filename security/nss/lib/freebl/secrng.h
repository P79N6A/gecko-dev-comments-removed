



#ifndef _SECRNG_H_
#define _SECRNG_H_













#include "blapi.h"


#define SYSTEM_RNG_SEED_COUNT 1024

SEC_BEGIN_PROTOS













extern size_t RNG_GetNoise(void *buf, size_t maxbytes);






extern void RNG_SystemInfoForRNG(void);





extern void RNG_FileForRNG(const char *filename);










extern size_t RNG_SystemRNG(void *buf, size_t maxbytes);

SEC_END_PROTOS

#endif 
