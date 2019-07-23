










































#include "pkix_ekuchecker.h"

SECOidTag ekuOidStrings[] = {
    PKIX_KEY_USAGE_SERVER_AUTH_OID,
    PKIX_KEY_USAGE_CLIENT_AUTH_OID,
    PKIX_KEY_USAGE_CODE_SIGN_OID,
    PKIX_KEY_USAGE_EMAIL_PROTECT_OID,
    PKIX_KEY_USAGE_TIME_STAMP_OID,
    PKIX_KEY_USAGE_OCSP_RESPONDER_OID,
    PKIX_UNKNOWN_OID
};

typedef struct pkix_EkuCheckerStruct {
    PKIX_List *requiredExtKeyUsageOids;
    PKIX_PL_OID *ekuOID;
} pkix_EkuChecker;






static PKIX_Error *
pkix_EkuChecker_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        pkix_EkuChecker *ekuCheckerState = NULL;

        PKIX_ENTER(EKUCHECKER, "pkix_EkuChecker_Destroy");
        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_CheckType(object, PKIX_EKUCHECKER_TYPE, plContext),
                    PKIX_OBJECTNOTANEKUCHECKERSTATE);

        ekuCheckerState = (pkix_EkuChecker *)object;

        PKIX_DECREF(ekuCheckerState->ekuOID);
        PKIX_DECREF(ekuCheckerState->requiredExtKeyUsageOids);

cleanup:

        PKIX_RETURN(EKUCHECKER);
}















PKIX_Error *
pkix_EkuChecker_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry *entry = &systemClasses[PKIX_EKUCHECKER_TYPE];

        PKIX_ENTER(EKUCHECKER, "pkix_EkuChecker_RegisterSelf");

        entry->description = "EkuChecker";
        entry->typeObjectSize = sizeof(pkix_EkuChecker);
        entry->destructor = pkix_EkuChecker_Destroy;

        PKIX_RETURN(EKUCHECKER);
}

























static PKIX_Error *
pkix_EkuChecker_Create(
        PKIX_ProcessingParams *params,
        pkix_EkuChecker **pState,
        void *plContext)
{
        pkix_EkuChecker *state = NULL;
        PKIX_CertSelector *certSelector = NULL;
        PKIX_ComCertSelParams *comCertSelParams = NULL;
        PKIX_List *requiredOids = NULL;

        PKIX_ENTER(EKUCHECKER, "pkix_EkuChecker_Create");
        PKIX_NULLCHECK_TWO(params, pState);

        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_EKUCHECKER_TYPE,
                    sizeof (pkix_EkuChecker),
                    (PKIX_PL_Object **)&state,
                    plContext),
                    PKIX_COULDNOTCREATEEKUCHECKERSTATEOBJECT);


        PKIX_CHECK(PKIX_ProcessingParams_GetTargetCertConstraints
                    (params, &certSelector, plContext),
                    PKIX_PROCESSINGPARAMSGETTARGETCERTCONSTRAINTSFAILED);

        if (certSelector != NULL) {

            
            PKIX_CHECK(PKIX_CertSelector_GetCommonCertSelectorParams
                       (certSelector, &comCertSelParams, plContext),
                       PKIX_CERTSELECTORGETCOMMONCERTSELECTORPARAMSFAILED);

            if (comCertSelParams != NULL) {
                PKIX_CHECK(PKIX_ComCertSelParams_GetExtendedKeyUsage
                           (comCertSelParams, &requiredOids, plContext),
                           PKIX_COMCERTSELPARAMSGETEXTENDEDKEYUSAGEFAILED);

            }
        }

        PKIX_CHECK(PKIX_PL_OID_Create
                    (PKIX_EXTENDEDKEYUSAGE_OID,
                    &state->ekuOID,
                    plContext),
                    PKIX_OIDCREATEFAILED);

        state->requiredExtKeyUsageOids = requiredOids;
        requiredOids = NULL;
        *pState = state;
        state = NULL;

cleanup:

        PKIX_DECREF(certSelector);
        PKIX_DECREF(comCertSelParams);
        PKIX_DECREF(requiredOids);
        PKIX_DECREF(state);

        PKIX_RETURN(EKUCHECKER);
}


























