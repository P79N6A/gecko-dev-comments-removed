







#ifndef _CERT_H_
#define _CERT_H_

#include "utilrename.h"
#include "plarena.h"
#include "plhash.h"
#include "prlong.h"
#include "prlog.h"

#include "seccomon.h"
#include "secdert.h"
#include "secoidt.h"
#include "keyt.h"
#include "certt.h"

SEC_BEGIN_PROTOS
   









extern CERTName *CERT_AsciiToName(const char *string);







extern char *CERT_NameToAscii(CERTName *name);






extern char *CERT_NameToAsciiInvertible(CERTName *name, 
                                        CertStrictnessLevel strict);

extern CERTAVA *CERT_CopyAVA(PLArenaPool *arena, CERTAVA *src);



extern char * CERT_GetOidString(const SECItem *oid);





extern SECOidTag CERT_GetAVATag(CERTAVA *ava);




extern SECComparison CERT_CompareAVA(const CERTAVA *a, const CERTAVA *b);





extern CERTRDN *CERT_CreateRDN(PLArenaPool *arena, CERTAVA *avas, ...);




extern SECStatus CERT_CopyRDN(PLArenaPool *arena, CERTRDN *dest, CERTRDN *src);






extern SECStatus CERT_AddAVA(PLArenaPool *arena, CERTRDN *rdn, CERTAVA *ava);




extern SECComparison CERT_CompareRDN(const CERTRDN *a, const CERTRDN *b);




extern CERTName *CERT_CreateName(CERTRDN *rdn, ...);







extern SECStatus CERT_CopyName(PLArenaPool *arena, CERTName *dest,
                               const CERTName *src);






extern void CERT_DestroyName(CERTName *name);






extern SECStatus CERT_AddRDN(CERTName *name, CERTRDN *rdn);




extern SECComparison CERT_CompareName(const CERTName *a, const CERTName *b);




extern char *CERT_FormatName (CERTName *name);





extern char *CERT_Hexify (SECItem *i, int do_colon);









extern SECStatus
CERT_RFC1485_EscapeAndQuote(char *dst, int dstlen, char *src, int srclen);












extern CERTValidity *CERT_CreateValidity(PRTime notBefore, PRTime notAfter);






extern void CERT_DestroyValidity(CERTValidity *v);







extern SECStatus CERT_CopyValidity
   (PLArenaPool *arena, CERTValidity *dest, CERTValidity *src);










PRInt32 CERT_GetSlopTime(void);

SECStatus CERT_SetSlopTime(PRInt32 slop);









extern CERTCertificate *
CERT_CreateCertificate (unsigned long serialNumber, CERTName *issuer,
			CERTValidity *validity, CERTCertificateRequest *req);








extern void CERT_DestroyCertificate(CERTCertificate *cert);





extern CERTCertificate *CERT_DupCertificate(CERTCertificate *c);








extern CERTCertificateRequest *
CERT_CreateCertificateRequest (CERTName *name, CERTSubjectPublicKeyInfo *spki,
			       SECItem **attributes);






extern void CERT_DestroyCertificateRequest(CERTCertificateRequest *r);




void *
CERT_StartCertificateRequestAttributes(CERTCertificateRequest *req);





SECStatus
CERT_FinishCertificateRequestAttributes(CERTCertificateRequest *req);




SECStatus
CERT_GetCertificateRequestExtensions(CERTCertificateRequest *req,
                                     CERTCertExtension ***exts);




extern SECKEYPublicKey *CERT_ExtractPublicKey(CERTCertificate *cert);





extern KeyType CERT_GetCertKeyType (const CERTSubjectPublicKeyInfo *spki);





extern SECStatus CERT_InitCertDB(CERTCertDBHandle *handle);

extern int CERT_GetDBContentVersion(CERTCertDBHandle *handle);




extern void CERT_SetDefaultCertDB(CERTCertDBHandle *handle);

extern CERTCertDBHandle *CERT_GetDefaultCertDB(void);

extern CERTCertList *CERT_GetCertChainFromCert(CERTCertificate *cert, 
					       PRTime time, 
					       SECCertUsage usage);
