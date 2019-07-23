










































#include "pkix_tools.h"

#define CACHE_ITEM_PERIOD_SECONDS  (3600)  /* one hour */





#define CACHE_TRUST_ITEM_PERIOD_SECONDS  (CACHE_ITEM_PERIOD_SECONDS/10)

extern PKIX_PL_HashTable *cachedCertChainTable;
extern PKIX_PL_HashTable *cachedCertTable;
extern PKIX_PL_HashTable *cachedCrlEntryTable;


extern int pkix_ccAddCount;
extern int pkix_ccLookupCount;
extern int pkix_ccRemoveCount;
extern int pkix_cAddCount;
extern int pkix_cLookupCount;
extern int pkix_cRemoveCount;
extern int pkix_ceAddCount;
extern int pkix_ceLookupCount;

#ifdef PKIX_OBJECT_LEAK_TEST

char *nonNullValue = "Non Empty Value";
PKIX_Boolean noErrorState = PKIX_TRUE;
PKIX_Boolean runningLeakTest;
PKIX_Boolean errorGenerated;
PKIX_UInt32 stackPosition;
PKIX_UInt32 *fnStackInvCountArr;
char **fnStackNameArr;
PLHashTable *fnInvTable;
PKIX_UInt32 testStartFnStackPosition;
char *errorFnStackString;
#endif 



#ifdef PKIX_OBJECT_LEAK_TEST







PLHashNumber PR_CALLBACK
pkix_ErrorGen_Hash (const void *key)
{
    char *str = NULL;
    PLHashNumber rv = (*(PRUint8*)key) << 5;
    PRUint32 i, counter = 0;
    PRUint8 *rvc = (PRUint8 *)&rv;

    while ((str = fnStackNameArr[counter++]) != NULL) {
        PRUint32 len = strlen(str);
        for( i = 0; i < len; i++ ) {
            rvc[ i % sizeof(rv) ] ^= *str;
            str++;
        }
    }

    return rv;
}

#endif 

























PKIX_Error *
pkix_IsCertSelfIssued(
        PKIX_PL_Cert *cert,
        PKIX_Boolean *pSelfIssued,
        void *plContext)
{
        PKIX_PL_X500Name *subject = NULL;
        PKIX_PL_X500Name *issuer = NULL;

        PKIX_ENTER(CERT, "pkix_IsCertSelfIssued");
        PKIX_NULLCHECK_TWO(cert, pSelfIssued);

        PKIX_CHECK(PKIX_PL_Cert_GetSubject(cert, &subject, plContext),
                    PKIX_CERTGETSUBJECTFAILED);

        PKIX_CHECK(PKIX_PL_Cert_GetIssuer(cert, &issuer, plContext),
                    PKIX_CERTGETISSUERFAILED);

        if (subject == NULL || issuer == NULL) {
                *pSelfIssued = PKIX_FALSE;
        } else {

                PKIX_CHECK(PKIX_PL_X500Name_Match
                    (subject, issuer, pSelfIssued, plContext),
                    PKIX_X500NAMEMATCHFAILED);
        }

cleanup:
        PKIX_DECREF(subject);
        PKIX_DECREF(issuer);

        PKIX_RETURN(CERT);
}


































