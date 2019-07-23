










































#include "pkix_validate.h"





































static PKIX_Error *
pkix_AddToVerifyLog(
        PKIX_PL_Cert *cert,
        PKIX_UInt32 depth,
        PKIX_Error *error,
        PKIX_VerifyNode **pVerifyTree,
        void *plContext)
{

        PKIX_VerifyNode *verifyNode = NULL;

        PKIX_ENTER(VALIDATE, "pkix_AddToVerifyLog");
        PKIX_NULLCHECK_ONE(cert);

        if (pVerifyTree) { 

                PKIX_CHECK(pkix_VerifyNode_Create
                        (cert, depth, error, &verifyNode, plContext),
                        PKIX_VERIFYNODECREATEFAILED);

                if (depth == 0) {
                        
                        *pVerifyTree = verifyNode;
                } else {
                        PKIX_CHECK(pkix_VerifyNode_AddToChain
                                (*pVerifyTree, verifyNode, plContext),
                                PKIX_VERIFYNODEADDTOCHAINFAILED);
                }
        }

cleanup:

        PKIX_RETURN(VALIDATE);

}












































static PKIX_Error *
pkix_CheckCert(
        PKIX_PL_Cert *cert,
        PKIX_List *checkers,
        PKIX_List *checkedExtOIDsList,
        PKIX_UInt32 *pCheckerIndex,
        void **pNBIOContext,
        void *plContext)
{
        PKIX_CertChainChecker_CheckCallback checkerCheck = NULL;
        PKIX_CertChainChecker *checker = NULL;
        PKIX_List *unresCritExtOIDs = NULL;
        PKIX_UInt32 numCheckers;
        PKIX_UInt32 numUnresCritExtOIDs = 0;
        PKIX_UInt32 checkerIndex = 0;
        void *nbioContext = NULL;

        PKIX_ENTER(VALIDATE, "pkix_CheckCert");
        PKIX_NULLCHECK_FOUR(cert, checkers, pCheckerIndex, pNBIOContext);

        nbioContext = *pNBIOContext;
        *pNBIOContext = NULL; 

        PKIX_CHECK(PKIX_PL_Cert_GetCriticalExtensionOIDs
                    (cert, &unresCritExtOIDs, plContext),
                    PKIX_CERTGETCRITICALEXTENSIONOIDSFAILED);

        PKIX_CHECK(PKIX_List_GetLength(checkers, &numCheckers, plContext),
                    PKIX_LISTGETLENGTHFAILED);

        for (checkerIndex = *pCheckerIndex;
                checkerIndex < numCheckers;
                checkerIndex++) {

                PKIX_CHECK(PKIX_List_GetItem
                        (checkers,
                        checkerIndex,
                        (PKIX_PL_Object **)&checker,
                        plContext),
                        PKIX_LISTGETITEMFAILED);

                PKIX_CHECK(PKIX_CertChainChecker_GetCheckCallback
                        (checker, &checkerCheck, plContext),
                        PKIX_CERTCHAINCHECKERGETCHECKCALLBACKFAILED);

                PKIX_CHECK(checkerCheck(checker, cert, unresCritExtOIDs,
                                        &nbioContext,  plContext),
                           PKIX_CERTCHAINCHECKERCHECKFAILED);

                if (nbioContext != NULL) {
                        *pCheckerIndex = checkerIndex;
                        *pNBIOContext = nbioContext;
                        goto cleanup;
                }

                PKIX_DECREF(checker);
        }

        if (unresCritExtOIDs){

#ifdef PKIX_VALIDATEDEBUG
                {
                        PKIX_PL_String *oidString = NULL;
                        PKIX_UInt32 length;
                        char *oidAscii = NULL;
                        PKIX_TOSTRING(unresCritExtOIDs, &oidString, plContext,
                                PKIX_LISTTOSTRINGFAILED);
                        PKIX_CHECK(PKIX_PL_String_GetEncoded
                                (oidString,
                                PKIX_ESCASCII,
                                (void **) &oidAscii,
                                &length,
                                plContext),
                                PKIX_STRINGGETENCODEDFAILED);
                        PKIX_VALIDATE_DEBUG_ARG
                                ("unrecognized critical extension OIDs:"
                                " %s\n", oidAscii);
                        PKIX_DECREF(oidString);
                        PKIX_PL_Free(oidAscii, plContext);
                }
#endif

                if (checkedExtOIDsList != NULL) {
                 
                        PKIX_CHECK(pkix_List_RemoveItems
                                (unresCritExtOIDs,
                                checkedExtOIDsList,
                                plContext),
                                PKIX_LISTREMOVEITEMSFAILED);
                }

                PKIX_CHECK(PKIX_List_GetLength
                        (unresCritExtOIDs, &numUnresCritExtOIDs, plContext),
                        PKIX_LISTGETLENGTHFAILED);

                if (numUnresCritExtOIDs != 0){
                        PKIX_ERROR(PKIX_UNRECOGNIZEDCRITICALEXTENSION);
                }

        }

cleanup:

        PKIX_DECREF(checker);
        PKIX_DECREF(unresCritExtOIDs);

        PKIX_RETURN(VALIDATE);

}






























