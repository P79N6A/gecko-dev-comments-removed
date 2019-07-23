












































#include "pkix_pl_httpcertstore.h"
extern PKIX_PL_HashTable *httpSocketCache;
SEC_ASN1_MKSUB(CERT_IssuerAndSNTemplate)
SEC_ASN1_MKSUB(SECOID_AlgorithmIDTemplate)
SEC_ASN1_MKSUB(SEC_SetOfAnyTemplate)
SEC_ASN1_MKSUB(CERT_SetOfSignedCrlTemplate)

SEC_ASN1_CHOOSER_DECLARE(CERT_IssuerAndSNTemplate)
SEC_ASN1_CHOOSER_DECLARE(SECOID_AlgorithmIDTemplate)
































static PKIX_Error *
pkix_pl_HttpCertStoreContext_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        const SEC_HttpClientFcnV1 *hcv1 = NULL;
        PKIX_PL_HttpCertStoreContext *context = NULL;

        PKIX_ENTER
                (HTTPCERTSTORECONTEXT, "pkix_pl_HttpCertStoreContext_Destroy");
        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_CheckType
                    (object, PKIX_HTTPCERTSTORECONTEXT_TYPE, plContext),
                    PKIX_OBJECTNOTANHTTPCERTSTORECONTEXT);

        context = (PKIX_PL_HttpCertStoreContext *)object;
        hcv1 = (const SEC_HttpClientFcnV1 *)(context->client);
        if (context->requestSession != NULL) {
            (*hcv1->freeFcn)(context->requestSession);
            context->requestSession = NULL;
        }
        if (context->serverSession != NULL) {
            (*hcv1->freeSessionFcn)(context->serverSession);
            context->serverSession = NULL;
        }
        if (context->path != NULL) {
            PORT_Free(context->path);
            context->path = NULL;
        }

cleanup:

        PKIX_RETURN(HTTPCERTSTORECONTEXT);
}















PKIX_Error *
pkix_pl_HttpCertStoreContext_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry *entry = &systemClasses[PKIX_HTTPCERTSTORECONTEXT_TYPE];

        PKIX_ENTER(HTTPCERTSTORECONTEXT,
                "pkix_pl_HttpCertStoreContext_RegisterSelf");

        entry->description = "HttpCertStoreContext";
        entry->typeObjectSize = sizeof(PKIX_PL_HttpCertStoreContext);
        entry->destructor = pkix_pl_HttpCertStoreContext_Destroy;

        PKIX_RETURN(HTTPCERTSTORECONTEXT);
}




typedef struct callbackContextStruct  {
        PKIX_List  *pkixCertList;
        PKIX_Error *error;
        void       *plContext;
} callbackContext;






























static SECStatus
certCallback(void *arg, SECItem **secitemCerts, int numcerts)
{
        callbackContext *cbContext;
        PKIX_List *pkixCertList = NULL;
        PKIX_Error *error = NULL;
        void *plContext = NULL;
        int itemNum = 0;

        if ((arg == NULL) || (secitemCerts == NULL)) {
                return (SECFailure);
        }

        cbContext = (callbackContext *)arg;
        plContext = cbContext->plContext;
        pkixCertList = cbContext->pkixCertList;

        for (; itemNum < numcerts; itemNum++ ) {
                error = pkix_pl_Cert_CreateToList(secitemCerts[itemNum],
                                                  pkixCertList, plContext);
                if (error != NULL) {
                    if (error->errClass == PKIX_FATAL_ERROR) {
                        cbContext->error = error;
                        return SECFailure;
                    } 
                    

                    error = PKIX_PL_Object_DecRef((PKIX_PL_Object *)error,
                                                        plContext);
                    if (error) {
                        


                        error->errClass = PKIX_FATAL_ERROR;
                        cbContext->error = error;
                        return SECFailure;
                    }
                }
        }

        return SECSuccess;
}


typedef SECStatus (*pkix_DecodeCertsFunc)(char *certbuf, int certlen,
                                          CERTImportCertificateFunc f, void *arg);


struct pkix_DecodeFuncStr {
    pkix_DecodeCertsFunc func;          

    PRLibrary *smimeLib;                
    PRCallOnceType once;
};

static struct pkix_DecodeFuncStr pkix_decodeFunc;
static const PRCallOnceType pkix_pristine;

