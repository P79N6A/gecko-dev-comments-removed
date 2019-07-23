









































#ifndef _CMS_H_
#define _CMS_H_

#include "seccomon.h"

#include "secoidt.h"
#include "certt.h"
#include "keyt.h"
#include "hasht.h"
#include "cmst.h"


SEC_BEGIN_PROTOS














extern NSSCMSDecoderContext *
NSS_CMSDecoder_Start(PLArenaPool *poolp,
		      NSSCMSContentCallback cb, void *cb_arg,
		      PK11PasswordFunc pwfn, void *pwfn_arg,
		      NSSCMSGetDecryptKeyCallback decrypt_key_cb, void *decrypt_key_cb_arg);




extern SECStatus
NSS_CMSDecoder_Update(NSSCMSDecoderContext *p7dcx, const char *buf, unsigned long len);




extern void
NSS_CMSDecoder_Cancel(NSSCMSDecoderContext *p7dcx);




extern NSSCMSMessage *
NSS_CMSDecoder_Finish(NSSCMSDecoderContext *p7dcx);




extern NSSCMSMessage *
NSS_CMSMessage_CreateFromDER(SECItem *DERmessage,
		    NSSCMSContentCallback cb, void *cb_arg,
		    PK11PasswordFunc pwfn, void *pwfn_arg,
		    NSSCMSGetDecryptKeyCallback decrypt_key_cb, void *decrypt_key_cb_arg);

















extern NSSCMSEncoderContext *
NSS_CMSEncoder_Start(NSSCMSMessage *cmsg,
			NSSCMSContentCallback outputfn, void *outputarg,
			SECItem *dest, PLArenaPool *destpoolp,
			PK11PasswordFunc pwfn, void *pwfn_arg,
			NSSCMSGetDecryptKeyCallback decrypt_key_cb, void *decrypt_key_cb_arg,
			SECAlgorithmID **detached_digestalgs, SECItem **detached_digests);








extern SECStatus
NSS_CMSEncoder_Update(NSSCMSEncoderContext *p7ecx, const char *data, unsigned long len);




extern SECStatus
NSS_CMSEncoder_Cancel(NSSCMSEncoderContext *p7ecx);






extern SECStatus
NSS_CMSEncoder_Finish(NSSCMSEncoderContext *p7ecx);










extern NSSCMSMessage *
NSS_CMSMessage_Create(PLArenaPool *poolp);











extern void
NSS_CMSMessage_SetEncodingParams(NSSCMSMessage *cmsg,
			PK11PasswordFunc pwfn, void *pwfn_arg,
			NSSCMSGetDecryptKeyCallback decrypt_key_cb, void *decrypt_key_cb_arg,
			SECAlgorithmID **detached_digestalgs, SECItem **detached_digests);




extern void
NSS_CMSMessage_Destroy(NSSCMSMessage *cmsg);







extern NSSCMSMessage *
NSS_CMSMessage_Copy(NSSCMSMessage *cmsg);




extern PLArenaPool *
NSS_CMSMessage_GetArena(NSSCMSMessage *cmsg);




extern NSSCMSContentInfo *
NSS_CMSMessage_GetContentInfo(NSSCMSMessage *cmsg);






extern SECItem *
NSS_CMSMessage_GetContent(NSSCMSMessage *cmsg);






extern int
NSS_CMSMessage_ContentLevelCount(NSSCMSMessage *cmsg);






extern NSSCMSContentInfo *
NSS_CMSMessage_ContentLevel(NSSCMSMessage *cmsg, int n);




extern PRBool
NSS_CMSMessage_ContainsCertsOrCrls(NSSCMSMessage *cmsg);




extern PRBool
NSS_CMSMessage_IsEncrypted(NSSCMSMessage *cmsg);











extern PRBool
NSS_CMSMessage_IsSigned(NSSCMSMessage *cmsg);







extern PRBool
NSS_CMSMessage_IsContentEmpty(NSSCMSMessage *cmsg, unsigned int minLen);








extern void
NSS_CMSContentInfo_Destroy(NSSCMSContentInfo *cinfo);




extern NSSCMSContentInfo *
NSS_CMSContentInfo_GetChildContentInfo(NSSCMSContentInfo *cinfo);




extern SECStatus
NSS_CMSContentInfo_SetContent(NSSCMSMessage *cmsg, NSSCMSContentInfo *cinfo, SECOidTag type, void *ptr);





extern SECStatus
NSS_CMSContentInfo_SetContent_Data(NSSCMSMessage *cmsg, NSSCMSContentInfo *cinfo, SECItem *data, PRBool detached);

