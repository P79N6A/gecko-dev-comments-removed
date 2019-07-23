










































#include "pkix_pl_ekuchecker.h"

char *ekuOidStrings[] = {
        "1.3.6.1.5.5.7.3.1",    
        "1.3.6.1.5.5.7.3.2",    
        "1.3.6.1.5.5.7.3.3",    
        "1.3.6.1.5.5.7.3.4",    
        "1.3.6.1.5.5.7.3.8",    
        "1.3.6.1.5.5.7.3.9",    
        NULL
};

#define CERTUSAGE_NONE (-1)

PKIX_Int32 ekuCertUsages[] = {
        1<<certUsageSSLServer,
        1<<certUsageSSLClient,
        1<<certUsageObjectSigner,
        1<<certUsageEmailRecipient | 1<<certUsageEmailSigner,
        CERTUSAGE_NONE,
        1<<certUsageStatusResponder
};





static PKIX_Error *
pkix_pl_EkuChecker_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        pkix_pl_EkuChecker *ekuCheckerState = NULL;

        PKIX_ENTER(EKUCHECKER, "pkix_pl_EkuChecker_Destroy");
        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_CheckType(object, PKIX_EKUCHECKER_TYPE, plContext),
                    PKIX_OBJECTNOTANEKUCHECKERSTATE);

        ekuCheckerState = (pkix_pl_EkuChecker *)object;

        PKIX_DECREF(ekuCheckerState->ekuOID);

cleanup:

        PKIX_RETURN(EKUCHECKER);
}




























PKIX_Error *
pkix_pl_EkuChecker_GetRequiredEku(
        PKIX_CertSelector *certSelector,
        PKIX_UInt32 *pRequiredExtKeyUsage,
        void *plContext)
{
        PKIX_ComCertSelParams *comCertSelParams = NULL;
        PKIX_List *supportedOids = NULL;
        PKIX_List *requiredOid = NULL;
        PKIX_UInt32 requiredExtKeyUsage = 0;
        PKIX_UInt32 numItems = 0;
        PKIX_PL_OID *ekuOid = NULL;
        PKIX_UInt32 i;
        PKIX_Boolean isContained = PKIX_FALSE;

        PKIX_ENTER(EKUCHECKER, "pkix_pl_EkuChecker_GetRequiredEku");
        PKIX_NULLCHECK_TWO(certSelector, pRequiredExtKeyUsage);

        
        PKIX_CHECK(PKIX_CertSelector_GetCommonCertSelectorParams
                    (certSelector, &comCertSelParams, plContext),
                    PKIX_CERTSELECTORGETCOMMONCERTSELECTORPARAMSFAILED);

        if (comCertSelParams != NULL) {

                PKIX_CHECK(PKIX_ComCertSelParams_GetExtendedKeyUsage
                        (comCertSelParams, &requiredOid, plContext),
                        PKIX_COMCERTSELPARAMSGETEXTENDEDKEYUSAGEFAILED);

        }

        

        if (requiredOid != NULL) {

            PKIX_CHECK(PKIX_List_Create(&supportedOids, plContext),
                        PKIX_LISTCREATEFAILED);

            
            i = 0;
            while (ekuOidStrings[i] != NULL) {

                    PKIX_CHECK(PKIX_PL_OID_Create
                                (ekuOidStrings[i],
                                &ekuOid,
                                plContext),
                                PKIX_OIDCREATEFAILED);

                    PKIX_CHECK(PKIX_List_AppendItem
                                (supportedOids,
                                (PKIX_PL_Object *)ekuOid,
                                plContext),
                                PKIX_LISTAPPENDITEMFAILED);

                    PKIX_DECREF(ekuOid);
                    i++;
            }

            
            PKIX_CHECK(PKIX_List_GetLength
                        (supportedOids, &numItems, plContext),
                        PKIX_LISTGETLENGTHFAILED);

            for (i = 0; i < numItems; i++) {

                    PKIX_CHECK(PKIX_List_GetItem
                        (supportedOids,
                        i,
                        (PKIX_PL_Object **)&ekuOid,
                        plContext),
                        PKIX_LISTGETITEMFAILED);

                    PKIX_CHECK(pkix_List_Contains
                        (requiredOid,
                        (PKIX_PL_Object *)ekuOid,
                        &isContained,
                        plContext),
                        PKIX_LISTCONTAINSFAILED);

                    PKIX_DECREF(ekuOid);

                    if (isContained == PKIX_TRUE &&
                        ekuCertUsages[i] != CERTUSAGE_NONE) {

                            requiredExtKeyUsage |= ekuCertUsages[i];
                    }
            }
        }

        *pRequiredExtKeyUsage = requiredExtKeyUsage;

cleanup:

        PKIX_DECREF(ekuOid);
        PKIX_DECREF(requiredOid);
        PKIX_DECREF(supportedOids);
        PKIX_DECREF(comCertSelParams);

        PKIX_RETURN(EKUCHECKER);
}

