#define SMIME_LIB_NAME SHLIB_PREFIX"smime3."SHLIB_SUFFIX







static PRStatus PR_CALLBACK pkix_getDecodeFunction(void)
{
    pkix_decodeFunc.smimeLib = 
		PR_LoadLibrary(SHLIB_PREFIX"smime3."SHLIB_SUFFIX);
    if (pkix_decodeFunc.smimeLib == NULL) {
	return PR_FAILURE;
    }

    pkix_decodeFunc.func = (pkix_DecodeCertsFunc) PR_FindFunctionSymbol(
		pkix_decodeFunc.smimeLib, "CERT_DecodeCertPackage");
    if (!pkix_decodeFunc.func) {
	return PR_FAILURE;
    }
    return PR_SUCCESS;

}




void
pkix_pl_HttpCertStore_Shutdown(void *plContext)
{
    if (pkix_decodeFunc.smimeLib) {
	PR_UnloadLibrary(pkix_decodeFunc.smimeLib);
	pkix_decodeFunc.smimeLib = NULL;
    }
    
    pkix_decodeFunc.func = NULL;
    pkix_decodeFunc.once = pkix_pristine;
}





PKIX_Error *
pkix_pl_HttpCertStore_DecodeCertPackage
        (const char *certbuf,
        int certlen,
        CERTImportCertificateFunc f,
        void *arg,
        void *plContext)
{
   
        PRStatus status;
        SECStatus rv;

        PKIX_ENTER
                (HTTPCERTSTORECONTEXT,
                "pkix_pl_HttpCertStore_DecodeCertPackage");
        PKIX_NULLCHECK_TWO(certbuf, f);

        status = PR_CallOnce(&pkix_decodeFunc.once, pkix_getDecodeFunction);

        if (status != PR_SUCCESS) {
               PKIX_ERROR(PKIX_CANTLOADLIBSMIME);
        }

        
        if (!pkix_decodeFunc.func) {
               PKIX_ERROR(PKIX_CANTLOADLIBSMIME);
        }

        rv = (*pkix_decodeFunc.func)((char*)certbuf, certlen, f, arg);

        if (rv != SECSuccess) {
                PKIX_ERROR (PKIX_SECREADPKCS7CERTSFAILED);
        }
    

cleanup:

        PKIX_RETURN(HTTPCERTSTORECONTEXT);
}
































PKIX_Error *
pkix_pl_HttpCertStore_ProcessCertResponse(
        PRUint16 responseCode,
        const char *responseContentType,
        const char *responseData,
        PRUint32 responseDataLen,
        PKIX_List **pCertList,
        void *plContext)
{
        callbackContext cbContext;

        PKIX_ENTER(HTTPCERTSTORECONTEXT,
                "pkix_pl_HttpCertStore_ProcessCertResponse");
        
        cbContext.error = NULL;
        cbContext.plContext = plContext;
        cbContext.pkixCertList = NULL;

        PKIX_NULLCHECK_ONE(pCertList);

        if (responseCode != 200) {
                PKIX_ERROR(PKIX_BADHTTPRESPONSE);
        }

        
        if (responseContentType == NULL) {
                PKIX_ERROR(PKIX_NOCONTENTTYPEINHTTPRESPONSE);
        }

        if (responseData == NULL) {
                PKIX_ERROR(PKIX_NORESPONSEDATAINHTTPRESPONSE);
        }

        PKIX_CHECK(
            PKIX_List_Create(&cbContext.pkixCertList, plContext),
            PKIX_LISTCREATEFAILED);
        
        PKIX_CHECK_ONLY_FATAL(
            pkix_pl_HttpCertStore_DecodeCertPackage(responseData,
                                                    responseDataLen,
                                                    certCallback,
                                                    &cbContext,
                                                    plContext),
            PKIX_HTTPCERTSTOREDECODECERTPACKAGEFAILED);
        if (cbContext.error) {
            
            pkixErrorResult = cbContext.error;
            goto cleanup;
        }
        
        *pCertList = cbContext.pkixCertList;
        cbContext.pkixCertList = NULL;

cleanup:

        PKIX_DECREF(cbContext.pkixCertList);

        PKIX_RETURN(HTTPCERTSTORECONTEXT);
}