static PKIX_Error *
pkix_InitializeCheckers(
        PKIX_TrustAnchor *anchor,
        PKIX_ProcessingParams *procParams,
        PKIX_UInt32 numCerts,
        PKIX_List **pCheckers,
        void *plContext)
{
        PKIX_CertChainChecker *targetCertChecker = NULL;
        PKIX_CertChainChecker *expirationChecker = NULL;
        PKIX_CertChainChecker *nameChainingChecker = NULL;
        PKIX_CertChainChecker *nameConstraintsChecker = NULL;
        PKIX_CertChainChecker *basicConstraintsChecker = NULL;
        PKIX_CertChainChecker *policyChecker = NULL;
        PKIX_CertChainChecker *sigChecker = NULL;
        PKIX_CertChainChecker *defaultCrlChecker = NULL;
        PKIX_CertChainChecker *userChecker = NULL;
        PKIX_PL_X500Name *trustedCAName = NULL;
        PKIX_PL_PublicKey *trustedPubKey = NULL;
        PKIX_List *checkers = NULL;
        PKIX_PL_Date *testDate = NULL;
        PKIX_CertSelector *certSelector = NULL;
        PKIX_PL_Cert *trustedCert = NULL;
        PKIX_PL_CertNameConstraints *trustedNC = NULL;
        PKIX_List *initialPolicies = NULL;
        PKIX_Boolean policyQualifiersRejected = PKIX_FALSE;
        PKIX_Boolean initialPolicyMappingInhibit = PKIX_FALSE;
        PKIX_Boolean initialAnyPolicyInhibit = PKIX_FALSE;
        PKIX_Boolean initialExplicitPolicy = PKIX_FALSE;
        PKIX_List *userCheckersList = NULL;
        PKIX_List *certStores = NULL;
        PKIX_UInt32 numCertCheckers = 0;
        PKIX_UInt32 i;

        PKIX_ENTER(VALIDATE, "pkix_InitializeCheckers");
        PKIX_NULLCHECK_THREE(anchor, procParams, pCheckers);
        PKIX_CHECK(PKIX_List_Create(&checkers, plContext),
                    PKIX_LISTCREATEFAILED);

        







        PKIX_CHECK(PKIX_TrustAnchor_GetTrustedCert
                (anchor, &trustedCert, plContext),
                    PKIX_TRUSTANCHORGETTRUSTEDCERTFAILED);

        if (trustedCert){
                PKIX_CHECK(PKIX_PL_Cert_GetSubjectPublicKey
                            (trustedCert, &trustedPubKey, plContext),
                            PKIX_CERTGETSUBJECTPUBLICKEYFAILED);

                PKIX_CHECK(PKIX_PL_Cert_GetSubject
                            (trustedCert, &trustedCAName, plContext),
                            PKIX_CERTGETSUBJECTFAILED);
        } else {
                PKIX_CHECK(PKIX_TrustAnchor_GetCAPublicKey
                            (anchor, &trustedPubKey, plContext),
                            PKIX_TRUSTANCHORGETCAPUBLICKEYFAILED);

                PKIX_CHECK(PKIX_TrustAnchor_GetCAName
                            (anchor, &trustedCAName, plContext),
                            PKIX_TRUSTANCHORGETCANAMEFAILED);
        }

        PKIX_NULLCHECK_TWO(trustedPubKey, trustedCAName);

        PKIX_CHECK(PKIX_TrustAnchor_GetNameConstraints
                (anchor, &trustedNC, plContext),
                PKIX_TRUSTANCHORGETNAMECONSTRAINTSFAILED);

        PKIX_CHECK(PKIX_ProcessingParams_GetTargetCertConstraints
                (procParams, &certSelector, plContext),
                PKIX_PROCESSINGPARAMSGETTARGETCERTCONSTRAINTSFAILED);

        PKIX_CHECK(PKIX_ProcessingParams_GetDate
                (procParams, &testDate, plContext),
                PKIX_PROCESSINGPARAMSGETDATEFAILED);

        PKIX_CHECK(PKIX_ProcessingParams_GetInitialPolicies
                (procParams, &initialPolicies, plContext),
                PKIX_PROCESSINGPARAMSGETINITIALPOLICIESFAILED);

        PKIX_CHECK(PKIX_ProcessingParams_GetPolicyQualifiersRejected
                (procParams, &policyQualifiersRejected, plContext),
                PKIX_PROCESSINGPARAMSGETPOLICYQUALIFIERSREJECTEDFAILED);

        PKIX_CHECK(PKIX_ProcessingParams_IsPolicyMappingInhibited
                (procParams, &initialPolicyMappingInhibit, plContext),
                PKIX_PROCESSINGPARAMSISPOLICYMAPPINGINHIBITEDFAILED);

        PKIX_CHECK(PKIX_ProcessingParams_IsAnyPolicyInhibited
                (procParams, &initialAnyPolicyInhibit, plContext),
                PKIX_PROCESSINGPARAMSISANYPOLICYINHIBITEDFAILED);

        PKIX_CHECK(PKIX_ProcessingParams_IsExplicitPolicyRequired
                (procParams, &initialExplicitPolicy, plContext),
                PKIX_PROCESSINGPARAMSISEXPLICITPOLICYREQUIREDFAILED);

        PKIX_CHECK(PKIX_ProcessingParams_GetCertStores
                (procParams, &certStores, plContext),
                PKIX_PROCESSINGPARAMSGETCERTSTORESFAILED);

        PKIX_CHECK(PKIX_ProcessingParams_GetCertChainCheckers
                (procParams, &userCheckersList, plContext),
                PKIX_PROCESSINGPARAMSGETCERTCHAINCHECKERSFAILED);

        
        PKIX_CHECK(pkix_TargetCertChecker_Initialize
                (certSelector, numCerts, &targetCertChecker, plContext),
                PKIX_TARGETCERTCHECKERINITIALIZEFAILED);

        PKIX_CHECK(pkix_ExpirationChecker_Initialize
                (testDate, &expirationChecker, plContext),
                PKIX_EXPIRATIONCHECKERINITIALIZEFAILED);

        PKIX_CHECK(pkix_NameChainingChecker_Initialize
                (trustedCAName, &nameChainingChecker, plContext),
                PKIX_NAMECHAININGCHECKERINITIALIZEFAILED);

        PKIX_CHECK(pkix_NameConstraintsChecker_Initialize
                (trustedNC, numCerts, &nameConstraintsChecker, plContext),
                PKIX_NAMECONSTRAINTSCHECKERINITIALIZEFAILED);

        PKIX_CHECK(pkix_BasicConstraintsChecker_Initialize
                (numCerts, &basicConstraintsChecker, plContext),
                PKIX_BASICCONSTRAINTSCHECKERINITIALIZEFAILED);

        PKIX_CHECK(pkix_PolicyChecker_Initialize
                (initialPolicies,
                policyQualifiersRejected,
                initialPolicyMappingInhibit,
                initialExplicitPolicy,
                initialAnyPolicyInhibit,
                numCerts,
                &policyChecker,
                plContext),
                PKIX_POLICYCHECKERINITIALIZEFAILED);

        PKIX_CHECK(pkix_SignatureChecker_Initialize
                    (trustedPubKey, numCerts, &sigChecker, plContext),
                    PKIX_SIGNATURECHECKERINITIALIZEFAILED);

        if (userCheckersList != NULL) {

                PKIX_CHECK(PKIX_List_GetLength
                    (userCheckersList, &numCertCheckers, plContext),
                    PKIX_LISTGETLENGTHFAILED);

                for (i = 0; i < numCertCheckers; i++) {

                        PKIX_CHECK(PKIX_List_GetItem
                            (userCheckersList,
                            i,
                            (PKIX_PL_Object **) &userChecker,
                            plContext),
                            PKIX_LISTGETITEMFAILED);

                        PKIX_CHECK(PKIX_List_AppendItem
                            (checkers,
                            (PKIX_PL_Object *)userChecker,
                            plContext),
                            PKIX_LISTAPPENDITEMFAILED);

                        PKIX_DECREF(userChecker);
                }
        }

        PKIX_CHECK(PKIX_List_AppendItem
            (checkers, (PKIX_PL_Object *)targetCertChecker, plContext),
            PKIX_LISTAPPENDITEMFAILED);

        PKIX_CHECK(PKIX_List_AppendItem
            (checkers, (PKIX_PL_Object *)expirationChecker, plContext),
            PKIX_LISTAPPENDITEMFAILED);

        PKIX_CHECK(PKIX_List_AppendItem
            (checkers, (PKIX_PL_Object *)nameChainingChecker, plContext),
            PKIX_LISTAPPENDITEMFAILED);

        PKIX_CHECK(PKIX_List_AppendItem
            (checkers, (PKIX_PL_Object *)nameConstraintsChecker, plContext),
            PKIX_LISTAPPENDITEMFAILED);

        PKIX_CHECK(PKIX_List_AppendItem
            (checkers, (PKIX_PL_Object *)basicConstraintsChecker, plContext),
            PKIX_LISTAPPENDITEMFAILED);

        PKIX_CHECK(PKIX_List_AppendItem
            (checkers, (PKIX_PL_Object *)policyChecker, plContext),
            PKIX_LISTAPPENDITEMFAILED);

        PKIX_CHECK(PKIX_List_AppendItem
            (checkers, (PKIX_PL_Object *)sigChecker, plContext),
            PKIX_LISTAPPENDITEMFAILED);

        *pCheckers = checkers;

cleanup:

        if (PKIX_ERROR_RECEIVED){
                PKIX_DECREF(checkers);
        }

        PKIX_DECREF(certSelector);
        PKIX_DECREF(testDate);
        PKIX_DECREF(initialPolicies);
        PKIX_DECREF(targetCertChecker);
        PKIX_DECREF(expirationChecker);
        PKIX_DECREF(nameChainingChecker);
        PKIX_DECREF(nameConstraintsChecker);
        PKIX_DECREF(basicConstraintsChecker);
        PKIX_DECREF(policyChecker);
        PKIX_DECREF(sigChecker);
        PKIX_DECREF(trustedCAName);
        PKIX_DECREF(trustedPubKey);
        PKIX_DECREF(trustedNC);
        PKIX_DECREF(trustedCert);
        PKIX_DECREF(defaultCrlChecker);
        PKIX_DECREF(userCheckersList);
        PKIX_DECREF(certStores);
        PKIX_DECREF(userChecker);

        PKIX_RETURN(VALIDATE);
}


























