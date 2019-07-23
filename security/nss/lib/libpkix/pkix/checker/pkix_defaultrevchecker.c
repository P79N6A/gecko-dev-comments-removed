










































#include "pkix_defaultrevchecker.h"







static PKIX_Error *
pkix_DefaultRevChecker_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_DefaultRevocationChecker *revChecker = NULL;

        PKIX_ENTER(DEFAULTREVOCATIONCHECKER,
                    "pkix_DefaultRevChecker_Destroy");
        PKIX_NULLCHECK_ONE(object);

        
        PKIX_CHECK(pkix_CheckType
                (object, PKIX_DEFAULTREVOCATIONCHECKER_TYPE, plContext),
                PKIX_OBJECTNOTDEFAULTREVOCATIONCHECKER);

        revChecker = (PKIX_DefaultRevocationChecker *)object;

        PKIX_DECREF(revChecker->certChainChecker);
        PKIX_DECREF(revChecker->certStores);
        PKIX_DECREF(revChecker->testDate);
        PKIX_DECREF(revChecker->trustedPubKey);

cleanup:

        PKIX_RETURN(DEFAULTREVOCATIONCHECKER);
}















PKIX_Error *
pkix_DefaultRevocationChecker_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(DEFAULTREVOCATIONCHECKER,
                    "pkix_DefaultRevocationChecker_RegisterSelf");

        entry.description = "DefaultRevocationChecker";
        entry.objCounter = 0;
        entry.typeObjectSize = sizeof(PKIX_DefaultRevocationChecker);
        entry.destructor = pkix_DefaultRevChecker_Destroy;
        entry.equalsFunction = NULL;
        entry.hashcodeFunction = NULL;
        entry.toStringFunction = NULL;
        entry.comparator = NULL;
        entry.duplicateFunction = NULL;

        systemClasses[PKIX_DEFAULTREVOCATIONCHECKER_TYPE] = entry;

        PKIX_RETURN(DEFAULTREVOCATIONCHECKER);
}

































static PKIX_Error *
pkix_DefaultRevChecker_Create(
        PKIX_List *certStores,
        PKIX_PL_Date *testDate,
        PKIX_PL_PublicKey *trustedPubKey,
        PKIX_UInt32 certsRemaining,
        PKIX_DefaultRevocationChecker **pRevChecker,
        void *plContext)
{
        PKIX_DefaultRevocationChecker *revChecker = NULL;

        PKIX_ENTER(DEFAULTREVOCATIONCHECKER, "pkix_DefaultRevChecker_Create");
        PKIX_NULLCHECK_THREE(certStores, trustedPubKey, pRevChecker);

        PKIX_CHECK(PKIX_PL_Object_Alloc
                (PKIX_DEFAULTREVOCATIONCHECKER_TYPE,
                sizeof (PKIX_DefaultRevocationChecker),
                (PKIX_PL_Object **)&revChecker,
                plContext),
                PKIX_COULDNOTCREATEDEFAULTREVOCATIONCHECKEROBJECT);

        

        revChecker->certChainChecker = NULL;
        revChecker->check = NULL;

        PKIX_INCREF(certStores);
        revChecker->certStores = certStores;

        PKIX_INCREF(testDate);
        revChecker->testDate = testDate;

        PKIX_INCREF(trustedPubKey);
        revChecker->trustedPubKey = trustedPubKey;

        revChecker->certsRemaining = certsRemaining;

        *pRevChecker = revChecker;
        revChecker = NULL;

cleanup:

        PKIX_DECREF(revChecker);

        PKIX_RETURN(DEFAULTREVOCATIONCHECKER);
}




































