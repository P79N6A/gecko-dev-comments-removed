





#ifndef _CRMF_H_
#define _CRMF_H_

#include "seccomon.h"
#include "cert.h"
#include "crmft.h"
#include "secoid.h"
#include "secpkcs7.h"

SEC_BEGIN_PROTOS



















extern SECStatus 
        CRMF_EncodeCertReqMsg (CRMFCertReqMsg            *inCertReqMsg, 
			       CRMFEncoderOutputCallback  fn,
			       void                      *arg);




















extern SECStatus CRMF_EncodeCertRequest (CRMFCertRequest           *inCertReq,
					 CRMFEncoderOutputCallback  fn,
					 void                      *arg);































extern SECStatus 
       CRMF_EncodeCertReqMessages(CRMFCertReqMsg           **inCertReqMsgs,
				  CRMFEncoderOutputCallback  fn,
				  void                      *arg);





















extern CRMFCertReqMsg* CRMF_CreateCertReqMsg(void);

















extern SECStatus CRMF_DestroyCertReqMsg(CRMFCertReqMsg *inCertReqMsg);






















extern SECStatus CRMF_CertReqMsgSetCertRequest(CRMFCertReqMsg  *inCertReqMsg, 
					       CRMFCertRequest *inCertReq);

















extern CRMFCertRequest *CRMF_CreateCertRequest (PRUint32 inRequestID);













extern SECStatus CRMF_DestroyCertRequest (CRMFCertRequest *inCertReq);



























extern CRMFCertExtension *CRMF_CreateCertExtension(SECOidTag      id, 
						   PRBool         isCritical,
						   SECItem       *data);














extern SECStatus CRMF_DestroyCertExtension(CRMFCertExtension *inExtension);














































































extern SECStatus
  CRMF_CertRequestSetTemplateField(CRMFCertRequest       *inCertReq, 
				   CRMFCertTemplateField  inTemplateField,
				   void                  *data);


















extern PRBool
  CRMF_CertRequestIsFieldPresent(CRMFCertRequest       *inCertReq,
				 CRMFCertTemplateField  inTemplateField);























extern PRBool
  CRMF_CertRequestIsControlPresent(CRMFCertRequest *inCertReq,
				   CRMFControlType  inControlType);
				   




















extern SECStatus CRMF_CertRequestSetRegTokenControl(CRMFCertRequest *inCertReq,
						    SECItem         *value);



















extern SECStatus 
       CRMF_CertRequestSetAuthenticatorControl (CRMFCertRequest *inCertReq,
						SECItem         *value);



















extern CRMFEncryptedKey* 
       CRMF_CreateEncryptedKeyWithEncryptedValue(SECKEYPrivateKey *inPrivKey,
						 CERTCertificate  *inCACert);












extern SECStatus CRMF_DestroyEncryptedKey(CRMFEncryptedKey *inEncrKey);
						
























extern CRMFPKIArchiveOptions*
       CRMF_CreatePKIArchiveOptions(CRMFPKIArchiveOptionsType  inType,
				    void                      *data);











extern SECStatus 
       CRMF_DestroyPKIArchiveOptions(CRMFPKIArchiveOptions *inArchOpt);

















extern SECStatus 
       CRMF_CertRequestSetPKIArchiveOptions(CRMFCertRequest       *inCertReq,
					    CRMFPKIArchiveOptions *inOptions);
















extern CRMFPOPChoice CRMF_CertReqMsgGetPOPType(CRMFCertReqMsg *inCertReqMsg);















extern SECStatus CRMF_CertReqMsgSetRAVerifiedPOP(CRMFCertReqMsg *inCertReqMsg);






















































extern SECStatus 
       CRMF_CertReqMsgSetSignaturePOP(CRMFCertReqMsg   *inCertReqMsg,
				      SECKEYPrivateKey *inPrivKey,
				      SECKEYPublicKey  *inPubKey,
				      CERTCertificate  *inCertForInput,
				      CRMFMACPasswordCallback  fn,
				      void                    *arg);



















































extern SECStatus 
      CRMF_CertReqMsgSetKeyEnciphermentPOP(CRMFCertReqMsg        *inCertReqMsg,
					   CRMFPOPOPrivKeyChoice  inKeyChoice,
					   CRMFSubseqMessOptions  subseqMess,
					   SECItem               *encPrivKey);












