PKIX_Error *
pkix_Throw(
        PKIX_ERRORCLASS errorClass,
        const char *funcName,
        PKIX_ERRORCODE errorCode,
        PKIX_ERRORCLASS overrideClass,
        PKIX_Error *cause,
        PKIX_Error **pError,
        void *plContext)
{
        PKIX_Error *error = NULL;

        PKIX_ENTER(ERROR, "pkix_Throw");
        PKIX_NULLCHECK_TWO(funcName, pError);

        *pError = NULL;

#ifdef PKIX_OBJECT_LEAK_TEST        
        noErrorState = PKIX_TRUE;
        if (pkixLog) {
#ifdef PKIX_ERROR_DESCRIPTION            
            PR_LOG(pkixLog, 4, ("Error in function \"%s\":\"%s\" with cause \"%s\"\n",
                                funcName, PKIX_ErrorText[errorCode],
                                (cause ? PKIX_ErrorText[cause->errCode] : "null")));
#else
            PR_LOG(pkixLog, 4, ("Error in function \"%s\": error code \"%d\"\n",
                                funcName, errorCode));
#endif 
            PORT_Assert(strcmp(funcName, "PKIX_PL_Object_DecRef"));
        }
#endif 

        
        if (cause) {
                if (cause->errClass == PKIX_FATAL_ERROR){
                        PKIX_INCREF(cause);
                        *pError = cause;
                        goto cleanup;
                }
        }
        
        if (overrideClass == PKIX_FATAL_ERROR){
                errorClass = overrideClass;
        }

       pkixTempResult = PKIX_Error_Create(errorClass, cause, NULL,
                                           errorCode, &error, plContext);
       
       if (!pkixTempResult) {
           


           if (!cause && !error->plErr) {
               error->plErr = PKIX_PL_GetPLErrorCode();
           }
       }

       *pError = error;

cleanup:

        PKIX_DEBUG_EXIT(ERROR);
        pkixErrorClass = 0;
#ifdef PKIX_OBJECT_LEAK_TEST        
        noErrorState = PKIX_FALSE;

        if (runningLeakTest && fnStackNameArr) {
            PR_LOG(pkixLog, 5,
                   ("%s%*s<- %s(%d) - %s\n", (errorGenerated ? "*" : " "),
                    stackPosition, " ", fnStackNameArr[stackPosition],
                    stackPosition, myFuncName));
            fnStackNameArr[stackPosition--] = NULL;
        }
#endif 
        return (pkixTempResult);
}

























PKIX_Error *
pkix_CheckTypes(
        PKIX_PL_Object *first,
        PKIX_PL_Object *second,
        PKIX_UInt32 type,
        void *plContext)
{
        PKIX_UInt32 firstType, secondType;

        PKIX_ENTER(OBJECT, "pkix_CheckTypes");
        PKIX_NULLCHECK_TWO(first, second);

        PKIX_CHECK(PKIX_PL_Object_GetType(first, &firstType, plContext),
                    PKIX_COULDNOTGETFIRSTOBJECTTYPE);

        PKIX_CHECK(PKIX_PL_Object_GetType(second, &secondType, plContext),
                    PKIX_COULDNOTGETSECONDOBJECTTYPE);

        if ((firstType != type)||(firstType != secondType)) {
                PKIX_ERROR(PKIX_OBJECTTYPESDONOTMATCH);
        }

cleanup:

        PKIX_RETURN(OBJECT);
}






















PKIX_Error *
pkix_CheckType(
        PKIX_PL_Object *object,
        PKIX_UInt32 type,
        void *plContext)
{
        return (pkix_CheckTypes(object, object, type, plContext));
}


























PKIX_Error *
pkix_hash(
        const unsigned char *bytes,
        PKIX_UInt32 length,
        PKIX_UInt32 *pHash,
        void *plContext)
{
        PKIX_UInt32 i;
        PKIX_UInt32 hash;

        PKIX_ENTER(OBJECT, "pkix_hash");
        PKIX_NULLCHECK_TWO(bytes, pHash);

        hash = 0;
        for (i = 0; i < length; i++) {
                
                hash = (hash << 5) - hash + bytes[i];
        }

        *pHash = hash;

        PKIX_RETURN(OBJECT);
}
















PKIX_UInt32
pkix_countArray(void **array)
{
        PKIX_UInt32 count = 0;

        if (array) {
                while (*array++) {
                        count++;
                }
        }
        return (count);
}











PKIX_Error *
pkix_duplicateImmutable(
        PKIX_PL_Object *object,
        PKIX_PL_Object **pNewObject,
        void *plContext)
{
        PKIX_ENTER(OBJECT, "pkix_duplicateImmutable");
        PKIX_NULLCHECK_TWO(object, pNewObject);

        PKIX_INCREF(object);

        *pNewObject = object;

cleanup:
        PKIX_RETURN(OBJECT);
}

















PKIX_UInt32
pkix_hex2i(char c)
{
        if ((c >= '0')&&(c <= '9'))
                return (c-'0');
        else if ((c >= 'a')&&(c <= 'f'))
                return (c-'a'+10);
        else if ((c >= 'A')&&(c <= 'F'))
                return (c-'A'+10);
        else
                return ((PKIX_UInt32)(-1));
}















char
pkix_i2hex(char digit)
{
        if ((digit >= 0)&&(digit <= 9))
                return (digit+'0');
        else if ((digit >= 0xa)&&(digit <= 0xf))
                return (digit - 10 + 'a');
        else
                return (-1);
}





















