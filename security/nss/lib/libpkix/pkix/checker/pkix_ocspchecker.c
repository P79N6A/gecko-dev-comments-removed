











































#include "pkix_ocspchecker.h"
#include "pkix_pl_ocspcertid.h"
#include "pkix_error.h"




typedef struct pkix_OcspCheckerStruct {
    
    pkix_RevocationMethod method;
    PKIX_PL_VerifyCallback certVerifyFcn;
} pkix_OcspChecker;







static PKIX_Error *
pkix_OcspChecker_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
    return NULL;
}













PKIX_Error *
pkix_OcspChecker_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry* entry = &systemClasses[PKIX_OCSPCHECKER_TYPE];

        PKIX_ENTER(OCSPCHECKER, "pkix_OcspChecker_RegisterSelf");

        entry->description = "OcspChecker";
        entry->typeObjectSize = sizeof(pkix_OcspChecker);
        entry->destructor = pkix_OcspChecker_Destroy;

        PKIX_RETURN(OCSPCHECKER);
}





PKIX_Error *
pkix_OcspChecker_Create(PKIX_RevocationMethodType methodType,
                        PKIX_UInt32 flags,
                        PKIX_UInt32 priority,
                        pkix_LocalRevocationCheckFn localRevChecker,
                        pkix_ExternalRevocationCheckFn externalRevChecker,
                        PKIX_PL_VerifyCallback verifyFn,
                        pkix_RevocationMethod **pChecker,
                        void *plContext)
{
        pkix_OcspChecker *method = NULL;

        PKIX_ENTER(OCSPCHECKER, "pkix_OcspChecker_Create");
        PKIX_NULLCHECK_ONE(pChecker);

        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_OCSPCHECKER_TYPE,
                    sizeof (pkix_OcspChecker),
                    (PKIX_PL_Object **)&method,
                    plContext),
                    PKIX_COULDNOTCREATECERTCHAINCHECKEROBJECT);

        pkixErrorResult = pkix_RevocationMethod_Init(
            (pkix_RevocationMethod*)method, methodType, flags,  priority,
            localRevChecker, externalRevChecker, plContext);
        if (pkixErrorResult) {
            goto cleanup;
        }
        method->certVerifyFcn = (PKIX_PL_VerifyCallback)verifyFn;

        *pChecker = (pkix_RevocationMethod*)method;
        method = NULL;

cleanup:
        PKIX_DECREF(method);

        PKIX_RETURN(OCSPCHECKER);
}



















PKIX_Error *
pkix_OcspChecker_CheckLocal(
        PKIX_PL_Cert *cert,
        PKIX_PL_Cert *issuer,
        PKIX_PL_Date *date,
        pkix_RevocationMethod *checkerObject,
        PKIX_ProcessingParams *procParams,
        PKIX_UInt32 methodFlags,
        PKIX_Boolean chainVerificationState,
        PKIX_RevocationStatus *pRevStatus,
        PKIX_UInt32 *pReasonCode,
        void *plContext)
{
        PKIX_PL_OcspCertID    *cid = NULL;
        PKIX_Boolean           hasFreshStatus = PKIX_FALSE;
        PKIX_Boolean           statusIsGood = PKIX_FALSE;
        SECErrorCodes          resultCode = SEC_ERROR_REVOKED_CERTIFICATE_OCSP;
        PKIX_RevocationStatus  revStatus = PKIX_RevStatus_NoInfo;

        PKIX_ENTER(OCSPCHECKER, "pkix_OcspChecker_CheckLocal");

        PKIX_CHECK(
            PKIX_PL_OcspCertID_Create(cert, NULL, &cid,
                                      plContext),
            PKIX_OCSPCERTIDCREATEFAILED);
        if (!cid) {
            goto cleanup;
        }

        PKIX_CHECK(
            PKIX_PL_OcspCertID_GetFreshCacheStatus(cid, date,
                                                   &hasFreshStatus,
                                                   &statusIsGood,
                                                   &resultCode,
                                                   plContext),
            PKIX_OCSPCERTIDGETFRESHCACHESTATUSFAILED);
        if (hasFreshStatus) {
            if (statusIsGood) {
                revStatus = PKIX_RevStatus_Success;
                resultCode = 0;
            } else {
                revStatus = PKIX_RevStatus_Revoked;
            }
        }

cleanup:
        *pRevStatus = revStatus;

        


        *pReasonCode = crlEntryReasonUnspecified;
        PKIX_DECREF(cid);

        PKIX_RETURN(OCSPCHECKER);
}