extern CERTCertificate *
CERT_NewTempCertificate (CERTCertDBHandle *handle, SECItem *derCert,
                         char *nickname, PRBool isperm, PRBool copyDER);
















extern CERTAVA *CERT_CreateAVA
   (PLArenaPool *arena, SECOidTag kind, int valueType, char *value);






extern SECStatus CERT_NameFromDERCert(SECItem *derCert, SECItem *derName);






extern SECStatus CERT_IssuerNameFromDERCert(SECItem *derCert, 
					    SECItem *derName);

extern SECItem *
CERT_EncodeGeneralName(CERTGeneralName *genName, SECItem *dest,
		       PLArenaPool *arena);

extern CERTGeneralName *
CERT_DecodeGeneralName(PLArenaPool *reqArena, SECItem *encodedName,
		       CERTGeneralName  *genName);










extern SECStatus CERT_KeyFromDERCert(PLArenaPool *reqArena, SECItem *derCert,
                                     SECItem *key);

extern SECStatus CERT_KeyFromIssuerAndSN(PLArenaPool *arena, SECItem *issuer,
					 SECItem *sn, SECItem *key);

extern SECStatus CERT_SerialNumberFromDERCert(SECItem *derCert, 
						SECItem *derName);









extern SECStatus CERT_KeyFromDERCrl(PLArenaPool *arena, SECItem *derCrl, SECItem *key);




extern SECStatus CERT_OpenCertDB(CERTCertDBHandle *handle, PRBool readOnly,
				 CERTDBNameFunc namecb, void *cbarg);


extern SECStatus CERT_OpenCertDBFilename(CERTCertDBHandle *handle,
					 char *certdbname, PRBool readOnly);





extern SECStatus CERT_OpenVolatileCertDB(CERTCertDBHandle *handle);








extern CERTCertNicknames *
  CERT_GetValidDNSPatternsFromCert(CERTCertificate *cert);





extern SECStatus CERT_VerifyCertName(const CERTCertificate *cert,
                                     const char *hostname);





extern SECStatus CERT_AddOKDomainName(CERTCertificate *cert, const char *hostname);









extern CERTCertificate *
CERT_DecodeDERCertificate (SECItem *derSignedCert, PRBool copyDER, char *nickname);





#define SEC_CRL_TYPE	1
#define SEC_KRL_TYPE	0 /* deprecated */

extern CERTSignedCrl *
CERT_DecodeDERCrl (PLArenaPool *arena, SECItem *derSignedCrl,int type);





extern CERTSignedCrl *
CERT_DecodeDERCrlWithFlags(PLArenaPool *narena, SECItem *derSignedCrl,
                          int type, PRInt32 options);



#define CRL_DECODE_DEFAULT_OPTIONS          0x00000000







#define CRL_DECODE_DONT_COPY_DER            0x00000001
#define CRL_DECODE_SKIP_ENTRIES             0x00000002
#define CRL_DECODE_KEEP_BAD_CRL             0x00000004
#define CRL_DECODE_ADOPT_HEAP_DER           0x00000008






extern SECStatus CERT_CompleteCRLDecodeEntries(CERTSignedCrl* crl);







extern CERTSignedCrl *
CERT_ImportCRL (CERTCertDBHandle *handle, SECItem *derCRL, char *url, 
						int type, void * wincx);

extern void CERT_DestroyCrl (CERTSignedCrl *crl);



void CERT_CRLCacheRefreshIssuer(CERTCertDBHandle* dbhandle, SECItem* crlKey);









SECStatus CERT_CacheCRL(CERTCertDBHandle* dbhandle, SECItem* newcrl);




SECStatus CERT_UncacheCRL(CERTCertDBHandle* dbhandle, SECItem* oldcrl);





extern CERTCertificate *CERT_FindCertByKey(CERTCertDBHandle *handle, SECItem *key);





extern CERTCertificate *
CERT_FindCertByName (CERTCertDBHandle *handle, SECItem *name);





extern CERTCertificate *
CERT_FindCertByNameString (CERTCertDBHandle *handle, char *name);






