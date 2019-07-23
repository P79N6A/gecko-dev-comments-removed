










































#ifndef _PKIX_STORE_H
#define _PKIX_STORE_H

#include "pkix_tools.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PKIX_CertStoreStruct {
        PKIX_CertStore_CertCallback certCallback;
        PKIX_CertStore_CRLCallback crlCallback;
        PKIX_CertStore_CertContinueFunction certContinue;
        PKIX_CertStore_CrlContinueFunction crlContinue;
        PKIX_CertStore_CheckTrustCallback trustCallback;
        PKIX_PL_Object *certStoreContext;
        PKIX_Boolean cacheFlag;
        PKIX_Boolean localFlag; 
};



PKIX_Error *pkix_CertStore_RegisterSelf(void *plContext);

#ifdef __cplusplus
}
#endif

#endif
