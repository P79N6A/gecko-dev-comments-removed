








































#include "pkix_pl_ocspresponse.h"






PKIX_Error *
PKIX_PL_OcspResponse_UseBuildChain(
        PKIX_PL_Cert *signerCert,
	PKIX_PL_Date *producedAt,
        PKIX_ProcessingParams *procParams,
        void **pNBIOContext,
        void **pState,
        PKIX_BuildResult **pBuildResult,
        PKIX_VerifyNode **pVerifyTree,
	void *plContext)
{
        PKIX_ProcessingParams *caProcParams = NULL;
        PKIX_PL_Date *date = NULL;
        PKIX_ComCertSelParams *certSelParams = NULL;
        PKIX_CertSelector *certSelector = NULL;
        void *nbioContext = NULL;
        PKIX_Error *buildError = NULL;

        PKIX_ENTER(OCSPRESPONSE, "pkix_OcspResponse_UseBuildChain");
        PKIX_NULLCHECK_THREE(signerCert, producedAt, procParams);
        PKIX_NULLCHECK_THREE(pNBIOContext, pState, pBuildResult);

        nbioContext = *pNBIOContext;
        *pNBIOContext = NULL;

        
        if (nbioContext == NULL) {
                
		PKIX_CHECK(PKIX_PL_Object_Duplicate
                        ((PKIX_PL_Object *)procParams,
                        (PKIX_PL_Object **)&caProcParams,
                        plContext),
        	        PKIX_OBJECTDUPLICATEFAILED);

		PKIX_CHECK(PKIX_ProcessingParams_SetDate(procParams, date, plContext),
	                PKIX_PROCESSINGPARAMSSETDATEFAILED);

	        

		PKIX_CHECK(PKIX_CertSelector_Create
	                (NULL, NULL, &certSelector, plContext),
	                PKIX_CERTSELECTORCREATEFAILED);

		PKIX_CHECK(PKIX_ComCertSelParams_Create
	                (&certSelParams, plContext),
	                PKIX_COMCERTSELPARAMSCREATEFAILED);

	        PKIX_CHECK(PKIX_ComCertSelParams_SetCertificate
        	        (certSelParams, signerCert, plContext),
                	PKIX_COMCERTSELPARAMSSETCERTIFICATEFAILED);

	        PKIX_CHECK(PKIX_CertSelector_SetCommonCertSelectorParams
	                (certSelector, certSelParams, plContext),
	                PKIX_CERTSELECTORSETCOMMONCERTSELECTORPARAMSFAILED);

	        PKIX_CHECK(PKIX_ProcessingParams_SetTargetCertConstraints
        	        (caProcParams, certSelector, plContext),
                	PKIX_PROCESSINGPARAMSSETTARGETCERTCONSTRAINTSFAILED);
	}

        buildError = PKIX_BuildChain
                (caProcParams,
                &nbioContext,
                pState,
                pBuildResult,
		pVerifyTree,
                plContext);

        
        if (nbioContext != NULL) {

                *pNBIOContext = nbioContext;

        
        } else if (buildError) {
                pkixErrorResult = buildError;
                buildError = NULL;
        } else {
                PKIX_DECREF(*pState);
        }

cleanup:

        PKIX_DECREF(caProcParams);
        PKIX_DECREF(date);
        PKIX_DECREF(certSelParams);
        PKIX_DECREF(certSelector);

        PKIX_RETURN(OCSPRESPONSE);
}







static PKIX_Error *
pkix_pl_OcspResponse_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_PL_OcspResponse *ocspRsp = NULL;
        const SEC_HttpClientFcn *httpClient = NULL;
        const SEC_HttpClientFcnV1 *hcv1 = NULL;

        PKIX_ENTER(OCSPRESPONSE, "pkix_pl_OcspResponse_Destroy");
        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_CheckType(object, PKIX_OCSPRESPONSE_TYPE, plContext),
                    PKIX_OBJECTNOTANOCSPRESPONSE);

        ocspRsp = (PKIX_PL_OcspResponse *)object;

        if (ocspRsp->nssOCSPResponse != NULL) {
                CERT_DestroyOCSPResponse(ocspRsp->nssOCSPResponse);
                ocspRsp->nssOCSPResponse = NULL;
        }

        if (ocspRsp->signerCert != NULL) {
                CERT_DestroyCertificate(ocspRsp->signerCert);
                ocspRsp->signerCert = NULL;
        }

        httpClient = (const SEC_HttpClientFcn *)(ocspRsp->httpClient);

        if (httpClient && (httpClient->version == 1)) {

                hcv1 = &(httpClient->fcnTable.ftable1);

                if (ocspRsp->requestSession != NULL) {
                        hcv1->freeFcn(ocspRsp->requestSession);
                        ocspRsp->requestSession = NULL;
                }

                if (ocspRsp->serverSession != NULL) {
                        hcv1->freeSessionFcn(ocspRsp->serverSession);
                        ocspRsp->serverSession = NULL;
                }
        }

        if (ocspRsp->arena != NULL) {
                PORT_FreeArena(ocspRsp->arena, PR_FALSE);
                ocspRsp->arena = NULL;
        }

	PKIX_DECREF(ocspRsp->producedAtDate);
	PKIX_DECREF(ocspRsp->pkixSignerCert);
	PKIX_DECREF(ocspRsp->request);

