










































#include "pkix_pl_pk11certstore.h"


























static PKIX_Error *
pkix_pl_Pk11CertStore_CheckTrust(
        PKIX_CertStore *store,
        PKIX_PL_Cert *cert,
        PKIX_Boolean *pTrusted,
        void *plContext)
{
        SECStatus rv = SECFailure;
        PKIX_Boolean trusted = PKIX_FALSE;
        SECCertUsage certUsage = 0;
        SECCertificateUsage certificateUsage;
        unsigned int requiredFlags;
        SECTrustType trustType;
        CERTCertTrust trust;

        PKIX_ENTER(CERTSTORE, "pkix_pl_Pk11CertStore_CheckTrust");
        PKIX_NULLCHECK_THREE(store, cert, pTrusted);
        PKIX_NULLCHECK_ONE(cert->nssCert);

        certificateUsage = ((PKIX_PL_NssContext*)plContext)->certificateUsage;

        
        PORT_Assert(!(certificateUsage & (certificateUsage - 1)));

        
        while (0 != (certificateUsage = certificateUsage >> 1)) { certUsage++; }

        rv = CERT_TrustFlagsForCACertUsage(certUsage, &requiredFlags, &trustType);
        if (rv == SECSuccess) {
                rv = CERT_GetCertTrust(cert->nssCert, &trust);
        }

        if (rv == SECSuccess) {
            unsigned int certFlags;

            if (certUsage != certUsageAnyCA &&
                certUsage != certUsageStatusResponder) {
                CERTCertificate *nssCert = cert->nssCert;
                
                if (certUsage == certUsageVerifyCA) {
                    if (nssCert->nsCertType & NS_CERT_TYPE_EMAIL_CA) {
                        trustType = trustEmail;
                    } else if (nssCert->nsCertType & NS_CERT_TYPE_SSL_CA) {
                        trustType = trustSSL;
                    } else {
                        trustType = trustObjectSigning;
                    }
                }
                
                certFlags = SEC_GET_TRUST_FLAGS((&trust), trustType);
                if ((certFlags & requiredFlags) == requiredFlags) {
                    trusted = PKIX_TRUE;
                }
            } else {
                for (trustType = trustSSL; trustType < trustTypeNone;
                     trustType++) {
                    certFlags =
                        SEC_GET_TRUST_FLAGS((&trust), trustType);
                    if ((certFlags & requiredFlags) == requiredFlags) {
                        trusted = PKIX_TRUE;
                        break;
                    }
                }
            }
        }

        *pTrusted = trusted;

        PKIX_RETURN(CERTSTORE);
}




























