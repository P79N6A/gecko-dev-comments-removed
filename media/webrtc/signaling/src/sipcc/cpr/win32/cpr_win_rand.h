






































#ifndef _CPR_WIN_RAND_H_
#define _CPR_WIN_RAND_H_

#define CPR_WIN_RAND_DEBUG

#define cpr_srand(seed) srand(seed)
#define cpr_rand()      rand()

extern int cpr_crypto_rand(uint8_t *buf, int *len);
#endif