PKIX_Boolean
pkix_isPlaintext(unsigned char c, PKIX_Boolean debug) {
        return ((c >= 0x01)&&(c <= 0x7E)&&(c != '&')&&(!debug || (c >= 20)));
}















































PKIX_Error *
pkix_CacheCertChain_Lookup(
        PKIX_PL_Cert* targetCert,
        PKIX_List* anchors,
        PKIX_PL_Date *testDate,
        PKIX_Boolean *pFound,
        PKIX_BuildResult **pBuildResult,
        void *plContext)
{
        PKIX_List *cachedValues = NULL;
        PKIX_List *cachedKeys = NULL;
        PKIX_Error *cachedCertChainError = NULL;
        PKIX_PL_Date *cacheValidUntilDate = NULL;
        PKIX_PL_Date *validityDate = NULL;
        PKIX_Int32 cmpValidTimeResult = 0;
        PKIX_Int32 cmpCacheTimeResult = 0;

        PKIX_ENTER(BUILD, "pkix_CacheCertChain_Lookup");

        PKIX_NULLCHECK_FOUR(targetCert, anchors, pFound, pBuildResult);

        *pFound = PKIX_FALSE;

        

        PKIX_CHECK(PKIX_List_Create(&cachedKeys, plContext),
                    PKIX_LISTCREATEFAILED);

        PKIX_CHECK(PKIX_List_AppendItem
                    (cachedKeys,
                    (PKIX_PL_Object *)targetCert,
                    plContext),
                    PKIX_LISTAPPENDITEMFAILED);

        PKIX_CHECK(PKIX_List_AppendItem
                    (cachedKeys,
                    (PKIX_PL_Object *)anchors,
                    plContext),
                    PKIX_LISTAPPENDITEMFAILED);

        cachedCertChainError = PKIX_PL_HashTable_Lookup
                    (cachedCertChainTable,
                    (PKIX_PL_Object *) cachedKeys,
                    (PKIX_PL_Object **) &cachedValues,
                    plContext);

        pkix_ccLookupCount++;

        

        if (cachedValues != NULL && cachedCertChainError == NULL) {

            PKIX_CHECK(PKIX_List_GetItem
                    (cachedValues,
                    0,
                    (PKIX_PL_Object **) &cacheValidUntilDate,
                    plContext),
                    PKIX_LISTGETITEMFAILED);

            
            PKIX_CHECK(PKIX_List_GetItem
                    (cachedValues,
                    1,
                    (PKIX_PL_Object **) &validityDate,
                    plContext),
                    PKIX_LISTGETITEMFAILED);

            
            if (testDate) {

                PKIX_CHECK(PKIX_PL_Object_Compare
                     ((PKIX_PL_Object *)testDate,
                     (PKIX_PL_Object *)cacheValidUntilDate,
                     &cmpCacheTimeResult,
                     plContext),
                     PKIX_OBJECTCOMPARATORFAILED);

                PKIX_CHECK(PKIX_PL_Object_Compare
                     ((PKIX_PL_Object *)testDate,
                     (PKIX_PL_Object *)validityDate,
                     &cmpValidTimeResult,
                     plContext),
                     PKIX_OBJECTCOMPARATORFAILED);
            }

            
            if (cmpValidTimeResult <= 0 && cmpCacheTimeResult <=0) {

                PKIX_CHECK(PKIX_List_GetItem
                    (cachedValues,
                    2,
                    (PKIX_PL_Object **) pBuildResult,
                    plContext),
                    PKIX_LISTGETITEMFAILED);

                *pFound = PKIX_TRUE;

            } else {

                pkix_ccRemoveCount++;
                *pFound = PKIX_FALSE;

                
                PKIX_CHECK(PKIX_PL_HashTable_Remove
                    (cachedCertChainTable,
                    (PKIX_PL_Object *) cachedKeys,
                    plContext),
                    PKIX_HASHTABLEREMOVEFAILED);
            }
        }

cleanup:

        PKIX_DECREF(cachedValues);
        PKIX_DECREF(cachedKeys);
        PKIX_DECREF(cachedCertChainError);
        PKIX_DECREF(cacheValidUntilDate);
        PKIX_DECREF(validityDate);

        PKIX_RETURN(BUILD);

}


