static PKIX_Error *
pkix_pl_Pk11CertStore_CertQuery(
        PKIX_ComCertSelParams *params,
        PKIX_List **pSelected,
        void *plContext)
{
        PRBool validOnly = PR_FALSE;
        PRTime prtime = 0;
        PKIX_PL_X500Name *subjectName = NULL;
        PKIX_PL_Date *certValid = NULL;
        PKIX_List *certList = NULL;
        PKIX_PL_Cert *cert = NULL;
        CERTCertList *pk11CertList = NULL;
        CERTCertListNode *node = NULL;
        CERTCertificate *nssCert = NULL;
        CERTCertDBHandle *dbHandle = NULL;

        PRArenaPool *arena = NULL;
        SECItem *nameItem = NULL;
        void *wincx = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_Pk11CertStore_CertQuery");
        PKIX_NULLCHECK_TWO(params, pSelected);

        
        PKIX_PL_NSSCALLRV(CERTSTORE, dbHandle, CERT_GetDefaultCertDB, ());

        










        PKIX_CHECK(PKIX_ComCertSelParams_GetSubject
                (params, &subjectName, plContext),
                PKIX_COMCERTSELPARAMSGETSUBJECTFAILED);

        PKIX_CHECK(PKIX_ComCertSelParams_GetCertificateValid
                (params, &certValid, plContext),
                PKIX_COMCERTSELPARAMSGETCERTIFICATEVALIDFAILED);

        
        if (certValid) {
                PKIX_CHECK(pkix_pl_Date_GetPRTime
                        (certValid, &prtime, plContext),
                        PKIX_DATEGETPRTIMEFAILED);
                validOnly = PR_TRUE;
        }

        




        if (subjectName) {
                arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
                if (arena) {

                        PKIX_CHECK(pkix_pl_X500Name_GetDERName
                                (subjectName, arena, &nameItem, plContext),
                                PKIX_X500NAMEGETSECNAMEFAILED);

                        if (nameItem) {

                            PKIX_PL_NSSCALLRV
                                (CERTSTORE,
                                pk11CertList,
                                CERT_CreateSubjectCertList,
                                (NULL, dbHandle, nameItem, prtime, validOnly));
                        }
                        PKIX_PL_NSSCALL
                                (CERTSTORE, PORT_FreeArena, (arena, PR_FALSE));
                        arena = NULL;
                }

        } else {

                PKIX_CHECK(pkix_pl_NssContext_GetWincx
                        ((PKIX_PL_NssContext *)plContext, &wincx),
                        PKIX_NSSCONTEXTGETWINCXFAILED);

                PKIX_PL_NSSCALLRV
                        (CERTSTORE,
                        pk11CertList,
                        PK11_ListCerts,
                        (PK11CertListAll, wincx));
        }

        if (pk11CertList) {

                PKIX_CHECK(PKIX_List_Create(&certList, plContext),
                        PKIX_LISTCREATEFAILED);

                for (node = CERT_LIST_HEAD(pk11CertList);
                    !(CERT_LIST_END(node, pk11CertList));
                    node = CERT_LIST_NEXT(node)) {

                        PKIX_PL_NSSCALLRV
                                (CERTSTORE,
                                nssCert,
                                CERT_NewTempCertificate,
                                        (dbHandle,
                                        &(node->cert->derCert),
                                        NULL, 
                                        PR_FALSE,
                                        PR_TRUE)); 

                        if (!nssCert) {
                                continue; 
                        }

                        PKIX_CHECK_ONLY_FATAL(pkix_pl_Cert_CreateWithNSSCert
                                (nssCert, &cert, plContext),
                                PKIX_CERTCREATEWITHNSSCERTFAILED);

                        if (PKIX_ERROR_RECEIVED) {
                                CERT_DestroyCertificate(nssCert);
                                nssCert = NULL;
                                continue; 
                        }

                        PKIX_CHECK_ONLY_FATAL(PKIX_List_AppendItem
                                (certList, (PKIX_PL_Object *)cert, plContext),
                                PKIX_LISTAPPENDITEMFAILED);

                        PKIX_DECREF(cert);

                }

                
                pkixTempErrorReceived = PKIX_FALSE;
        }

        *pSelected = certList;
        certList = NULL;

cleanup:
        
        if (pk11CertList) {
            CERT_DestroyCertList(pk11CertList);
        }
        if (arena) {
            PORT_FreeArena(arena, PR_FALSE);
        }

        PKIX_DECREF(subjectName);
        PKIX_DECREF(certValid);
        PKIX_DECREF(cert);
        PKIX_DECREF(certList);

        PKIX_RETURN(CERTSTORE);
}



















static PKIX_Error *
pkix_pl_Pk11CertStore_ImportCrl(
        PKIX_CertStore *store,
        PKIX_PL_X500Name *issuerName,
        PKIX_List *crlList,
        void *plContext)
{
    CERTCertDBHandle *certHandle = CERT_GetDefaultCertDB();
    PKIX_PL_CRL *crl = NULL;
    SECItem *derCrl = NULL;

    PKIX_ENTER(CERTSTORE, "pkix_pl_Pk11CertStore_ImportCrl");
    PKIX_NULLCHECK_TWO(store, plContext);
    
    if (!crlList) {
        goto cleanup;
    }
    while (crlList->length > 0) {
        PKIX_CHECK(
            PKIX_List_GetItem(crlList, 0, (PKIX_PL_Object**)&crl,
                              plContext),
            PKIX_LISTGETITEMFAILED);

        


        PKIX_CHECK(
            PKIX_List_DeleteItem(crlList, 0, plContext),
            PKIX_LISTDELETEITEMFAILED);

        
        pkixErrorResult =
            PKIX_PL_CRL_ReleaseDerCrl(crl, &derCrl, plContext);
        PORT_Assert(!pkixErrorResult && derCrl);
        if (pkixErrorResult || !derCrl) {
            

            PKIX_DECREF(pkixErrorResult);
            PKIX_DECREF(crl);
            continue;
        }
        cert_CacheCRLByGeneralName(certHandle, derCrl, 
                                        crl->derGenName);
        

        derCrl = NULL;
        PKIX_DECREF(crl);
    }

cleanup:
    PKIX_DECREF(crl);

    PKIX_RETURN(CERTSTORE);
}