extern SECStatus
NSS_CMSContentInfo_SetContent_SignedData(NSSCMSMessage *cmsg, NSSCMSContentInfo *cinfo, NSSCMSSignedData *sigd);

extern SECStatus
NSS_CMSContentInfo_SetContent_EnvelopedData(NSSCMSMessage *cmsg, NSSCMSContentInfo *cinfo, NSSCMSEnvelopedData *envd);

extern SECStatus
NSS_CMSContentInfo_SetContent_DigestedData(NSSCMSMessage *cmsg, NSSCMSContentInfo *cinfo, NSSCMSDigestedData *digd);

extern SECStatus
NSS_CMSContentInfo_SetContent_EncryptedData(NSSCMSMessage *cmsg, NSSCMSContentInfo *cinfo, NSSCMSEncryptedData *encd);






extern void *
NSS_CMSContentInfo_GetContent(NSSCMSContentInfo *cinfo);






extern SECItem *
NSS_CMSContentInfo_GetInnerContent(NSSCMSContentInfo *cinfo);





extern SECOidTag
NSS_CMSContentInfo_GetContentTypeTag(NSSCMSContentInfo *cinfo);

extern SECItem *
NSS_CMSContentInfo_GetContentTypeOID(NSSCMSContentInfo *cinfo);





extern SECOidTag
NSS_CMSContentInfo_GetContentEncAlgTag(NSSCMSContentInfo *cinfo);




extern SECAlgorithmID *
NSS_CMSContentInfo_GetContentEncAlg(NSSCMSContentInfo *cinfo);

extern SECStatus
NSS_CMSContentInfo_SetContentEncAlg(PLArenaPool *poolp, NSSCMSContentInfo *cinfo,
				    SECOidTag bulkalgtag, SECItem *parameters, int keysize);

extern SECStatus
NSS_CMSContentInfo_SetContentEncAlgID(PLArenaPool *poolp, NSSCMSContentInfo *cinfo,
				    SECAlgorithmID *algid, int keysize);

extern void
NSS_CMSContentInfo_SetBulkKey(NSSCMSContentInfo *cinfo, PK11SymKey *bulkkey);

extern PK11SymKey *
NSS_CMSContentInfo_GetBulkKey(NSSCMSContentInfo *cinfo);

extern int
NSS_CMSContentInfo_GetBulkKeySize(NSSCMSContentInfo *cinfo);












extern SECStatus
NSS_CMSArray_SortByDER(void **objs, const SEC_ASN1Template *objtemplate, void **objs2);





extern int
NSS_CMSUtil_DERCompare(void *a, void *b);












extern int
NSS_CMSAlgArray_GetIndexByAlgID(SECAlgorithmID **algorithmArray, SECAlgorithmID *algid);












extern int
NSS_CMSAlgArray_GetIndexByAlgTag(SECAlgorithmID **algorithmArray, SECOidTag algtag);

extern const SECHashObject *
NSS_CMSUtil_GetHashObjByAlgID(SECAlgorithmID *algid);

extern const SEC_ASN1Template *
NSS_CMSUtil_GetTemplateByTypeTag(SECOidTag type);

extern size_t
NSS_CMSUtil_GetSizeByTypeTag(SECOidTag type);

extern NSSCMSContentInfo *
NSS_CMSContent_GetContentInfo(void *msg, SECOidTag type);

extern const char *
NSS_CMSUtil_VerificationStatusToString(NSSCMSVerificationStatus vs);





extern NSSCMSSignedData *
NSS_CMSSignedData_Create(NSSCMSMessage *cmsg);

extern void
NSS_CMSSignedData_Destroy(NSSCMSSignedData *sigd);












extern SECStatus
NSS_CMSSignedData_Encode_BeforeStart(NSSCMSSignedData *sigd);

extern SECStatus
NSS_CMSSignedData_Encode_BeforeData(NSSCMSSignedData *sigd);











extern SECStatus
NSS_CMSSignedData_Encode_AfterData(NSSCMSSignedData *sigd);

extern SECStatus
NSS_CMSSignedData_Decode_BeforeData(NSSCMSSignedData *sigd);





extern SECStatus
NSS_CMSSignedData_Decode_AfterData(NSSCMSSignedData *sigd);





extern SECStatus
NSS_CMSSignedData_Decode_AfterEnd(NSSCMSSignedData *sigd);




extern NSSCMSSignerInfo **
NSS_CMSSignedData_GetSignerInfos(NSSCMSSignedData *sigd);

