








#ifndef _DES_H_
#define _DES_H_ 1

#include "blapi.h"

typedef unsigned char BYTE;
typedef unsigned int  HALF;

#define HALFPTR(x) ((HALF *)(x))
#define SHORTPTR(x) ((unsigned short *)(x))
#define BYTEPTR(x) ((BYTE *)(x))

typedef enum {
    DES_ENCRYPT = 0x5555,
    DES_DECRYPT = 0xAAAA
} DESDirection;

typedef void DESFunc(struct DESContextStr *cx, BYTE *out, const BYTE *in, 
                     unsigned int len);

struct DESContextStr {
    
    HALF ks0 [32];
    HALF ks1 [32];
    HALF ks2 [32];
    HALF iv  [2];
    DESDirection direction;
    DESFunc  *worker;
};

void DES_MakeSchedule( HALF * ks, const BYTE * key,   DESDirection direction);
void DES_Do1Block(     HALF * ks, const BYTE * inbuf, BYTE * outbuf);

#endif