static PKIX_Error *
pkix_RetrieveOutputs(
        PKIX_List *checkers,
        PKIX_PL_PublicKey **pFinalSubjPubKey,
        PKIX_PolicyNode **pPolicyTree,
        void *plContext)
{
        PKIX_PL_PublicKey *finalSubjPubKey = NULL;
        PKIX_PolicyNode *validPolicyTree = NULL;
        PKIX_CertChainChecker *checker = NULL;
        PKIX_PL_Object *state = NULL;
        PKIX_UInt32 numCheckers = 0;
        PKIX_UInt32 type;
        PKIX_Int32 j;

        PKIX_ENTER(VALIDATE, "pkix_RetrieveOutputs");

        PKIX_NULLCHECK_TWO(checkers, pPolicyTree);

        






        PKIX_CHECK(PKIX_List_GetLength(checkers, &numCheckers, plContext),
                PKIX_LISTGETLENGTHFAILED);

        for (j = numCheckers - 1; j >= 0; j--){
                PKIX_CHECK(PKIX_List_GetItem
                        (checkers, j, (PKIX_PL_Object **)&checker, plContext),
                        PKIX_LISTGETITEMFAILED);

                PKIX_CHECK(PKIX_CertChainChecker_GetCertChainCheckerState
                        (checker, &state, plContext),
                        PKIX_CERTCHAINCHECKERGETCERTCHAINCHECKERSTATEFAILED);

                
                if (state != NULL) {

                    PKIX_CHECK(PKIX_PL_Object_GetType(state, &type, plContext),
                            PKIX_OBJECTGETTYPEFAILED);

                    if (type == PKIX_SIGNATURECHECKERSTATE_TYPE){
                        
                        finalSubjPubKey =
                            ((pkix_SignatureCheckerState *)state)->
                                prevPublicKey;
                        PKIX_INCREF(finalSubjPubKey);
                        *pFinalSubjPubKey = finalSubjPubKey;
                    }

                    if (type == PKIX_CERTPOLICYCHECKERSTATE_TYPE) {
                        validPolicyTree =
                            ((PKIX_PolicyCheckerState *)state)->validPolicyTree;
                        break;
                    }
                }

                PKIX_DECREF(checker);
                PKIX_DECREF(state);
        }

        PKIX_INCREF(validPolicyTree);
        *pPolicyTree = validPolicyTree;

cleanup:

        PKIX_DECREF(checker);
        PKIX_DECREF(state);

        PKIX_RETURN(VALIDATE);

}

















































