extern CERTCertificate *
CERT_FindCertByKeyID (CERTCertDBHandle *handle, SECItem *name, SECItem *keyID);






extern CERTCertificate *
CERT_FindCertByIssuerAndSN (CERTCertDBHandle *handle, CERTIssuerAndSN *issuerAndSN);





extern CERTCertificate *
CERT_FindCertBySubjectKeyID (CERTCertDBHandle *handle, SECItem *subjKeyID);





extern SECStatus 
CERT_EncodeSubjectKeyID(PLArenaPool *arena, const SECItem* srcString,
                        SECItem *encodedValue);





extern CERTCertificate *
CERT_FindCertByNickname (CERTCertDBHandle *handle, const char *nickname);





extern CERTCertificate *
CERT_FindCertByDERCert(CERTCertDBHandle *handle, SECItem *derCert);





CERTCertificate *
CERT_FindCertByEmailAddr(CERTCertDBHandle *handle, char *emailAddr);





CERTCertificate *
CERT_FindCertByNicknameOrEmailAddr(CERTCertDBHandle *handle, const char *name);






CERTCertificate *
CERT_FindCertByNicknameOrEmailAddrForUsage(CERTCertDBHandle *handle,
                                           const char *name, 
                                           SECCertUsage lookingForUsage);





extern CERTCertificate *
CERT_FindCertBySPKDigest(CERTCertDBHandle *handle, SECItem *spkDigest);




CERTCertificate *
CERT_FindCertIssuer(CERTCertificate *cert, PRTime validTime, SECCertUsage usage);









extern SECCertTimeValidity CERT_CheckCertValidTimes(const CERTCertificate *cert,
						    PRTime t,
						    PRBool allowOverride);









extern SECStatus CERT_CertTimesValid(CERTCertificate *cert);







extern SECStatus
CERT_GetCertTimes (const CERTCertificate *c, PRTime *notBefore,
		   PRTime *notAfter);




extern CERTIssuerAndSN *CERT_GetCertIssuerAndSN(PLArenaPool *, 
							CERTCertificate *);






extern SECStatus CERT_VerifySignedData(CERTSignedData *sd,
				       CERTCertificate *cert,
				       PRTime t,
				       void *wincx);



extern SECStatus
CERT_VerifySignedDataWithPublicKeyInfo(CERTSignedData *sd,
                                       CERTSubjectPublicKeyInfo *pubKeyInfo,
                                       void *wincx);




extern SECStatus
CERT_VerifySignedDataWithPublicKey(const CERTSignedData *sd,
                                   SECKEYPublicKey *pubKey, void *wincx);









extern SECStatus
CERT_VerifyCertificate(CERTCertDBHandle *handle, CERTCertificate *cert,
		PRBool checkSig, SECCertificateUsage requiredUsages,
                PRTime t, void *wincx, CERTVerifyLog *log,
                SECCertificateUsage* returnedUsages);


extern SECStatus
CERT_VerifyCertificateNow(CERTCertDBHandle *handle, CERTCertificate *cert,
		   PRBool checkSig, SECCertificateUsage requiredUsages,
                   void *wincx, SECCertificateUsage* returnedUsages);






extern SECStatus
CERT_VerifyCACertForUsage(CERTCertDBHandle *handle, CERTCertificate *cert,
		PRBool checkSig, SECCertUsage certUsage, PRTime t,
		void *wincx, CERTVerifyLog *log);









extern SECStatus
CERT_VerifyCert(CERTCertDBHandle *handle, CERTCertificate *cert,
		PRBool checkSig, SECCertUsage certUsage, PRTime t,
		void *wincx, CERTVerifyLog *log);


extern SECStatus
CERT_VerifyCertNow(CERTCertDBHandle *handle, CERTCertificate *cert,
		   PRBool checkSig, SECCertUsage certUsage, void *wincx);

SECStatus
CERT_VerifyCertChain(CERTCertDBHandle *handle, CERTCertificate *cert,
		     PRBool checkSig, SECCertUsage certUsage, PRTime t,
		     void *wincx, CERTVerifyLog *log);






