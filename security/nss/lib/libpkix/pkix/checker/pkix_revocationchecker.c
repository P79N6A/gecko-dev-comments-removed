










































#include "pkix_revocationchecker.h"
#include "pkix_tools.h"







static PKIX_Error *
pkix_RevocationChecker_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_RevocationChecker *checker = NULL;

        PKIX_ENTER(REVOCATIONCHECKER, "pkix_RevocationChecker_Destroy");
        PKIX_NULLCHECK_ONE(object);

        
        PKIX_CHECK(pkix_CheckType
                    (object, PKIX_REVOCATIONCHECKER_TYPE, plContext),
                    PKIX_OBJECTNOTREVOCATIONCHECKER);

        checker = (PKIX_RevocationChecker *)object;

        PKIX_DECREF(checker->date);
        PKIX_DECREF(checker->leafMethodList);
        PKIX_DECREF(checker->chainMethodList);
        
cleanup:

        PKIX_RETURN(REVOCATIONCHECKER);
}





static PKIX_Error *
pkix_RevocationChecker_Duplicate(
        PKIX_PL_Object *object,
        PKIX_PL_Object **pNewObject,
        void *plContext)
{
        PKIX_RevocationChecker *checker = NULL;
        PKIX_RevocationChecker *checkerDuplicate = NULL;
        PKIX_List *dupLeafList = NULL;
        PKIX_List *dupChainList = NULL;

        PKIX_ENTER(REVOCATIONCHECKER, "pkix_RevocationChecker_Duplicate");
        PKIX_NULLCHECK_TWO(object, pNewObject);

        PKIX_CHECK(pkix_CheckType
                    (object, PKIX_REVOCATIONCHECKER_TYPE, plContext),
                    PKIX_OBJECTNOTCERTCHAINCHECKER);

        checker = (PKIX_RevocationChecker *)object;

        if (checker->leafMethodList){
                PKIX_CHECK(PKIX_PL_Object_Duplicate
                            ((PKIX_PL_Object *)checker->leafMethodList,
                            (PKIX_PL_Object **)&dupLeafList,
                            plContext),
                            PKIX_OBJECTDUPLICATEFAILED);
        }
        if (checker->chainMethodList){
                PKIX_CHECK(PKIX_PL_Object_Duplicate
                            ((PKIX_PL_Object *)checker->chainMethodList,
                            (PKIX_PL_Object **)&dupChainList,
                            plContext),
                            PKIX_OBJECTDUPLICATEFAILED);
        }

        PKIX_CHECK(
            PKIX_RevocationChecker_Create(checker->date,
                                          checker->leafMethodListFlags,
                                          checker->chainMethodListFlags,
                                          &checkerDuplicate,
                                          plContext),
            PKIX_REVOCATIONCHECKERCREATEFAILED);

        checkerDuplicate->leafMethodList = dupLeafList;
        checkerDuplicate->chainMethodList = dupChainList;
        dupLeafList = NULL;
        dupChainList = NULL;

        *pNewObject = (PKIX_PL_Object *)checkerDuplicate;

cleanup:
        PKIX_DECREF(dupLeafList);
        PKIX_DECREF(dupChainList);

        PKIX_RETURN(REVOCATIONCHECKER);
}













PKIX_Error *
pkix_RevocationChecker_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(REVOCATIONCHECKER, "pkix_RevocationChecker_RegisterSelf");

        entry.description = "RevocationChecker";
        entry.objCounter = 0;
        entry.typeObjectSize = sizeof(PKIX_RevocationChecker);
        entry.destructor = pkix_RevocationChecker_Destroy;
        entry.equalsFunction = NULL;
        entry.hashcodeFunction = NULL;
        entry.toStringFunction = NULL;
        entry.comparator = NULL;
        entry.duplicateFunction = pkix_RevocationChecker_Duplicate;

        systemClasses[PKIX_REVOCATIONCHECKER_TYPE] = entry;

        PKIX_RETURN(REVOCATIONCHECKER);
}


static PKIX_Error *
pkix_RevocationChecker_SortComparator(
        PKIX_PL_Object *obj1,
        PKIX_PL_Object *obj2,
        PKIX_Int32 *pResult,
        void *plContext)
{
    pkix_RevocationMethod *method1 = NULL, *method2 = NULL;
    
    PKIX_ENTER(BUILD, "pkix_RevocationChecker_SortComparator");
    
    method1 = (pkix_RevocationMethod *)obj1;
    method2 = (pkix_RevocationMethod *)obj2;
    
    *pResult = (method1->priority > method2->priority);
    
    PKIX_RETURN(BUILD);
}