cleanup:

        PKIX_RETURN(OCSPRESPONSE);
}





static PKIX_Error *
pkix_pl_OcspResponse_Hashcode(
        PKIX_PL_Object *object,
        PKIX_UInt32 *pHashcode,
        void *plContext)
{
        PKIX_PL_OcspResponse *ocspRsp = NULL;

        PKIX_ENTER(OCSPRESPONSE, "pkix_pl_OcspResponse_Hashcode");
        PKIX_NULLCHECK_TWO(object, pHashcode);

        PKIX_CHECK(pkix_CheckType(object, PKIX_OCSPRESPONSE_TYPE, plContext),
                    PKIX_OBJECTNOTANOCSPRESPONSE);

        ocspRsp = (PKIX_PL_OcspResponse *)object;

        if (ocspRsp->encodedResponse->data == NULL) {
                *pHashcode = 0;
        } else {
                PKIX_CHECK(pkix_hash
                        (ocspRsp->encodedResponse->data,
                        ocspRsp->encodedResponse->len,
                        pHashcode,
                        plContext),
                        PKIX_HASHFAILED);
        }

cleanup:

        PKIX_RETURN(OCSPRESPONSE);
}





static PKIX_Error *
pkix_pl_OcspResponse_Equals(
        PKIX_PL_Object *firstObj,
        PKIX_PL_Object *secondObj,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_UInt32 secondType = 0;
        PKIX_UInt32 firstLen = 0;
        PKIX_UInt32 i = 0;
        PKIX_PL_OcspResponse *rsp1 = NULL;
        PKIX_PL_OcspResponse *rsp2 = NULL;
        const unsigned char *firstData = NULL;
        const unsigned char *secondData = NULL;

        PKIX_ENTER(OCSPRESPONSE, "pkix_pl_OcspResponse_Equals");
        PKIX_NULLCHECK_THREE(firstObj, secondObj, pResult);

        
        PKIX_CHECK(pkix_CheckType(firstObj, PKIX_OCSPRESPONSE_TYPE, plContext),
                    PKIX_FIRSTOBJARGUMENTNOTANOCSPRESPONSE);

        



        if (firstObj == secondObj){
                *pResult = PKIX_TRUE;
                goto cleanup;
        }

        



        *pResult = PKIX_FALSE;
        PKIX_CHECK(PKIX_PL_Object_GetType(secondObj, &secondType, plContext),
                PKIX_COULDNOTGETTYPEOFSECONDARGUMENT);
        if (secondType != PKIX_OCSPRESPONSE_TYPE) {
                goto cleanup;
        }

        rsp1 = (PKIX_PL_OcspResponse *)firstObj;
        rsp2 = (PKIX_PL_OcspResponse *)secondObj;

        
        firstData = (const unsigned char *)rsp1->encodedResponse->data;
        secondData = (const unsigned char *)rsp2->encodedResponse->data;
        if ((firstData == NULL) || (secondData == NULL)) {
                goto cleanup;
        }

        firstLen = rsp1->encodedResponse->len;

        if (firstLen != rsp2->encodedResponse->len) {
                goto cleanup;
        }

        for (i = 0; i < firstLen; i++) {
                if (*firstData++ != *secondData++) {
                        goto cleanup;
                }
        }

        *pResult = PKIX_TRUE;

cleanup:

        PKIX_RETURN(OCSPRESPONSE);
}
