PKIX_Error *
pkix_pl_HttpCertStore_ProcessCrlResponse(
        PRUint16 responseCode,
        const char *responseContentType,
        const char *responseData,
        PRUint32 responseDataLen,
        PKIX_List **pCrlList,
        void *plContext)
{
        SECItem encodedResponse;
        PRInt16 compareVal = 0;
        PKIX_List *crls = NULL;
        SECItem *derCrlCopy = NULL;
        CERTSignedCrl *nssCrl = NULL;
        PKIX_PL_CRL *crl = NULL;

        PKIX_ENTER(HTTPCERTSTORECONTEXT,
                "pkix_pl_HttpCertStore_ProcessCrlResponse");
        PKIX_NULLCHECK_ONE(pCrlList);

        if (responseCode != 200) {
                PKIX_ERROR(PKIX_BADHTTPRESPONSE);
        }

        
        if (responseContentType == NULL) {
                PKIX_ERROR(PKIX_NOCONTENTTYPEINHTTPRESPONSE);
        }

        compareVal = PORT_Strcasecmp(responseContentType,
                                     "application/pkix-crl");
        if (compareVal != 0) {
                PKIX_ERROR(PKIX_CONTENTTYPENOTPKIXCRL);
        }
        encodedResponse.type = siBuffer;
        encodedResponse.data = (void*)responseData;
        encodedResponse.len = responseDataLen;

        derCrlCopy = SECITEM_DupItem(&encodedResponse);
        if (!derCrlCopy) {
            PKIX_ERROR(PKIX_ALLOCERROR);
        }
        
        nssCrl =
            CERT_DecodeDERCrlWithFlags(NULL, derCrlCopy, SEC_CRL_TYPE,
                                       CRL_DECODE_DONT_COPY_DER |
                                       CRL_DECODE_SKIP_ENTRIES);
        if (!nssCrl) {
            PKIX_ERROR(PKIX_FAILEDTODECODECRL);
        }
        
        PKIX_CHECK(
            pkix_pl_CRL_CreateWithSignedCRL(nssCrl, derCrlCopy, NULL,
                                            &crl, plContext),
            PKIX_CRLCREATEWITHSIGNEDCRLFAILED);
        

        derCrlCopy = NULL;
        nssCrl = NULL;
        PKIX_CHECK(PKIX_List_Create(&crls, plContext),
                   PKIX_LISTCREATEFAILED);
        PKIX_CHECK(PKIX_List_AppendItem
                   (crls, (PKIX_PL_Object *) crl, plContext),
                   PKIX_LISTAPPENDITEMFAILED);
        *pCrlList = crls;
        crls = NULL;
cleanup:
        if (derCrlCopy) {
            SECITEM_FreeItem(derCrlCopy, PR_TRUE);
        }
        if (nssCrl) {
            SEC_DestroyCrl(nssCrl);
        }
        PKIX_DECREF(crl);
        PKIX_DECREF(crls);

        PKIX_RETURN(HTTPCERTSTORECONTEXT);
}





















PKIX_Error *
pkix_pl_HttpCertStore_CreateRequestSession(
        PKIX_PL_HttpCertStoreContext *context,
        void *plContext)
{
        const SEC_HttpClientFcnV1 *hcv1 = NULL;
        SECStatus rv = SECFailure;

        PKIX_ENTER
                (HTTPCERTSTORECONTEXT,
                "pkix_pl_HttpCertStore_CreateRequestSession");
        PKIX_NULLCHECK_TWO(context, context->serverSession);

        if (context->client->version != 1) {
                PKIX_ERROR(PKIX_UNSUPPORTEDVERSIONOFHTTPCLIENT);
        }

        hcv1 = &(context->client->fcnTable.ftable1);
        if (context->requestSession != NULL) {
            (*hcv1->freeFcn)(context->requestSession);
            context->requestSession = 0;
        }
        
        rv = (*hcv1->createFcn)(context->serverSession, "http",
                         context->path, "GET",
                         PR_SecondsToInterval(
                             ((PKIX_PL_NssContext*)plContext)->timeoutSeconds),
                         &(context->requestSession));
        
        if (rv != SECSuccess) {
            PKIX_ERROR(PKIX_HTTPSERVERERROR);
        }
cleanup:

        PKIX_RETURN(HTTPCERTSTORECONTEXT);

}





