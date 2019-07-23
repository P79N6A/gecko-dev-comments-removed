










































#ifndef _PKIX_OCSPCHECKER_H
#define _PKIX_OCSPCHECKER_H

#include "pkix_tools.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PKIX_OcspCheckerStruct {
        PKIX_PL_OcspResponse *response;
        PKIX_PL_Date *validityTime;
        PKIX_Boolean clientIsDefault;
        void *passwordInfo;
        void *responder;
        PKIX_PL_OcspResponse_VerifyCallback verifyFcn;
        void *nbioContext;
        PKIX_PL_Cert *cert;
};



PKIX_Error *pkix_OcspChecker_RegisterSelf(void *plContext);

PKIX_Error *
PKIX_OcspChecker_SetPasswordInfo(
        PKIX_OcspChecker *checker,
        void *passwordInfo,
        void *plContext);

PKIX_Error *
PKIX_OcspChecker_SetOCSPResponder(
        PKIX_OcspChecker *checker,
        void *ocspResponder,
        void *plContext);

PKIX_Error *
PKIX_OcspChecker_SetVerifyFcn(
        PKIX_OcspChecker *checker,
        PKIX_PL_OcspResponse_VerifyCallback verifyFcn,
        void *plContext);

PKIX_Error *
PKIX_OcspChecker_Initialize(
        PKIX_PL_Date *validityTime,
        void *passwordInfo,
        void *responder,
        PKIX_RevocationChecker **pChecker,
        void *plContext);

#ifdef __cplusplus
}
#endif

#endif
