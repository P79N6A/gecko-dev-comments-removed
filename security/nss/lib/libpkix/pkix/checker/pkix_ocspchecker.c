











































#include "pkix_ocspchecker.h"
#include "pkix_pl_ocspcertid.h"
#include "pkix_error.h"








static PKIX_Error *
pkix_OcspChecker_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_OcspChecker *checker = NULL;

        PKIX_ENTER(OCSPCHECKER, "pkix_OcspChecker_Destroy");
        PKIX_NULLCHECK_ONE(object);

        
        PKIX_CHECK(pkix_CheckType
                    (object, PKIX_OCSPCHECKER_TYPE, plContext),
                    PKIX_OBJECTNOTOCSPCHECKER);

        checker = (PKIX_OcspChecker *)object;

	PKIX_DECREF(checker->response);
        PKIX_DECREF(checker->validityTime);
        PKIX_DECREF(checker->cert);

        
        
        
        

cleanup:

        PKIX_RETURN(OCSPCHECKER);
}













PKIX_Error *
pkix_OcspChecker_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(OCSPCHECKER, "pkix_OcspChecker_RegisterSelf");

        entry.description = "OcspChecker";
        entry.objCounter = 0;
        entry.typeObjectSize = sizeof(PKIX_OcspChecker);
        entry.destructor = pkix_OcspChecker_Destroy;
        entry.equalsFunction = NULL;
        entry.hashcodeFunction = NULL;
        entry.toStringFunction = NULL;
        entry.comparator = NULL;
        entry.duplicateFunction = NULL;

        systemClasses[PKIX_OCSPCHECKER_TYPE] = entry;

        PKIX_RETURN(OCSPCHECKER);
}



















static PKIX_Error *
pkix_OcspChecker_Check(
        PKIX_PL_Object *checkerObject,
        PKIX_PL_Cert *cert,
        PKIX_ProcessingParams *procParams,
        void **pNBIOContext,
        PKIX_UInt32 *pResultCode,
        void *plContext)
{
        SECErrorCodes resultCode = SEC_ERROR_REVOKED_CERTIFICATE_OCSP;
        PKIX_Boolean uriFound = PKIX_FALSE;
        PKIX_Boolean passed = PKIX_FALSE;
        PKIX_OcspChecker *checker = NULL;
        PKIX_PL_OcspCertID *cid = NULL;
        PKIX_PL_OcspRequest *request = NULL;
        PKIX_PL_Date *validity = NULL;
        void *nbioContext = NULL;

        PKIX_ENTER(OCSPCHECKER, "pkix_OcspChecker_Check");
        PKIX_NULLCHECK_FOUR(checkerObject, cert, pNBIOContext, pResultCode);

        PKIX_CHECK(pkix_CheckType
                (checkerObject, PKIX_OCSPCHECKER_TYPE, plContext),
                PKIX_OBJECTNOTOCSPCHECKER);

        checker = (PKIX_OcspChecker *)checkerObject;

        nbioContext = *pNBIOContext;
        *pNBIOContext = 0;

        

        if (nbioContext == 0) {
                

                PKIX_Boolean hasFreshStatus = PKIX_FALSE;
                PKIX_Boolean statusIsGood = PKIX_FALSE;

                PKIX_CHECK(PKIX_PL_OcspCertID_Create
                        (cert,
                        validity,
                        &cid,
                        plContext),
                        PKIX_OCSPCERTIDCREATEFAILED);

                if (!cid) {
                        goto cleanup;
                }

                PKIX_CHECK(PKIX_PL_OcspCertID_GetFreshCacheStatus
                        (cid, 
                        validity, 
                        &hasFreshStatus, 
                        &statusIsGood,
                        &resultCode,
                        plContext),
                        PKIX_OCSPCERTIDGETFRESHCACHESTATUSFAILED);

                if (hasFreshStatus) {
                        
                        passed = PKIX_TRUE; 

                        if (statusIsGood) {
                                resultCode = 0;
                        }
                        goto cleanup;
                }
                PKIX_INCREF(cert);
                PKIX_DECREF(checker->cert);
                checker->cert = cert;

                
                PKIX_CHECK(pkix_pl_OcspRequest_Create
                        (cert,
                        cid,
                        validity,
                        NULL,           
                        &uriFound,
                        &request,
                        plContext),
                        PKIX_OCSPREQUESTCREATEFAILED);
                
                
                if (uriFound == PKIX_FALSE) {
                        
                        passed = PKIX_TRUE;
                        resultCode = 0;
                        goto cleanup;
                }

        }

        
        if ((checker->response) == NULL) {
        	
	        PKIX_CHECK(pkix_pl_OcspResponse_Create
	                (request,
	                checker->responder,
	                checker->verifyFcn,
	                &nbioContext,
	                &(checker->response),
	                plContext),
	                PKIX_OCSPRESPONSECREATEFAILED);

        	if (nbioContext != 0) {
                	*pNBIOContext = nbioContext;
	                goto cleanup;
	        }

	        PKIX_CHECK(pkix_pl_OcspResponse_Decode
        	        ((checker->response), &passed, &resultCode, plContext),
                	PKIX_OCSPRESPONSEDECODEFAILED);
                
	        if (passed == PKIX_FALSE) {
        	        goto cleanup;
	        }

        	PKIX_CHECK(pkix_pl_OcspResponse_GetStatus
                	((checker->response), &passed, &resultCode, plContext),
	                PKIX_OCSPRESPONSEGETSTATUSRETURNEDANERROR);
                
        	if (passed == PKIX_FALSE) {
                	goto cleanup;
	        }
	}

        PKIX_CHECK(pkix_pl_OcspResponse_VerifySignature
                ((checker->response),
                cert,
                procParams,
                &passed,
		&nbioContext,
                plContext),
                PKIX_OCSPRESPONSEVERIFYSIGNATUREFAILED);

       	if (nbioContext != 0) {
               	*pNBIOContext = nbioContext;
                goto cleanup;
        }

        if (passed == PKIX_FALSE) {
                resultCode = PORT_GetError();
                goto cleanup;
        }

        PKIX_CHECK(pkix_pl_OcspResponse_GetStatusForCert
                (cid, (checker->response), &passed, &resultCode, plContext),
                PKIX_OCSPRESPONSEGETSTATUSFORCERTFAILED);

cleanup:
        if (!passed && cid && cid->certID) {
                



                PKIX_Error *err;
                err = PKIX_PL_OcspCertID_RememberOCSPProcessingFailure(
                        cid, plContext);
                if (err) {
                        PKIX_PL_Object_DecRef((PKIX_PL_Object*)err, plContext);
                }
        }

        *pResultCode = (PKIX_UInt32)resultCode;

        PKIX_DECREF(cid);
        PKIX_DECREF(request);
        if (checker) {
            PKIX_DECREF(checker->response);
        }

        PKIX_RETURN(OCSPCHECKER);

}