PKIX_Error *
pkix_CacheCertChain_Remove(
        PKIX_PL_Cert* targetCert,
        PKIX_List* anchors,
        void *plContext)
{
        PKIX_List *cachedKeys = NULL;

        PKIX_ENTER(BUILD, "pkix_CacheCertChain_Remove");
        PKIX_NULLCHECK_TWO(targetCert, anchors);

        

        PKIX_CHECK(PKIX_List_Create(&cachedKeys, plContext),
                    PKIX_LISTCREATEFAILED);

        PKIX_CHECK(PKIX_List_AppendItem
                    (cachedKeys,
                    (PKIX_PL_Object *)targetCert,
                    plContext),
                    PKIX_LISTAPPENDITEMFAILED);

        PKIX_CHECK(PKIX_List_AppendItem
                    (cachedKeys,
                    (PKIX_PL_Object *)anchors,
                    plContext),
                    PKIX_LISTAPPENDITEMFAILED);

        PKIX_CHECK_ONLY_FATAL(PKIX_PL_HashTable_Remove
                    (cachedCertChainTable,
                    (PKIX_PL_Object *) cachedKeys,
                    plContext),
                    PKIX_HASHTABLEREMOVEFAILED);

        pkix_ccRemoveCount++;

cleanup:

        PKIX_DECREF(cachedKeys);

        PKIX_RETURN(BUILD);

}










































PKIX_Error *
pkix_CacheCertChain_Add(
        PKIX_PL_Cert* targetCert,
        PKIX_List* anchors,
        PKIX_PL_Date *validityDate,
        PKIX_BuildResult *buildResult,
        void *plContext)
{
        PKIX_List *cachedValues = NULL;
        PKIX_List *cachedKeys = NULL;
        PKIX_Error *cachedCertChainError = NULL;
        PKIX_PL_Date *cacheValidUntilDate = NULL;

        PKIX_ENTER(BUILD, "pkix_CacheCertChain_Add");

        PKIX_NULLCHECK_FOUR(targetCert, anchors, validityDate, buildResult);

        PKIX_CHECK(PKIX_List_Create(&cachedKeys, plContext),
                PKIX_LISTCREATEFAILED);

        PKIX_CHECK(PKIX_List_AppendItem
                (cachedKeys, (PKIX_PL_Object *)targetCert, plContext),
                PKIX_LISTAPPENDITEMFAILED);

        PKIX_CHECK(PKIX_List_AppendItem
                (cachedKeys, (PKIX_PL_Object *)anchors, plContext),
                PKIX_LISTAPPENDITEMFAILED);

        PKIX_CHECK(PKIX_List_Create(&cachedValues, plContext),
                PKIX_LISTCREATEFAILED);

        PKIX_CHECK(PKIX_PL_Date_Create_CurrentOffBySeconds
                (CACHE_ITEM_PERIOD_SECONDS,
                &cacheValidUntilDate,
                plContext),
               PKIX_DATECREATECURRENTOFFBYSECONDSFAILED);

        PKIX_CHECK(PKIX_List_AppendItem
                (cachedValues,
                (PKIX_PL_Object *)cacheValidUntilDate,
                plContext),
                PKIX_LISTAPPENDITEMFAILED);

        PKIX_CHECK(PKIX_List_AppendItem
                (cachedValues, (PKIX_PL_Object *)validityDate, plContext),
                PKIX_LISTAPPENDITEMFAILED);

        PKIX_CHECK(PKIX_List_AppendItem
                (cachedValues, (PKIX_PL_Object *)buildResult, plContext),
                PKIX_LISTAPPENDITEMFAILED);

        cachedCertChainError = PKIX_PL_HashTable_Add
                (cachedCertChainTable,
                (PKIX_PL_Object *) cachedKeys,
                (PKIX_PL_Object *) cachedValues,
                plContext);

        pkix_ccAddCount++;

        if (cachedCertChainError != NULL) {
                PKIX_DEBUG("PKIX_PL_HashTable_Add for CertChain skipped: "
                        "entry existed\n");
        }

cleanup:

        PKIX_DECREF(cachedValues);
        PKIX_DECREF(cachedKeys);
        PKIX_DECREF(cachedCertChainError);
        PKIX_DECREF(cacheValidUntilDate);

        PKIX_RETURN(BUILD);
}















