PKIX_Error *
pkix_pl_OcspResponse_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(OCSPRESPONSE, "pkix_pl_OcspResponse_RegisterSelf");

        entry.description = "OcspResponse";
        entry.objCounter = 0;
        entry.typeObjectSize = sizeof(PKIX_PL_OcspResponse);
        entry.destructor = pkix_pl_OcspResponse_Destroy;
        entry.equalsFunction = pkix_pl_OcspResponse_Equals;
        entry.hashcodeFunction = pkix_pl_OcspResponse_Hashcode;
        entry.toStringFunction = NULL;
        entry.comparator = NULL;
        entry.duplicateFunction = pkix_duplicateImmutable;

        systemClasses[PKIX_OCSPRESPONSE_TYPE] = entry;

        PKIX_RETURN(OCSPRESPONSE);
}


















































PKIX_Error *
pkix_pl_OcspResponse_Create(
        PKIX_PL_OcspRequest *request,
        void *responder,
        PKIX_PL_OcspResponse_VerifyCallback verifyFcn,
        void **pNBIOContext,
        PKIX_PL_OcspResponse **pResponse,
        void *plContext)
{
        void *nbioContext = NULL;
        PKIX_PL_OcspResponse *ocspResponse = NULL;
        const SEC_HttpClientFcn *httpClient = NULL;
        const SEC_HttpClientFcnV1 *hcv1 = NULL;
        SECStatus rv = SECFailure;
        char *location = NULL;
        char *hostname = NULL;
        char *path = NULL;
        PRUint16 port = 0;
        SEC_HTTP_SERVER_SESSION serverSession = NULL;
        SEC_HTTP_REQUEST_SESSION requestSession = NULL;
        SECItem *encodedRequest = NULL;
        PRUint16 responseCode = 0;
        char *responseData = NULL;
        PRUint32 responseDataLen = 0;

        PKIX_ENTER(OCSPRESPONSE, "pkix_pl_OcspResponse_Create");
        PKIX_NULLCHECK_TWO(pNBIOContext, pResponse);

        nbioContext = *pNBIOContext;
        *pNBIOContext = NULL;

        if (nbioContext != NULL) {

                ocspResponse = *pResponse;
                PKIX_NULLCHECK_ONE(ocspResponse);

                httpClient = ocspResponse->httpClient;
                serverSession = ocspResponse->serverSession;
                requestSession = ocspResponse->requestSession;
                PKIX_NULLCHECK_THREE(httpClient, serverSession, requestSession);

        } else {

                PKIX_NULLCHECK_ONE(request);

                PKIX_CHECK(pkix_pl_OcspRequest_GetEncoded
                        (request, &encodedRequest, plContext),
                        PKIX_OCSPREQUESTGETENCODEDFAILED);

                

                
                if (!responder) {
                        PKIX_PL_NSSCALLRV
                                (OCSPRESPONSE,
                                responder,
                                (void *)SEC_GetRegisteredHttpClient,
                                ());
                }

                httpClient = (const SEC_HttpClientFcn *)responder;

                if (httpClient && (httpClient->version == 1)) {

                        hcv1 = &(httpClient->fcnTable.ftable1);

                        PKIX_CHECK(pkix_pl_OcspRequest_GetLocation
                                (request, &location, plContext),
                                PKIX_OCSPREQUESTGETLOCATIONFAILED);

                            
                        PKIX_PL_NSSCALLRV(OCSPRESPONSE, rv, CERT_ParseURL,
                                (location, &hostname, &port, &path));

                        if ((hostname == NULL) || (path == NULL)) {
                                PKIX_ERROR(PKIX_URLPARSINGFAILED);
                        }

                        PKIX_PL_NSSCALLRV
                                (OCSPRESPONSE,
                                rv,
                                hcv1->createSessionFcn,
                                (hostname, port, &serverSession));

                        if (rv != SECSuccess) {
                                PKIX_ERROR(PKIX_OCSPSERVERERROR);
                        }       

                        PKIX_PL_NSSCALLRV
                                (OCSPRESPONSE, rv, hcv1->createFcn,
                                (serverSession,
                                "http",
                                path,
                                "POST",
                                PR_TicksPerSecond() * 60,
                                &requestSession));

                        if (rv != SECSuccess) {
                                PKIX_ERROR(PKIX_OCSPSERVERERROR);
                        }       

                        PKIX_PL_NSSCALLRV
                                (OCSPRESPONSE, rv, hcv1->setPostDataFcn,
                                (requestSession,
                                (char *)encodedRequest->data,
                                encodedRequest->len,
                                "application/ocsp-request"));

                        if (rv != SECSuccess) {
                                PKIX_ERROR(PKIX_OCSPSERVERERROR);
                        }       

                        
                        PKIX_CHECK(PKIX_PL_Object_Alloc
                                    (PKIX_OCSPRESPONSE_TYPE,
                                    sizeof (PKIX_PL_OcspResponse),
                                    (PKIX_PL_Object **)&ocspResponse,
                                    plContext),
                                    PKIX_COULDNOTCREATEOBJECT);

                        PKIX_INCREF(request);
                        ocspResponse->request = request;
                        ocspResponse->httpClient = httpClient;
                        ocspResponse->serverSession = serverSession;
                        ocspResponse->requestSession = requestSession;
                        ocspResponse->verifyFcn = verifyFcn;
                        ocspResponse->handle = CERT_GetDefaultCertDB();
                        ocspResponse->encodedResponse = NULL;
                        ocspResponse->arena = NULL;
                        ocspResponse->producedAt = 0;
                        ocspResponse->producedAtDate = NULL;
                        ocspResponse->pkixSignerCert = NULL;
                        ocspResponse->nssOCSPResponse = NULL;
                        ocspResponse->signerCert = NULL;
                }
        }

        
        if (httpClient && (httpClient->version == 1)) {

                hcv1 = &(httpClient->fcnTable.ftable1);

                responseDataLen = MAX_OCSP_RESPONSE_LEN;

                PKIX_PL_NSSCALLRV(OCSPRESPONSE, rv, hcv1->trySendAndReceiveFcn,
                        (requestSession,
                        (PRPollDesc **)&nbioContext,
                        &responseCode,
                        NULL,   
                        NULL,   
                        (const char **)&responseData,
                        &responseDataLen));

                if (rv != SECSuccess) {
                        PKIX_ERROR(PKIX_OCSPSERVERERROR);
                }       

                if (nbioContext != NULL) {
                        *pNBIOContext = nbioContext;
                        goto cleanup;
                }

                if (responseCode != 200) {
                        PKIX_ERROR(PKIX_OCSPBADHTTPRESPONSE);
                }


                PKIX_PL_NSSCALLRV
                        (OCSPRESPONSE, ocspResponse->arena, PORT_NewArena,
                        (DER_DEFAULT_CHUNKSIZE));

                if (ocspResponse->arena == NULL) {
                        PKIX_ERROR(PKIX_OUTOFMEMORY);
                }

                PKIX_PL_NSSCALLRV
                        (OCSPRESPONSE,
                        ocspResponse->encodedResponse,
                        SECITEM_AllocItem,
                        (ocspResponse->arena, NULL, responseDataLen));

                if (ocspResponse->encodedResponse == NULL) {
                        PKIX_ERROR(PKIX_OUTOFMEMORY);
                }

                PKIX_PL_NSSCALL(OCSPRESPONSE, PORT_Memcpy,
                        (ocspResponse->encodedResponse->data,
                        responseData,
                        responseDataLen));

        }

        *pResponse = ocspResponse;

cleanup:

        if (path != NULL) {
                PKIX_PL_NSSCALL(OCSPRESPONSE, PORT_Free, (path));
        }

        if (hostname != NULL) {
                PKIX_PL_NSSCALL(OCSPRESPONSE, PORT_Free, (hostname));
        }

        if (PKIX_ERROR_RECEIVED){
                PKIX_DECREF(ocspResponse);
        }

        PKIX_RETURN(OCSPRESPONSE);
}



























