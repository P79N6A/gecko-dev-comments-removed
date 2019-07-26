









#ifndef _PKIX_PL_BIGINT_H
#define _PKIX_PL_BIGINT_H

#include "pkix_pl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PKIX_PL_BigIntStruct {
        char *dataRep;
        PKIX_UInt32 length;
};



PKIX_Error *
pkix_pl_BigInt_CreateWithBytes(
        char *bytes,
        PKIX_UInt32 length,
        PKIX_PL_BigInt **pBigInt,
        void *plContext);

PKIX_Error *pkix_pl_BigInt_RegisterSelf(void *plContext);

#ifdef __cplusplus
}
#endif

#endif