PKIX_Error *
pkix_CacheCert_Lookup(
        PKIX_CertStore *store,
        PKIX_ComCertSelParams *certSelParams,
        PKIX_PL_Date *testDate,
        PKIX_Boolean *pFound,
        PKIX_List** pCerts,
        void *plContext)
{
        PKIX_PL_Cert *cert = NULL;
        PKIX_List *cachedKeys = NULL;
        PKIX_List *cachedValues = NULL;
        PKIX_List *cachedCertList = NULL;
        PKIX_List *selCertList = NULL;
        PKIX_PL_X500Name *subject = NULL;
        PKIX_PL_Date *invalidAfterDate = NULL;
        PKIX_PL_Date *cacheValidUntilDate = NULL;
        PKIX_CertSelector *certSel = NULL;
        PKIX_Error *cachedCertError = NULL;
        PKIX_Error *selectorError = NULL;
        PKIX_CertSelector_MatchCallback selectorMatch = NULL;
        PKIX_Int32 cmpValidTimeResult = PKIX_FALSE;
        PKIX_Int32 cmpCacheTimeResult = 0;
        PKIX_UInt32 numItems = 0;
        PKIX_UInt32 i;

        PKIX_ENTER(BUILD, "pkix_CacheCert_Lookup");
        PKIX_NULLCHECK_TWO(store, certSelParams);
        PKIX_NULLCHECK_TWO(pFound, pCerts);

        *pFound = PKIX_FALSE;

        PKIX_CHECK(PKIX_List_Create(&cachedKeys, plContext),
                PKIX_LISTCREATEFAILED);

        PKIX_CHECK(PKIX_List_AppendItem
                (cachedKeys, (PKIX_PL_Object *)store, plContext),
                PKIX_LISTAPPENDITEMFAILED);

        PKIX_CHECK(PKIX_ComCertSelParams_GetSubject
                (certSelParams, &subject, plContext),
                PKIX_COMCERTSELPARAMSGETSUBJECTFAILED);

        PKIX_NULLCHECK_ONE(subject);

        PKIX_CHECK(PKIX_List_AppendItem
                (cachedKeys, (PKIX_PL_Object *)subject, plContext),
                PKIX_LISTAPPENDITEMFAILED);

        cachedCertError = PKIX_PL_HashTable_Lookup
                    (cachedCertTable,
                    (PKIX_PL_Object *) cachedKeys,
                    (PKIX_PL_Object **) &cachedValues,
                    plContext);
        pkix_cLookupCount++;

        if (cachedValues != NULL && cachedCertError == NULL) {

                PKIX_CHECK(PKIX_List_GetItem
                        (cachedValues,
                        0,
                        (PKIX_PL_Object **) &cacheValidUntilDate,
                        plContext),
                        PKIX_LISTGETITEMFAILED);

                if (testDate) {
                    PKIX_CHECK(PKIX_PL_Object_Compare
                         ((PKIX_PL_Object *)testDate,
                         (PKIX_PL_Object *)cacheValidUntilDate,
                         &cmpCacheTimeResult,
                         plContext),
                         PKIX_OBJECTCOMPARATORFAILED);
                }

                if (cmpCacheTimeResult <= 0) {

                    PKIX_CHECK(PKIX_List_GetItem
                        (cachedValues,
                        1,
                        (PKIX_PL_Object **) &cachedCertList,
                        plContext),
                        PKIX_LISTGETITEMFAILED);

                    



                    PKIX_CHECK(PKIX_CertSelector_Create
                          (NULL, NULL, &certSel, plContext),
                          PKIX_CERTSELECTORCREATEFAILED);

                    PKIX_CHECK(PKIX_CertSelector_SetCommonCertSelectorParams
                          (certSel, certSelParams, plContext),
                          PKIX_CERTSELECTORSETCOMMONCERTSELECTORPARAMSFAILED);

                    PKIX_CHECK(PKIX_CertSelector_GetMatchCallback
                          (certSel, &selectorMatch, plContext),
                          PKIX_CERTSELECTORGETMATCHCALLBACKFAILED);

                    PKIX_CHECK(PKIX_List_Create(&selCertList, plContext),
                            PKIX_LISTCREATEFAILED);

                    



                    PKIX_CHECK(PKIX_List_GetLength
                        (cachedCertList, &numItems, plContext),
                        PKIX_LISTGETLENGTHFAILED);

                    for (i = 0; i < numItems; i++){

                        PKIX_CHECK(PKIX_List_GetItem
                            (cachedCertList,
                            i,
                            (PKIX_PL_Object **)&cert,
                            plContext),
                            PKIX_LISTGETITEMFAILED);

                        PKIX_CHECK(PKIX_PL_Cert_GetValidityNotAfter
                            (cert, &invalidAfterDate, plContext),
                            PKIX_CERTGETVALIDITYNOTAFTERFAILED);

                        if (testDate) {
                            PKIX_CHECK(PKIX_PL_Object_Compare
                                ((PKIX_PL_Object *)invalidAfterDate,
                                (PKIX_PL_Object *)testDate,
                                &cmpValidTimeResult,
                                plContext),
                                PKIX_OBJECTCOMPARATORFAILED);
                        }

                        if (cmpValidTimeResult < 0) {

                            pkix_cRemoveCount++;
                            *pFound = PKIX_FALSE;

                            
                            PKIX_CHECK(PKIX_PL_HashTable_Remove
                                    (cachedCertTable,
                                    (PKIX_PL_Object *) cachedKeys,
                                    plContext),
                                    PKIX_HASHTABLEREMOVEFAILED);
                            goto cleanup;
                        }

                        selectorError = selectorMatch(certSel, cert, plContext);
                        if (!selectorError){
                            
                            PKIX_CHECK(PKIX_List_AppendItem
                                   (selCertList,
                                   (PKIX_PL_Object *)cert,
                                   plContext),
                                  PKIX_LISTAPPENDITEMFAILED);
                        } else {
                            PKIX_DECREF(selectorError);
                        }

                        PKIX_DECREF(cert);
                        PKIX_DECREF(invalidAfterDate);

                    }

                    if (*pFound) {
                        PKIX_INCREF(selCertList);
                        *pCerts = selCertList;
                    }

                } else {

                    pkix_cRemoveCount++;
                    *pFound = PKIX_FALSE;
                    
                    PKIX_CHECK(PKIX_PL_HashTable_Remove
                                (cachedCertTable,
                                (PKIX_PL_Object *) cachedKeys,
                                plContext),
                                PKIX_HASHTABLEREMOVEFAILED);
                }

        } 

cleanup:

        PKIX_DECREF(subject);
        PKIX_DECREF(certSel);
        PKIX_DECREF(cachedKeys);
        PKIX_DECREF(cachedValues);
        PKIX_DECREF(cacheValidUntilDate);
        PKIX_DECREF(cert);
        PKIX_DECREF(cachedCertList);
        PKIX_DECREF(selCertList);
        PKIX_DECREF(invalidAfterDate);
        PKIX_DECREF(cachedCertError);
        PKIX_DECREF(selectorError);

        PKIX_RETURN(BUILD);
}





