PKIX_Error *
PKIX_RevocationChecker_Create(
    PKIX_PL_Date *revDate,
    PKIX_UInt32 leafMethodListFlags,
    PKIX_UInt32 chainMethodListFlags,
    PKIX_RevocationChecker **pChecker,
    void *plContext)
{
    PKIX_RevocationChecker *checker = NULL;
    
    PKIX_ENTER(REVOCATIONCHECKER, "PKIX_RevocationChecker_Create");
    PKIX_NULLCHECK_ONE(pChecker);
    
    PKIX_CHECK(
        PKIX_PL_Object_Alloc(PKIX_REVOCATIONCHECKER_TYPE,
                             sizeof (PKIX_RevocationChecker),
                             (PKIX_PL_Object **)&checker,
                             plContext),
        PKIX_COULDNOTCREATECERTCHAINCHECKEROBJECT);
    
    PKIX_INCREF(revDate);
    checker->date = revDate;

    checker->leafMethodListFlags = leafMethodListFlags;
    checker->chainMethodListFlags = chainMethodListFlags;
    checker->leafMethodList = NULL;
    checker->chainMethodList = NULL;
    
    *pChecker = checker;
    checker = NULL;
    
cleanup:
    PKIX_DECREF(checker);
    
    PKIX_RETURN(REVOCATIONCHECKER);
}




PKIX_Error *
PKIX_RevocationChecker_CreateAndAddMethod(
    PKIX_RevocationChecker *revChecker,
    PKIX_ProcessingParams *params,
    PKIX_RevocationMethodType methodType,
    PKIX_UInt32 flags,
    PKIX_UInt32 priority,
    PKIX_PL_VerifyCallback verificationFn,
    PKIX_Boolean isLeafMethod,
    void *plContext)
{
    PKIX_List **methodList = NULL;
    PKIX_List  *unsortedList = NULL;
    PKIX_List  *certStores = NULL;
    pkix_RevocationMethod *method = NULL;
    pkix_LocalRevocationCheckFn *localRevChecker = NULL;
    pkix_ExternalRevocationCheckFn *externRevChecker = NULL;
    
    PKIX_ENTER(REVOCATIONCHECKER, "PKIX_RevocationChecker_CreateAndAddMethod");
    PKIX_NULLCHECK_ONE(revChecker);

    switch (methodType) {
    case PKIX_RevocationMethod_CRL:
        localRevChecker = pkix_CrlChecker_CheckLocal;
        externRevChecker = pkix_CrlChecker_CheckExternal;
        PKIX_CHECK(
            PKIX_ProcessingParams_GetCertStores(params, &certStores,
                                                plContext),
            PKIX_PROCESSINGPARAMSGETCERTSTORESFAILED);
        PKIX_CHECK(
            pkix_CrlChecker_Create(methodType, flags, priority,
                                   localRevChecker, externRevChecker,
                                   certStores, verificationFn,
                                   &method,
                                   plContext),
            PKIX_COULDNOTCREATECRLCHECKEROBJECT);
        break;
    case PKIX_RevocationMethod_OCSP:
        localRevChecker = pkix_OcspChecker_CheckLocal;
        externRevChecker = pkix_OcspChecker_CheckExternal;
        PKIX_CHECK(
            pkix_OcspChecker_Create(methodType, flags, priority,
                                    localRevChecker, externRevChecker,
                                    verificationFn,
                                    &method,
                                    plContext),
            PKIX_COULDNOTCREATEOCSPCHECKEROBJECT);
        break;
    default:
        PKIX_ERROR(PKIX_INVALIDREVOCATIONMETHOD);
    }

    if (isLeafMethod) {
        methodList = &revChecker->leafMethodList;
    } else {
        methodList = &revChecker->chainMethodList;
    }
    
    if (*methodList == NULL) {
        PKIX_CHECK(
            PKIX_List_Create(methodList, plContext),
            PKIX_LISTCREATEFAILED);
    }
    unsortedList = *methodList;
    PKIX_CHECK(
        PKIX_List_AppendItem(unsortedList, (PKIX_PL_Object*)method, plContext),
        PKIX_LISTAPPENDITEMFAILED);
    PKIX_CHECK(
        pkix_List_BubbleSort(unsortedList, 
                             pkix_RevocationChecker_SortComparator,
                             methodList, plContext),
        PKIX_LISTBUBBLESORTFAILED);

cleanup:
    PKIX_DECREF(method);
    PKIX_DECREF(unsortedList);
    PKIX_DECREF(certStores);
    
    PKIX_RETURN(REVOCATIONCHECKER);
}