extern SECStatus 
       CRMF_CertReqMsgSetKeyAgreementPOP(CRMFCertReqMsg        *inCertReqMsg,
					 CRMFPOPOPrivKeyChoice  inKeyChoice,
					 CRMFSubseqMessOptions  subseqMess,
					 SECItem               *encPrivKey);


















extern CRMFCertReqMsg* CRMF_CreateCertReqMsgFromDER(const char *buf, long len);



















 
extern CRMFCertReqMessages*
       CRMF_CreateCertReqMessagesFromDER(const char *buf, long len);









 
extern SECStatus 
       CRMF_DestroyCertReqMessages(CRMFCertReqMessages *inCertReqMsgs);










extern int 
       CRMF_CertReqMessagesGetNumMessages(CRMFCertReqMessages *inCertReqMsgs);




















extern CRMFCertReqMsg*
       CRMF_CertReqMessagesGetCertReqMsgAtIndex(CRMFCertReqMessages *inReqMsgs,
						int                  index);
















extern SECStatus CRMF_CertReqMsgGetID(CRMFCertReqMsg *inCertReqMsg, 
				      long           *destID);






















extern PRBool CRMF_DoesRequestHaveField(CRMFCertRequest       *inCertReq,
					CRMFCertTemplateField  inField);

















extern CRMFCertRequest *
       CRMF_CertReqMsgGetCertRequest(CRMFCertReqMsg *inCertReqMsg);
















extern SECStatus 
       CRMF_CertRequestGetCertTemplateVersion(CRMFCertRequest *inCertReq, 
					      long            *version);
















extern SECStatus 
       CRMF_CertRequestGetCertTemplateSerialNumber(CRMFCertRequest *inCertReq, 
						   long         *serialNumber);
















extern SECStatus 
       CRMF_CertRequestGetCertTemplateSigningAlg(CRMFCertRequest *inCertReq,
						 SECAlgorithmID  *destAlg);














extern SECStatus 
       CRMF_CertRequestGetCertTemplateIssuer(CRMFCertRequest *inCertReq,
					     CERTName        *destIssuer);




















extern SECStatus 
       CRMF_CertRequestGetCertTemplateValidity(CRMFCertRequest *inCertReq,
					       CRMFGetValidity *destValidity);


















extern SECStatus 
       CRMF_DestroyGetValidity(CRMFGetValidity *inValidity);
















extern SECStatus 
       CRMF_CertRequestGetCertTemplateSubject (CRMFCertRequest *inCertReq,
					       CERTName        *destSubject);
















extern SECStatus 
       CRMF_CertRequestGetCertTemplatePublicKey(CRMFCertRequest *inCertReq,
				      CERTSubjectPublicKeyInfo *destPublicKey);






















extern SECStatus 
       CRMF_CertRequestGetCertTemplateIssuerUID(CRMFCertRequest *inCertReq,
						SECItem        *destIssuerUID);





















extern SECStatus CRMF_GetCertTemplateSubjectUID(CRMFCertRequest *inCertReq,
						SECItem       *destSubjectUID);









extern int CRMF_CertRequestGetNumberOfExtensions(CRMFCertRequest *inCertReq);




















extern CRMFCertExtension *
       CRMF_CertRequestGetExtensionAtIndex(CRMFCertRequest *inCertReq,
					   int              index);









extern SECOidTag CRMF_CertExtensionGetOidTag(CRMFCertExtension *inExtension);











extern PRBool CRMF_CertExtensionGetIsCritical(CRMFCertExtension *inExt);
             














extern SECItem*  CRMF_CertExtensionGetValue(CRMFCertExtension *inExtension);



















extern SECStatus 
       CRMF_CertReqMsgGetPOPOSigningKey(CRMFCertReqMsg      *inCertReqMsg,
					CRMFPOPOSigningKey **destKey);











extern SECStatus CRMF_DestroyPOPOSigningKey (CRMFPOPOSigningKey *inKey);











extern SECAlgorithmID* 
       CRMF_POPOSigningKeyGetAlgID(CRMFPOPOSigningKey *inSignKey);













extern SECItem* CRMF_POPOSigningKeyGetSignature(CRMFPOPOSigningKey *inSignKey);



