static PKIX_Error *
NameCacheHasFetchedCrlInfo(PKIX_PL_Cert *pkixCert,
                           PRTime time,
                           PKIX_Boolean *pHasFetchedCrlInCache,
                           void *plContext)
{
    

    NamedCRLCache* nameCrlCache = NULL;
    PKIX_Boolean hasFetchedCrlInCache = PKIX_TRUE;
    PKIX_List *dpList = NULL;
    pkix_pl_CrlDp *dp = NULL;
    CERTCertificate *cert;
    PKIX_UInt32 dpIndex = 0;
    SECStatus rv = SECSuccess;
    PRTime reloadDelay = 0, badCrlInvalDelay = 0;

    PKIX_ENTER(CERTSTORE, "ChechCacheHasFetchedCrl");

    cert = pkixCert->nssCert;
    reloadDelay = 
        ((PKIX_PL_NssContext*)plContext)->crlReloadDelay *
                                                PR_USEC_PER_SEC;
    badCrlInvalDelay =
        ((PKIX_PL_NssContext*)plContext)->badDerCrlReloadDelay *
                                                PR_USEC_PER_SEC;
    if (!time) {
        time = PR_Now();
    }
    

    PKIX_CHECK(
        PKIX_PL_Cert_GetCrlDp(pkixCert, &dpList, plContext),
        PKIX_CERTGETCRLDPFAILED);
    if (dpList && dpList->length) {
        hasFetchedCrlInCache = PKIX_FALSE;
        rv = cert_AcquireNamedCRLCache(&nameCrlCache);
        if (rv != SECSuccess) {
            PKIX_DECREF(dpList);
        }
    } else {
        

        PKIX_DECREF(dpList);
    }
    for (;!hasFetchedCrlInCache &&
             dpList && dpIndex < dpList->length;dpIndex++) {
        SECItem **derDpNames = NULL;
        pkixErrorResult =
            PKIX_List_GetItem(dpList, dpIndex, (PKIX_PL_Object **)&dp,
                              plContext);
        if (pkixErrorResult) {
            PKIX_DECREF(pkixErrorResult);
            continue;
        }
        if (dp->nssdp->distPointType == generalName) {
            
            derDpNames = dp->nssdp->derFullName;
        }
        while (derDpNames && *derDpNames != NULL) {
            NamedCRLCacheEntry* cacheEntry = NULL;
            const SECItem *derDpName = *derDpNames++;
            rv = cert_FindCRLByGeneralName(nameCrlCache, derDpName,
                                           &cacheEntry);
            if (rv == SECSuccess && cacheEntry) {
                if ((cacheEntry->inCRLCache &&
                    (cacheEntry->successfulInsertionTime + reloadDelay > time ||
                     (cacheEntry->dupe &&
                      cacheEntry->lastAttemptTime + reloadDelay > time))) ||
                    (cacheEntry->badDER &&
                     cacheEntry->lastAttemptTime + badCrlInvalDelay > time)) {
                    hasFetchedCrlInCache = PKIX_TRUE;
                    break;
                }
            }
        }
        PKIX_DECREF(dp);
    }
cleanup:
    *pHasFetchedCrlInCache = hasFetchedCrlInCache;
    if (nameCrlCache) {
        cert_ReleaseNamedCRLCache(nameCrlCache);
    }
    PKIX_DECREF(dpList);

    PKIX_RETURN(CERTSTORE);
}



















