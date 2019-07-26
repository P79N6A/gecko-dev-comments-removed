









#ifndef _PKIX_BUILDRESULT_H
#define _PKIX_BUILDRESULT_H

#include "pkix_tools.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PKIX_BuildResultStruct {
        PKIX_ValidateResult *valResult;
        PKIX_List *certChain;
};



PKIX_Error *
pkix_BuildResult_Create(
        PKIX_ValidateResult *valResult,
        PKIX_List *certChain,
        PKIX_BuildResult **pResult,
        void *plContext);

PKIX_Error *pkix_BuildResult_RegisterSelf(void *plContext);

#ifdef __cplusplus
}
#endif

#endif