PKIX_Error *
pkix_OcspChecker_Create(
        PKIX_PL_Date *validityTime,
        void *passwordInfo,
        void *responder,
        PKIX_OcspChecker **pChecker,
        void *plContext)
{
        PKIX_OcspChecker *checkerObject = NULL;

        PKIX_ENTER(OCSPCHECKER, "pkix_OcspChecker_Create");
        PKIX_NULLCHECK_ONE(pChecker);

        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_OCSPCHECKER_TYPE,
                    sizeof (PKIX_OcspChecker),
                    (PKIX_PL_Object **)&checkerObject,
                    plContext),
                    PKIX_COULDNOTCREATECERTCHAINCHECKEROBJECT);

        
        checkerObject->response = NULL;
        PKIX_INCREF(validityTime);
        checkerObject->validityTime = validityTime;
        checkerObject->clientIsDefault = PKIX_FALSE;
        checkerObject->verifyFcn = NULL;
        checkerObject->cert = NULL;

        
        checkerObject->passwordInfo = passwordInfo;
        checkerObject->responder = responder;
        checkerObject->nbioContext = NULL;

        *pChecker = checkerObject;
        checkerObject = NULL;

cleanup:

        PKIX_DECREF(checkerObject);

        PKIX_RETURN(OCSPCHECKER);

}





PKIX_Error *
PKIX_OcspChecker_SetPasswordInfo(
        PKIX_OcspChecker *checker,
        void *passwordInfo,
        void *plContext)
{
        PKIX_ENTER(OCSPCHECKER, "PKIX_OcspChecker_SetPasswordInfo");
        PKIX_NULLCHECK_ONE(checker);

        checker->passwordInfo = passwordInfo;

        PKIX_RETURN(OCSPCHECKER);
}





PKIX_Error *
PKIX_OcspChecker_SetOCSPResponder(
        PKIX_OcspChecker *checker,
        void *ocspResponder,
        void *plContext)
{
        PKIX_ENTER(OCSPCHECKER, "PKIX_OcspChecker_SetOCSPResponder");
        PKIX_NULLCHECK_ONE(checker);

        checker->responder = ocspResponder;

        PKIX_RETURN(OCSPCHECKER);
}





PKIX_Error *
PKIX_OcspChecker_SetVerifyFcn(
        PKIX_OcspChecker *checker,
        PKIX_PL_OcspResponse_VerifyCallback verifyFcn,
        void *plContext)
{
        PKIX_ENTER(OCSPCHECKER, "PKIX_OcspChecker_SetVerifyFcn");
        PKIX_NULLCHECK_ONE(checker);

        checker->verifyFcn = verifyFcn;

        PKIX_RETURN(OCSPCHECKER);
}

PKIX_Error *
PKIX_OcspChecker_Initialize(
        PKIX_PL_Date *validityTime,
        void *passwordInfo,
        void *responder,
        PKIX_RevocationChecker **pChecker,
        void *plContext)
{
        PKIX_OcspChecker *oChecker = NULL;

        PKIX_ENTER(OCSPCHECKER, "PKIX_OcspChecker_Initialize");
        PKIX_NULLCHECK_ONE(pChecker);

        PKIX_CHECK(pkix_OcspChecker_Create
                (validityTime, passwordInfo, responder, &oChecker, plContext),
                PKIX_OCSPCHECKERCREATEFAILED);

        PKIX_CHECK(PKIX_RevocationChecker_Create
                (pkix_OcspChecker_Check,
                (PKIX_PL_Object *)oChecker,
                pChecker,
                plContext),
                PKIX_REVOCATIONCHECKERCREATEFAILED);

cleanup:

        PKIX_DECREF(oChecker);

        PKIX_RETURN(OCSPCHECKER);
}