extern CERTCertificate *CERT_ConvertAndDecodeCertificate(char *certstr);









extern CERTCertificate *CERT_DecodeCertFromPackage(char *certbuf, int certlen);

extern SECStatus
CERT_ImportCAChain (SECItem *certs, int numcerts, SECCertUsage certUsage);

extern SECStatus
CERT_ImportCAChainTrusted(SECItem *certs, int numcerts, SECCertUsage certUsage);









typedef SECStatus (PR_CALLBACK *CERTImportCertificateFunc)
   (void *arg, SECItem **certs, int numcerts);

extern SECStatus
CERT_DecodeCertPackage(char *certbuf, int certlen, CERTImportCertificateFunc f,
		       void *arg);









extern SECItem *CERT_DecodeAVAValue(const SECItem *derAVAValue);








extern char *CERT_GetCertificateEmailAddress(CERTCertificate *cert);

extern char *CERT_GetCertEmailAddress(const CERTName *name);

extern const char * CERT_GetFirstEmailAddress(CERTCertificate * cert);

extern const char * CERT_GetNextEmailAddress(CERTCertificate * cert, 
                                             const char * prev);


extern char *CERT_GetCommonName(const CERTName *name);

extern char *CERT_GetCountryName(const CERTName *name);

extern char *CERT_GetLocalityName(const CERTName *name);

extern char *CERT_GetStateName(const CERTName *name);

extern char *CERT_GetOrgName(const CERTName *name);

extern char *CERT_GetOrgUnitName(const CERTName *name);

extern char *CERT_GetDomainComponentName(const CERTName *name);

extern char *CERT_GetCertUid(const CERTName *name);



extern SECStatus CERT_GetCertTrust(const CERTCertificate *cert,
                                   CERTCertTrust *trust);

extern SECStatus
CERT_ChangeCertTrust (CERTCertDBHandle *handle, CERTCertificate *cert,
		      CERTCertTrust *trust);

extern SECStatus
CERT_ChangeCertTrustByUsage(CERTCertDBHandle *certdb, CERTCertificate *cert,
			    SECCertUsage usage);












extern void *CERT_StartCertExtensions(CERTCertificate *cert);










extern SECStatus CERT_AddExtension (void *exthandle, int idtag, 
			SECItem *value, PRBool critical, PRBool copyData);

extern SECStatus CERT_AddExtensionByOID (void *exthandle, SECItem *oid,
			 SECItem *value, PRBool critical, PRBool copyData);

extern SECStatus CERT_EncodeAndAddExtension
   (void *exthandle, int idtag, void *value, PRBool critical,
    const SEC_ASN1Template *atemplate);

extern SECStatus CERT_EncodeAndAddBitStrExtension
   (void *exthandle, int idtag, SECItem *value, PRBool critical);


extern SECStatus
CERT_EncodeAltNameExtension(PLArenaPool *arena,  CERTGeneralName  *value, SECItem *encodedValue);








extern SECStatus CERT_FinishExtensions(void *exthandle);






SECStatus
CERT_MergeExtensions(void *exthandle, CERTCertExtension **exts);




extern SECStatus CERT_GetExtenCriticality
   (CERTCertExtension **extensions, int tag, PRBool *isCritical);

extern void
CERT_DestroyOidSequence(CERTOidSequence *oidSeq);












extern SECStatus CERT_EncodeBasicConstraintValue
   (PLArenaPool *arena, CERTBasicConstraints *value, SECItem *encodedValue);




extern SECStatus CERT_EncodeAuthKeyID
   (PLArenaPool *arena, CERTAuthKeyID *value, SECItem *encodedValue);




extern SECStatus CERT_EncodeCRLDistributionPoints
   (PLArenaPool *arena, CERTCrlDistributionPoints *value,SECItem *derValue);






extern SECStatus CERT_DecodeBasicConstraintValue
   (CERTBasicConstraints *value, const SECItem *encodedValue);







extern CERTAuthKeyID *CERT_DecodeAuthKeyID 
			(PLArenaPool *arena, const SECItem *encodedValue);