PKIX_Error *
pkix_pl_HttpCertStore_GetCert(
        PKIX_CertStore *store,
        PKIX_CertSelector *selector,
        PKIX_VerifyNode *verifyNode,
        void **pNBIOContext,
        PKIX_List **pCertList,
        void *plContext)
{
        const SEC_HttpClientFcnV1 *hcv1 = NULL;
        PKIX_PL_HttpCertStoreContext *context = NULL;
        void *nbioContext = NULL;
        SECStatus rv = SECFailure;
        PRUint16 responseCode = 0;
        const char *responseContentType = NULL;
        const char *responseData = NULL;
        PRUint32 responseDataLen = 0;
        PKIX_List *certList = NULL;

        PKIX_ENTER(HTTPCERTSTORECONTEXT, "pkix_pl_HttpCertStore_GetCert");
        PKIX_NULLCHECK_THREE(store, selector, pCertList);

        nbioContext = *pNBIOContext;
        *pNBIOContext = NULL;

        PKIX_CHECK(PKIX_CertStore_GetCertStoreContext
                (store, (PKIX_PL_Object **)&context, plContext),
                PKIX_CERTSTOREGETCERTSTORECONTEXTFAILED);

        if (context->client->version != 1) {
            PKIX_ERROR(PKIX_UNSUPPORTEDVERSIONOFHTTPCLIENT);
        }
        
        hcv1 = &(context->client->fcnTable.ftable1);

        PKIX_CHECK(pkix_pl_HttpCertStore_CreateRequestSession
                   (context, plContext),
                   PKIX_HTTPCERTSTORECREATEREQUESTSESSIONFAILED);
        
        responseDataLen = 
            ((PKIX_PL_NssContext*)plContext)->maxResponseLength;
        
        rv = (*hcv1->trySendAndReceiveFcn)(context->requestSession,
                                        (PRPollDesc **)&nbioContext,
                                        &responseCode,
                                        (const char **)&responseContentType,
                                        NULL, 
                                        (const char **)&responseData,
                                        &responseDataLen);
        if (rv != SECSuccess) {
            PKIX_ERROR(PKIX_HTTPSERVERERROR);
        }
        
        if (nbioContext != 0) {
            *pNBIOContext = nbioContext;
            goto cleanup;
        }

        PKIX_CHECK(pkix_pl_HttpCertStore_ProcessCertResponse
                (responseCode,
                responseContentType,
                responseData,
                responseDataLen,
                &certList,
                plContext),
                PKIX_HTTPCERTSTOREPROCESSCERTRESPONSEFAILED);

        *pCertList = certList;

cleanup:
        PKIX_DECREF(context);

        PKIX_RETURN(CERTSTORE);
}





PKIX_Error *
pkix_pl_HttpCertStore_GetCertContinue(
        PKIX_CertStore *store,
        PKIX_CertSelector *selector,
        PKIX_VerifyNode *verifyNode,
        void **pNBIOContext,
        PKIX_List **pCertList,
        void *plContext)
{
        const SEC_HttpClientFcnV1 *hcv1 = NULL;
        PKIX_PL_HttpCertStoreContext *context = NULL;
        void *nbioContext = NULL;
        SECStatus rv = SECFailure;
        PRUint16 responseCode = 0;
        const char *responseContentType = NULL;
        const char *responseData = NULL;
        PRUint32 responseDataLen = 0;
        PKIX_List *certList = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_HttpCertStore_GetCertContinue");
        PKIX_NULLCHECK_THREE(store, selector, pCertList);

        nbioContext = *pNBIOContext;
        *pNBIOContext = NULL;

        PKIX_CHECK(PKIX_CertStore_GetCertStoreContext
                (store, (PKIX_PL_Object **)&context, plContext),
                PKIX_CERTSTOREGETCERTSTORECONTEXTFAILED);

        if (context->client->version != 1) {
                PKIX_ERROR(PKIX_UNSUPPORTEDVERSIONOFHTTPCLIENT);
        }

        hcv1 = &(context->client->fcnTable.ftable1);
        PKIX_NULLCHECK_ONE(context->requestSession);

        responseDataLen = 
            ((PKIX_PL_NssContext*)plContext)->maxResponseLength;

        rv = (*hcv1->trySendAndReceiveFcn)(context->requestSession,
                        (PRPollDesc **)&nbioContext,
                        &responseCode,
                        (const char **)&responseContentType,
                        NULL, 
                        (const char **)&responseData,
                        &responseDataLen);

        if (rv != SECSuccess) {
            PKIX_ERROR(PKIX_HTTPSERVERERROR);
        }
        
        if (nbioContext != 0) {
            *pNBIOContext = nbioContext;
            goto cleanup;
        }

        PKIX_CHECK(pkix_pl_HttpCertStore_ProcessCertResponse
                (responseCode,
                responseContentType,
                responseData,
                responseDataLen,
                &certList,
                plContext),
                PKIX_HTTPCERTSTOREPROCESSCERTRESPONSEFAILED);

        *pCertList = certList;

cleanup:
        PKIX_DECREF(context);
        
        PKIX_RETURN(CERTSTORE);
}





