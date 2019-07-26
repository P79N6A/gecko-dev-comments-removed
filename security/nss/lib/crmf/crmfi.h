





#ifndef _CRMFI_H_
#define _CRMFI_H_





#include "secasn1.h"
#include "crmfit.h"
#include "secerr.h"
#include "blapit.h"

#define CRMF_DEFAULT_ARENA_SIZE   1024




























#define MAX_WRAPPED_KEY_LEN       RSA_MAX_MODULUS_BITS

#define CRMF_BITS_TO_BYTES(bits) (((bits)+7)/8)
#define CRMF_BYTES_TO_BITS(bytes) ((bytes)*8)

struct crmfEncoderArg {
    SECItem *buffer;
    long     allocatedLen;
};

struct crmfEncoderOutput {
    CRMFEncoderOutputCallback fn;
    void *outputArg;
};






extern void
       crmf_encoder_out(void *arg, const char *buf, unsigned long len,
                        int depth, SEC_ASN1EncodingPart data_kind);





extern SECStatus 
       crmf_init_encoder_callback_arg (struct crmfEncoderArg *encoderArg,
				       SECItem               *derDest);






extern void
crmf_generic_encoder_callback(void *arg, const char* buf, unsigned long len,
			      int depth, SEC_ASN1EncodingPart data_kind);




extern const SEC_ASN1Template CRMFCertReqMsgTemplate[];
extern const SEC_ASN1Template CRMFRAVerifiedTemplate[];
extern const SEC_ASN1Template CRMFPOPOSigningKeyTemplate[];
extern const SEC_ASN1Template CRMFPOPOKeyEnciphermentTemplate[];
extern const SEC_ASN1Template CRMFPOPOKeyAgreementTemplate[];
extern const SEC_ASN1Template CRMFThisMessageTemplate[];
extern const SEC_ASN1Template CRMFSubsequentMessageTemplate[];
extern const SEC_ASN1Template CRMFDHMACTemplate[];
extern const SEC_ASN1Template CRMFEncryptedKeyWithEncryptedValueTemplate[];
extern const SEC_ASN1Template CRMFEncryptedValueTemplate[];




extern const unsigned char hexTrue;
extern const unsigned char hexFalse;



extern SECStatus crmf_encode_integer(PRArenaPool *poolp, SECItem *dest, 
				     long value);
extern SECStatus crmf_make_bitstring_copy(PRArenaPool *arena, SECItem *dest, 
					  SECItem *src);

extern SECStatus crmf_copy_pkiarchiveoptions(PRArenaPool           *poolp, 
					     CRMFPKIArchiveOptions *destOpt,
					     CRMFPKIArchiveOptions *srcOpt);
extern SECStatus  
       crmf_destroy_pkiarchiveoptions(CRMFPKIArchiveOptions *inArchOptions,
				      PRBool                 freeit);
extern const SEC_ASN1Template*
       crmf_get_pkiarchiveoptions_subtemplate(CRMFControl *inControl);

extern SECStatus crmf_copy_encryptedkey(PRArenaPool       *poolp,
					CRMFEncryptedKey  *srcEncrKey,
					CRMFEncryptedKey  *destEncrKey);
extern SECStatus
crmf_copy_encryptedvalue(PRArenaPool        *poolp,
			 CRMFEncryptedValue *srcValue,
			 CRMFEncryptedValue *destValue);

extern SECStatus
crmf_copy_encryptedvalue_secalg(PRArenaPool     *poolp,
				SECAlgorithmID  *srcAlgId,
				SECAlgorithmID **destAlgId);

extern SECStatus crmf_template_copy_secalg(PRArenaPool *poolp, 
					   SECAlgorithmID **dest,
					   SECAlgorithmID *src);

extern SECStatus crmf_copy_cert_name(PRArenaPool *poolp, CERTName **dest, 
				     CERTName *src);

extern SECStatus crmf_template_add_public_key(PRArenaPool               *poolp,
					      CERTSubjectPublicKeyInfo **dest,
					      CERTSubjectPublicKeyInfo  *pubKey);

extern CRMFCertExtension* crmf_create_cert_extension(PRArenaPool *poolp, 
						     SECOidTag    tag, 
						     PRBool       isCritical,
						     SECItem     *data);
extern CRMFCertRequest*
crmf_copy_cert_request(PRArenaPool *poolp, CRMFCertRequest *srcReq);

extern SECStatus crmf_destroy_encrypted_value(CRMFEncryptedValue *inEncrValue, 
					      PRBool freeit);

extern CRMFEncryptedValue *
crmf_create_encrypted_value_wrapped_privkey(SECKEYPrivateKey   *inPrivKey,
					    SECKEYPublicKey    *inPubKey,
					    CRMFEncryptedValue *destValue);

extern CK_MECHANISM_TYPE 
       crmf_get_mechanism_from_public_key(SECKEYPublicKey *inPubKey);

extern SECStatus
crmf_encrypted_value_unwrap_priv_key(PRArenaPool        *poolp,
				     CRMFEncryptedValue *encValue,
				     SECKEYPrivateKey   *privKey,
				     SECKEYPublicKey    *newPubKey,
				     SECItem            *nickname,
				     PK11SlotInfo       *slot,
				     unsigned char       keyUsage,
				     SECKEYPrivateKey  **unWrappedKey,
				     void               *wincx);

extern SECItem*
crmf_get_public_value(SECKEYPublicKey *pubKey, SECItem *dest);

extern CRMFCertExtension*
crmf_copy_cert_extension(PRArenaPool *poolp, CRMFCertExtension *inExtension);

extern SECStatus
crmf_create_prtime(SECItem *src, PRTime **dest);
#endif 