extern CERTCrlDistributionPoints * CERT_DecodeCRLDistributionPoints
   (PLArenaPool *arena, SECItem *der);


extern void *CERT_GetGeneralNameByType
   (CERTGeneralName *genNames, CERTGeneralNameType type, PRBool derFormat);


extern CERTOidSequence *
CERT_DecodeOidSequence(const SECItem *seqItem);










extern SECStatus CERT_FindCertExtension
   (const CERTCertificate *cert, int tag, SECItem *value);

extern SECStatus CERT_FindNSCertTypeExtension
   (CERTCertificate *cert, SECItem *value);

extern char * CERT_FindNSStringExtension (CERTCertificate *cert, int oidtag);

extern SECStatus CERT_FindCertExtensionByOID
   (CERTCertificate *cert, SECItem *oid, SECItem *value);




extern CERTAuthKeyID * CERT_FindAuthKeyIDExten (PLArenaPool *arena,CERTCertificate *cert);



extern SECStatus CERT_FindBasicConstraintExten
   (CERTCertificate *cert, CERTBasicConstraints *value);




extern CERTCrlDistributionPoints * CERT_FindCRLDistributionPoints
   (CERTCertificate *cert);





extern SECStatus CERT_FindKeyUsageExtension (CERTCertificate *cert, 
							SECItem *value);




extern SECStatus CERT_FindSubjectKeyIDExtension (CERTCertificate *cert, 
							   SECItem *retItem);










extern SECStatus CERT_CheckCertUsage (CERTCertificate *cert, 
							unsigned char usage);







extern SECStatus CERT_FindCRLExtensionByOID
   (CERTCrl *crl, SECItem *oid, SECItem *value);

extern SECStatus CERT_FindCRLExtension
   (CERTCrl *crl, int tag, SECItem *value);

extern SECStatus
   CERT_FindInvalidDateExten (CERTCrl *crl, PRTime *value);






extern void *CERT_StartCRLExtensions(CERTCrl *crl);







extern void *CERT_StartCRLEntryExtensions(CERTCrl *crl, CERTCrlEntry *entry);

extern CERTCertNicknames *CERT_GetCertNicknames (CERTCertDBHandle *handle,
						 int what, void *wincx);




extern SECStatus CERT_FindCRLNumberExten (PLArenaPool *arena, CERTCrl *crl,
                                          SECItem *value);

extern SECStatus CERT_FindCRLEntryReasonExten (CERTCrlEntry *crlEntry,
					       CERTCRLEntryReasonCode *value);

extern void CERT_FreeNicknames(CERTCertNicknames *nicknames);

extern PRBool CERT_CompareCerts(const CERTCertificate *c1,
                                const CERTCertificate *c2);

extern PRBool CERT_CompareCertsForRedirection(CERTCertificate *c1,
							 CERTCertificate *c2);





extern CERTDistNames *CERT_GetSSLCACerts(CERTCertDBHandle *handle);

extern void CERT_FreeDistNames(CERTDistNames *names);


extern CERTDistNames *CERT_DupDistNames(CERTDistNames *orig);




extern CERTDistNames *CERT_DistNamesFromNicknames
   (CERTCertDBHandle *handle, char **nicknames, int nnames);




extern CERTDistNames *CERT_DistNamesFromCertList(CERTCertList *list);




extern CERTCertificateList *
CERT_CertChainFromCert(CERTCertificate *cert, SECCertUsage usage,
		       PRBool includeRoot);

extern CERTCertificateList *
CERT_CertListFromCert(CERTCertificate *cert);

extern CERTCertificateList *
CERT_DupCertList(const CERTCertificateList * oldList);

extern void CERT_DestroyCertificateList(CERTCertificateList *list);





PRBool CERT_IsUserCert(CERTCertificate* cert);


PRBool CERT_IsNewer(CERTCertificate *certa, CERTCertificate *certb);


PRBool
CERT_IsCertRevoked(CERTCertificate *cert);

void
CERT_DestroyCertArray(CERTCertificate **certs, unsigned int ncerts);


char *CERT_FixupEmailAddr(const char *emailAddr);