PKIX_Error *
PKIX_RevocationChecker_Check(
    PKIX_PL_Cert *cert,
    PKIX_PL_Cert *issuer,
    PKIX_RevocationChecker *revChecker,
    PKIX_ProcessingParams *procParams,
    PKIX_Boolean chainVerificationState,
    PKIX_Boolean testingLeafCert,
    PKIX_RevocationStatus *pRevStatus,
    PKIX_UInt32 *pReasonCode,
    void **pNbioContext,
    void *plContext)
{
    PKIX_RevocationStatus overallStatus = PKIX_RevStatus_NoInfo;
    PKIX_RevocationStatus methodStatus[PKIX_RevocationMethod_MAX];
    PKIX_Boolean onlyUseRemoteMethods = PKIX_FALSE;
    PKIX_UInt32 revFlags = 0;
    PKIX_List *revList = NULL;
    pkix_RevocationMethod *method = NULL;
    void *nbioContext;
    int tries;
    
    PKIX_ENTER(REVOCATIONCHECKER, "PKIX_RevocationChecker_Check");
    PKIX_NULLCHECK_ONE(revChecker);

    nbioContext = *pNbioContext;
    *pNbioContext = NULL;
    
    if (testingLeafCert) {
        revList = revChecker->leafMethodList;
        revFlags = revChecker->leafMethodListFlags;        
    } else {
        revList = revChecker->chainMethodList;
        revFlags = revChecker->chainMethodListFlags;
    }
    if (!revList) {
        
        goto cleanup;
    }

    PORT_Memset(methodStatus, PKIX_RevStatus_NoInfo,
                sizeof(PKIX_RevocationStatus) * PKIX_RevocationMethod_MAX);

    


    for (tries = 0;tries < 2;tries++) {
        int methodNum = 0;
        for (;methodNum < revList->length;methodNum++) {
            PKIX_UInt32 methodFlags = 0;

            PKIX_DECREF(method);
            pkixErrorResult = PKIX_List_GetItem(revList, methodNum,
                                                (PKIX_PL_Object**)&method,
                                                plContext);
            if (pkixErrorResult) {
                
                goto cleanup;
            }
            methodFlags = method->flags;
            if (!(methodFlags & PKIX_REV_M_TEST_USING_THIS_METHOD)) {
                
                continue;
            }
            if (!onlyUseRemoteMethods &&
                methodStatus[methodNum] == PKIX_RevStatus_NoInfo) {
                PKIX_RevocationStatus revStatus = PKIX_RevStatus_NoInfo;

                pkixErrorResult =
                    (*method->localRevChecker)(cert, issuer,
                                               revChecker->date,
                                               method, procParams,
                                               methodFlags, &revStatus,
                                               pReasonCode, plContext);
                methodStatus[methodNum] = revStatus;
                if (pkixErrorResult) {
                    
                    PKIX_PL_Object_DecRef((PKIX_PL_Object*)pkixErrorResult,
                                          plContext);
                    pkixErrorResult = NULL;
                }
                if (revStatus == PKIX_RevStatus_Revoked) {
                    overallStatus = PKIX_RevStatus_Revoked;
                    goto cleanup;
                }
            }
            if ((!(revFlags & PKIX_REV_MI_TEST_ALL_LOCAL_INFORMATION_FIRST) ||
                 onlyUseRemoteMethods) &&
                chainVerificationState &&
                methodStatus[methodNum] == PKIX_RevStatus_NoInfo) {
                if (!(methodFlags & PKIX_REV_M_FORBID_NETWORK_FETCHING)) {
                    PKIX_RevocationStatus revStatus = PKIX_RevStatus_NoInfo;
                    pkixErrorResult =
                        (*method->externalRevChecker)(cert, issuer,
                                                      revChecker->date,
                                                      method,
                                                      procParams, methodFlags,
                                                      &revStatus, pReasonCode,
                                                      &nbioContext, plContext);
                    methodStatus[methodNum] = revStatus;
                    if (pkixErrorResult) {
                        
                        PKIX_PL_Object_DecRef((PKIX_PL_Object*)pkixErrorResult,
                                              plContext);
                        pkixErrorResult = NULL;
                    }
                    if (revStatus == PKIX_RevStatus_Revoked) {
                        overallStatus = PKIX_RevStatus_Revoked;
                        goto cleanup;
                    }
                } else if (methodFlags &
                           PKIX_REV_M_FAIL_ON_MISSING_FRESH_INFO) {
                    


                    overallStatus = PKIX_RevStatus_Revoked;
                    goto cleanup;
                }
            }
            

            if (methodStatus[methodNum] == PKIX_RevStatus_Success &&
                !(methodFlags & PKIX_REV_M_CONTINUE_TESTING_ON_FRESH_INFO)) {
                overallStatus = PKIX_RevStatus_Success;
                goto cleanup;
            }
        } 
        if (!onlyUseRemoteMethods &&
            revFlags & PKIX_REV_MI_TEST_ALL_LOCAL_INFORMATION_FIRST &&
            chainVerificationState) {
            onlyUseRemoteMethods = PKIX_TRUE;
            continue;
        }
        break;
    } 
    
    if (overallStatus == PKIX_RevStatus_NoInfo &&
        chainVerificationState) {
        






        




        int methodNum = 0;
        for (;methodNum < PKIX_RevocationMethod_MAX;methodNum++) {
            if (methodStatus[methodNum] == PKIX_RevStatus_Success) {
                overallStatus = PKIX_RevStatus_Success;
                goto cleanup;
            }
        }
        if (revFlags & PKIX_REV_MI_REQUIRE_SOME_FRESH_INFO_AVAILABLE) {
            overallStatus = PKIX_RevStatus_Revoked;
        }
    }

cleanup:
    *pRevStatus = overallStatus;
    PKIX_DECREF(method);

    PKIX_RETURN(REVOCATIONCHECKER);
}

