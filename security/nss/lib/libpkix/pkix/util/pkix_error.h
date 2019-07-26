









#ifndef _PKIX_ERROR_H
#define _PKIX_ERROR_H

#include "pkix_tools.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PKIX_ErrorStruct {
        PKIX_ERRORCODE errCode;
        PKIX_ERRORCLASS errClass;  
        PKIX_Int32 plErr;
        PKIX_Error *cause;
        PKIX_PL_Object *info;
};



extern PKIX_Error * pkix_Error_RegisterSelf(void *plContext);

#ifdef __cplusplus
}
#endif

#endif