PKIX_Error *
pkix_OcspChecker_CheckExternal(
        PKIX_PL_Cert *cert,
        PKIX_PL_Cert *issuer,
        PKIX_PL_Date *date,
        pkix_RevocationMethod *checkerObject,
        PKIX_ProcessingParams *procParams,
        PKIX_UInt32 methodFlags,
        PKIX_RevocationStatus *pRevStatus,
        PKIX_UInt32 *pReasonCode,
        void **pNBIOContext,
        void *plContext)
{
        SECErrorCodes resultCode = SEC_ERROR_REVOKED_CERTIFICATE_OCSP;
        PKIX_Boolean uriFound = PKIX_FALSE;
        PKIX_Boolean passed = PKIX_TRUE;
        pkix_OcspChecker *checker = NULL;
        PKIX_PL_OcspCertID *cid = NULL;
        PKIX_PL_OcspRequest *request = NULL;
        PKIX_PL_OcspResponse *response = NULL;
        PKIX_PL_Date *validity = NULL;
        PKIX_RevocationStatus revStatus = PKIX_RevStatus_NoInfo;
        void *nbioContext = NULL;

        PKIX_ENTER(OCSPCHECKER, "pkix_OcspChecker_CheckExternal");

        PKIX_CHECK(
            pkix_CheckType((PKIX_PL_Object*)checkerObject,
                           PKIX_OCSPCHECKER_TYPE, plContext),
                PKIX_OBJECTNOTOCSPCHECKER);

        checker = (pkix_OcspChecker *)checkerObject;

        PKIX_CHECK(
            PKIX_PL_OcspCertID_Create(cert, NULL, &cid,
                                      plContext),
            PKIX_OCSPCERTIDCREATEFAILED);
        
        
        PKIX_CHECK(
            pkix_pl_OcspRequest_Create(cert, cid, validity, NULL, 
                                       methodFlags, &uriFound, &request,
                                       plContext),
            PKIX_OCSPREQUESTCREATEFAILED);
        
        if (uriFound == PKIX_FALSE) {
            
            resultCode = 0;
            goto cleanup;
        }

        
        PKIX_CHECK(
            pkix_pl_OcspResponse_Create(request, NULL,
                                        checker->certVerifyFcn,
                                        &nbioContext,
                                        &response,
                                        plContext),
            PKIX_OCSPRESPONSECREATEFAILED);
        if (nbioContext != 0) {
            *pNBIOContext = nbioContext;
            goto cleanup;
        }
        
        PKIX_CHECK(
            pkix_pl_OcspResponse_Decode(response, &passed,
                                        &resultCode, plContext),
            PKIX_OCSPRESPONSEDECODEFAILED);
        if (passed == PKIX_FALSE) {
            goto cleanup;
        }
        
        PKIX_CHECK(
            pkix_pl_OcspResponse_GetStatus(response, &passed,
                                           &resultCode, plContext),
            PKIX_OCSPRESPONSEGETSTATUSRETURNEDANERROR);
        if (passed == PKIX_FALSE) {
            goto cleanup;
        }

        PKIX_CHECK(
            pkix_pl_OcspResponse_VerifySignature(response, cert,
                                                 procParams, &passed, 
                                                 &nbioContext, plContext),
            PKIX_OCSPRESPONSEVERIFYSIGNATUREFAILED);
       	if (nbioContext != 0) {
               	*pNBIOContext = nbioContext;
                goto cleanup;
        }
        if (passed == PKIX_FALSE) {
                goto cleanup;
        }

        PKIX_CHECK(
            pkix_pl_OcspResponse_GetStatusForCert(cid, response, date,
                                                  &passed, &resultCode,
                                                  plContext),
            PKIX_OCSPRESPONSEGETSTATUSFORCERTFAILED);
        if (passed == PKIX_FALSE) {
            revStatus = PKIX_RevStatus_Revoked;
        } else {
            revStatus = PKIX_RevStatus_Success;
        }

cleanup:
        if (revStatus == PKIX_RevStatus_NoInfo && (uriFound || 
	    methodFlags & PKIX_REV_M_REQUIRE_INFO_ON_MISSING_SOURCE) &&
            methodFlags & PKIX_REV_M_FAIL_ON_MISSING_FRESH_INFO) {
            revStatus = PKIX_RevStatus_Revoked;
        }
        *pRevStatus = revStatus;

        


        *pReasonCode = crlEntryReasonUnspecified;

        if (!passed && cid && cid->certID) {
                



                PKIX_Error *err;
                err = PKIX_PL_OcspCertID_RememberOCSPProcessingFailure(
                        cid, plContext);
                if (err) {
                        PKIX_PL_Object_DecRef((PKIX_PL_Object*)err, plContext);
                }
        }
        PKIX_DECREF(cid);
        PKIX_DECREF(request);
        PKIX_DECREF(response);

        PKIX_RETURN(OCSPCHECKER);
}