static PKIX_Error *
pkix_pl_EkuChecker_Create(
        PKIX_ProcessingParams *params,
        pkix_pl_EkuChecker **pState,
        void *plContext)
{
        pkix_pl_EkuChecker *state = NULL;
        PKIX_CertSelector *certSelector = NULL;
        PKIX_UInt32 requiredExtKeyUsage = 0;

        PKIX_ENTER(EKUCHECKER, "pkix_pl_EkuChecker_Create");
        PKIX_NULLCHECK_TWO(params, pState);

        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_EKUCHECKER_TYPE,
                    sizeof (pkix_pl_EkuChecker),
                    (PKIX_PL_Object **)&state,
                    plContext),
                    PKIX_COULDNOTCREATEEKUCHECKERSTATEOBJECT);


        PKIX_CHECK(PKIX_ProcessingParams_GetTargetCertConstraints
                    (params, &certSelector, plContext),
                    PKIX_PROCESSINGPARAMSGETTARGETCERTCONSTRAINTSFAILED);

        if (certSelector != NULL) {

                PKIX_CHECK(pkix_pl_EkuChecker_GetRequiredEku
                            (certSelector, &requiredExtKeyUsage, plContext),
                            PKIX_EKUCHECKERGETREQUIREDEKUFAILED);
        }

        PKIX_CHECK(PKIX_PL_OID_Create
                    (PKIX_EXTENDEDKEYUSAGE_OID,
                    &state->ekuOID,
                    plContext),
                    PKIX_OIDCREATEFAILED);

        state->requiredExtKeyUsage = requiredExtKeyUsage;

        *pState = state;
        state = NULL;

cleanup:

        PKIX_DECREF(certSelector);

        PKIX_DECREF(state);

        PKIX_RETURN(EKUCHECKER);
}


























static PKIX_Error *
pkix_pl_EkuChecker_Check(
        PKIX_CertChainChecker *checker,
        PKIX_PL_Cert *cert,
        PKIX_List *unresolvedCriticalExtensions,
        void **pNBIOContext,
        void *plContext)
{
        pkix_pl_EkuChecker *state = NULL;
        PKIX_Boolean checkPassed = PKIX_TRUE;

        PKIX_ENTER(EKUCHECKER, "pkix_pl_EkuChecker_Check");
        PKIX_NULLCHECK_THREE(checker, cert, pNBIOContext);

        *pNBIOContext = NULL; 

        PKIX_CHECK(PKIX_CertChainChecker_GetCertChainCheckerState
                    (checker, (PKIX_PL_Object **)&state, plContext),
                    PKIX_CERTCHAINCHECKERGETCERTCHAINCHECKERSTATEFAILED);

        if (state->requiredExtKeyUsage != 0) {

                PKIX_CHECK(pkix_pl_Cert_CheckExtendedKeyUsage
                        (cert,
                        state->requiredExtKeyUsage,
                        &checkPassed,
                        plContext),
                        PKIX_CERTCHECKEXTENDEDKEYUSAGEFAILED);

                if (checkPassed == PKIX_FALSE) {
                        PKIX_ERROR(PKIX_EXTENDEDKEYUSAGECHECKINGFAILED);
                }

        }

cleanup:

        PKIX_DECREF(state);

        PKIX_RETURN(EKUCHECKER);
}















PKIX_Error *
pkix_pl_EkuChecker_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER
            (EKUCHECKER,
                "pkix_pl_EkuChecker_RegisterSelf");

        entry.description = "EkuChecker";
        entry.objCounter = 0;
        entry.typeObjectSize = sizeof(pkix_pl_EkuChecker);
        entry.destructor = pkix_pl_EkuChecker_Destroy,
        entry.equalsFunction = NULL;
        entry.hashcodeFunction = NULL;
        entry.toStringFunction = NULL;
        entry.comparator = NULL;
        entry.duplicateFunction = NULL;

        systemClasses[PKIX_EKUCHECKER_TYPE] = entry;

        PKIX_RETURN(EKUCHECKER);
}





PKIX_Error *
PKIX_PL_EkuChecker_Create(
        PKIX_ProcessingParams *params,
        void *plContext)
{
        PKIX_CertChainChecker *checker = NULL;
        pkix_pl_EkuChecker *state = NULL;
        PKIX_List *critExtOIDsList = NULL;

        PKIX_ENTER(EKUCHECKER, "PKIX_PL_EkuChecker_Initialize");
        PKIX_NULLCHECK_ONE(params);

        




        PKIX_CHECK(pkix_pl_EkuChecker_Create
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
                (pkix_pl_EkuChecker_Check,
                PKIX_TRUE,                 
                PKIX_FALSE,                
                critExtOIDsList,
                (PKIX_PL_Object *) state,
                &checker,
                plContext),
                PKIX_CERTCHAINCHECKERCREATEFAILED);

        PKIX_CHECK(PKIX_ProcessingParams_AddCertChainChecker
                    (params, checker, plContext),
                    PKIX_PROCESSINGPARAMSADDCERTCHAINCHECKERFAILED);

cleanup:

        PKIX_DECREF(critExtOIDsList);
        PKIX_DECREF(checker);
        PKIX_DECREF(state);

        PKIX_RETURN(EKUCHECKER);
}
