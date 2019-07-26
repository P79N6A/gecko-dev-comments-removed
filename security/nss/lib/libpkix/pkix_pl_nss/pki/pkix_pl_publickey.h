









#ifndef _PKIX_PL_PUBLICKEY_H
#define _PKIX_PL_PUBLICKEY_H

#include "pkix_pl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PKIX_PL_PublicKeyStruct {
        CERTSubjectPublicKeyInfo *nssSPKI;
};



PKIX_Error *pkix_pl_PublicKey_RegisterSelf(void *plContext);

PKIX_Error *
PKIX_PL_PublicKey_NeedsDSAParameters(
        PKIX_PL_PublicKey *pubKey,
        PKIX_Boolean *pNeedsParams,
        void *plContext);

#ifdef __cplusplus
}
#endif

#endif
