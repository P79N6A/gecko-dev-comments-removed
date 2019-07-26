






#ifndef _OCSPI_H_
#define _OCSPI_H_

SECStatus OCSP_InitGlobal(void);
SECStatus OCSP_ShutdownGlobal(void);

ocspResponseData *
ocsp_GetResponseData(CERTOCSPResponse *response, SECItem **tbsResponseDataDER);

ocspSignature *
ocsp_GetResponseSignature(CERTOCSPResponse *response);

SECItem *
ocsp_DigestValue(PLArenaPool *arena, SECOidTag digestAlg,
                 SECItem *fill, const SECItem *src);

PRBool
ocsp_CertIsOCSPDefaultResponder(CERTCertDBHandle *handle, CERTCertificate *cert);

CERTCertificate *
ocsp_GetSignerCertificate(CERTCertDBHandle *handle, ocspResponseData *tbsData,
                          ocspSignature *signature, CERTCertificate *issuer);

SECStatus
ocsp_VerifyResponseSignature(CERTCertificate *signerCert,
                             ocspSignature *signature,
                             SECItem *tbsResponseDataDER,
                             void *pwArg);

CERTOCSPRequest *
cert_CreateSingleCertOCSPRequest(CERTOCSPCertID *certID, 
                                 CERTCertificate *singleCert, 
                                 PRTime time,
                                 PRBool addServiceLocator,
                                 CERTCertificate *signerCert);

SECStatus
ocsp_GetCachedOCSPResponseStatusIfFresh(CERTOCSPCertID *certID, 
                                        PRTime time,
                                        PRBool ignoreOcspFailureMode,
                                        SECStatus *rvOcsp,
                                        SECErrorCodes *missingResponseError);

































SECStatus
cert_ProcessOCSPResponse(CERTCertDBHandle *handle, 
                         CERTOCSPResponse *response, 
                         CERTOCSPCertID   *certID,
                         CERTCertificate  *signerCert,
                         PRTime            time,
                         PRBool           *certIDWasConsumed,
                         SECStatus        *cacheUpdateStatus);
















SECStatus
cert_RememberOCSPProcessingFailure(CERTOCSPCertID *certID,
                                   PRBool         *certIDWasConsumed);
















char *
ocsp_GetResponderLocation(CERTCertDBHandle *handle,
                          CERTCertificate *cert,
                          PRBool canUseDefaultLocation,
                          PRBool *isDefault);








PRBool
ocsp_FetchingFailureIsVerificationFailure(void);

size_t
ocsp_UrlEncodeBase64Buf(const char *base64Buf, char *outputBuf);

SECStatus
ocsp_GetVerifiedSingleResponseForCertID(CERTCertDBHandle *handle, 
                                        CERTOCSPResponse *response, 
                                        CERTOCSPCertID   *certID,
                                        CERTCertificate  *signerCert,
                                        PRTime            time,
                                        CERTOCSPSingleResponse **pSingleResponse);

SECStatus
ocsp_CertHasGoodStatus(ocspCertStatus *status, PRTime time);

void
ocsp_CacheSingleResponse(CERTOCSPCertID *certID,
			 CERTOCSPSingleResponse *single,
			 PRBool *certIDWasConsumed);

#endif 