PKIX_Error *
pkix_CacheCert_Add(
        PKIX_CertStore *store,
        PKIX_ComCertSelParams *certSelParams,
        PKIX_List* certs,
        void *plContext)
{
        PKIX_List *cachedKeys = NULL;
        PKIX_List *cachedValues = NULL;
        PKIX_PL_Date *cacheValidUntilDate = NULL;
        PKIX_PL_X500Name *subject = NULL;
        PKIX_Error *cachedCertError = NULL;
        PKIX_CertStore_CheckTrustCallback trustCallback = NULL;
        PKIX_UInt32 cachePeriod = CACHE_ITEM_PERIOD_SECONDS;
        PKIX_UInt32 numCerts = 0;

        PKIX_ENTER(BUILD, "pkix_CacheCert_Add");
        PKIX_NULLCHECK_THREE(store, certSelParams, certs);

        PKIX_CHECK(PKIX_List_GetLength(certs, &numCerts,
                                       plContext),
                   PKIX_LISTGETLENGTHFAILED);
        if (numCerts == 0) {
            
            goto cleanup;
        }

        PKIX_CHECK(PKIX_List_Create(&cachedKeys, plContext),
                PKIX_LISTCREATEFAILED);

        PKIX_CHECK(PKIX_List_AppendItem
                (cachedKeys, (PKIX_PL_Object *)store, plContext),
                PKIX_LISTAPPENDITEMFAILED);

        PKIX_CHECK(PKIX_ComCertSelParams_GetSubject
                (certSelParams, &subject, plContext),
                PKIX_COMCERTSELPARAMSGETSUBJECTFAILED);

        PKIX_NULLCHECK_ONE(subject);

        PKIX_CHECK(PKIX_List_AppendItem
                (cachedKeys, (PKIX_PL_Object *)subject, plContext),
                PKIX_LISTAPPENDITEMFAILED);

        PKIX_CHECK(PKIX_List_Create(&cachedValues, plContext),
                PKIX_LISTCREATEFAILED);

        PKIX_CHECK(PKIX_CertStore_GetTrustCallback
                (store, &trustCallback, plContext),
                PKIX_CERTSTOREGETTRUSTCALLBACKFAILED);

        if (trustCallback) {
                cachePeriod = CACHE_TRUST_ITEM_PERIOD_SECONDS;
        }

        PKIX_CHECK(PKIX_PL_Date_Create_CurrentOffBySeconds
               (cachePeriod, &cacheValidUntilDate, plContext),
               PKIX_DATECREATECURRENTOFFBYSECONDSFAILED);

        PKIX_CHECK(PKIX_List_AppendItem
                (cachedValues,
                (PKIX_PL_Object *)cacheValidUntilDate,
                plContext),
                PKIX_LISTAPPENDITEMFAILED);

        PKIX_CHECK(PKIX_List_AppendItem
                (cachedValues,
                (PKIX_PL_Object *)certs,
                plContext),
                PKIX_LISTAPPENDITEMFAILED);

        cachedCertError = PKIX_PL_HashTable_Add
                    (cachedCertTable,
                    (PKIX_PL_Object *) cachedKeys,
                    (PKIX_PL_Object *) cachedValues,
                    plContext);

        pkix_cAddCount++;

        if (cachedCertError != NULL) {
                PKIX_DEBUG("PKIX_PL_HashTable_Add for Certs skipped: "
                        "entry existed\n");
        }

cleanup:

        PKIX_DECREF(subject);
        PKIX_DECREF(cachedKeys);
        PKIX_DECREF(cachedValues);
        PKIX_DECREF(cacheValidUntilDate);
        PKIX_DECREF(cachedCertError);

        PKIX_RETURN(BUILD);
}






