PKIX_Error *
pkix_CheckChain(
        PKIX_List *certs,
        PKIX_UInt32 numCerts,
        PKIX_TrustAnchor *anchor,
        PKIX_List *checkers,
        PKIX_RevocationChecker *revChecker,
        PKIX_List *removeCheckedExtOIDs,
        PKIX_ProcessingParams *procParams,
        PKIX_UInt32 *pCertCheckedIndex,
        PKIX_UInt32 *pCheckerIndex,
        PKIX_Boolean *pRevChecking,
        PKIX_UInt32 *pReasonCode,
        void **pNBIOContext,
        PKIX_PL_PublicKey **pFinalSubjPubKey,
        PKIX_PolicyNode **pPolicyTree,
        PKIX_VerifyNode **pVerifyTree,
        void *plContext)
{
        PKIX_UInt32 j = 0;
        PKIX_Boolean revChecking = PKIX_FALSE;
        PKIX_Error *checkCertError = NULL;
        void *nbioContext = NULL;
        PKIX_PL_Cert *cert = NULL;
        PKIX_PL_Cert *issuer = NULL;

        PKIX_ENTER(VALIDATE, "pkix_CheckChain");
        PKIX_NULLCHECK_FOUR(certs, checkers, revChecker, pCertCheckedIndex);
        PKIX_NULLCHECK_FOUR(pCheckerIndex, pRevChecking, pReasonCode, anchor);
        PKIX_NULLCHECK_THREE(pNBIOContext, pFinalSubjPubKey, pPolicyTree);

        nbioContext = *pNBIOContext;
        *pNBIOContext = NULL;
        revChecking = *pRevChecking;

        PKIX_CHECK(PKIX_TrustAnchor_GetTrustedCert
                (anchor, &cert, plContext),
                   PKIX_TRUSTANCHORGETTRUSTEDCERTFAILED);
        
        for (j = *pCertCheckedIndex; j < numCerts; j++) {

                PORT_Assert(cert);
                PKIX_DECREF(issuer);
                issuer = cert;
                cert = NULL;

                PKIX_CHECK(PKIX_List_GetItem(
                               certs, j, (PKIX_PL_Object **)&cert, plContext),
                           PKIX_LISTGETITEMFAILED);
                
                
                PORT_Assert(cert);
                if (cert == NULL) {
                    continue;
                }

                if (revChecking == PKIX_FALSE) {

                        PKIX_CHECK(pkix_CheckCert
                                (cert,
                                checkers,
                                removeCheckedExtOIDs,
                                pCheckerIndex,
                                &nbioContext,
                                plContext),
                                PKIX_CHECKCERTFAILED);

                        if (nbioContext != NULL) {
                                *pCertCheckedIndex = j;
                                *pRevChecking = revChecking;
                                *pNBIOContext = nbioContext;
                                goto cleanup;
                        }

                        revChecking = PKIX_TRUE;
                        *pCheckerIndex = 0;
                }

                if (revChecking == PKIX_TRUE) {
                        PKIX_RevocationStatus revStatus;
                        pkixErrorResult =
                            PKIX_RevocationChecker_Check(
                                      cert, issuer, revChecker,
                                      procParams, PKIX_TRUE,
                                      (j == numCerts - 1) ? PKIX_TRUE : PKIX_FALSE,
                                      &revStatus, pReasonCode,
                                      &nbioContext, plContext);
                        if (nbioContext != NULL) {
                                *pCertCheckedIndex = j;
                                *pRevChecking = revChecking;
                                *pNBIOContext = nbioContext;
                                goto cleanup;
                        }
                        if (revStatus == PKIX_RevStatus_Revoked ||
                            pkixErrorResult) {
                            if (!pkixErrorResult) {
                                


                                PKIX_ERROR_CREATE(VALIDATE,
                                                  PKIX_CERTIFICATEREVOKED,
                                                  pkixErrorResult);
                            }
                            goto cleanup;
                        }
                        revChecking = PKIX_FALSE;
                        *pCheckerIndex = 0;
                }

                PKIX_CHECK(pkix_AddToVerifyLog
                        (cert, j, NULL, pVerifyTree, plContext),
                        PKIX_ADDTOVERIFYLOGFAILED);
        }

        PKIX_CHECK(pkix_RetrieveOutputs
                    (checkers, pFinalSubjPubKey, pPolicyTree, plContext),
                    PKIX_RETRIEVEOUTPUTSFAILED);

        *pNBIOContext = NULL;

cleanup:
        if (PKIX_ERROR_RECEIVED && cert) {
            checkCertError = pkixErrorResult;
            
            PKIX_CHECK_FATAL(
                pkix_AddToVerifyLog(cert, j, checkCertError, pVerifyTree,
                                    plContext),
                PKIX_ADDTOVERIFYLOGFAILED);
            pkixErrorResult = checkCertError;
            pkixErrorCode = pkixErrorResult->errCode;
            checkCertError = NULL;
        }

fatal:
        PKIX_DECREF(checkCertError);
        PKIX_DECREF(cert);
        PKIX_DECREF(issuer);

        PKIX_RETURN(VALIDATE);
}





































