




#ifndef _CMMF_H_
#define _CMMF_H_







#include "seccomon.h"
#include "cmmft.h"
#include "crmf.h"

SEC_BEGIN_PROTOS





















extern CMMFCertRepContent* CMMF_CreateCertRepContent(void);





















extern CMMFCertRepContent* 
       CMMF_CreateCertRepContentFromDER(CERTCertDBHandle *db, 
					const char       *buf, 
					long              len);



















extern CMMFCertResponse* CMMF_CreateCertResponse(long inCertReqId);





















extern CMMFKeyRecRepContent *CMMF_CreateKeyRecRepContent(void);





















extern CMMFKeyRecRepContent* 
       CMMF_CreateKeyRecRepContentFromDER(CERTCertDBHandle *db,
					  const char       *buf,
					  long              len);


















extern CMMFPOPODecKeyChallContent*
       CMMF_CreatePOPODecKeyChallContent(void);
















extern CMMFPOPODecKeyChallContent*
       CMMF_CreatePOPODecKeyChallContentFromDER(const char *buf, long len);
















extern CMMFPOPODecKeyRespContent*
       CMMF_CreatePOPODecKeyRespContentFromDER(const char *buf, long len);


























extern SECStatus 
      CMMF_CertRepContentSetCertResponses(CMMFCertRepContent *inCertRepContent,
					  CMMFCertResponse  **inCertResponses,
					  int                 inNumResponses);





















extern SECStatus 
       CMMF_CertRepContentSetCAPubs (CMMFCertRepContent  *inCertRepContent,
				     CERTCertList        *inCAPubs);



















extern SECStatus 
     CMMF_CertResponseSetPKIStatusInfoStatus (CMMFCertResponse *inCertResp,
					      CMMFPKIStatus     inPKIStatus);



















extern SECStatus 
       CMMF_CertResponseSetCertificate (CMMFCertResponse *inCertResp,
					CERTCertificate  *inCertificate);



















extern SECStatus 
CMMF_KeyRecRepContentSetPKIStatusInfoStatus(CMMFKeyRecRepContent *inKeyRecRep,
					    CMMFPKIStatus         inPKIStatus);

















extern SECStatus 
       CMMF_KeyRecRepContentSetNewSignCert(CMMFKeyRecRepContent *inKeyRecRep,
					   CERTCertificate     *inNewSignCert);



















extern SECStatus 
       CMMF_KeyRecRepContentSetCACerts(CMMFKeyRecRepContent *inKeyRecRep,
				       CERTCertList         *inCACerts);























extern SECStatus 
    CMMF_KeyRecRepContentSetCertifiedKeyPair(CMMFKeyRecRepContent *inKeyRecRep,
					     CERTCertificate      *inCert,
					     SECKEYPrivateKey     *inPrivKey,
					     SECKEYPublicKey      *inPubKey);































extern SECStatus
CMMF_POPODecKeyChallContentSetNextChallenge
                                   (CMMFPOPODecKeyChallContent *inDecKeyChall,
				    long                        inRandom,
				    CERTGeneralName            *inSender,
				    SECKEYPublicKey            *inPubKey,
				    void                       *passwdArg);






























extern SECStatus 
       CMMF_EncodeCertRepContent (CMMFCertRepContent        *inCertRepContent,
				  CRMFEncoderOutputCallback  inCallback,
				  void                      *inArg);



























extern SECStatus
       CMMF_EncodeKeyRecRepContent(CMMFKeyRecRepContent      *inKeyRecRep,
				   CRMFEncoderOutputCallback  inCallback,
				   void                      *inArg);





























extern SECStatus 
CMMF_EncodePOPODecKeyChallContent(CMMFPOPODecKeyChallContent *inDecKeyChall,
				  CRMFEncoderOutputCallback inCallback,
				  void                     *inArg);































extern SECStatus 
      CMMF_EncodePOPODecKeyRespContent(long                     *inDecodedRand,
				       int                       inNumRand,
				       CRMFEncoderOutputCallback inCallback,
				       void                     *inArg); 





























extern CERTCertList* 
       CMMF_CertRepContentGetCAPubs (CMMFCertRepContent *inCertRepContent);














