










































#include "pkix_signaturechecker.h"





static PKIX_Error *
pkix_SignatureCheckerState_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        pkix_SignatureCheckerState *state = NULL;

        PKIX_ENTER(SIGNATURECHECKERSTATE,
                    "pkix_SignatureCheckerState_Destroy");
        PKIX_NULLCHECK_ONE(object);

        
        PKIX_CHECK(pkix_CheckType
                    (object, PKIX_SIGNATURECHECKERSTATE_TYPE, plContext),
                    PKIX_OBJECTNOTSIGNATURECHECKERSTATE);

        state = (pkix_SignatureCheckerState *) object;

        state->prevCertCertSign = PKIX_FALSE;

        PKIX_DECREF(state->prevPublicKey);
        PKIX_DECREF(state->prevPublicKeyList);
        PKIX_DECREF(state->keyUsageOID);

cleanup:

        PKIX_RETURN(SIGNATURECHECKERSTATE);
}















PKIX_Error *
pkix_SignatureCheckerState_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(SIGNATURECHECKERSTATE,
                    "pkix_SignatureCheckerState_RegisterSelf");

        entry.description = "SignatureCheckerState";
        entry.objCounter = 0;
        entry.typeObjectSize = sizeof(pkix_SignatureCheckerState);
        entry.destructor = pkix_SignatureCheckerState_Destroy;
        entry.equalsFunction = NULL;
        entry.hashcodeFunction = NULL;
        entry.toStringFunction = NULL;
        entry.comparator = NULL;
        entry.duplicateFunction = NULL;

        systemClasses[PKIX_SIGNATURECHECKERSTATE_TYPE] = entry;

        PKIX_RETURN(SIGNATURECHECKERSTATE);
}



























static PKIX_Error *
pkix_SignatureCheckerState_Create(
    PKIX_PL_PublicKey *trustedPubKey,
    PKIX_UInt32 certsRemaining,
    pkix_SignatureCheckerState **pCheckerState,
    void *plContext)
{
        pkix_SignatureCheckerState *state = NULL;
        PKIX_PL_OID *keyUsageOID = NULL;

        PKIX_ENTER(SIGNATURECHECKERSTATE, "pkix_SignatureCheckerState_Create");
        PKIX_NULLCHECK_TWO(trustedPubKey, pCheckerState);

        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_SIGNATURECHECKERSTATE_TYPE,
                    sizeof (pkix_SignatureCheckerState),
                    (PKIX_PL_Object **)&state,
                    plContext),
                    PKIX_COULDNOTCREATESIGNATURECHECKERSTATEOBJECT);

        

        state->prevCertCertSign = PKIX_TRUE;
        state->prevPublicKeyList = NULL;
        state->certsRemaining = certsRemaining;

        PKIX_INCREF(trustedPubKey);
        state->prevPublicKey = trustedPubKey;

        PKIX_CHECK(PKIX_PL_OID_Create
                    (PKIX_CERTKEYUSAGE_OID,
                    &keyUsageOID,
                    plContext),
                    PKIX_OIDCREATEFAILED);

        state->keyUsageOID = keyUsageOID;
        keyUsageOID = NULL;

        *pCheckerState = state;
        state = NULL;

cleanup:

        PKIX_DECREF(keyUsageOID);
        PKIX_DECREF(state); 

        PKIX_RETURN(SIGNATURECHECKERSTATE);
}