PKIX_Error *
pkix_pl_HttpCertStore_GetCRL(
        PKIX_CertStore *store,
        PKIX_CRLSelector *selector,
        void **pNBIOContext,
        PKIX_List **pCrlList,
        void *plContext)
{

        const SEC_HttpClientFcnV1 *hcv1 = NULL;
        PKIX_PL_HttpCertStoreContext *context = NULL;
        void *nbioContext = NULL;
        SECStatus rv = SECFailure;
        PRUint16 responseCode = 0;
        const char *responseContentType = NULL;
        const char *responseData = NULL;
        PRUint32 responseDataLen = 0;
        PKIX_List *crlList = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_HttpCertStore_GetCRL");
        PKIX_NULLCHECK_THREE(store, selector, pCrlList);

        nbioContext = *pNBIOContext;
        *pNBIOContext = NULL;

        PKIX_CHECK(PKIX_CertStore_GetCertStoreContext
                (store, (PKIX_PL_Object **)&context, plContext),
                PKIX_CERTSTOREGETCERTSTORECONTEXTFAILED);

        if (context->client->version != 1) {
                PKIX_ERROR(PKIX_UNSUPPORTEDVERSIONOFHTTPCLIENT);
        }

        hcv1 = &(context->client->fcnTable.ftable1);
        PKIX_CHECK(pkix_pl_HttpCertStore_CreateRequestSession
                   (context, plContext),
                   PKIX_HTTPCERTSTORECREATEREQUESTSESSIONFAILED);

        responseDataLen = 
            ((PKIX_PL_NssContext*)plContext)->maxResponseLength;

        rv = (*hcv1->trySendAndReceiveFcn)(context->requestSession,
                        (PRPollDesc **)&nbioContext,
                        &responseCode,
                        (const char **)&responseContentType,
                        NULL, 
                        (const char **)&responseData,
                        &responseDataLen);

        if (rv != SECSuccess) {
            PKIX_ERROR(PKIX_HTTPSERVERERROR);
        }
        
        if (nbioContext != 0) {
            *pNBIOContext = nbioContext;
            goto cleanup;
        }

        PKIX_CHECK(pkix_pl_HttpCertStore_ProcessCrlResponse
                (responseCode,
                responseContentType,
                responseData,
                responseDataLen,
                &crlList,
                plContext),
                PKIX_HTTPCERTSTOREPROCESSCRLRESPONSEFAILED);

        *pCrlList = crlList;

cleanup:
        PKIX_DECREF(context);

        PKIX_RETURN(CERTSTORE);
}





