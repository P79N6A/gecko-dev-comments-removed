










































#ifndef _PKIX_COMCRLSELPARAMS_H
#define _PKIX_COMCRLSELPARAMS_H

#include "pkix_tools.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PKIX_ComCRLSelParamsStruct {
        PKIX_List *issuerNames; 
        PKIX_PL_Cert *cert; 
        PKIX_List *crldpList;
        PKIX_PL_Date *date;
        PKIX_Boolean nistPolicyEnabled;
        PKIX_PL_BigInt *maxCRLNumber;
        PKIX_PL_BigInt *minCRLNumber;
};



PKIX_Error *pkix_ComCRLSelParams_RegisterSelf(void *plContext);

#ifdef __cplusplus
}
#endif

#endif
