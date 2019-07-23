









































#ifndef _OCSP_H_
#define _OCSP_H_


#include "plarena.h"
#include "seccomon.h"
#include "secoidt.h"
#include "keyt.h"
#include "certt.h"
#include "ocspt.h"



SEC_BEGIN_PROTOS










extern SECStatus
SEC_RegisterDefaultHttpClient(const SEC_HttpClientFcn *fcnTable);





extern const SEC_HttpClientFcn *
SEC_GetRegisteredHttpClient(void);














extern SECStatus
CERT_OCSPCacheSettings(PRInt32 maxCacheEntries,
                       PRUint32 minimumSecondsToNextFetchAttempt,
                       PRUint32 maximumSecondsToNextFetchAttempt);





extern SECStatus
CERT_SetOCSPFailureMode(SEC_OcspFailureMode ocspFailureMode);




extern SECStatus
CERT_SetOCSPTimeout(PRUint32 seconds);




extern SECStatus
CERT_ClearOCSPCache(void);











extern SECStatus
CERT_EnableOCSPChecking(CERTCertDBHandle *handle);
















extern SECStatus
CERT_DisableOCSPChecking(CERTCertDBHandle *handle);

























extern SECStatus
CERT_SetOCSPDefaultResponder(CERTCertDBHandle *handle,
			     const char *url, const char *name);


















extern SECStatus
CERT_EnableOCSPDefaultResponder(CERTCertDBHandle *handle);













extern SECStatus
CERT_DisableOCSPDefaultResponder(CERTCertDBHandle *handle);









































extern CERTOCSPRequest *
CERT_CreateOCSPRequest(CERTCertList *certList, PRTime time, 
		       PRBool addServiceLocator,
		       CERTCertificate *signerCert);

















extern SECStatus
CERT_AddOCSPAcceptableResponses(CERTOCSPRequest *request,
				SECOidTag responseType0, ...);



















extern SECItem *
CERT_EncodeOCSPRequest(PLArenaPool *arena, CERTOCSPRequest *request, 
		       void *pwArg);












extern CERTOCSPRequest *
CERT_DecodeOCSPRequest(SECItem *src);










extern void
CERT_DestroyOCSPRequest(CERTOCSPRequest *request);














extern CERTOCSPResponse *
CERT_DecodeOCSPResponse(SECItem *src);










extern void
CERT_DestroyOCSPResponse(CERTOCSPResponse *response);















































extern SECItem *
CERT_GetEncodedOCSPResponse(PLArenaPool *arena, CERTCertList *certList,
			    char *location, PRTime time,
			    PRBool addServiceLocator,
			    CERTCertificate *signerCert, void *pwArg,
			    CERTOCSPRequest **pRequest);
































extern SECStatus
CERT_VerifyOCSPResponseSignature(CERTOCSPResponse *response,	
				 CERTCertDBHandle *handle, void *pwArg,
				 CERTCertificate **pSignerCert,
				 CERTCertificate *issuerCert);


















extern char *
CERT_GetOCSPAuthorityInfoAccessLocation(CERTCertificate *cert);














extern SECStatus
CERT_RegisterAlternateOCSPAIAInfoCallBack(
			CERT_StringFromCertFcn   newCallback,
			CERT_StringFromCertFcn * oldCallback);





















extern SECStatus
CERT_ParseURL(const char *url, char **pHostname, PRUint16 *pPort, char **pPath);


















































    
extern SECStatus 
CERT_CheckOCSPStatus(CERTCertDBHandle *handle, CERTCertificate *cert,
		     PRTime time, void *pwArg);



















extern SECStatus
CERT_GetOCSPStatusForCertID(CERTCertDBHandle *handle, 
			    CERTOCSPResponse *response,
			    CERTOCSPCertID   *certID,
			    CERTCertificate  *signerCert,
                            PRTime            time);


















extern SECStatus
CERT_GetOCSPResponseStatus(CERTOCSPResponse *response);
















extern CERTOCSPCertID*
CERT_CreateOCSPCertID(CERTCertificate *cert, PRTime time);













extern SECStatus
CERT_DestroyOCSPCertID(CERTOCSPCertID* certID);

SEC_END_PROTOS

#endif 