PKIX_Error *
pkix_CacheCrlEntry_Lookup(
        PKIX_CertStore *store,
        PKIX_PL_X500Name *certIssuer,
        PKIX_PL_BigInt *certSerialNumber,
        PKIX_Boolean *pFound,
        PKIX_List** pCrls,
        void *plContext)
{
        PKIX_List *cachedKeys = NULL;
        PKIX_List *cachedCrlEntryList = NULL;
        PKIX_Error *cachedCrlEntryError = NULL;

        PKIX_ENTER(BUILD, "pkix_CacheCrlEntry_Lookup");
        PKIX_NULLCHECK_THREE(store, certIssuer, certSerialNumber);
        PKIX_NULLCHECK_TWO(pFound, pCrls);

        *pFound = PKIX_FALSE;

        
         
        PKIX_CHECK(PKIX_List_Create(&cachedKeys, plContext),
                    PKIX_LISTCREATEFAILED);

        PKIX_CHECK(PKIX_List_AppendItem
                    (cachedKeys, (PKIX_PL_Object *)store, plContext),
                    PKIX_LISTAPPENDITEMFAILED);

        PKIX_CHECK(PKIX_List_AppendItem
                    (cachedKeys, (PKIX_PL_Object *)certIssuer, plContext),
                    PKIX_LISTAPPENDITEMFAILED);

        PKIX_CHECK(PKIX_List_AppendItem
                    (cachedKeys,
                    (PKIX_PL_Object *)certSerialNumber,
                    plContext),
                    PKIX_LISTAPPENDITEMFAILED);

        cachedCrlEntryError = PKIX_PL_HashTable_Lookup
                    (cachedCrlEntryTable,
                    (PKIX_PL_Object *) cachedKeys,
                    (PKIX_PL_Object **) &cachedCrlEntryList,
                    plContext);
        pkix_ceLookupCount++;

        





        if (cachedCrlEntryList != NULL && cachedCrlEntryError == NULL ) {

                PKIX_INCREF(cachedCrlEntryList);
                *pCrls = cachedCrlEntryList;

                *pFound = PKIX_TRUE;

        } else {

                *pFound = PKIX_FALSE;
        }

cleanup:

        PKIX_DECREF(cachedKeys);
        PKIX_DECREF(cachedCrlEntryList);
        PKIX_DECREF(cachedCrlEntryError);

        PKIX_RETURN(BUILD);
}



