static PKIX_Error *
pkix_pl_Pk11CertStore_CheckRevByCrl(
        PKIX_CertStore *store,
        PKIX_PL_Cert *pkixCert,
        PKIX_PL_Cert *pkixIssuer,
        PKIX_PL_Date *date,
        PKIX_Boolean  crlDownloadDone,
        PKIX_UInt32  *pReasonCode,
        PKIX_RevocationStatus *pStatus,
        void *plContext)
{
    PKIX_RevocationStatus pkixRevStatus = PKIX_RevStatus_NoInfo;
    CERTRevocationStatus revStatus = certRevocationStatusUnknown;
    PKIX_Boolean hasFetchedCrlInCache = PKIX_TRUE;
    CERTCertificate *cert = NULL, *issuer = NULL;
    SECStatus rv = SECSuccess;
    void *wincx = NULL;
    PRTime time = 0;

    PKIX_ENTER(CERTSTORE, "pkix_pl_Pk11CertStore_CheckRevByCrl");
    PKIX_NULLCHECK_FOUR(store, pkixCert, pkixIssuer, plContext);

    cert = pkixCert->nssCert;
    issuer = pkixIssuer->nssCert;
    if (date) {
        PKIX_CHECK(
            pkix_pl_Date_GetPRTime(date, &time, plContext),
            PKIX_DATEGETPRTIMEFAILED);
    }
    PKIX_CHECK(
        pkix_pl_NssContext_GetWincx((PKIX_PL_NssContext*)plContext,
                                    &wincx),
        PKIX_NSSCONTEXTGETWINCXFAILED);
    



    rv = cert_CheckCertRevocationStatus(cert, issuer, NULL,
                                        

                                        time, wincx, &revStatus, pReasonCode);
    if (rv == SECFailure) {
        pkixRevStatus = PKIX_RevStatus_Revoked;
        goto cleanup;
    }
    if (crlDownloadDone) {
        if (revStatus == certRevocationStatusRevoked) {
            pkixRevStatus = PKIX_RevStatus_Revoked;
        } else if (revStatus == certRevocationStatusValid) {
            pkixRevStatus = PKIX_RevStatus_Success;
        } 
    } else {
        pkixErrorResult =
            NameCacheHasFetchedCrlInfo(pkixCert, time, &hasFetchedCrlInCache,
                                       plContext);
        if (pkixErrorResult) {
            goto cleanup;
        }
        if (revStatus == certRevocationStatusRevoked &&
            (hasFetchedCrlInCache ||
             *pReasonCode != crlEntryReasoncertificatedHold)) {
            pkixRevStatus = PKIX_RevStatus_Revoked;
        } else if (revStatus == certRevocationStatusValid &&
                   hasFetchedCrlInCache) {
            pkixRevStatus = PKIX_RevStatus_Success;
        }
    }
cleanup:
    *pStatus = pkixRevStatus;

    PKIX_RETURN(CERTSTORE);    
}