extern int
NSS_CMSSignedData_SignerInfoCount(NSSCMSSignedData *sigd);

extern NSSCMSSignerInfo *
NSS_CMSSignedData_GetSignerInfo(NSSCMSSignedData *sigd, int i);




extern SECAlgorithmID **
NSS_CMSSignedData_GetDigestAlgs(NSSCMSSignedData *sigd);




extern NSSCMSContentInfo *
NSS_CMSSignedData_GetContentInfo(NSSCMSSignedData *sigd);




extern SECItem **
NSS_CMSSignedData_GetCertificateList(NSSCMSSignedData *sigd);

extern SECStatus
NSS_CMSSignedData_ImportCerts(NSSCMSSignedData *sigd, CERTCertDBHandle *certdb,
				SECCertUsage certusage, PRBool keepcerts);




extern PRBool
NSS_CMSSignedData_HasDigests(NSSCMSSignedData *sigd);










extern SECStatus
NSS_CMSSignedData_VerifySignerInfo(NSSCMSSignedData *sigd, int i, CERTCertDBHandle *certdb,
				    SECCertUsage certusage);




extern SECStatus
NSS_CMSSignedData_VerifyCertsOnly(NSSCMSSignedData *sigd, 
                                  CERTCertDBHandle *certdb, 
                                  SECCertUsage usage);

extern SECStatus
NSS_CMSSignedData_AddCertList(NSSCMSSignedData *sigd, CERTCertificateList *certlist);




extern SECStatus
NSS_CMSSignedData_AddCertChain(NSSCMSSignedData *sigd, CERTCertificate *cert);

extern SECStatus
NSS_CMSSignedData_AddCertificate(NSSCMSSignedData *sigd, CERTCertificate *cert);

extern PRBool
NSS_CMSSignedData_ContainsCertsOrCrls(NSSCMSSignedData *sigd);

extern SECStatus
NSS_CMSSignedData_AddSignerInfo(NSSCMSSignedData *sigd,
				NSSCMSSignerInfo *signerinfo);

extern SECStatus
NSS_CMSSignedData_SetDigests(NSSCMSSignedData *sigd,
				SECAlgorithmID **digestalgs,
				SECItem **digests);

extern SECStatus
NSS_CMSSignedData_SetDigestValue(NSSCMSSignedData *sigd,
				SECOidTag digestalgtag,
				SECItem *digestdata);

extern SECStatus
NSS_CMSSignedData_AddDigest(PLArenaPool *poolp,
				NSSCMSSignedData *sigd,
				SECOidTag digestalgtag,
				SECItem *digest);

extern SECItem *
NSS_CMSSignedData_GetDigestValue(NSSCMSSignedData *sigd, SECOidTag digestalgtag);











extern NSSCMSSignedData *
NSS_CMSSignedData_CreateCertsOnly(NSSCMSMessage *cmsg, CERTCertificate *cert, PRBool include_chain);





extern NSSCMSSignerInfo *
NSS_CMSSignerInfo_Create(NSSCMSMessage *cmsg, CERTCertificate *cert, SECOidTag digestalgtag);
extern NSSCMSSignerInfo *
NSS_CMSSignerInfo_CreateWithSubjKeyID(NSSCMSMessage *cmsg, SECItem *subjKeyID, SECKEYPublicKey *pubKey, SECKEYPrivateKey *signingKey, SECOidTag digestalgtag);




extern void
NSS_CMSSignerInfo_Destroy(NSSCMSSignerInfo *si);





extern SECStatus
NSS_CMSSignerInfo_Sign(NSSCMSSignerInfo *signerinfo, SECItem *digest, SECItem *contentType);

extern SECStatus
NSS_CMSSignerInfo_VerifyCertificate(NSSCMSSignerInfo *signerinfo, CERTCertDBHandle *certdb,
			    SECCertUsage certusage);







extern SECStatus
NSS_CMSSignerInfo_Verify(NSSCMSSignerInfo *signerinfo, SECItem *digest, SECItem *contentType);

extern NSSCMSVerificationStatus
NSS_CMSSignerInfo_GetVerificationStatus(NSSCMSSignerInfo *signerinfo);

extern SECOidData *
NSS_CMSSignerInfo_GetDigestAlg(NSSCMSSignerInfo *signerinfo);

extern SECOidTag
NSS_CMSSignerInfo_GetDigestAlgTag(NSSCMSSignerInfo *signerinfo);

extern int
NSS_CMSSignerInfo_GetVersion(NSSCMSSignerInfo *signerinfo);

