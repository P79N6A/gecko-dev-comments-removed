









#ifndef _PKIX_BUILDPARAMS_H
#define _PKIX_BUILDPARAMS_H

#include "pkix_tools.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PKIX_BuildParamsStruct {
        PKIX_ProcessingParams *procParams;      
};



PKIX_Error *pkix_BuildParams_RegisterSelf(void *plContext);

#ifdef __cplusplus
}
#endif

#endif