PKIX_Error *
pkix_pl_OcspResponse_Decode(
        PKIX_PL_OcspResponse *response,
        PKIX_Boolean *pPassed,
        SECErrorCodes *pReturnCode,
        void *plContext)
{

        PKIX_ENTER(OCSPRESPONSE, "PKIX_PL_OcspResponse_Decode");
        PKIX_NULLCHECK_TWO(response, response->encodedResponse);

        response->nssOCSPResponse =
            CERT_DecodeOCSPResponse(response->encodedResponse);

	if (response->nssOCSPResponse != NULL) {
                *pPassed = PKIX_TRUE;
                *pReturnCode = 0;
        } else {
                *pPassed = PKIX_FALSE;
                *pReturnCode = PORT_GetError();
        }

        PKIX_RETURN(OCSPRESPONSE);
}

























PKIX_Error *
pkix_pl_OcspResponse_GetStatus(
        PKIX_PL_OcspResponse *response,
        PKIX_Boolean *pPassed,
        SECErrorCodes *pReturnCode,
        void *plContext)
{
        SECStatus rv = SECFailure;

        PKIX_ENTER(OCSPRESPONSE, "PKIX_PL_OcspResponse_GetStatus");
        PKIX_NULLCHECK_FOUR(response, response->nssOCSPResponse, pPassed, pReturnCode);

        rv = CERT_GetOCSPResponseStatus(response->nssOCSPResponse);

	if (rv == SECSuccess) {
                *pPassed = PKIX_TRUE;
                *pReturnCode = 0;
        } else {
                *pPassed = PKIX_FALSE;
                *pReturnCode = PORT_GetError();
        }

        PKIX_RETURN(OCSPRESPONSE);
}