PKIX_Error *
pkix_pl_Pk11CertStore_GetCert(
        PKIX_CertStore *store,
        PKIX_CertSelector *selector,
        PKIX_VerifyNode *parentVerifyNode,
        void **pNBIOContext,
        PKIX_List **pCertList,
        void *plContext)
{
        PKIX_UInt32 i = 0;
        PKIX_UInt32 numFound = 0;
        PKIX_PL_Cert *candidate = NULL;
        PKIX_List *selected = NULL;
        PKIX_List *filtered = NULL;
        PKIX_CertSelector_MatchCallback selectorCallback = NULL;
        PKIX_CertStore_CheckTrustCallback trustCallback = NULL;
        PKIX_ComCertSelParams *params = NULL;
        PKIX_Boolean cacheFlag = PKIX_FALSE;
        PKIX_VerifyNode *verifyNode = NULL;
        PKIX_Error *selectorError = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_Pk11CertStore_GetCert");
        PKIX_NULLCHECK_FOUR(store, selector, pNBIOContext, pCertList);

        *pNBIOContext = NULL;   

        PKIX_CHECK(PKIX_CertSelector_GetMatchCallback
                (selector, &selectorCallback, plContext),
                PKIX_CERTSELECTORGETMATCHCALLBACKFAILED);

        PKIX_CHECK(PKIX_CertSelector_GetCommonCertSelectorParams
                (selector, &params, plContext),
                PKIX_CERTSELECTORGETCOMCERTSELPARAMSFAILED);

        PKIX_CHECK(pkix_pl_Pk11CertStore_CertQuery
                (params, &selected, plContext),
                PKIX_PK11CERTSTORECERTQUERYFAILED);

        if (selected) {
                PKIX_CHECK(PKIX_List_GetLength(selected, &numFound, plContext),
                        PKIX_LISTGETLENGTHFAILED);
        }

        PKIX_CHECK(PKIX_CertStore_GetCertStoreCacheFlag
                (store, &cacheFlag, plContext),
                PKIX_CERTSTOREGETCERTSTORECACHEFLAGFAILED);

        PKIX_CHECK(PKIX_CertStore_GetTrustCallback
                (store, &trustCallback, plContext),
                PKIX_CERTSTOREGETTRUSTCALLBACKFAILED);

        PKIX_CHECK(PKIX_List_Create(&filtered, plContext),
                PKIX_LISTCREATEFAILED);

        for (i = 0; i < numFound; i++) {
                PKIX_CHECK_ONLY_FATAL(PKIX_List_GetItem
                        (selected,
                        i,
                        (PKIX_PL_Object **)&candidate,
                        plContext),
                        PKIX_LISTGETITEMFAILED);

                if (PKIX_ERROR_RECEIVED) {
                        continue; 
                }

                selectorError =
                    selectorCallback(selector, candidate, plContext);
                if (!selectorError) {
                        PKIX_CHECK(PKIX_PL_Cert_SetCacheFlag
                                (candidate, cacheFlag, plContext),
                                PKIX_CERTSETCACHEFLAGFAILED);

                        if (trustCallback) {
                                PKIX_CHECK(PKIX_PL_Cert_SetTrustCertStore
                                    (candidate, store, plContext),
                                    PKIX_CERTSETTRUSTCERTSTOREFAILED);
                        }

                        PKIX_CHECK_ONLY_FATAL(PKIX_List_AppendItem
                                (filtered,
                                (PKIX_PL_Object *)candidate,
                                plContext),
                                PKIX_LISTAPPENDITEMFAILED);
                } else if (parentVerifyNode) {
                    PKIX_CHECK_FATAL(
                        pkix_VerifyNode_Create(candidate, 0, selectorError,
                                               &verifyNode, plContext),
                        PKIX_VERIFYNODECREATEFAILED);
                    PKIX_CHECK_FATAL(
                        pkix_VerifyNode_AddToTree(parentVerifyNode,
                                                  verifyNode,
                                                  plContext),
                        PKIX_VERIFYNODEADDTOTREEFAILED);
                    PKIX_DECREF(verifyNode);
                }
                PKIX_DECREF(selectorError);
                PKIX_DECREF(candidate);
        }

        
        pkixTempErrorReceived = PKIX_FALSE;

        *pCertList = filtered;
        filtered = NULL;

cleanup:
fatal:
        PKIX_DECREF(filtered);
        PKIX_DECREF(candidate);
        PKIX_DECREF(selected);
        PKIX_DECREF(params);
        PKIX_DECREF(verifyNode);
        PKIX_DECREF(selectorError);

        PKIX_RETURN(CERTSTORE);
}