PKIX_Error *
pkix_SignatureChecker_Check(
        PKIX_CertChainChecker *checker,
        PKIX_PL_Cert *cert,
        PKIX_List *unresolvedCriticalExtensions,
        void **pNBIOContext,
        void *plContext)
{
        pkix_SignatureCheckerState *state = NULL;
        PKIX_PL_PublicKey *prevPubKey = NULL;
        PKIX_PL_PublicKey *currPubKey = NULL;
        PKIX_PL_PublicKey *newPubKey = NULL;
        PKIX_PL_PublicKey *pKey = NULL;
        PKIX_PL_CertBasicConstraints *basicConstraints = NULL;
        PKIX_Error *checkKeyUsageFail = NULL;
        PKIX_Error *verifyFail = NULL;
        PKIX_Boolean certVerified = PKIX_FALSE;

        PKIX_ENTER(CERTCHAINCHECKER, "pkix_SignatureChecker_Check");
        PKIX_NULLCHECK_THREE(checker, cert, pNBIOContext);

        *pNBIOContext = NULL; 

        PKIX_CHECK(PKIX_CertChainChecker_GetCertChainCheckerState
                    (checker, (PKIX_PL_Object **)&state, plContext),
                    PKIX_CERTCHAINCHECKERGETCERTCHAINCHECKERSTATEFAILED);

        (state->certsRemaining)--;

        PKIX_INCREF(state->prevPublicKey);
        prevPubKey = state->prevPublicKey;

        




        if (state->prevCertCertSign == PKIX_FALSE &&
                state->prevPublicKeyList == NULL) {
                    PKIX_ERROR(PKIX_KEYUSAGEKEYCERTSIGNBITNOTON);
        }

        
        if (state->prevCertCertSign == PKIX_TRUE) {
                verifyFail = PKIX_PL_Cert_VerifySignature
                        (cert, prevPubKey, plContext);
                if (verifyFail == NULL) {
                        certVerified = PKIX_TRUE;
                } else {
                        certVerified = PKIX_FALSE;
                        PKIX_DECREF(verifyFail);
                }
        }

#ifdef NIST_TEST_4_5_4_AND_4_5_6

        























        
        if (certVerified == PKIX_FALSE && state->prevPublicKeyList != NULL) {

                
                PKIX_CHECK(PKIX_List_GetLength
                        (state->prevPublicKeyList, &numKeys, plContext),
                        PKIX_LISTGETLENGTHFAILED);

                for (i = numKeys - 1; i >= 0; i--) {

                        PKIX_CHECK(PKIX_List_GetItem
                                (state->prevPublicKeyList,
                                i,
                                (PKIX_PL_Object **) &pKey,
                                plContext),
                                PKIX_LISTGETITEMFAILED);

                        verifyFail = PKIX_PL_Cert_VerifySignature
                                (cert, pKey, plContext);

                        if (verifyFail == NULL) {
                                certVerified = PKIX_TRUE;
                                break;
                        } else {
                                certVerified = PKIX_FALSE;
                                PKIX_DECREF(verifyFail);
                        }

                        PKIX_DECREF(pKey);
                }
        }
#endif

        if (certVerified == PKIX_FALSE) {
                PKIX_ERROR(PKIX_VALIDATIONFAILEDCERTSIGNATURECHECKING);
        }

#ifdef NIST_TEST_4_5_4_AND_4_5_6
        



        PKIX_CHECK(pkix_IsCertSelfIssued(cert, &selfIssued, plContext),
                    PKIX_ISCERTSELFISSUEFAILED);

        








        if (selfIssued == PKIX_TRUE) {

            
            if (state->prevCertCertSign == PKIX_TRUE) {

                if (state->prevPublicKeyList == NULL) {

                        PKIX_CHECK(PKIX_List_Create
                                (&state->prevPublicKeyList, plContext),
                                PKIX_LISTCREATEFALIED);

                }

                PKIX_CHECK(PKIX_List_AppendItem
                            (state->prevPublicKeyList,
                            (PKIX_PL_Object *) state->prevPublicKey,
                            plContext),
                            PKIX_LISTAPPENDITEMFAILED);
            }

        } else {
            
            PKIX_DECREF(state->prevPublicKeyList);
        }
#endif

        
        PKIX_CHECK(PKIX_PL_Cert_GetSubjectPublicKey
                    (cert, &currPubKey, plContext),
                    PKIX_CERTGETSUBJECTPUBLICKEYFAILED);

        PKIX_CHECK(PKIX_PL_PublicKey_MakeInheritedDSAPublicKey
                    (currPubKey, prevPubKey, &newPubKey, plContext),
                    PKIX_PUBLICKEYMAKEINHERITEDDSAPUBLICKEYFAILED);

        if (newPubKey == NULL){
                PKIX_INCREF(currPubKey);
                newPubKey = currPubKey;
        }

        PKIX_INCREF(newPubKey);
        PKIX_DECREF(state->prevPublicKey);

        state->prevPublicKey = newPubKey;

        
        if (state->certsRemaining != 0) {
                checkKeyUsageFail = PKIX_PL_Cert_VerifyKeyUsage
                        (cert, PKIX_KEY_CERT_SIGN, plContext);

                state->prevCertCertSign = (checkKeyUsageFail == NULL)?
                        PKIX_TRUE:PKIX_FALSE;

                PKIX_DECREF(checkKeyUsageFail);
        }

        
        if (unresolvedCriticalExtensions != NULL) {

                PKIX_CHECK(pkix_List_Remove
                            (unresolvedCriticalExtensions,
                            (PKIX_PL_Object *) state->keyUsageOID,
                            plContext),
                            PKIX_LISTREMOVEFAILED);
        }

        PKIX_CHECK(PKIX_CertChainChecker_SetCertChainCheckerState
                    (checker, (PKIX_PL_Object *)state, plContext),
                    PKIX_CERTCHAINCHECKERSETCERTCHAINCHECKERSTATEFAILED);

cleanup:

        PKIX_DECREF(state);
        PKIX_DECREF(pKey);
        PKIX_DECREF(prevPubKey);
        PKIX_DECREF(currPubKey);
        PKIX_DECREF(newPubKey);
        PKIX_DECREF(basicConstraints);
        PKIX_DECREF(verifyFail);
        PKIX_DECREF(checkKeyUsageFail);

        PKIX_RETURN(CERTCHAINCHECKER);

}




























PKIX_Error *
pkix_SignatureChecker_Initialize(
        PKIX_PL_PublicKey *trustedPubKey,
        PKIX_UInt32 certsRemaining,
        PKIX_CertChainChecker **pChecker,
        void *plContext)
{
        pkix_SignatureCheckerState* state = NULL;
        PKIX_ENTER(CERTCHAINCHECKER, "PKIX_SignatureChecker_Initialize");
        PKIX_NULLCHECK_TWO(pChecker, trustedPubKey);

        PKIX_CHECK(pkix_SignatureCheckerState_Create
                    (trustedPubKey, certsRemaining, &state, plContext),
                    PKIX_SIGNATURECHECKERSTATECREATEFAILED);

        PKIX_CHECK(PKIX_CertChainChecker_Create
                    (pkix_SignatureChecker_Check,
                    PKIX_FALSE,
                    PKIX_FALSE,
                    NULL,
                    (PKIX_PL_Object *) state,
                    pChecker,
                    plContext),
                    PKIX_CERTCHAINCHECKERCREATEFAILED);

cleanup:

        PKIX_DECREF(state);

        PKIX_RETURN(CERTCHAINCHECKER);

}
