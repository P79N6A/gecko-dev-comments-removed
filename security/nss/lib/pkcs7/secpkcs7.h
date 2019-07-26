









#ifndef _SECPKCS7_H_
#define _SECPKCS7_H_

#include "seccomon.h"

#include "secoidt.h"
#include "certt.h"
#include "keyt.h"
#include "hasht.h"
#include "pkcs7t.h"

extern const SEC_ASN1Template sec_PKCS7ContentInfoTemplate[];


SEC_BEGIN_PROTOS








extern SECOidTag SEC_PKCS7ContentType (SEC_PKCS7ContentInfo *cinfo);




extern void SEC_PKCS7DestroyContentInfo(SEC_PKCS7ContentInfo *contentInfo);




extern SEC_PKCS7ContentInfo *
SEC_PKCS7CopyContentInfo(SEC_PKCS7ContentInfo *contentInfo);





extern SECItem *SEC_PKCS7GetContent(SEC_PKCS7ContentInfo *cinfo);





extern SEC_PKCS7DecoderContext *
SEC_PKCS7DecoderStart(SEC_PKCS7DecoderContentCallback callback,
		      void *callback_arg,
		      SECKEYGetPasswordKey pwfn, void *pwfn_arg,
		      SEC_PKCS7GetDecryptKeyCallback decrypt_key_cb, 
		      void *decrypt_key_cb_arg,
		      SEC_PKCS7DecryptionAllowedCallback decrypt_allowed_cb);

extern SECStatus
SEC_PKCS7DecoderUpdate(SEC_PKCS7DecoderContext *p7dcx,
		       const char *buf, unsigned long len);

extern SEC_PKCS7ContentInfo *
SEC_PKCS7DecoderFinish(SEC_PKCS7DecoderContext *p7dcx);



void SEC_PKCS7DecoderAbort(SEC_PKCS7DecoderContext *p7dcx, int error);

extern SEC_PKCS7ContentInfo *
SEC_PKCS7DecodeItem(SECItem *p7item,
		    SEC_PKCS7DecoderContentCallback cb, void *cb_arg,
		    SECKEYGetPasswordKey pwfn, void *pwfn_arg,
		    SEC_PKCS7GetDecryptKeyCallback decrypt_key_cb, 
		    void *decrypt_key_cb_arg,
		    SEC_PKCS7DecryptionAllowedCallback decrypt_allowed_cb);

extern PRBool SEC_PKCS7ContainsCertsOrCrls(SEC_PKCS7ContentInfo *cinfo);







extern PRBool 
SEC_PKCS7IsContentEmpty(SEC_PKCS7ContentInfo *cinfo, unsigned int minLen); 

extern PRBool SEC_PKCS7ContentIsEncrypted(SEC_PKCS7ContentInfo *cinfo);









extern PRBool SEC_PKCS7ContentIsSigned(SEC_PKCS7ContentInfo *cinfo);










extern PRBool SEC_PKCS7VerifySignature(SEC_PKCS7ContentInfo *cinfo,
				       SECCertUsage certusage,
				       PRBool keepcerts);











extern PRBool SEC_PKCS7VerifyDetachedSignature(SEC_PKCS7ContentInfo *cinfo,
					       SECCertUsage certusage,
					       const SECItem *detached_digest,
					       HASH_HashType digest_type,
					       PRBool keepcerts);









extern char *SEC_PKCS7GetSignerCommonName(SEC_PKCS7ContentInfo *cinfo);
extern char *SEC_PKCS7GetSignerEmailAddress(SEC_PKCS7ContentInfo *cinfo);




extern SECItem *SEC_PKCS7GetSigningTime(SEC_PKCS7ContentInfo *cinfo);


































extern SEC_PKCS7ContentInfo *
SEC_PKCS7CreateSignedData (CERTCertificate *cert,
			   SECCertUsage certusage,
			   CERTCertDBHandle *certdb,
			   SECOidTag digestalg,
			   SECItem *digest,
		           SECKEYGetPasswordKey pwfn, void *pwfn_arg);


















extern SEC_PKCS7ContentInfo *
SEC_PKCS7CreateCertsOnly (CERTCertificate *cert,
			  PRBool include_chain,
			  CERTCertDBHandle *certdb);



