static PKIX_Error *
pkix_DefaultRevChecker_Check(
        PKIX_PL_Object *checkerContext,
        PKIX_PL_Cert *cert,
        PKIX_ProcessingParams *procParams,
        void **pNBIOContext,
        PKIX_UInt32 *pReasonCode,
        void *plContext)
{
        PKIX_DefaultRevocationChecker *defaultRevChecker = NULL;
        PKIX_CertChainChecker *crlChecker = NULL;
        PKIX_PL_Object *crlCheckerState = NULL;
        PKIX_CertChainChecker_CheckCallback check = NULL;
        void *nbioContext = NULL;

        PKIX_ENTER(REVOCATIONCHECKER, "pkix_DefaultRevChecker_Check");
        PKIX_NULLCHECK_FOUR(checkerContext, cert, pNBIOContext, pReasonCode);

        
        PKIX_CHECK(pkix_CheckType
                ((PKIX_PL_Object *)checkerContext,
                PKIX_DEFAULTREVOCATIONCHECKER_TYPE,
                plContext),
                PKIX_OBJECTNOTDEFAULTREVOCATIONCHECKER);

        defaultRevChecker = (PKIX_DefaultRevocationChecker *)checkerContext;

        nbioContext = *pNBIOContext;
        *pNBIOContext = 0;
        *pReasonCode = 0;

        



        if (defaultRevChecker->certChainChecker == NULL) {
                PKIX_Boolean nistCRLPolicyEnabled = PR_TRUE;
                if (procParams) {
                    PKIX_CHECK(
                        pkix_ProcessingParams_GetNISTRevocationPolicyEnabled
                        (procParams, &nistCRLPolicyEnabled, plContext),
                        PKIX_PROCESSINGPARAMSGETNISTREVPOLICYENABLEDFAILED);
                }

                PKIX_CHECK(pkix_DefaultCRLChecker_Initialize
                        (defaultRevChecker->certStores,
                        defaultRevChecker->testDate,
                        defaultRevChecker->trustedPubKey,
                        defaultRevChecker->certsRemaining,
                        nistCRLPolicyEnabled,
                        &crlChecker,
                        plContext),
                        PKIX_DEFAULTCRLCHECKERINITIALIZEFAILED);

                PKIX_CHECK(PKIX_CertChainChecker_GetCheckCallback
                        (crlChecker, &check, plContext),
                        PKIX_CERTCHAINCHECKERGETCHECKCALLBACKFAILED);

                defaultRevChecker->certChainChecker = crlChecker;
                defaultRevChecker->check = check;
        }

        



        PKIX_CHECK(PKIX_CertChainChecker_GetCertChainCheckerState
                (defaultRevChecker->certChainChecker,
                &crlCheckerState,
                plContext),
                PKIX_CERTCHAINCHECKERGETCERTCHAINCHECKERSTATEFAILED);

        PKIX_CHECK(pkix_CheckType
                (crlCheckerState, PKIX_DEFAULTCRLCHECKERSTATE_TYPE, plContext),
                PKIX_OBJECTNOTDEFAULTCRLCHECKERSTATE);

        
        PKIX_CHECK(pkix_DefaultCRLChecker_Check_SetSelector
                (cert,
                (pkix_DefaultCRLCheckerState *)crlCheckerState,
                plContext),
                PKIX_DEFAULTCRLCHECKERCHECKSETSELECTORFAILED);

        PKIX_CHECK
                (PKIX_CertChainChecker_SetCertChainCheckerState
                (defaultRevChecker->certChainChecker,
                crlCheckerState,
                plContext),
                PKIX_CERTCHAINCHECKERSETCERTCHAINCHECKERSTATEFAILED);

        PKIX_CHECK(defaultRevChecker->check
                (defaultRevChecker->certChainChecker,
                cert,
                NULL,
                &nbioContext,
                plContext),
                PKIX_CERTCHAINCHECKERCHECKCALLBACKFAILED);

        *pNBIOContext = nbioContext;

cleanup:

        PKIX_DECREF(crlCheckerState);

        PKIX_RETURN(REVOCATIONCHECKER);
}






























PKIX_Error *
pkix_DefaultRevChecker_Initialize(
        PKIX_List *certStores,
        PKIX_PL_Date *testDate,
        PKIX_PL_PublicKey *trustedPubKey,
        PKIX_UInt32 certsRemaining,
        PKIX_RevocationChecker **pChecker,
        void *plContext)
{
        PKIX_DefaultRevocationChecker *revChecker = NULL;

        PKIX_ENTER(REVOCATIONCHECKER, "pkix_DefaultRevChecker_Initialize");
        PKIX_NULLCHECK_TWO(certStores, pChecker);

        PKIX_CHECK(pkix_DefaultRevChecker_Create
                (certStores,
                testDate,
                trustedPubKey,
                certsRemaining,
                &revChecker,
                plContext),
                PKIX_DEFAULTREVCHECKERCREATEFAILED);

        PKIX_CHECK(PKIX_RevocationChecker_Create
                (pkix_DefaultRevChecker_Check,
                (PKIX_PL_Object *)revChecker,
                pChecker,
                plContext),
                PKIX_REVOCATIONCHECKERCREATEFAILED);

cleanup:

        PKIX_DECREF(revChecker);

        PKIX_RETURN(REVOCATIONCHECKER);
}