PKIX_Error*
pkix_pl_OcspResponse_CallCertVerify(
        PKIX_PL_OcspResponse *response,
        PKIX_ProcessingParams *procParams,
        SECCertUsage certUsage,
        void **state,
        PKIX_BuildResult **buildResult,
        void **pNBIOContext,
        void *plContext)
{
    SECStatus rv = SECFailure;

    PKIX_ENTER(OCSPRESPONSE, "pkix_pl_OcspResponse_CallCertVerify");

    if (response->verifyFcn != NULL) {
        void *lplContext = NULL;
        
        PKIX_CHECK(
            PKIX_PL_NssContext_Create(((SECCertificateUsage)1) << certUsage,
                                      PKIX_FALSE, NULL, &lplContext),
            PKIX_NSSCONTEXTCREATEFAILED);

        PKIX_CHECK(
            (response->verifyFcn)(response->pkixSignerCert,
                                  response->producedAtDate,
                                  procParams, pNBIOContext,
                                  state, buildResult,
                                  NULL, lplContext),
            PKIX_CERTVERIFYKEYUSAGEFAILED);
        rv = SECSuccess;
    } else {
        rv = CERT_VerifyCert(response->handle, response->signerCert, PKIX_TRUE,
                             certUsage, response->producedAt, NULL, NULL);
        if (rv != SECSuccess) {
            PKIX_ERROR(PKIX_CERTVERIFYKEYUSAGEFAILED);
        }
    }

cleanup:
    if (rv != SECSuccess) {
        PORT_SetError(SEC_ERROR_OCSP_INVALID_SIGNING_CERT);
    }

    PKIX_RETURN(OCSPRESPONSE);
}


































