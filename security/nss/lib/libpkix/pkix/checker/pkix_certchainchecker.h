









#ifndef _PKIX_CERTCHAINCHECKER_H
#define _PKIX_CERTCHAINCHECKER_H

#include "pkix_tools.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PKIX_CertChainCheckerStruct {
        PKIX_CertChainChecker_CheckCallback checkCallback;
        PKIX_List *extensions;
        PKIX_PL_Object *state;
        PKIX_Boolean forwardChecking;
        PKIX_Boolean isForwardDirectionExpected;
};



PKIX_Error *pkix_CertChainChecker_RegisterSelf(void *plContext);

#ifdef __cplusplus
}
#endif

#endif