static PKIX_Error *
pkix_ExtractParameters(
        PKIX_ValidateParams *valParams,
        PKIX_List **pCerts,
        PKIX_UInt32 *pNumCerts,
        PKIX_ProcessingParams **pProcParams,
        PKIX_List **pAnchors,
        PKIX_UInt32 *pNumAnchors,
        void *plContext)
{
        PKIX_ENTER(VALIDATE, "pkix_ExtractParameters");
        PKIX_NULLCHECK_THREE(valParams, pCerts, pNumCerts);
        PKIX_NULLCHECK_THREE(pProcParams, pAnchors, pNumAnchors);

        
        PKIX_CHECK(PKIX_ValidateParams_GetCertChain
                (valParams, pCerts, plContext),
                PKIX_VALIDATEPARAMSGETCERTCHAINFAILED);

        PKIX_CHECK(PKIX_List_GetLength(*pCerts, pNumCerts, plContext),
                PKIX_LISTGETLENGTHFAILED);

        
        PKIX_CHECK(PKIX_ValidateParams_GetProcessingParams
                (valParams, pProcParams, plContext),
                PKIX_VALIDATEPARAMSGETPROCESSINGPARAMSFAILED);

        PKIX_CHECK(PKIX_ProcessingParams_GetTrustAnchors
                (*pProcParams, pAnchors, plContext),
                PKIX_PROCESSINGPARAMSGETTRUSTANCHORSFAILED);

        PKIX_CHECK(PKIX_List_GetLength(*pAnchors, pNumAnchors, plContext),
                PKIX_LISTGETLENGTHFAILED);

cleanup:

        PKIX_RETURN(VALIDATE);
}






