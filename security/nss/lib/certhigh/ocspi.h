








































#ifndef _OCSPI_H_
#define _OCSPI_H_

SECStatus OCSP_InitGlobal(void);
SECStatus OCSP_ShutdownGlobal(void);

ocspResponseData *
ocsp_GetResponseData(CERTOCSPResponse *response, SECItem **tbsResponseDataDER);

ocspSignature *
ocsp_GetResponseSignature(CERTOCSPResponse *response);

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
                                 int64 time, 
                                 PRBool addServiceLocator,
                                 CERTCertificate *signerCert);

SECStatus
ocsp_GetCachedOCSPResponseStatusIfFresh(CERTOCSPCertID *certID, 
                                        int64 time, 
                                        PRBool ignoreOcspFailureMode,
                                        SECStatus *rvOcsp,
                                        SECErrorCodes *missingResponseError);

































SECStatus
cert_ProcessOCSPResponse(CERTCertDBHandle *handle, 
                         CERTOCSPResponse *response, 
                         CERTOCSPCertID   *certID,
                         CERTCertificate  *signerCert,
                         int64             time,
                         PRBool           *certIDWasConsumed,
                         SECStatus        *cacheUpdateStatus);
















SECStatus
cert_RememberOCSPProcessingFailure(CERTOCSPCertID *certID,
                                   PRBool         *certIDWasConsumed);
















char *
ocsp_GetResponderLocation(CERTCertDBHandle *handle,
                          CERTCertificate *cert,
                          PRBool *isDefault);


#endif 