PKIX_Error *
pkix_pl_HttpCertStore_GetCRLContinue(
        PKIX_CertStore *store,
        PKIX_CRLSelector *selector,
        void **pNBIOContext,
        PKIX_List **pCrlList,
        void *plContext)
{
        const SEC_HttpClientFcnV1 *hcv1 = NULL;
        PKIX_PL_HttpCertStoreContext *context = NULL;
        void *nbioContext = NULL;
        SECStatus rv = SECFailure;
        PRUint16 responseCode = 0;
        const char *responseContentType = NULL;
        const char *responseData = NULL;
        PRUint32 responseDataLen = 0;
        PKIX_List *crlList = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_HttpCertStore_GetCRLContinue");
        PKIX_NULLCHECK_FOUR(store, selector, pNBIOContext, pCrlList);

        nbioContext = *pNBIOContext;
        *pNBIOContext = NULL;

        PKIX_CHECK(PKIX_CertStore_GetCertStoreContext
                (store, (PKIX_PL_Object **)&context, plContext),
                PKIX_CERTSTOREGETCERTSTORECONTEXTFAILED);

        if (context->client->version != 1) {
            PKIX_ERROR(PKIX_UNSUPPORTEDVERSIONOFHTTPCLIENT);
        }
        hcv1 = &(context->client->fcnTable.ftable1);
                
        PKIX_CHECK(pkix_pl_HttpCertStore_CreateRequestSession
                   (context, plContext),
                   PKIX_HTTPCERTSTORECREATEREQUESTSESSIONFAILED);
        
        responseDataLen = 
            ((PKIX_PL_NssContext*)plContext)->maxResponseLength;

        rv = (*hcv1->trySendAndReceiveFcn)(context->requestSession,
                        (PRPollDesc **)&nbioContext,
                        &responseCode,
                        (const char **)&responseContentType,
                        NULL, 
                        (const char **)&responseData,
                        &responseDataLen);
        
        if (rv != SECSuccess) {
            PKIX_ERROR(PKIX_HTTPSERVERERROR);
        }
        
        if (nbioContext != 0) {
            *pNBIOContext = nbioContext;
            goto cleanup;
        }

        PKIX_CHECK(pkix_pl_HttpCertStore_ProcessCrlResponse
                (responseCode,
                responseContentType,
                responseData,
                responseDataLen,
                &crlList,
                plContext),
                PKIX_HTTPCERTSTOREPROCESSCRLRESPONSEFAILED);

        *pCrlList = crlList;

cleanup:
        PKIX_DECREF(context);

        PKIX_RETURN(CERTSTORE);
}





























PKIX_Error *
pkix_pl_HttpCertStore_CreateWithAsciiName(
        PKIX_PL_HttpClient *client,
        char *locationAscii,
        PKIX_CertStore **pCertStore,
        void *plContext)
{
        const SEC_HttpClientFcn *clientFcn = NULL;
        const SEC_HttpClientFcnV1 *hcv1 = NULL;
        PKIX_PL_HttpCertStoreContext *httpCertStore = NULL;
        PKIX_CertStore *certStore = NULL;
        char *hostname = NULL;
        char *path = NULL;
        PRUint16 port = 0;
        SECStatus rv = SECFailure;

        PKIX_ENTER(CERTSTORE, "pkix_pl_HttpCertStore_CreateWithAsciiName");
        PKIX_NULLCHECK_TWO(locationAscii, pCertStore);

        if (client == NULL) {
                clientFcn = SEC_GetRegisteredHttpClient();
                if (clientFcn == NULL) {
                        PKIX_ERROR(PKIX_NOREGISTEREDHTTPCLIENT);
                }
        } else {
                clientFcn = (const SEC_HttpClientFcn *)client;
        }

        if (clientFcn->version != 1) {
                PKIX_ERROR(PKIX_UNSUPPORTEDVERSIONOFHTTPCLIENT);
        }

        
        PKIX_CHECK(PKIX_PL_Object_Alloc
                  (PKIX_HTTPCERTSTORECONTEXT_TYPE,
                  sizeof (PKIX_PL_HttpCertStoreContext),
                  (PKIX_PL_Object **)&httpCertStore,
                  plContext),
                  PKIX_COULDNOTCREATEOBJECT);

        
        httpCertStore->client = clientFcn; 

        
        rv = CERT_ParseURL(locationAscii, &hostname, &port, &path);
        if (rv == SECFailure || hostname == NULL || path == NULL) {
                PKIX_ERROR(PKIX_URLPARSINGFAILED);
        }

        httpCertStore->path = path;
        path = NULL;

        hcv1 = &(clientFcn->fcnTable.ftable1);
        rv = (*hcv1->createSessionFcn)(hostname, port,
                                       &(httpCertStore->serverSession));
        if (rv != SECSuccess) {
            PKIX_ERROR(PKIX_HTTPCLIENTCREATESESSIONFAILED);
        }

        httpCertStore->requestSession = NULL;

        PKIX_CHECK(PKIX_CertStore_Create
                (pkix_pl_HttpCertStore_GetCert,
                pkix_pl_HttpCertStore_GetCRL,
                pkix_pl_HttpCertStore_GetCertContinue,
                pkix_pl_HttpCertStore_GetCRLContinue,
                NULL,       
                NULL,      
                NULL,      
                (PKIX_PL_Object *)httpCertStore,
                PKIX_TRUE,  
                PKIX_FALSE, 
                &certStore,
                plContext),
                PKIX_CERTSTORECREATEFAILED);

        *pCertStore = certStore;
        certStore = NULL;

cleanup:
        PKIX_DECREF(httpCertStore);
        if (hostname) {
            PORT_Free(hostname);
        }
        if (path) {
            PORT_Free(path);
        }

        PKIX_RETURN(CERTSTORE);
}





