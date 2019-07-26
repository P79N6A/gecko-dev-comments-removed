







#ifndef _SECDIGT_H_
#define _SECDIGT_H_

#include "utilrename.h"
#include "plarena.h"
#include "secoidt.h"
#include "secitem.h"




struct SGNDigestInfoStr {
    PLArenaPool *  arena;
    SECAlgorithmID digestAlgorithm;
    SECItem        digest;
};
typedef struct SGNDigestInfoStr SGNDigestInfo;



#endif 