SECStatus
CERT_DecodeTrustString(CERTCertTrust *trust, const char *trusts);


char *
CERT_EncodeTrustString(CERTCertTrust *trust);


CERTCertificate *
CERT_PrevSubjectCert(CERTCertificate *cert);
CERTCertificate *
CERT_NextSubjectCert(CERTCertificate *cert);





SECStatus
CERT_ImportCerts(CERTCertDBHandle *certdb, SECCertUsage usage,
		 unsigned int ncerts, SECItem **derCerts,
		 CERTCertificate ***retCerts, PRBool keepCerts,
		 PRBool caOnly, char *nickname);

char *
CERT_MakeCANickname(CERTCertificate *cert);

PRBool
CERT_IsCACert(CERTCertificate *cert, unsigned int *rettype);

PRBool
CERT_IsCADERCert(SECItem *derCert, unsigned int *rettype);

PRBool
CERT_IsRootDERCert(SECItem *derCert);

SECStatus
CERT_SaveSMimeProfile(CERTCertificate *cert, SECItem *emailProfile,
		      SECItem *profileTime);




SECItem *
CERT_FindSMimeProfile(CERTCertificate *cert);

SECStatus
CERT_AddNewCerts(CERTCertDBHandle *handle);

CERTCertificatePolicies *
CERT_DecodeCertificatePoliciesExtension(const SECItem *extnValue);

void
CERT_DestroyCertificatePoliciesExtension(CERTCertificatePolicies *policies);

CERTCertificatePolicyMappings *
CERT_DecodePolicyMappingsExtension(SECItem *encodedCertPolicyMaps);

SECStatus
CERT_DestroyPolicyMappingsExtension(CERTCertificatePolicyMappings *mappings);

SECStatus
CERT_DecodePolicyConstraintsExtension(
    CERTCertificatePolicyConstraints *decodedValue,
    const SECItem *encodedValue);

SECStatus CERT_DecodeInhibitAnyExtension
    (CERTCertificateInhibitAny *decodedValue, SECItem *extnValue);

CERTUserNotice *
CERT_DecodeUserNotice(SECItem *noticeItem);

extern CERTGeneralName *
CERT_DecodeAltNameExtension(PLArenaPool *reqArena, SECItem *EncodedAltName);

extern CERTNameConstraints *
CERT_DecodeNameConstraintsExtension(PLArenaPool *arena, 
                                    const SECItem *encodedConstraints);


extern CERTAuthInfoAccess **
CERT_DecodeAuthInfoAccessExtension(PLArenaPool *reqArena,
				   const SECItem *encodedExtension);

extern CERTPrivKeyUsagePeriod *
CERT_DecodePrivKeyUsagePeriodExtension(PLArenaPool *arena, SECItem *extnValue);

extern CERTGeneralName *
CERT_GetNextGeneralName(CERTGeneralName *current);

extern CERTGeneralName *
CERT_GetPrevGeneralName(CERTGeneralName *current);












SECStatus
CERT_GetImposedNameConstraints(const SECItem *derSubject, SECItem *extensions);

CERTNameConstraint *
CERT_GetNextNameConstraint(CERTNameConstraint *current);

CERTNameConstraint *
CERT_GetPrevNameConstraint(CERTNameConstraint *current);

void
CERT_DestroyUserNotice(CERTUserNotice *userNotice);

typedef char * (* CERTPolicyStringCallback)(char *org,
					       unsigned long noticeNumber,
					       void *arg);
void
CERT_SetCAPolicyStringCallback(CERTPolicyStringCallback cb, void *cbarg);

char *
CERT_GetCertCommentString(CERTCertificate *cert);

PRBool
CERT_GovtApprovedBitSet(CERTCertificate *cert);

SECStatus
CERT_AddPermNickname(CERTCertificate *cert, char *nickname);

CERTCertList *
CERT_MatchUserCert(CERTCertDBHandle *handle,
		   SECCertUsage usage,
		   int nCANames, char **caNames,
		   void *proto_win);

CERTCertList *
CERT_NewCertList(void);


void
CERT_DestroyCertList(CERTCertList *certs);