extern int 
 CMMF_CertRepContentGetNumResponses (CMMFCertRepContent *inCertRepContent);




















extern CMMFCertResponse*
CMMF_CertRepContentGetResponseAtIndex (CMMFCertRepContent *inCertRepContent,
				       int                 inIndex);















extern long CMMF_CertResponseGetCertReqId(CMMFCertResponse *inCertResp);















extern CMMFPKIStatus 
       CMMF_CertResponseGetPKIStatusInfoStatus(CMMFCertResponse *inCertResp);





















extern CERTCertificate*
       CMMF_CertResponseGetCertificate(CMMFCertResponse *inCertResp,
                                       CERTCertDBHandle *inCertdb);













extern CMMFPKIStatus 
CMMF_KeyRecRepContentGetPKIStatusInfoStatus(CMMFKeyRecRepContent *inKeyRecRep);















extern CERTCertificate*
       CMMF_KeyRecRepContentGetNewSignCert(CMMFKeyRecRepContent *inKeyRecRep);





















extern CERTCertList*
       CMMF_KeyRecRepContentGetCACerts(CMMFKeyRecRepContent *inKeyRecRep);










extern int 
       CMMF_KeyRecRepContentGetNumKeyPairs(CMMFKeyRecRepContent *inKeyRecRep);


















extern CMMFCertifiedKeyPair*
      CMMF_KeyRecRepContentGetCertKeyAtIndex(CMMFKeyRecRepContent *inKeyRecRep,
					     int                   inIndex);






















extern CERTCertificate*
      CMMF_CertifiedKeyPairGetCertificate(CMMFCertifiedKeyPair *inCertKeyPair,
					  CERTCertDBHandle     *inCertdb);










extern int CMMF_POPODecKeyChallContentGetNumChallenges
                                  (CMMFPOPODecKeyChallContent *inKeyChallCont);



















extern SECItem* CMMF_POPODecKeyChallContentGetPublicValue
                                   (CMMFPOPODecKeyChallContent *inKeyChallCont,
				    int                         inIndex);



























extern SECStatus CMMF_POPODecKeyChallContentGetRandomNumber
                                      (CMMFPOPODecKeyChallContent *inKeyChallCont,
				       int                          inIndex,
				       long                        *inDest);









extern int 
 CMMF_POPODecKeyRespContentGetNumResponses(CMMFPOPODecKeyRespContent *inRespCont);























extern SECStatus
     CMMF_POPODecKeyRespContentGetResponse (CMMFPOPODecKeyRespContent *inRespCont,
					    int                        inIndex,
					    long                      *inDest);















extern SECStatus CMMF_DestroyCertResponse(CMMFCertResponse *inCertResp);














extern SECStatus 
       CMMF_DestroyCertRepContent (CMMFCertRepContent *inCertRepContent);














extern SECStatus 
       CMMF_DestroyKeyRecRepContent(CMMFKeyRecRepContent *inKeyRecRep);














extern SECStatus 
       CMMF_DestroyCertifiedKeyPair(CMMFCertifiedKeyPair *inCertKeyPair);















extern SECStatus
       CMMF_DestroyPOPODecKeyRespContent(CMMFPOPODecKeyRespContent *inDecKeyResp);



 































extern SECStatus 
       CMMF_CertifiedKeyPairUnwrapPrivKey(CMMFCertifiedKeyPair *inKeyPair,
					  SECKEYPrivateKey     *inPrivKey,
					  SECItem              *inNickName,
					  PK11SlotInfo         *inSlot,
                                          CERTCertDBHandle     *inCertdb,
					  SECKEYPrivateKey    **destPrivKey,
					  void                 *wincx);












extern PRBool 
       CMMF_KeyRecRepContentHasCACerts(CMMFKeyRecRepContent *inKeyRecRep);


































extern SECStatus 
  CMMF_POPODecKeyChallContDecryptChallenge(CMMFPOPODecKeyChallContent *inChalCont,
					   int                         inIndex,
					   SECKEYPrivateKey           *inPrivKey);















extern SECStatus 
 CMMF_DestroyPOPODecKeyChallContent (CMMFPOPODecKeyChallContent *inDecKeyCont);

SEC_END_PROTOS
#endif 