static PKIX_Error *
pkix_EkuChecker_Check(
        PKIX_CertChainChecker *checker,
        PKIX_PL_Cert *cert,
        PKIX_List *unresolvedCriticalExtensions,
        void **pNBIOContext,
        void *plContext)
{
        pkix_EkuChecker *state = NULL;
        PKIX_List *requiredExtKeyUsageList = NULL;
        PKIX_List *certExtKeyUsageList = NULL;
        PKIX_PL_OID *ekuOid = NULL;
        PKIX_Boolean isContained = PKIX_FALSE;
        PKIX_UInt32 numItems = 0;
        PKIX_UInt32 i;
        PKIX_Boolean checkResult = PKIX_TRUE;

        PKIX_ENTER(EKUCHECKER, "pkix_EkuChecker_Check");
        PKIX_NULLCHECK_THREE(checker, cert, pNBIOContext);

        *pNBIOContext = NULL; 

        PKIX_CHECK(
            PKIX_CertChainChecker_GetCertChainCheckerState
            (checker, (PKIX_PL_Object **)&state, plContext),
            PKIX_CERTCHAINCHECKERGETCERTCHAINCHECKERSTATEFAILED);

        requiredExtKeyUsageList = state->requiredExtKeyUsageOids;
        if (requiredExtKeyUsageList == NULL) {
            goto cleanup;
        }

        PKIX_CHECK(
            PKIX_List_GetLength(requiredExtKeyUsageList, &numItems,
                                plContext),
            PKIX_LISTGETLENGTHFAILED);
        if (numItems == 0) {
            goto cleanup;
        }

        PKIX_CHECK(
            PKIX_PL_Cert_GetExtendedKeyUsage(cert, &certExtKeyUsageList,
                                             plContext),
            PKIX_CERTGETEXTENDEDKEYUSAGEFAILED);

        if (certExtKeyUsageList == NULL) {
            goto cleanup;
        }
        
        for (i = 0; i < numItems; i++) {
            
            PKIX_CHECK(
                PKIX_List_GetItem(requiredExtKeyUsageList, i,
                                  (PKIX_PL_Object **)&ekuOid, plContext),
                PKIX_LISTGETITEMFAILED);

            PKIX_CHECK(
                pkix_List_Contains(certExtKeyUsageList,
                                   (PKIX_PL_Object *)ekuOid,
                                   &isContained,
                                   plContext),
                PKIX_LISTCONTAINSFAILED);
            
            PKIX_DECREF(ekuOid);
            if (isContained != PKIX_TRUE) {
                checkResult = PKIX_FALSE;
                goto cleanup;
            }
        }

cleanup:
        if (!pkixErrorResult && checkResult == PKIX_FALSE) {
            pkixErrorReceived = PKIX_TRUE;
            pkixErrorCode = PKIX_EXTENDEDKEYUSAGECHECKINGFAILED;
        }
        
        PKIX_DECREF(ekuOid);
        PKIX_DECREF(certExtKeyUsageList);
        PKIX_DECREF(state);

        PKIX_RETURN(EKUCHECKER);
}





PKIX_Error *
PKIX_EkuChecker_Create(
        PKIX_ProcessingParams *params,
        PKIX_CertChainChecker **pEkuChecker,
        void *plContext)
{
        pkix_EkuChecker *state = NULL;
        PKIX_List *critExtOIDsList = NULL;

        PKIX_ENTER(EKUCHECKER, "PKIX_EkuChecker_Initialize");
        PKIX_NULLCHECK_ONE(params);

        




        PKIX_CHECK(pkix_EkuChecker_Create
                    (params, &state, plContext),
                    PKIX_EKUCHECKERSTATECREATEFAILED);

        PKIX_CHECK(PKIX_List_Create(&critExtOIDsList, plContext),
                    PKIX_LISTCREATEFAILED);

        PKIX_CHECK(PKIX_List_AppendItem
                    (critExtOIDsList,
                    (PKIX_PL_Object *)state->ekuOID,
                    plContext),
                    PKIX_LISTAPPENDITEMFAILED);

        PKIX_CHECK(PKIX_CertChainChecker_Create
                (pkix_EkuChecker_Check,
                PKIX_TRUE,                 
                PKIX_FALSE,                
                critExtOIDsList,
                (PKIX_PL_Object *) state,
                pEkuChecker,
                plContext),
                PKIX_CERTCHAINCHECKERCREATEFAILED);
cleanup:

        PKIX_DECREF(critExtOIDsList);
        PKIX_DECREF(state);

        PKIX_RETURN(EKUCHECKER);
}