PKIX_Error *
PKIX_ValidateChain(
        PKIX_ValidateParams *valParams,
        PKIX_ValidateResult **pResult,
        PKIX_VerifyNode **pVerifyTree,
        void *plContext)
{
        PKIX_Error *chainFailed = NULL;

        PKIX_ProcessingParams *procParams = NULL;
        PKIX_CertChainChecker *userChecker = NULL;
        PKIX_RevocationChecker *revChecker = NULL;
        PKIX_List *certs = NULL;
        PKIX_List *checkers = NULL;
        PKIX_List *anchors = NULL;
        PKIX_List *userCheckers = NULL;
        PKIX_List *userCheckerExtOIDs = NULL;
        PKIX_List *validateCheckedCritExtOIDsList = NULL;
        PKIX_TrustAnchor *anchor = NULL;
        PKIX_ValidateResult *valResult = NULL;
        PKIX_PL_PublicKey *finalPubKey = NULL;
        PKIX_PolicyNode *validPolicyTree = NULL;
        PKIX_Boolean supportForwarding = PKIX_FALSE;
        PKIX_Boolean revChecking = PKIX_FALSE;
        PKIX_UInt32 i, numCerts, numAnchors;
        PKIX_UInt32 numUserCheckers = 0;
        PKIX_UInt32 certCheckedIndex = 0;
        PKIX_UInt32 checkerIndex = 0;
        PKIX_UInt32 reasonCode = 0;
        void *nbioContext = NULL;

        PKIX_ENTER(VALIDATE, "PKIX_ValidateChain");
        PKIX_NULLCHECK_TWO(valParams, pResult);

        
        PKIX_CHECK(pkix_ExtractParameters
                    (valParams,
                    &certs,
                    &numCerts,
                    &procParams,
                    &anchors,
                    &numAnchors,
                    plContext),
                    PKIX_EXTRACTPARAMETERSFAILED);

        






        PKIX_CHECK(PKIX_ProcessingParams_GetCertChainCheckers
                    (procParams, &userCheckers, plContext),
                    PKIX_PROCESSINGPARAMSGETCERTCHAINCHECKERSFAILED);

        if (userCheckers != NULL) {

                PKIX_CHECK(PKIX_List_Create
                    (&validateCheckedCritExtOIDsList,
                    plContext),
                    PKIX_LISTCREATEFAILED);

                PKIX_CHECK(PKIX_List_GetLength
                    (userCheckers, &numUserCheckers, plContext),
                    PKIX_LISTGETLENGTHFAILED);

                for (i = 0; i < numUserCheckers; i++) {

                    PKIX_CHECK(PKIX_List_GetItem
                        (userCheckers,
                        i,
                        (PKIX_PL_Object **) &userChecker,
                        plContext),
                        PKIX_LISTGETITEMFAILED);

                    PKIX_CHECK
                        (PKIX_CertChainChecker_IsForwardCheckingSupported
                        (userChecker, &supportForwarding, plContext),
                        PKIX_CERTCHAINCHECKERISFORWARDCHECKINGSUPPORTEDFAILED);

                    if (supportForwarding == PKIX_FALSE) {

                        PKIX_CHECK
                            (PKIX_CertChainChecker_GetSupportedExtensions
                            (userChecker, &userCheckerExtOIDs, plContext),
                            PKIX_CERTCHAINCHECKERGETSUPPORTEDEXTENSIONSFAILED);

                        if (userCheckerExtOIDs != NULL) {
                            PKIX_CHECK(pkix_List_AppendList
                                (validateCheckedCritExtOIDsList,
                                userCheckerExtOIDs,
                                plContext),
                                PKIX_LISTAPPENDLISTFAILED);
                        }
                    }

                    PKIX_DECREF(userCheckerExtOIDs);
                    PKIX_DECREF(userChecker);
                }
        }

        PKIX_CHECK(PKIX_ProcessingParams_GetRevocationChecker
                (procParams, &revChecker, plContext),
                PKIX_PROCESSINGPARAMSGETREVOCATIONCHECKERFAILED);

        
        for (i = 0; i < numAnchors; i++){

                
                PKIX_CHECK(PKIX_List_GetItem
                        (anchors, i, (PKIX_PL_Object **)&anchor, plContext),
                        PKIX_LISTGETITEMFAILED);

                
                PKIX_CHECK(pkix_InitializeCheckers
                        (anchor, procParams, numCerts, &checkers, plContext),
                        PKIX_INITIALIZECHECKERSFAILED);

                




                certCheckedIndex = 0;
                checkerIndex = 0;
                revChecking = PKIX_FALSE;
                chainFailed = pkix_CheckChain
                        (certs,
                        numCerts,
                        anchor,
                        checkers,
                        revChecker,
                        validateCheckedCritExtOIDsList,
                        procParams,
                        &certCheckedIndex,
                        &checkerIndex,
                        &revChecking,
                        &reasonCode,
                        &nbioContext,
                        &finalPubKey,
                        &validPolicyTree,
                        pVerifyTree,
                        plContext);

                if (chainFailed || (reasonCode != 0)) {

                        

                        PKIX_DECREF(chainFailed);
                        PKIX_DECREF(anchor);
                        PKIX_DECREF(checkers);
                        PKIX_DECREF(validPolicyTree);

                        
                        if (i == (numAnchors - 1)) { 
                                PKIX_ERROR(PKIX_VALIDATECHAINFAILED);
                        }

                } else {

                        
                        PKIX_CHECK(pkix_ValidateResult_Create
                                (finalPubKey,
                                anchor,
                                validPolicyTree,
                                &valResult,
                                plContext),
                                PKIX_VALIDATERESULTCREATEFAILED);

                        *pResult = valResult;

                        
                        goto cleanup;
                }
        }

cleanup:

        PKIX_DECREF(finalPubKey);
        PKIX_DECREF(certs);
        PKIX_DECREF(anchors);
        PKIX_DECREF(anchor);
        PKIX_DECREF(checkers);
        PKIX_DECREF(revChecker);
        PKIX_DECREF(validPolicyTree);
        PKIX_DECREF(chainFailed);
        PKIX_DECREF(procParams);
        PKIX_DECREF(userCheckers);
        PKIX_DECREF(validateCheckedCritExtOIDsList);

        PKIX_RETURN(VALIDATE);
}

