void
CERT_RemoveCertListNode(CERTCertListNode *node);


SECStatus
CERT_AddCertToListTail(CERTCertList *certs, CERTCertificate *cert);


SECStatus
CERT_AddCertToListHead(CERTCertList *certs, CERTCertificate *cert);





SECStatus
CERT_AddCertToListTailWithData(CERTCertList *certs, CERTCertificate *cert,
							 void *appData);





SECStatus
CERT_AddCertToListHeadWithData(CERTCertList *certs, CERTCertificate *cert,
							 void *appData);

typedef PRBool (* CERTSortCallback)(CERTCertificate *certa,
				    CERTCertificate *certb,
				    void *arg);
SECStatus
CERT_AddCertToListSorted(CERTCertList *certs, CERTCertificate *cert,
			 CERTSortCallback f, void *arg);




PRBool
CERT_SortCBValidity(CERTCertificate *certa,
		    CERTCertificate *certb,
		    void *arg);

SECStatus
CERT_CheckForEvilCert(CERTCertificate *cert);

CERTGeneralName *
CERT_GetCertificateNames(CERTCertificate *cert, PLArenaPool *arena);

CERTGeneralName *
CERT_GetConstrainedCertificateNames(const CERTCertificate *cert,
                                    PLArenaPool *arena,
                                    PRBool includeSubjectCommonName);






CERTCertList *
CERT_CreateSubjectCertList(CERTCertList *certList, CERTCertDBHandle *handle,
			   const SECItem *name, PRTime sorttime,
			   PRBool validOnly);





SECStatus
CERT_FilterCertListByUsage(CERTCertList *certList, SECCertUsage usage,
			   PRBool ca);




SECStatus
CERT_CheckKeyUsage(CERTCertificate *cert, unsigned int requiredUsage);




SECStatus
CERT_KeyUsageAndTypeForCertUsage(SECCertUsage usage,
				 PRBool ca,
				 unsigned int *retKeyUsage,
				 unsigned int *retCertType);



SECStatus
CERT_TrustFlagsForCACertUsage(SECCertUsage usage,
			      unsigned int *retFlags,
			      SECTrustType *retTrustType);











CERTCertList *
CERT_FindUserCertsByUsage(CERTCertDBHandle *handle,
			  SECCertUsage usage,
			  PRBool oneCertPerName,
			  PRBool validOnly,
			  void *proto_win);










CERTCertificate *
CERT_FindUserCertByUsage(CERTCertDBHandle *handle,
			 const char *nickname,
			 SECCertUsage usage,
			 PRBool validOnly,
			 void *proto_win);











SECStatus
CERT_FilterCertListByCANames(CERTCertList *certList, int nCANames,
			     char **caNames, SECCertUsage usage);




SECStatus
CERT_FilterCertListForUserCerts(CERTCertList *certList);










CERTCertNicknames *
CERT_NicknameStringsFromCertList(CERTCertList *certList, char *expiredString,
				 char *notYetGoodString);













char *
CERT_ExtractNicknameString(char *namestring, char *expiredString,
			   char *notYetGoodString);














char *
CERT_GetCertNicknameWithValidity(PLArenaPool *arena, CERTCertificate *cert,
				 char *expiredString, char *notYetGoodString);





char *
CERT_DerNameToAscii(SECItem *dername);











CERTCertificate *
CERT_FindMatchingCert(CERTCertDBHandle *handle, SECItem *derName,
		      CERTCertOwner owner, SECCertUsage usage,
		      PRBool preferTrusted, PRTime validTime, PRBool validOnly);









void
CERT_LockDB(CERTCertDBHandle *handle);




void
CERT_UnlockDB(CERTCertDBHandle *handle);





CERTStatusConfig *
CERT_GetStatusConfig(CERTCertDBHandle *handle);







void
CERT_SetStatusConfig(CERTCertDBHandle *handle, CERTStatusConfig *config);









void
CERT_LockCertRefCount(CERTCertificate *cert);




void
CERT_UnlockCertRefCount(CERTCertificate *cert);







void
CERT_LockCertTrust(const CERTCertificate *cert);




