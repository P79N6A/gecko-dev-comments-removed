









#ifndef _CMMFI_H_
#define _CMMFI_H_
#include "cmmfit.h"
#include "crmfi.h"

#define CMMF_MAX_CHALLENGES 10
#define CMMF_MAX_KEY_PAIRS  50




extern const SEC_ASN1Template CMMFCertOrEncCertCertificateTemplate[];
extern const SEC_ASN1Template CMMFCertOrEncCertEncryptedCertTemplate[];
extern const SEC_ASN1Template CMMFPOPODecKeyRespContentTemplate[];
extern const SEC_ASN1Template CMMFRandTemplate[];
extern const SEC_ASN1Template CMMFSequenceOfCertsTemplate[];
extern const SEC_ASN1Template CMMFPKIStatusInfoTemplate[];
extern const SEC_ASN1Template CMMFCertifiedKeyPairTemplate[];







extern SECStatus cmmf_CopyCertResponse (PLArenaPool      *poolp,
					CMMFCertResponse *dest, 
					CMMFCertResponse *src);

extern SECStatus cmmf_CopyPKIStatusInfo (PLArenaPool       *poolp,
					 CMMFPKIStatusInfo *dest,
					 CMMFPKIStatusInfo *src);

extern SECStatus cmmf_CopyCertifiedKeyPair(PLArenaPool          *poolp,
					   CMMFCertifiedKeyPair *dest,
					   CMMFCertifiedKeyPair *src);

extern SECStatus cmmf_DestroyPKIStatusInfo(CMMFPKIStatusInfo *info, 
					   PRBool freeit);

extern SECStatus cmmf_DestroyCertOrEncCert(CMMFCertOrEncCert *certOrEncCert, 
					   PRBool freeit);

extern SECStatus cmmf_PKIStatusInfoSetStatus(CMMFPKIStatusInfo    *statusInfo,
					     PLArenaPool          *poolp,
					     CMMFPKIStatus         inStatus);

extern SECStatus cmmf_ExtractCertsFromList(CERTCertList      *inCertList,
					   PLArenaPool       *poolp,
					   CERTCertificate ***certArray);

extern SECStatus 
       cmmf_CertOrEncCertSetCertificate(CMMFCertOrEncCert *certOrEncCert,
					PLArenaPool       *poolp,
					CERTCertificate   *inCert);

extern CMMFPKIStatus 
       cmmf_PKIStatusInfoGetStatus(CMMFPKIStatusInfo *inStatus);

extern CERTCertList*
       cmmf_MakeCertList(CERTCertificate **inCerts);

extern CERTCertificate*
cmmf_CertOrEncCertGetCertificate(CMMFCertOrEncCert *certOrEncCert,
                                 CERTCertDBHandle  *certdb);

extern SECStatus
cmmf_decode_process_cert_response(PLArenaPool      *poolp,
				  CERTCertDBHandle *db,
				  CMMFCertResponse *inCertResp);

extern SECStatus
cmmf_decode_process_certified_key_pair(PLArenaPool          *poolp,
				       CERTCertDBHandle     *db,
				       CMMFCertifiedKeyPair *inCertKeyPair);

extern SECStatus
cmmf_user_encode(void *src, CRMFEncoderOutputCallback inCallback, void *inArg,
		 const SEC_ASN1Template *inTemplate);

extern SECStatus
cmmf_copy_secitem (PLArenaPool *poolp, SECItem *dest, SECItem *src);
#endif 