PKIX_Error *
PKIX_PL_HttpCertStore_Create(
        PKIX_PL_HttpClient *client,
        PKIX_PL_GeneralName *location,
        PKIX_CertStore **pCertStore,
        void *plContext)
{
        PKIX_PL_String *locationString = NULL;
        char *locationAscii = NULL;
        PKIX_UInt32 len = 0;

        PKIX_ENTER(CERTSTORE, "PKIX_PL_HttpCertStore_Create");
        PKIX_NULLCHECK_TWO(location, pCertStore);

        PKIX_TOSTRING(location, &locationString, plContext,
                PKIX_GENERALNAMETOSTRINGFAILED);

        PKIX_CHECK(PKIX_PL_String_GetEncoded
                (locationString,
                PKIX_ESCASCII,
                (void **)&locationAscii,
                &len,
                plContext),
                PKIX_STRINGGETENCODEDFAILED);

        PKIX_CHECK(pkix_pl_HttpCertStore_CreateWithAsciiName
                (client, locationAscii, pCertStore, plContext),
                PKIX_HTTPCERTSTORECREATEWITHASCIINAMEFAILED);

cleanup:

        PKIX_DECREF(locationString);

        PKIX_RETURN(CERTSTORE);
}





































PKIX_Error *
pkix_HttpCertStore_FindSocketConnection(
        PRIntervalTime timeout,
        char *hostname,
        PRUint16 portnum,
        PRErrorCode *pStatus,
        PKIX_PL_Socket **pSocket,
        void *plContext)
{
        PKIX_PL_String *formatString = NULL;
        PKIX_PL_String *hostString = NULL;
        PKIX_PL_String *domainString = NULL;
        PKIX_PL_Socket *socket = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_HttpCertStore_FindSocketConnection");
        PKIX_NULLCHECK_THREE(hostname, pStatus, pSocket);

        *pStatus = 0;

        
        PKIX_CHECK(PKIX_PL_String_Create
                (PKIX_ESCASCII, "%s:%d", 0, &formatString, plContext),
                PKIX_STRINGCREATEFAILED);

#if 0
hostname = "variation.red.iplanet.com";
portnum = 2001;
#endif

        PKIX_CHECK(PKIX_PL_String_Create
                (PKIX_ESCASCII, hostname, 0, &hostString, plContext),
                PKIX_STRINGCREATEFAILED);

        PKIX_CHECK(PKIX_PL_Sprintf
                (&domainString, plContext, formatString, hostString, portnum),
                PKIX_STRINGCREATEFAILED);

#ifdef PKIX_SOCKETCACHE
        
        PKIX_CHECK(PKIX_PL_HashTable_Lookup
                (httpSocketCache,
                (PKIX_PL_Object *)domainString,
                (PKIX_PL_Object **)&socket,
                plContext),
                PKIX_HASHTABLELOOKUPFAILED);
#endif
        if (socket == NULL) {

                
                PKIX_CHECK(pkix_pl_Socket_CreateByHostAndPort
                        (PKIX_FALSE,       
                        timeout,
                        hostname,
                        portnum,
                        pStatus,
                        &socket,
                        plContext),
                        PKIX_SOCKETCREATEBYHOSTANDPORTFAILED);

#ifdef PKIX_SOCKETCACHE
                PKIX_CHECK(PKIX_PL_HashTable_Add
                        (httpSocketCache,
                        (PKIX_PL_Object *)domainString,
                        (PKIX_PL_Object *)socket,
                        plContext),
                        PKIX_HASHTABLEADDFAILED);
#endif 
        }

        *pSocket = socket;
        socket = NULL;

cleanup:

        PKIX_DECREF(formatString);
        PKIX_DECREF(hostString);
        PKIX_DECREF(domainString);
        PKIX_DECREF(socket);

        PKIX_RETURN(CERTSTORE);
}