static PKIX_Error *
RemovePartitionedDpsFromList(PKIX_List *dpList, PKIX_PL_Date *date,
                             void *plContext)
{
    NamedCRLCache* nameCrlCache = NULL;
    pkix_pl_CrlDp *dp = NULL;
    int dpIndex = 0;
    PRTime time;
    PRTime reloadDelay = 0, badCrlInvalDelay = 0;
    SECStatus rv;

    PKIX_ENTER(CERTSTORE, "pkix_pl_Pk11CertStore_ListRemovePrtDp");

    if (!dpList || !dpList->length) {
        PKIX_RETURN(CERTSTORE);
    }
    reloadDelay = 
        ((PKIX_PL_NssContext*)plContext)->crlReloadDelay *
                                                PR_USEC_PER_SEC;
    badCrlInvalDelay =
        ((PKIX_PL_NssContext*)plContext)->badDerCrlReloadDelay *
                                                PR_USEC_PER_SEC;
    PKIX_CHECK(pkix_pl_Date_GetPRTime(date, &time, plContext),
               PKIX_DATEGETPRTIMEFAILED);
    rv = cert_AcquireNamedCRLCache(&nameCrlCache);
    if (rv == SECFailure) {
        
        PKIX_RETURN(CERTSTORE);
    }
    while (dpIndex < dpList->length) {
        SECItem **derDpNames = NULL;
        PKIX_Boolean removeDp = PKIX_FALSE;
        
        PKIX_CHECK(
            PKIX_List_GetItem(dpList, dpIndex, (PKIX_PL_Object **)&dp,
                              plContext),
            PKIX_LISTGETITEMFAILED);
        if (!dp->isPartitionedByReasonCode) {
            

            if (dp->nssdp->distPointType == generalName) {
                
                derDpNames = dp->nssdp->derFullName;
            } else {
                removeDp = PKIX_TRUE;
            }
            while (derDpNames && *derDpNames != NULL) {
                NamedCRLCacheEntry* cacheEntry = NULL;
                const SECItem *derDpName = *derDpNames++;
                
                rv = cert_FindCRLByGeneralName(nameCrlCache, derDpName,
                                               &cacheEntry);
                if (rv && cacheEntry) {
                    if (cacheEntry->unsupported ||
                        (cacheEntry->inCRLCache &&
                         (cacheEntry->successfulInsertionTime + reloadDelay > time ||
                          (cacheEntry->dupe &&
                           cacheEntry->lastAttemptTime + reloadDelay > time))) ||
                          (cacheEntry->badDER &&
                           cacheEntry->lastAttemptTime + badCrlInvalDelay > time)) {
                        removeDp = PKIX_TRUE;
                    }
                }
            }
        } else {
            


            removeDp = PKIX_TRUE;
        }
        if (removeDp) {
            PKIX_CHECK_ONLY_FATAL(
                pkix_List_Remove(dpList,(PKIX_PL_Object*)dp,
                                 plContext),
                PKIX_LISTGETITEMFAILED); 
        } else {
            dpIndex += 1;
        }
        PKIX_DECREF(dp);
    }

cleanup:
    if (nameCrlCache) {
        cert_ReleaseNamedCRLCache(nameCrlCache);
    }
    PKIX_DECREF(dp);
    
    PKIX_RETURN(CERTSTORE);
}




