










































#include "pkix_pl_lifecycle.h"

PKIX_Boolean pkix_pl_initialized = PKIX_FALSE;
pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
PRLock *classTableLock;
PRLogModuleInfo *pkixLog = NULL;









struct PKIX_Alloc_Error_ObjectStruct {
        PKIX_PL_Object header;
        PKIX_Error error;
};
typedef struct PKIX_Alloc_Error_ObjectStruct PKIX_Alloc_Error_Object;

static const PKIX_Alloc_Error_Object pkix_Alloc_Error_Data = {
    {
        PKIX_MAGIC_HEADER, 		
        (PKIX_UInt32)PKIX_ERROR_TYPE,   
        (PKIX_UInt32)1,                 
        
        (void *)0,                      
        (PKIX_PL_String *)0,            
        (PKIX_UInt32)0,                 
        (PKIX_Boolean)PKIX_FALSE,       
    }, {
        (PKIX_ERRORCODE)0,              
        (PKIX_ERRORCLASS)PKIX_FATAL_ERROR,
        (PKIX_UInt32)SEC_ERROR_LIBPKIX_INTERNAL, 
        (PKIX_Error *)0,                
        (PKIX_PL_Object *)0,            
   }
};

PKIX_Error* PKIX_ALLOC_ERROR(void)
{
    return (PKIX_Error *)&pkix_Alloc_Error_Data.error;
}

#ifdef PKIX_OBJECT_LEAK_TEST
SECStatus
pkix_pl_lifecycle_ObjectTableUpdate(int *objCountTable)
{
    int   typeCounter = 0;

    for (; typeCounter < PKIX_NUMTYPES; typeCounter++) {
        pkix_ClassTable_Entry *entry = &systemClasses[typeCounter];

        objCountTable[typeCounter] = entry->objCounter;
    }

    return SECSuccess;
}
#endif 


PKIX_UInt32
pkix_pl_lifecycle_ObjectLeakCheck(int *initObjCountTable)
{
        int   typeCounter = 0;
        PKIX_UInt32 numObjects = 0;
        char  classNameBuff[128];
        char *className = NULL;

        for (; typeCounter < PKIX_NUMTYPES; typeCounter++) {
                pkix_ClassTable_Entry *entry = &systemClasses[typeCounter];
                PKIX_UInt32 objCountDiff = entry->objCounter;

                if (initObjCountTable) {
                    PKIX_UInt32 initialCount = initObjCountTable[typeCounter];
                    objCountDiff = (entry->objCounter > initialCount) ?
                        entry->objCounter - initialCount : 0;
                }

                numObjects += objCountDiff;
                
                if (!pkixLog || !objCountDiff) {
                    continue;
                }
                className = entry->description;
                if (!className) {
                    className = classNameBuff;
                    PR_snprintf(className, 128, "Unknown(ref %d)", 
                            entry->objCounter);
                }

                PR_LOG(pkixLog, 1, ("Class %s leaked %d objects of "
                        "size %d bytes, total = %d bytes\n", className, 
                        objCountDiff, entry->typeObjectSize,
                        objCountDiff * entry->typeObjectSize));
        }
 
        return numObjects;
}




