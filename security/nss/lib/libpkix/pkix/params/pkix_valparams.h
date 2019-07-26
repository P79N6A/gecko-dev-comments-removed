









#ifndef _PKIX_VALIDATEPARAMS_H
#define _PKIX_VALIDATEPARAMS_H

#include "pkix_tools.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PKIX_ValidateParamsStruct {
        PKIX_ProcessingParams *procParams;      
        PKIX_List *chain;                       
};



PKIX_Error *pkix_ValidateParams_RegisterSelf(void *plContext);

#ifdef __cplusplus
}
#endif

#endif