static PKIX_Error *
pkix_Validate_BuildUserOIDs(
        PKIX_List *userCheckers,
        PKIX_List **pUserCritOIDs,
        void *plContext)
{
        PKIX_UInt32 numUserCheckers = 0;
        PKIX_UInt32 i = 0;
        PKIX_List *userCritOIDs = NULL;
        PKIX_List *userCheckerExtOIDs = NULL;
        PKIX_Boolean supportForwarding = PKIX_FALSE;
        PKIX_CertChainChecker *userChecker = NULL;

        PKIX_ENTER(VALIDATE, "pkix_Validate_BuildUserOIDs");
        PKIX_NULLCHECK_ONE(pUserCritOIDs);

        if (userCheckers != NULL) {
            PKIX_CHECK(PKIX_List_Create(&userCritOIDs, plContext),
                PKIX_LISTCREATEFAILED);

            PKIX_CHECK(PKIX_List_GetLength
                (userCheckers, &numUserCheckers, plContext),
                PKIX_LISTGETLENGTHFAILED);

            for (i = 0; i < numUserCheckers; i++) {
                PKIX_CHECK(PKIX_List_GetItem
                    (userCheckers,
                    i,
                    (PKIX_PL_Object **) &userChecker,
                    plContext),
                    PKIX_LISTGETITEMFAILED);

                PKIX_CHECK(PKIX_CertChainChecker_IsForwardCheckingSupported
                    (userChecker, &supportForwarding, plContext),
                    PKIX_CERTCHAINCHECKERISFORWARDCHECKINGSUPPORTEDFAILED);

                if (supportForwarding == PKIX_FALSE) {

                    PKIX_CHECK(PKIX_CertChainChecker_GetSupportedExtensions
                        (userChecker, &userCheckerExtOIDs, plContext),
                        PKIX_CERTCHAINCHECKERGETSUPPORTEDEXTENSIONSFAILED);

                    if (userCheckerExtOIDs != NULL) {
                        PKIX_CHECK(pkix_List_AppendList
                            (userCritOIDs, userCheckerExtOIDs, plContext),
                            PKIX_LISTAPPENDLISTFAILED);
                    }
                }

                PKIX_DECREF(userCheckerExtOIDs);
                PKIX_DECREF(userChecker);
            }
        }

        *pUserCritOIDs = userCritOIDs;

cleanup:

        if (PKIX_ERROR_RECEIVED){
                PKIX_DECREF(userCritOIDs);
        }

        PKIX_DECREF(userCheckerExtOIDs);
        PKIX_DECREF(userChecker);

        PKIX_RETURN(VALIDATE);
}