extern CERTCertificateList *
NSS_CMSSignerInfo_GetCertList(NSSCMSSignerInfo *signerinfo);










extern SECStatus
NSS_CMSSignerInfo_GetSigningTime(NSSCMSSignerInfo *sinfo, PRTime *stime);






extern CERTCertificate *
NSS_CMSSignerInfo_GetSigningCertificate(NSSCMSSignerInfo *signerinfo, CERTCertDBHandle *certdb);









extern char *
NSS_CMSSignerInfo_GetSignerCommonName(NSSCMSSignerInfo *sinfo);









extern char *
NSS_CMSSignerInfo_GetSignerEmailAddress(NSSCMSSignerInfo *sinfo);





extern SECStatus
NSS_CMSSignerInfo_AddAuthAttr(NSSCMSSignerInfo *signerinfo, NSSCMSAttribute *attr);





extern SECStatus
NSS_CMSSignerInfo_AddUnauthAttr(NSSCMSSignerInfo *signerinfo, NSSCMSAttribute *attr);














extern SECStatus
NSS_CMSSignerInfo_AddSigningTime(NSSCMSSignerInfo *signerinfo, PRTime t);








extern SECStatus
NSS_CMSSignerInfo_AddSMIMECaps(NSSCMSSignerInfo *signerinfo);







SECStatus
NSS_CMSSignerInfo_AddSMIMEEncKeyPrefs(NSSCMSSignerInfo *signerinfo, CERTCertificate *cert, CERTCertDBHandle *certdb);








SECStatus
NSS_CMSSignerInfo_AddMSSMIMEEncKeyPrefs(NSSCMSSignerInfo *signerinfo, CERTCertificate *cert, CERTCertDBHandle *certdb);




extern SECStatus
NSS_CMSSignerInfo_AddCounterSignature(NSSCMSSignerInfo *signerinfo,
				    SECOidTag digestalg, CERTCertificate signingcert);





extern SECStatus
NSS_SMIMESignerInfo_SaveSMIMEProfile(NSSCMSSignerInfo *signerinfo);




extern SECStatus
NSS_CMSSignerInfo_IncludeCerts(NSSCMSSignerInfo *signerinfo, NSSCMSCertChainMode cm, SECCertUsage usage);








extern NSSCMSEnvelopedData *
NSS_CMSEnvelopedData_Create(NSSCMSMessage *cmsg, SECOidTag algorithm, int keysize);




extern void
NSS_CMSEnvelopedData_Destroy(NSSCMSEnvelopedData *edp);




extern NSSCMSContentInfo *
NSS_CMSEnvelopedData_GetContentInfo(NSSCMSEnvelopedData *envd);






extern SECStatus
NSS_CMSEnvelopedData_AddRecipient(NSSCMSEnvelopedData *edp, NSSCMSRecipientInfo *rip);













extern SECStatus
NSS_CMSEnvelopedData_Encode_BeforeStart(NSSCMSEnvelopedData *envd);




extern SECStatus
NSS_CMSEnvelopedData_Encode_BeforeData(NSSCMSEnvelopedData *envd);




extern SECStatus
NSS_CMSEnvelopedData_Encode_AfterData(NSSCMSEnvelopedData *envd);





extern SECStatus
NSS_CMSEnvelopedData_Decode_BeforeData(NSSCMSEnvelopedData *envd);




extern SECStatus
NSS_CMSEnvelopedData_Decode_AfterData(NSSCMSEnvelopedData *envd);




extern SECStatus
NSS_CMSEnvelopedData_Decode_AfterEnd(NSSCMSEnvelopedData *envd);












extern NSSCMSRecipientInfo *
NSS_CMSRecipientInfo_Create(NSSCMSMessage *cmsg, CERTCertificate *cert);

extern NSSCMSRecipientInfo *
NSS_CMSRecipientInfo_CreateWithSubjKeyID(NSSCMSMessage   *cmsg, 
                                         SECItem         *subjKeyID,
                                         SECKEYPublicKey *pubKey);

extern NSSCMSRecipientInfo *
NSS_CMSRecipientInfo_CreateWithSubjKeyIDFromCert(NSSCMSMessage *cmsg, 
                                                 CERTCertificate *cert);






extern NSSCMSRecipientInfo *
NSS_CMSRecipientInfo_CreateNew(void* pwfn_arg);






extern NSSCMSRecipientInfo *
NSS_CMSRecipientInfo_CreateFromDER(SECItem* input, void* pwfn_arg);

extern void
NSS_CMSRecipientInfo_Destroy(NSSCMSRecipientInfo *ri);