PKIX_Error *
pkix_pl_OcspResponse_VerifySignature(
        PKIX_PL_OcspResponse *response,
        PKIX_PL_Cert *cert,
        PKIX_ProcessingParams *procParams,
        PKIX_Boolean *pPassed,
        void **pNBIOContext,
        void *plContext)
{
        SECStatus rv = SECFailure;
        CERTOCSPResponse *nssOCSPResponse = NULL;
        CERTCertificate *issuerCert = NULL;
        PKIX_BuildResult *buildResult = NULL;
        void *nbio = NULL;
        void *state = NULL;

        ocspSignature *signature = NULL;
        ocspResponseData *tbsData = NULL;
        SECItem *tbsResponseDataDER = NULL;


        PKIX_ENTER(OCSPRESPONSE, "pkix_pl_OcspResponse_VerifySignature");
        PKIX_NULLCHECK_FOUR(response, cert, pPassed,  pNBIOContext);

        nbio = *pNBIOContext;
        *pNBIOContext = NULL;

        nssOCSPResponse = response->nssOCSPResponse;
        if (nssOCSPResponse == NULL) {
            PORT_SetError(SEC_ERROR_OCSP_MALFORMED_RESPONSE);
            goto cleanup;
        }

        tbsData =
            ocsp_GetResponseData(nssOCSPResponse, &tbsResponseDataDER);
        
        signature = ocsp_GetResponseSignature(nssOCSPResponse);


        
        if (nbio == NULL) {
            

            issuerCert = CERT_FindCertIssuer(cert->nssCert, PR_Now(),
                                             certUsageAnyCA);
            
            



            if (signature->wasChecked) {
                if (signature->status == SECSuccess) {
                    response->signerCert =
                        CERT_DupCertificate(signature->cert);
                } else {
                    PORT_SetError(signature->failureReason);
                    goto cleanup;
                }
            }
            
            response->signerCert = 
                ocsp_GetSignerCertificate(response->handle, tbsData,
                                          signature, issuerCert);
            
            if (response->signerCert == NULL) {
                PORT_SetError(SEC_ERROR_UNKNOWN_SIGNER);
                goto cleanup;
            }
            
            PKIX_CHECK( 
                PKIX_PL_Cert_CreateFromCERTCertificate(response->signerCert,
                                                       &(response->pkixSignerCert),
                                                       plContext),
                PKIX_CERTCREATEWITHNSSCERTFAILED);
            
            






            signature->wasChecked = PR_TRUE;
            
            





            rv = DER_GeneralizedTimeToTime(&response->producedAt,
                                           &tbsData->producedAt);
            if (rv != SECSuccess) {
                PORT_SetError(SEC_ERROR_OCSP_MALFORMED_RESPONSE);
                goto cleanup;
            }
            
            





            
            PKIX_CHECK(
                pkix_pl_Date_CreateFromPRTime((PRTime)response->producedAt,
                                              &(response->producedAtDate),
                                              plContext),
                PKIX_DATECREATEFROMPRTIMEFAILED);
            
	}
        
        




        if (ocsp_CertIsOCSPDefaultResponder(response->handle,
                                            response->signerCert)) {
            rv = SECSuccess;
        } else {
            SECCertUsage certUsage;
            if (CERT_IsCACert(response->signerCert, NULL)) {
                certUsage = certUsageVerifyCA;
            } else {
                certUsage = certUsageStatusResponder;
            }
            PKIX_CHECK_ONLY_FATAL(
                pkix_pl_OcspResponse_CallCertVerify(response, procParams,
                                                    certUsage, &state,
                                                    &buildResult, &nbio,
                                                    plContext),
                PKIX_CERTVERIFYKEYUSAGEFAILED);
            if (pkixTempErrorReceived) {
                rv = SECFailure;
                goto cleanup;
            }
            if (nbio != NULL) {
                *pNBIOContext = nbio;
                goto cleanup;
            }            
        }

        rv = ocsp_VerifyResponseSignature(response->signerCert, signature,
                                          tbsResponseDataDER, NULL);
        
cleanup:
        if (rv == SECSuccess) {
            *pPassed = PKIX_TRUE;
        } else {
            *pPassed = PKIX_FALSE;
        }
        
        if (signature) {
            if (signature->wasChecked) {
                signature->status = rv;
            }
            
            if (rv != SECSuccess) {
                signature->failureReason = PORT_GetError();
                if (response->signerCert != NULL) {
                    CERT_DestroyCertificate(response->signerCert);
                    response->signerCert = NULL;
                }
            } else {
                
                signature->cert = CERT_DupCertificate(response->signerCert);
            }
        }

	if (issuerCert)
	    CERT_DestroyCertificate(issuerCert);
        
        PKIX_RETURN(OCSPRESPONSE);
}


























PKIX_Error *
pkix_pl_OcspResponse_GetStatusForCert(
        PKIX_PL_OcspCertID *cid,
        PKIX_PL_OcspResponse *response,
        PKIX_Boolean *pPassed,
        SECErrorCodes *pReturnCode,
        void *plContext)
{
        SECStatus rv = SECFailure;
        SECStatus rvCache;
        PRBool certIDWasConsumed = PR_FALSE;

        PKIX_ENTER(OCSPRESPONSE, "pkix_pl_OcspResponse_GetStatusForCert");
        PKIX_NULLCHECK_THREE(response, pPassed, pReturnCode);

        




        PKIX_NULLCHECK_TWO(response->signerCert, response->request);
        PKIX_NULLCHECK_TWO(cid, cid->certID);

        rv = cert_ProcessOCSPResponse(response->handle,
                                      response->nssOCSPResponse,
                                      cid->certID,
                                      response->signerCert,
                                      PR_Now(),
                                      &certIDWasConsumed,
                                      &rvCache);
        if (certIDWasConsumed) {
                cid->certID = NULL;
        }

	if (rv == SECSuccess) {
                *pPassed = PKIX_TRUE;
                *pReturnCode = 0;
        } else {
                *pPassed = PKIX_FALSE;
                *pReturnCode = PORT_GetError();
        }

        PKIX_RETURN(OCSPRESPONSE);
}