PKIX_Error *
PKIX_ValidateChain_NB(
        PKIX_ValidateParams *valParams,
        PKIX_UInt32 *pCertIndex,
        PKIX_UInt32 *pAnchorIndex,
        PKIX_UInt32 *pCheckerIndex,
        PKIX_Boolean *pRevChecking,
        PKIX_List **pCheckers,
        void **pNBIOContext,
        PKIX_ValidateResult **pResult,
        PKIX_VerifyNode **pVerifyTree,
        void *plContext)
{
        PKIX_UInt32 numCerts = 0;
        PKIX_UInt32 numAnchors = 0;
        PKIX_UInt32 i = 0;
        PKIX_UInt32 certIndex = 0;
        PKIX_UInt32 anchorIndex = 0;
        PKIX_UInt32 checkerIndex = 0;
        PKIX_UInt32 reasonCode = 0;
        PKIX_Boolean revChecking = PKIX_FALSE;
        PKIX_List *certs = NULL;
        PKIX_List *anchors = NULL;
        PKIX_List *checkers = NULL;
        PKIX_List *userCheckers = NULL;
        PKIX_List *validateCheckedCritExtOIDsList = NULL;
        PKIX_TrustAnchor *anchor = NULL;
        PKIX_ValidateResult *valResult = NULL;
        PKIX_PL_PublicKey *finalPubKey = NULL;
        PKIX_PolicyNode *validPolicyTree = NULL;
        PKIX_ProcessingParams *procParams = NULL;
        PKIX_RevocationChecker *revChecker = NULL;
        PKIX_Error *chainFailed = NULL;
        void *nbioContext = NULL;

        PKIX_ENTER(VALIDATE, "PKIX_ValidateChain_NB");
        PKIX_NULLCHECK_FOUR
                (valParams, pCertIndex, pAnchorIndex, pCheckerIndex);
        PKIX_NULLCHECK_FOUR(pRevChecking, pCheckers, pNBIOContext, pResult);

        nbioContext = *pNBIOContext;
        *pNBIOContext = NULL;

        
        PKIX_CHECK(pkix_ExtractParameters
                    (valParams,
                    &certs,
                    &numCerts,
                    &procParams,
                    &anchors,
                    &numAnchors,
                    plContext),
                    PKIX_EXTRACTPARAMETERSFAILED);

        






        PKIX_CHECK(PKIX_ProcessingParams_GetCertChainCheckers
                (procParams, &userCheckers, plContext),
                PKIX_PROCESSINGPARAMSGETCERTCHAINCHECKERSFAILED);

        PKIX_CHECK(pkix_Validate_BuildUserOIDs
                (userCheckers, &validateCheckedCritExtOIDsList, plContext),
                PKIX_VALIDATEBUILDUSEROIDSFAILED);

        PKIX_CHECK(PKIX_ProcessingParams_GetRevocationChecker
                (procParams, &revChecker, plContext),
                PKIX_PROCESSINGPARAMSGETREVOCATIONCHECKERFAILED);

        
        if (nbioContext != NULL) {
                
                certIndex = *pCertIndex;
                anchorIndex = *pAnchorIndex;
                checkerIndex = *pCheckerIndex;
                revChecking = *pRevChecking;
                checkers = *pCheckers;
                *pCheckers = NULL;
        }

        
        for (i = anchorIndex; i < numAnchors; i++) {

                
                PKIX_CHECK(PKIX_List_GetItem
                        (anchors, i, (PKIX_PL_Object **)&anchor, plContext),
                        PKIX_LISTGETITEMFAILED);

                
                if (nbioContext == NULL) {
                        PKIX_CHECK(pkix_InitializeCheckers
                                (anchor,
                                procParams,
                                numCerts,
                                &checkers,
                                plContext),
                                PKIX_INITIALIZECHECKERSFAILED);
                }

                



                chainFailed = pkix_CheckChain
                        (certs,
                        numCerts,
                        anchor,
                        checkers,
                        revChecker,
                        validateCheckedCritExtOIDsList,
                        procParams,
                        &certIndex,
                        &checkerIndex,
                        &revChecking,
                        &reasonCode,
                        &nbioContext,
                        &finalPubKey,
                        &validPolicyTree,
                        pVerifyTree,
                        plContext);

                if (nbioContext != NULL) {
                        *pCertIndex = certIndex;
                        *pAnchorIndex = anchorIndex;
                        *pCheckerIndex = checkerIndex;
                        *pRevChecking = revChecking;
                        PKIX_INCREF(checkers);
                        *pCheckers = checkers;
                        *pNBIOContext = nbioContext;
                        goto cleanup;
                }

                if (chainFailed || (reasonCode != 0)) {

                        

                        PKIX_DECREF(chainFailed);
                        PKIX_DECREF(anchor);
                        PKIX_DECREF(checkers);
                        PKIX_DECREF(validPolicyTree);

                        
                        if (i == (numAnchors - 1)) { 
                                PKIX_ERROR(PKIX_VALIDATECHAINFAILED);
                        }

                } else {

                        
                        PKIX_CHECK(pkix_ValidateResult_Create
                                (finalPubKey,
                                anchor,
                                validPolicyTree,
                                &valResult,
                                plContext),
                                PKIX_VALIDATERESULTCREATEFAILED);

                        *pResult = valResult;

                        
                        goto cleanup;
                }
        }

cleanup:

        PKIX_DECREF(finalPubKey);
        PKIX_DECREF(certs);
        PKIX_DECREF(anchors);
        PKIX_DECREF(anchor);
        PKIX_DECREF(checkers);
        PKIX_DECREF(revChecker);
        PKIX_DECREF(validPolicyTree);
        PKIX_DECREF(chainFailed);
        PKIX_DECREF(procParams);
        PKIX_DECREF(userCheckers);
        PKIX_DECREF(validateCheckedCritExtOIDsList);

        PKIX_RETURN(VALIDATE);
}
