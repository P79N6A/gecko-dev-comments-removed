










































#ifndef _PKIX_REVOCATIONMETHOD_H
#define _PKIX_REVOCATIONMETHOD_H

#include "pkixt.h"
#include "pkix_revocationchecker.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pkix_RevocationMethodStruct pkix_RevocationMethod;




typedef PKIX_Error *
pkix_LocalRevocationCheckFn(PKIX_PL_Cert *cert, PKIX_PL_Cert *issuer,
                            PKIX_PL_Date *date, 
                            pkix_RevocationMethod *checkerObject,
                            PKIX_ProcessingParams *procParams,
                            PKIX_UInt32 methodFlags,
                            PKIX_Boolean chainVerificationState,
                            PKIX_RevocationStatus *pRevStatus,
                            PKIX_UInt32 *reasonCode,
                            void *plContext);




typedef PKIX_Error *
pkix_ExternalRevocationCheckFn(PKIX_PL_Cert *cert, PKIX_PL_Cert *issuer,
                               PKIX_PL_Date *date,
                               pkix_RevocationMethod *checkerObject,
                               PKIX_ProcessingParams *procParams,
                               PKIX_UInt32 methodFlags,
                               PKIX_RevocationStatus *pRevStatus,
                               PKIX_UInt32 *reasonCode,
                               void **pNBIOContext, void *plContext);




struct pkix_RevocationMethodStruct {
    PKIX_RevocationMethodType methodType;
    PKIX_UInt32 flags;
    PKIX_UInt32 priority;
    pkix_LocalRevocationCheckFn (*localRevChecker);
    pkix_ExternalRevocationCheckFn (*externalRevChecker);
};

PKIX_Error *
pkix_RevocationMethod_Duplicate(PKIX_PL_Object *object,
                                PKIX_PL_Object *newObject,
                                void *plContext);

PKIX_Error *
pkix_RevocationMethod_Init(pkix_RevocationMethod *method,
                           PKIX_RevocationMethodType methodType,
                           PKIX_UInt32 flags,
                           PKIX_UInt32 priority,
                           pkix_LocalRevocationCheckFn localRevChecker,
                           pkix_ExternalRevocationCheckFn externalRevChecker,
                           void *plContext);


#ifdef __cplusplus
}
#endif

#endif