static PKIX_Error *
DownloadCrl(pkix_pl_CrlDp *dp, PKIX_PL_CRL **crl,
            const SEC_HttpClientFcnV1 *hcv1, void *plContext)
{
    char *location = NULL;
    char *hostname = NULL;
    char *path = NULL;
    PRUint16 port;
    SEC_HTTP_SERVER_SESSION pServerSession = NULL;
    SEC_HTTP_REQUEST_SESSION pRequestSession = NULL;
    PRUint16 myHttpResponseCode;
    const char *myHttpResponseData = NULL;
    PRUint32 myHttpResponseDataLen;
    SECItem *uri = NULL;
    SECItem *derCrlCopy = NULL;
    CERTSignedCrl *nssCrl = NULL;
    CERTGeneralName *genName = NULL;
    PKIX_Int32 savedError = -1;
    SECItem **derGenNames = NULL;
    SECItem  *derGenName = NULL;

    PKIX_ENTER(CERTSTORE, "pkix_pl_Pk11CertStore_DownloadCrl");

    

    if (dp->distPointType != generalName ||
        !dp->nssdp->derFullName) {
        PKIX_ERROR(PKIX_UNSUPPORTEDCRLDPTYPE);
    }
    genName = dp->name.fullName;
    derGenNames = dp->nssdp->derFullName;
    do {
        derGenName = *derGenNames;
        do {
            if (!derGenName ||
                !genName->name.other.data) {
                
                savedError = PKIX_UNSUPPORTEDCRLDPTYPE;
                break;
            }
            uri = &genName->name.other;
            location = (char*)PR_Malloc(1 + uri->len);
            if (!location) {
                savedError = PKIX_ALLOCERROR;
                break;
            }
            PORT_Memcpy(location, uri->data, uri->len);
            location[uri->len] = 0;
            if (CERT_ParseURL(location, &hostname,
                              &port, &path) != SECSuccess) {
                PORT_SetError(SEC_ERROR_BAD_INFO_ACCESS_LOCATION);
                savedError = PKIX_URLPARSINGFAILED;
                break;
            }
    
            PORT_Assert(hostname != NULL);
            PORT_Assert(path != NULL);

            if ((*hcv1->createSessionFcn)(hostname, port, 
                                          &pServerSession) != SECSuccess) {
                PORT_SetError(SEC_ERROR_BAD_INFO_ACCESS_LOCATION);
                savedError = PKIX_URLPARSINGFAILED;
                break;
            }

            if ((*hcv1->createFcn)(pServerSession, "http", path, "GET",
                



                          PR_SecondsToInterval(
                              ((PKIX_PL_NssContext*)plContext)->timeoutSeconds),
                                   &pRequestSession) != SECSuccess) {
                savedError = PKIX_HTTPSERVERERROR;
                break;
            }

            myHttpResponseDataLen =
                ((PKIX_PL_NssContext*)plContext)->maxResponseLength;

            




            
            if ((*hcv1->trySendAndReceiveFcn)(
                    pRequestSession, 
                    NULL,
                    &myHttpResponseCode,
                    NULL,
                    NULL,
                    &myHttpResponseData,
                    &myHttpResponseDataLen) != SECSuccess) {
                savedError = PKIX_HTTPSERVERERROR;
                break;
            }

            if (myHttpResponseCode != 200) {
                savedError = PKIX_HTTPSERVERERROR;
                break;
            }
        } while(0);
        if (!myHttpResponseData) {
            
            genName = CERT_GetNextGeneralName(genName);
            derGenNames++;
        }
        

    } while (!myHttpResponseData && *derGenNames &&
             genName != dp->name.fullName);
    
    PORT_Assert(derGenName);

    if (!myHttpResponseData) {
        
        SECItem derCrl = {siBuffer, (void*)"BadCrl", 6 };
        
        derCrlCopy = SECITEM_DupItem(&derCrl);
        if (!derCrlCopy) {
            PKIX_ERROR(PKIX_ALLOCERROR);
        }
        derGenName = *dp->nssdp->derFullName;
    } else {
        SECItem derCrl = { siBuffer,
                           (void*)myHttpResponseData,
                           myHttpResponseDataLen };
        derCrlCopy = SECITEM_DupItem(&derCrl);
        if (!derCrlCopy) {
            PKIX_ERROR(PKIX_ALLOCERROR);
        }
        
        nssCrl =
            CERT_DecodeDERCrlWithFlags(NULL, derCrlCopy, SEC_CRL_TYPE,
                                       CRL_DECODE_DONT_COPY_DER |
                                       CRL_DECODE_SKIP_ENTRIES);
    }
    
    PKIX_CHECK(
        pkix_pl_CRL_CreateWithSignedCRL(nssCrl, derCrlCopy,
                                        derGenName,
                                        crl, plContext),
        PKIX_CRLCREATEWITHSIGNEDCRLFAILED);
    
    derCrlCopy = NULL;
    nssCrl = NULL;

cleanup:
    if (derCrlCopy)
        PORT_Free(derCrlCopy);
    if (nssCrl)
        SEC_DestroyCrl(nssCrl);
    if (pRequestSession != NULL) 
        (*hcv1->freeFcn)(pRequestSession);
    if (pServerSession != NULL)
        (*hcv1->freeSessionFcn)(pServerSession);
    if (path != NULL)
        PORT_Free(path);
    if (hostname != NULL)
        PORT_Free(hostname);
    if (location) {
        PORT_Free(location);
    }

    PKIX_RETURN(CERTSTORE);
}