PKIX_Error *
PKIX_PL_Initialize(
        PKIX_Boolean platformInitNeeded,
        PKIX_Boolean useArenas,
        void **pPlContext)
{
        void *plContext = NULL;

        PKIX_ENTER(OBJECT, "PKIX_PL_Initialize");

        



        if (pkix_pl_initialized) {
            PKIX_RETURN(OBJECT);
        }

        classTableLock = PR_NewLock();
        if (classTableLock == NULL) {
            return PKIX_ALLOC_ERROR();
        }

        if (PR_GetEnv("NSS_STRICT_SHUTDOWN")) {
            pkixLog = PR_NewLogModule("pkix");
        }
        


        pkix_pl_Object_RegisterSelf(plContext);

        



        pkix_Error_RegisterSelf(plContext);
        pkix_pl_String_RegisterSelf(plContext);


        






        pkix_pl_BigInt_RegisterSelf(plContext);   
        pkix_pl_ByteArray_RegisterSelf(plContext);
        pkix_pl_HashTable_RegisterSelf(plContext);
        pkix_List_RegisterSelf(plContext);
        pkix_Logger_RegisterSelf(plContext);
        pkix_pl_Mutex_RegisterSelf(plContext);
        pkix_pl_OID_RegisterSelf(plContext);
        pkix_pl_RWLock_RegisterSelf(plContext);

        pkix_pl_CertBasicConstraints_RegisterSelf(plContext); 
        pkix_pl_Cert_RegisterSelf(plContext);
        pkix_pl_CRL_RegisterSelf(plContext);
        pkix_pl_CRLEntry_RegisterSelf(plContext);
        pkix_pl_Date_RegisterSelf(plContext);
        pkix_pl_GeneralName_RegisterSelf(plContext);
        pkix_pl_CertNameConstraints_RegisterSelf(plContext);
        pkix_pl_PublicKey_RegisterSelf(plContext);
        pkix_TrustAnchor_RegisterSelf(plContext);

        pkix_pl_X500Name_RegisterSelf(plContext);   
        pkix_pl_HttpCertStoreContext_RegisterSelf(plContext);
        pkix_BuildResult_RegisterSelf(plContext);
        pkix_ProcessingParams_RegisterSelf(plContext);
        pkix_ValidateParams_RegisterSelf(plContext);
        pkix_ValidateResult_RegisterSelf(plContext);
        pkix_CertStore_RegisterSelf(plContext);
        pkix_CertChainChecker_RegisterSelf(plContext);
        pkix_RevocationChecker_RegisterSelf(plContext);
        pkix_CertSelector_RegisterSelf(plContext);

        pkix_ComCertSelParams_RegisterSelf(plContext);   
        pkix_CRLSelector_RegisterSelf(plContext);
        pkix_ComCRLSelParams_RegisterSelf(plContext);
        pkix_pl_CertPolicyInfo_RegisterSelf(plContext);
        pkix_pl_CertPolicyQualifier_RegisterSelf(plContext);
        pkix_pl_CertPolicyMap_RegisterSelf(plContext);
        pkix_PolicyNode_RegisterSelf(plContext);
        pkix_TargetCertCheckerState_RegisterSelf(plContext);
        pkix_BasicConstraintsCheckerState_RegisterSelf(plContext);
        pkix_PolicyCheckerState_RegisterSelf(plContext);

        pkix_pl_CollectionCertStoreContext_RegisterSelf(plContext); 
        pkix_CrlChecker_RegisterSelf(plContext);
        pkix_ForwardBuilderState_RegisterSelf(plContext);
        pkix_SignatureCheckerState_RegisterSelf(plContext);
        pkix_NameConstraintsCheckerState_RegisterSelf(plContext);
        pkix_pl_LdapRequest_RegisterSelf(plContext);
        pkix_pl_LdapResponse_RegisterSelf(plContext);
        pkix_pl_LdapDefaultClient_RegisterSelf(plContext);
        pkix_pl_Socket_RegisterSelf(plContext);

        pkix_ResourceLimits_RegisterSelf(plContext); 
        pkix_pl_MonitorLock_RegisterSelf(plContext);
        pkix_pl_InfoAccess_RegisterSelf(plContext);
        pkix_pl_AIAMgr_RegisterSelf(plContext);
        pkix_OcspChecker_RegisterSelf(plContext);
        pkix_pl_OcspCertID_RegisterSelf(plContext);
        pkix_pl_OcspRequest_RegisterSelf(plContext);
        pkix_pl_OcspResponse_RegisterSelf(plContext);
        pkix_pl_HttpDefaultClient_RegisterSelf(plContext);
        pkix_VerifyNode_RegisterSelf(plContext);
        pkix_EkuChecker_RegisterSelf(plContext);

        if (pPlContext) {
            PKIX_CHECK(PKIX_PL_NssContext_Create
                       (0, useArenas, NULL, &plContext),
                       PKIX_NSSCONTEXTCREATEFAILED);
            
            *pPlContext = plContext;
        }

        pkix_pl_initialized = PKIX_TRUE;

cleanup:

        PKIX_RETURN(OBJECT);
}




PKIX_Error *
PKIX_PL_Shutdown(void *plContext)
{
        PKIX_UInt32 numLeakedObjects = 0;

        PKIX_ENTER(OBJECT, "PKIX_PL_Shutdown");

        if (!pkix_pl_initialized) {
            
            PKIX_RETURN(OBJECT);
        }

        PR_DestroyLock(classTableLock);

        pkix_pl_HttpCertStore_Shutdown(plContext);

        numLeakedObjects = pkix_pl_lifecycle_ObjectLeakCheck(NULL);
        if (PR_GetEnv("NSS_STRICT_SHUTDOWN")) {
           PORT_Assert(numLeakedObjects == 0);
        }

        if (plContext != NULL) {
                PKIX_PL_NssContext_Destroy(plContext);
        }

        pkix_pl_initialized = PKIX_FALSE;

        PKIX_RETURN(OBJECT);
}