void
CERT_UnlockCertTrust(const CERTCertificate *cert);











 
extern SECItem *
CERT_GetSubjectPublicKeyDigest(PLArenaPool *arena, const CERTCertificate *cert,
                               SECOidTag digestAlg, SECItem *fill);




extern SECItem *
CERT_GetSubjectNameDigest(PLArenaPool *arena, const CERTCertificate *cert,
                          SECOidTag digestAlg, SECItem *fill);

SECStatus CERT_CheckCRL(CERTCertificate* cert, CERTCertificate* issuer,
                        const SECItem* dp, PRTime t, void* wincx);





extern CERTNameConstraint *
CERT_AddNameConstraint(CERTNameConstraint *list, 
		       CERTNameConstraint *constraint);






extern CERTNameConstraint *
CERT_CopyNameConstraint(PLArenaPool         *arena, 
			CERTNameConstraint  *dest, 
			CERTNameConstraint  *src);





extern SECStatus
CERT_CheckNameSpace(PLArenaPool          *arena,
		    const CERTNameConstraints *constraints,
		    const CERTGeneralName *currentName);







extern SECStatus
CERT_FindNameConstraintsExten(PLArenaPool      *arena,
			      CERTCertificate  *cert,
			      CERTNameConstraints **constraints);




extern CERTGeneralName *
CERT_NewGeneralName(PLArenaPool *arena, CERTGeneralNameType type);




extern CERTGeneralNameType
CERT_GetGeneralNameTypeFromString(const char *string);




extern SECStatus
CERT_EncodePolicyConstraintsExtension(PLArenaPool *arena,
                                      CERTCertificatePolicyConstraints *constr,
                                      SECItem *dest);
extern SECStatus
CERT_EncodeInhibitAnyExtension(PLArenaPool *arena,
                               CERTCertificateInhibitAny *inhibitAny,
                               SECItem *dest);
extern SECStatus
CERT_EncodePolicyMappingExtension(PLArenaPool *arena,
                                  CERTCertificatePolicyMappings *maps,
                                  SECItem *dest);

extern SECStatus CERT_EncodeInfoAccessExtension(PLArenaPool *arena,
                                                    CERTAuthInfoAccess **info,
                                                    SECItem *dest);
extern SECStatus
CERT_EncodeUserNotice(PLArenaPool *arena,
                      CERTUserNotice *notice,
                      SECItem *dest);

extern SECStatus
CERT_EncodeDisplayText(PLArenaPool *arena,
                       SECItem *text,
                       SECItem *dest);

extern SECStatus
CERT_EncodeCertPoliciesExtension(PLArenaPool *arena,
                                 CERTPolicyInfo **info,
                                 SECItem *dest);
extern SECStatus
CERT_EncodeNoticeReference(PLArenaPool *arena,
                           CERTNoticeReference *reference,
                           SECItem *dest);




extern const CERTRevocationFlags*
CERT_GetPKIXVerifyNistRevocationPolicy(void);




extern const CERTRevocationFlags*
CERT_GetClassicOCSPEnabledSoftFailurePolicy(void);




extern const CERTRevocationFlags*
CERT_GetClassicOCSPEnabledHardFailurePolicy(void);




extern const CERTRevocationFlags*
CERT_GetClassicOCSPDisabledPolicy(void);








extern SECStatus CERT_PKIXVerifyCert(
	CERTCertificate *cert,
	SECCertificateUsage usages,
	CERTValInParam *paramsIn,
	CERTValOutParam *paramsOut,
	void *wincx);





extern SECStatus CERT_SetUsePKIXForValidation(PRBool enable);



extern PRBool CERT_GetUsePKIXForValidation(void);






extern CERTRevocationFlags *
CERT_AllocCERTRevocationFlags(
    PRUint32 number_leaf_methods, PRUint32 number_leaf_pref_methods,
    PRUint32 number_chain_methods, PRUint32 number_chain_pref_methods);





extern void
CERT_DestroyCERTRevocationFlags(CERTRevocationFlags *flags);

SEC_END_PROTOS

#endif 