SECStatus NSS_CMSRecipientInfo_GetCertAndKey(NSSCMSRecipientInfo *ri,
   CERTCertificate** retcert, SECKEYPrivateKey** retkey);

extern int
NSS_CMSRecipientInfo_GetVersion(NSSCMSRecipientInfo *ri);

extern SECItem *
NSS_CMSRecipientInfo_GetEncryptedKey(NSSCMSRecipientInfo *ri, int subIndex);




SECStatus NSS_CMSRecipientInfo_Encode(PLArenaPool* poolp,
                                      const NSSCMSRecipientInfo *src,
                                      SECItem* returned);

extern SECOidTag
NSS_CMSRecipientInfo_GetKeyEncryptionAlgorithmTag(NSSCMSRecipientInfo *ri);

extern SECStatus
NSS_CMSRecipientInfo_WrapBulkKey(NSSCMSRecipientInfo *ri, PK11SymKey *bulkkey, SECOidTag bulkalgtag);

extern PK11SymKey *
NSS_CMSRecipientInfo_UnwrapBulkKey(NSSCMSRecipientInfo *ri, int subIndex,
		CERTCertificate *cert, SECKEYPrivateKey *privkey, SECOidTag bulkalgtag);













extern NSSCMSEncryptedData *
NSS_CMSEncryptedData_Create(NSSCMSMessage *cmsg, SECOidTag algorithm, int keysize);




extern void
NSS_CMSEncryptedData_Destroy(NSSCMSEncryptedData *encd);




extern NSSCMSContentInfo *
NSS_CMSEncryptedData_GetContentInfo(NSSCMSEncryptedData *encd);









extern SECStatus
NSS_CMSEncryptedData_Encode_BeforeStart(NSSCMSEncryptedData *encd);




extern SECStatus
NSS_CMSEncryptedData_Encode_BeforeData(NSSCMSEncryptedData *encd);




extern SECStatus
NSS_CMSEncryptedData_Encode_AfterData(NSSCMSEncryptedData *encd);




extern SECStatus
NSS_CMSEncryptedData_Decode_BeforeData(NSSCMSEncryptedData *encd);




extern SECStatus
NSS_CMSEncryptedData_Decode_AfterData(NSSCMSEncryptedData *encd);




extern SECStatus
NSS_CMSEncryptedData_Decode_AfterEnd(NSSCMSEncryptedData *encd);












extern NSSCMSDigestedData *
NSS_CMSDigestedData_Create(NSSCMSMessage *cmsg, SECAlgorithmID *digestalg);




extern void
NSS_CMSDigestedData_Destroy(NSSCMSDigestedData *digd);




extern NSSCMSContentInfo *
NSS_CMSDigestedData_GetContentInfo(NSSCMSDigestedData *digd);








extern SECStatus
NSS_CMSDigestedData_Encode_BeforeStart(NSSCMSDigestedData *digd);








extern SECStatus
NSS_CMSDigestedData_Encode_BeforeData(NSSCMSDigestedData *digd);








extern SECStatus
NSS_CMSDigestedData_Encode_AfterData(NSSCMSDigestedData *digd);








extern SECStatus
NSS_CMSDigestedData_Decode_BeforeData(NSSCMSDigestedData *digd);








extern SECStatus
NSS_CMSDigestedData_Decode_AfterData(NSSCMSDigestedData *digd);







extern SECStatus
NSS_CMSDigestedData_Decode_AfterEnd(NSSCMSDigestedData *digd);









extern NSSCMSDigestContext *
NSS_CMSDigestContext_StartMultiple(SECAlgorithmID **digestalgs);





extern NSSCMSDigestContext *
NSS_CMSDigestContext_StartSingle(SECAlgorithmID *digestalg);




extern void
NSS_CMSDigestContext_Update(NSSCMSDigestContext *cmsdigcx, const unsigned char *data, int len);




extern void
NSS_CMSDigestContext_Cancel(NSSCMSDigestContext *cmsdigcx);





extern SECStatus
NSS_CMSDigestContext_FinishMultiple(NSSCMSDigestContext *cmsdigcx, PLArenaPool *poolp,
			    SECItem ***digestsp);





extern SECStatus
NSS_CMSDigestContext_FinishSingle(NSSCMSDigestContext *cmsdigcx, PLArenaPool *poolp,
			    SECItem *digest);












extern SECStatus
NSS_CMSDEREncode(NSSCMSMessage *cmsg, SECItem *input, SECItem *derOut, 
                 PLArenaPool *arena);



SEC_END_PROTOS

#endif 