PKIX_Error *
pkix_CacheCrlEntry_Add(
        PKIX_CertStore *store,
        PKIX_PL_X500Name *certIssuer,
        PKIX_PL_BigInt *certSerialNumber,
        PKIX_List* crls,
        void *plContext)
{
        PKIX_List *cachedKeys = NULL;
        PKIX_Error *cachedCrlEntryError = NULL;

        PKIX_ENTER(BUILD, "pkix_CacheCrlEntry_Add");
        PKIX_NULLCHECK_THREE(store, certIssuer, certSerialNumber);
        PKIX_NULLCHECK_ONE(crls);

        
         
        PKIX_CHECK(PKIX_List_Create(&cachedKeys, plContext),
                    PKIX_LISTCREATEFAILED);

        PKIX_CHECK(PKIX_List_AppendItem
                    (cachedKeys, (PKIX_PL_Object *)store, plContext),
                    PKIX_LISTAPPENDITEMFAILED);

        PKIX_CHECK(PKIX_List_AppendItem
                    (cachedKeys, (PKIX_PL_Object *)certIssuer, plContext),
                    PKIX_LISTAPPENDITEMFAILED);

        PKIX_CHECK(PKIX_List_AppendItem
                    (cachedKeys,
                    (PKIX_PL_Object *)certSerialNumber,
                    plContext),
                    PKIX_LISTAPPENDITEMFAILED);

        cachedCrlEntryError = PKIX_PL_HashTable_Add
                    (cachedCrlEntryTable,
                    (PKIX_PL_Object *) cachedKeys,
                    (PKIX_PL_Object *) crls,
                    plContext);
        pkix_ceAddCount++;

cleanup:

        PKIX_DECREF(cachedKeys);
        PKIX_DECREF(cachedCrlEntryError);

        PKIX_RETURN(BUILD);
}

#ifdef PKIX_OBJECT_LEAK_TEST









#define TEST_START_FN "PKIX_BuildChain"

PKIX_Error*
pkix_CheckForGeneratedError(PKIX_StdVars * stdVars, 
                            PKIX_ERRORCLASS errClass, 
                            char * fnName,
                            PKIX_Boolean *errSetFlag,
                            void * plContext)
{
    PKIX_Error *genErr = NULL;
    PKIX_UInt32 pos = 0;
    PKIX_UInt32 strLen = 0;

    if (fnName) { 
        if (fnStackNameArr[testStartFnStackPosition] == NULL ||
            strcmp(fnStackNameArr[testStartFnStackPosition], TEST_START_FN)
            ) {
            
            return NULL;
        }
        if (!strcmp(fnName, TEST_START_FN)) {
            *errSetFlag = PKIX_TRUE;
            noErrorState = PKIX_FALSE;
            errorGenerated = PKIX_FALSE;
        }
    }   

    if (noErrorState || errorGenerated)  return NULL;

    if (fnName && (
        !strcmp(fnName, "PKIX_PL_Object_DecRef") ||
        !strcmp(fnName, "PKIX_PL_Object_Unlock") ||
        !strcmp(fnName, "pkix_UnlockObject") ||
        !strcmp(fnName, "pkix_Throw") ||
        !strcmp(fnName, "pkix_trace_dump_cert") ||
        !strcmp(fnName, "PKIX_PL_Free"))) {
        
        noErrorState = PKIX_TRUE;
        *errSetFlag = PKIX_TRUE;
        return NULL;
    }

    if (PL_HashTableLookup(fnInvTable, &fnStackInvCountArr[stackPosition - 1])) {
        return NULL;
    }

    PL_HashTableAdd(fnInvTable, &fnStackInvCountArr[stackPosition - 1], nonNullValue);
    errorGenerated = PKIX_TRUE;
    noErrorState = PKIX_TRUE;
    genErr = PKIX_DoThrow(stdVars, errClass, PKIX_MEMLEAKGENERATEDERROR,
                          errClass, plContext);
    while(fnStackNameArr[pos]) {
        strLen += PORT_Strlen(fnStackNameArr[pos++]) + 1;
    }
    strLen += 1; 
    pos = 0;
    errorFnStackString = PORT_ZAlloc(strLen);
    while(fnStackNameArr[pos]) {
        strcat(errorFnStackString, "/");
        strcat(errorFnStackString, fnStackNameArr[pos++]);
    }
    noErrorState = PKIX_FALSE;
    
    return genErr;
}
#endif 
