
















extern CRMFSinglePubInfo* 
      CRMF_CreateSinglePubInfo(CRMFPublicationMethod  inPubMethod,
			       CRMFGeneralName       *pubLocation);





extern CRMFPKIPublicationInfo *
     CRMF_CreatePKIPublicationInfo(CRMFPublicationAction  inAction,
				   CRMFSinglePubInfo    **inPubInfoArray,
				   int                    numPubInfo);






extern SECStatus 
       CRMF_DestroyPKIPublicationInfo(CRMFPKIPublicationInfo *inPubInfo);

extern SECStatus CRMF_AddPubInfoControl(CRMFCertRequest        *inCertReq,
					CRMFPKIPublicationInfo *inPubInfo);





extern CRMFCertID* CRMF_CreateCertID(CRMFGeneralName *issuer,
				     long             serialNumber);

extern SECStatus CRMF_DestroyCertID(CRMFCertID* certID);

extern SECStatus CRMF_AddCertIDControl(CRMFCertRequest *inCertReq,
				       CRMFCertID      *certID);

extern SECStatus 
       CRMF_AddProtocolEncryptioKeyControl(CRMFCertRequest          *inCertReq,
					   CERTSubjectPublicKeyInfo *spki);





extern SECStatus
       CRMF_AddUTF8PairsRegInfo(CRMFCertRequest *inCertReq,
				 SECItem         *asciiPairs);




extern SECStatus
       CRMF_AddCertReqToRegInfo(CRMFCertRequest *certReqToAddTo,
				CRMFCertRequest *certReqBeingAdded);




extern CRMFPOPOSkiInputAuthChoice 
       CRMF_GetSignKeyInputAuthChoice(CRMFPOPOSigningKeyInput *inKeyInput);









extern SECStatus 
       CRMF_GetSignKeyInputPKMACValue(CRMFPOPOSigningKeyInput *inKeyInput,
				      CRMFPKMACValue          **destValue);



extern CERTSubjectPublicKeyInfo *
       CRMF_GetSignKeyInputPublicKey(CRMFPOPOSigningKeyInput *inKeyInput);









extern CRMFPKIPublicationInfo* CRMF_GetPKIPubInfo(CRMFControl *inControl);




extern SECStatus 
       CRMF_DestroyPKIPublicationInfo(CRMFPKIPublicationInfo *inPubInfo);




extern CRMFPublicationAction 
       CRMF_GetPublicationAction(CRMFPKIPublicationInfo *inPubInfo);




extern int CRMF_GetNumPubInfos(CRMFPKIPublicationInfo *inPubInfo);





extern CRMFSinglePubInfo* 
       CRMF_GetPubInfoAtIndex(CRMFPKIPublicationInfo *inPubInfo,
			      int                     index);




extern SECStatus CRMF_DestroySinglePubInfo(CRMFSinglePubInfo *inPubInfo);




extern CRMFPublicationMethod 
       CRMF_GetPublicationMethod(CRMFSinglePubInfo *inPubInfo);






extern CRMFGeneralName* CRMF_GetPubLocation(CRMFSinglePubInfo *inPubInfo);









extern SECStatus CRMF_GetSignKeyInputSender(CRMFPOPOSigningKeyInput *keyInput,
					    CRMFGeneralName        **destName);



























extern SECStatus
CMMF_POPODecKeyChallContentSetNextChallenge
                                   (CMMFPOPODecKeyChallContent *inDecKeyChall,
				    long                        inRandom,
				    CERTGeneralName            *inSender,
				    SECKEYPublicKey            *inPubKey);










extern int CMMF_POPODecKeyChallContentGetNumChallenges
                                  (CMMFPOPODecKeyChallContent *inKeyChallCont);























extern SECStatus CMMF_ChallengeGetRandomNumber(CMMFChallenge *inChallenge,
					       long          *inDest);



















extern CERTGeneralName* CMMF_ChallengeGetSender(CMMFChallenge *inChallenge);





















extern SECStatus CMMF_ChallengeGetAlgId(CMMFChallenge  *inChallenge,
					SECAlgorithmID *inAlgId);














extern SECStatus CMMF_DestroyChallenge (CMMFChallenge *inChallenge);















extern SECStatus
     CMMF_DestroyPOPODecKeyRespContent(CMMFPOPODecKeyRespContent *inDecKeyResp);


























extern SECStatus CMMF_ChallengeDecryptWitness(CMMFChallenge    *inChallenge,
					      SECKEYPrivateKey *inPrivKey);











extern PRBool CMMF_ChallengeIsDecrypted(CMMFChallenge *inChallenge);















extern SECStatus 
 CMMF_DestroyPOPODecKeyChallContent (CMMFPOPODecKeyChallContent *inDecKeyCont);