extern SECItem* CRMF_POPOSigningKeyGetInput(CRMFPOPOSigningKey *inSignKey);






















extern SECStatus 
       CRMF_CertReqMsgGetPOPKeyEncipherment(CRMFCertReqMsg   *inCertReqMsg,
					    CRMFPOPOPrivKey **destKey);






















extern SECStatus 
       CRMF_CertReqMsgGetPOPKeyAgreement(CRMFCertReqMsg   *inCertReqMsg,
					 CRMFPOPOPrivKey **destKey);















extern SECStatus CRMF_DestroyPOPOPrivKey(CRMFPOPOPrivKey *inPrivKey);











extern CRMFPOPOPrivKeyChoice CRMF_POPOPrivKeyGetChoice(CRMFPOPOPrivKey *inKey);




















extern SECStatus CRMF_POPOPrivKeyGetThisMessage(CRMFPOPOPrivKey  *inKey,
						SECItem          *destString);



















extern SECStatus CRMF_POPOPrivKeyGetSubseqMess(CRMFPOPOPrivKey       *inKey,
					       CRMFSubseqMessOptions *destOpt);
























extern SECStatus CRMF_POPOPrivKeyGetDHMAC(CRMFPOPOPrivKey *inKey,
					  SECItem         *destMAC);









extern int CRMF_CertRequestGetNumControls (CRMFCertRequest *inCertReq);


















extern CRMFControl* 
       CRMF_CertRequestGetControlAtIndex(CRMFCertRequest *inCertReq, 
					 int              index);













extern SECStatus CRMF_DestroyControl(CRMFControl *inControl);














extern CRMFControlType CRMF_ControlGetControlType(CRMFControl *inControl);



















extern SECItem* CRMF_ControlGetRegTokenControlValue(CRMFControl *inControl);



















extern SECItem* CRMF_ControlGetAuthicatorControlValue(CRMFControl *inControl);















extern CRMFPKIArchiveOptions* 
       CRMF_ControlGetPKIArchiveOptions(CRMFControl *inControl);
  













extern SECStatus 
       CRMF_DestroyPKIArchiveOptions(CRMFPKIArchiveOptions *inOptions);










extern CRMFPKIArchiveOptionsType
       CRMF_PKIArchiveOptionsGetOptionType(CRMFPKIArchiveOptions *inOptions);
















extern CRMFEncryptedKey*
      CRMF_PKIArchiveOptionsGetEncryptedPrivKey(CRMFPKIArchiveOptions *inOpts);















extern CRMFEncryptedKeyChoice 
       CRMF_EncryptedKeyGetChoice(CRMFEncryptedKey *inEncrKey);
















extern CRMFEncryptedValue* 
       CRMF_EncryptedKeyGetEncryptedValue(CRMFEncryptedKey *inKey);















extern SECStatus CRMF_DestroyEncryptedValue(CRMFEncryptedValue *inEncrValue);
















extern SECItem* CRMF_EncryptedValueGetEncValue(CRMFEncryptedValue *inEncValue);
















extern SECAlgorithmID* 
       CRMF_EncryptedValueGetIntendedAlg(CRMFEncryptedValue  *inEncValue);

















extern SECAlgorithmID* 
       CRMF_EncryptedValueGetSymmAlg(CRMFEncryptedValue  *inEncValue);

















extern SECAlgorithmID* 
       CRMF_EncryptedValueGetKeyAlg(CRMFEncryptedValue *inEncValue);



















extern SECItem* 
       CRMF_EncryptedValueGetValueHint(CRMFEncryptedValue  *inEncValue);























extern SECItem* 
       CRMF_EncryptedValueGetEncSymmKey(CRMFEncryptedValue *inEncValue);



















extern SECItem* 
   CRMF_PKIArchiveOptionsGetKeyGenParameters(CRMFPKIArchiveOptions *inOptions);


















extern SECStatus 
    CRMF_PKIArchiveOptionsGetArchiveRemGenPrivKey(CRMFPKIArchiveOptions *inOpt,
						  PRBool             *destVal);





extern CK_MECHANISM_TYPE CRMF_GetBestWrapPadMechanism(PK11SlotInfo *slot); 





extern SECItem* CRMF_GetIVFromMechanism(CK_MECHANISM_TYPE mechType);
 
SEC_END_PROTOS
#endif 