static PKIX_Error *
pkix_pl_Pk11CertStore_GetCRL(
        PKIX_CertStore *store,
        PKIX_CRLSelector *selector,
        void **pNBIOContext,
        PKIX_List **pCrlList,
        void *plContext)
{
    PKIX_UInt32 dpIndex = 0;
    PKIX_PL_CRL *crl = NULL;
    PKIX_List *crlList = NULL;
    PKIX_List *dpList = NULL;
    pkix_pl_CrlDp *dp = NULL;
    PKIX_PL_Date *date = NULL;
    const SEC_HttpClientFcn *registeredHttpClient = NULL;

    PKIX_ENTER(CERTSTORE, "pkix_pl_Pk11CertStore_GetCRL");
    PKIX_NULLCHECK_THREE(store, pNBIOContext, pCrlList);
    PKIX_NULLCHECK_TWO(selector, selector->params);

    registeredHttpClient = SEC_GetRegisteredHttpClient();
    if (!registeredHttpClient || registeredHttpClient->version != 1) {
        goto cleanup;
    }
    dpList = selector->params->crldpList;
    date = selector->params->date;
    PKIX_CHECK(
        RemovePartitionedDpsFromList(dpList, date,
                                     plContext),
        PKIX_FAILTOREMOVEDPFROMLIST);
    for (;dpIndex < dpList->length;dpIndex++) {
        PKIX_DECREF(dp);
        pkixErrorResult =
            PKIX_List_GetItem(dpList, dpIndex,
                              (PKIX_PL_Object **)&dp,
                              plContext);
        if (pkixErrorResult) {
            PKIX_DECREF(pkixErrorResult);
            continue;
        }
        pkixErrorResult =
            DownloadCrl(dp, &crl,
                        &registeredHttpClient->fcnTable.ftable1,
                        plContext);
        if (pkixErrorResult || !crl) {
            

            PKIX_DECREF(pkixErrorResult);
            continue;
        }
        if (!crlList) {
            PKIX_CHECK(PKIX_List_Create(&crlList, plContext),
                       PKIX_LISTCREATEFAILED);
        }
        pkixErrorResult =
            PKIX_List_AppendItem(crlList, (PKIX_PL_Object *)crl,
                                 plContext);
        if (pkixErrorResult) {
            PKIX_DECREF(pkixErrorResult);
        }
        PKIX_DECREF(crl);
    }
    *pCrlList = crlList;
    crlList = NULL;

cleanup:
    PKIX_DECREF(dp);
    PKIX_DECREF(crl);
    PKIX_DECREF(crlList);

    PKIX_RETURN(CERTSTORE);
}








PKIX_Error *
PKIX_PL_Pk11CertStore_Create(
        PKIX_CertStore **pCertStore,
        void *plContext)
{
        PKIX_CertStore *certStore = NULL;

        PKIX_ENTER(CERTSTORE, "PKIX_PL_Pk11CertStore_Create");
        PKIX_NULLCHECK_ONE(pCertStore);

        PKIX_CHECK(PKIX_CertStore_Create
                (pkix_pl_Pk11CertStore_GetCert,
                pkix_pl_Pk11CertStore_GetCRL,
                NULL, 
                NULL, 
                pkix_pl_Pk11CertStore_CheckTrust,
                pkix_pl_Pk11CertStore_ImportCrl,
                pkix_pl_Pk11CertStore_CheckRevByCrl,
                NULL,
                PKIX_TRUE, 
                PKIX_TRUE, 
                &certStore,
                plContext),
                PKIX_CERTSTORECREATEFAILED);

        *pCertStore = certStore;

cleanup:

        PKIX_RETURN(CERTSTORE);
}