extern SEC_PKCS7ContentInfo *
SEC_PKCS7CreateEnvelopedData (CERTCertificate *cert,
			      SECCertUsage certusage,
			      CERTCertDBHandle *certdb,
			      SECOidTag encalg,
			      int keysize,
		              SECKEYGetPasswordKey pwfn, void *pwfn_arg);













extern SEC_PKCS7ContentInfo *SEC_PKCS7CreateData (void);









extern SEC_PKCS7ContentInfo *
SEC_PKCS7CreateEncryptedData (SECOidTag algorithm, int keysize,
			      SECKEYGetPasswordKey pwfn, void *pwfn_arg);

























extern SECStatus SEC_PKCS7AddSignedAttribute (SEC_PKCS7ContentInfo *cinfo,
					      SECOidTag oidtag,
					      SECItem *value);










extern SECStatus SEC_PKCS7AddCertChain (SEC_PKCS7ContentInfo *cinfo,
					CERTCertificate *cert,
					CERTCertDBHandle *certdb);







extern SECStatus SEC_PKCS7AddCertificate (SEC_PKCS7ContentInfo *cinfo,
					  CERTCertificate *cert);


















extern SECStatus SEC_PKCS7AddRecipient (SEC_PKCS7ContentInfo *cinfo,
					CERTCertificate *cert,
					SECCertUsage certusage,
					CERTCertDBHandle *certdb);

















extern SECStatus SEC_PKCS7AddSigningTime (SEC_PKCS7ContentInfo *cinfo);











extern SECStatus SEC_PKCS7AddSymmetricCapabilities(SEC_PKCS7ContentInfo *cinfo);












extern SECStatus SEC_PKCS7IncludeCertChain (SEC_PKCS7ContentInfo *cinfo,
					    CERTCertDBHandle *certdb);










extern SECStatus SEC_PKCS7SetContent (SEC_PKCS7ContentInfo *cinfo,
				      const char *buf, unsigned long len);
























extern SECStatus SEC_PKCS7Encode (SEC_PKCS7ContentInfo *cinfo,
				  SEC_PKCS7EncoderOutputCallback outputfn,
				  void *outputarg,
				  PK11SymKey *bulkkey,
				  SECKEYGetPasswordKey pwfn,
				  void *pwfnarg);




























extern SECItem *SEC_PKCS7EncodeItem (PLArenaPool *pool,
				     SECItem *dest,
				     SEC_PKCS7ContentInfo *cinfo,
				     PK11SymKey *bulkkey,
				     SECKEYGetPasswordKey pwfn,
				     void *pwfnarg);







extern SECStatus SEC_PKCS7PrepareForEncode (SEC_PKCS7ContentInfo *cinfo,
					    PK11SymKey *bulkkey,
					    SECKEYGetPasswordKey pwfn,
					    void *pwfnarg);





















extern SEC_PKCS7EncoderContext *
SEC_PKCS7EncoderStart (SEC_PKCS7ContentInfo *cinfo,
		       SEC_PKCS7EncoderOutputCallback outputfn,
		       void *outputarg,
		       PK11SymKey *bulkkey);




extern SECStatus SEC_PKCS7EncoderUpdate (SEC_PKCS7EncoderContext *p7ecx,
					 const char *buf,
					 unsigned long len);











extern SECStatus SEC_PKCS7EncoderFinish (SEC_PKCS7EncoderContext *p7ecx,
					 SECKEYGetPasswordKey pwfn,
					 void *pwfnarg);


void SEC_PKCS7EncoderAbort(SEC_PKCS7EncoderContext *p7dcx, int error);





 
extern SECAlgorithmID *
SEC_PKCS7GetEncryptionAlgorithm(SEC_PKCS7ContentInfo *cinfo); 















extern SECStatus 
SEC_PKCS7EncryptContents(PLArenaPool *poolp,
			 SEC_PKCS7ContentInfo *cinfo, 
			 SECItem *key,
			 void *wincx); 
	














extern SECStatus 
SEC_PKCS7DecryptContents(PLArenaPool *poolp,
			 SEC_PKCS7ContentInfo *cinfo, 
			 SECItem *key,
			 void *wincx); 






extern SECItem **
SEC_PKCS7GetCertificateList(SEC_PKCS7ContentInfo *cinfo);




extern int 
SEC_PKCS7GetKeyLength(SEC_PKCS7ContentInfo *cinfo);
 


SEC_END_PROTOS

#endif 
