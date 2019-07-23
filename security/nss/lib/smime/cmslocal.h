














































#ifndef _CMSLOCAL_H_
#define _CMSLOCAL_H_

#include "cms.h"
#include "cmsreclist.h"
#include "secasn1t.h"

extern const SEC_ASN1Template NSSCMSContentInfoTemplate[];


SEC_BEGIN_PROTOS









extern NSSCMSCipherContext *
NSS_CMSCipherContext_StartDecrypt(PK11SymKey *key, SECAlgorithmID *algid);






extern NSSCMSCipherContext *
NSS_CMSCipherContext_StartEncrypt(PRArenaPool *poolp, PK11SymKey *key, SECAlgorithmID *algid);

extern void
NSS_CMSCipherContext_Destroy(NSSCMSCipherContext *cc);














extern unsigned int
NSS_CMSCipherContext_DecryptLength(NSSCMSCipherContext *cc, unsigned int input_len, PRBool final);










extern unsigned int
NSS_CMSCipherContext_EncryptLength(NSSCMSCipherContext *cc, unsigned int input_len, PRBool final);

















 
extern SECStatus
NSS_CMSCipherContext_Decrypt(NSSCMSCipherContext *cc, unsigned char *output,
		  unsigned int *output_len_p, unsigned int max_output_len,
		  const unsigned char *input, unsigned int input_len,
		  PRBool final);

















 
extern SECStatus
NSS_CMSCipherContext_Encrypt(NSSCMSCipherContext *cc, unsigned char *output,
		  unsigned int *output_len_p, unsigned int max_output_len,
		  const unsigned char *input, unsigned int input_len,
		  PRBool final);











extern SECStatus
NSS_CMSUtil_EncryptSymKey_RSA(PLArenaPool *poolp, CERTCertificate *cert,
                              PK11SymKey *key,
                              SECItem *encKey);

extern SECStatus
NSS_CMSUtil_EncryptSymKey_RSAPubKey(PLArenaPool *poolp,
                                    SECKEYPublicKey *publickey,
                                    PK11SymKey *bulkkey, SECItem *encKey);








extern PK11SymKey *
NSS_CMSUtil_DecryptSymKey_RSA(SECKEYPrivateKey *privkey, SECItem *encKey, SECOidTag bulkalgtag);

extern SECStatus
NSS_CMSUtil_EncryptSymKey_MISSI(PLArenaPool *poolp, CERTCertificate *cert, PK11SymKey *key,
			SECOidTag symalgtag, SECItem *encKey, SECItem **pparams, void *pwfn_arg);

extern PK11SymKey *
NSS_CMSUtil_DecryptSymKey_MISSI(SECKEYPrivateKey *privkey, SECItem *encKey,
			SECAlgorithmID *keyEncAlg, SECOidTag bulkalgtag, void *pwfn_arg);

extern SECStatus
NSS_CMSUtil_EncryptSymKey_ESDH(PLArenaPool *poolp, CERTCertificate *cert, PK11SymKey *key,
			SECItem *encKey, SECItem **ukm, SECAlgorithmID *keyEncAlg,
			SECItem *originatorPubKey);

extern PK11SymKey *
NSS_CMSUtil_DecryptSymKey_ESDH(SECKEYPrivateKey *privkey, SECItem *encKey,
			SECAlgorithmID *keyEncAlg, SECOidTag bulkalgtag, void *pwfn_arg);




extern NSSCMSRecipient **nss_cms_recipient_list_create(NSSCMSRecipientInfo **recipientinfos);
extern void nss_cms_recipient_list_destroy(NSSCMSRecipient **recipient_list);
extern NSSCMSRecipientEncryptedKey *NSS_CMSRecipientEncryptedKey_Create(PLArenaPool *poolp);







extern void **
NSS_CMSArray_Alloc(PRArenaPool *poolp, int n);




extern SECStatus
NSS_CMSArray_Add(PRArenaPool *poolp, void ***array, void *obj);




extern PRBool
NSS_CMSArray_IsEmpty(void **array);




extern int
NSS_CMSArray_Count(void **array);











extern void
NSS_CMSArray_Sort(void **primary, int (*compare)(void *,void *), void **secondary, void **tertiary);










extern NSSCMSAttribute *
NSS_CMSAttribute_Create(PRArenaPool *poolp, SECOidTag oidtag, SECItem *value, PRBool encoded);




extern SECStatus
NSS_CMSAttribute_AddValue(PLArenaPool *poolp, NSSCMSAttribute *attr, SECItem *value);




extern SECOidTag
NSS_CMSAttribute_GetType(NSSCMSAttribute *attr);








extern SECItem *
NSS_CMSAttribute_GetValue(NSSCMSAttribute *attr);




extern PRBool
NSS_CMSAttribute_CompareValue(NSSCMSAttribute *attr, SECItem *av);










extern SECItem *
NSS_CMSAttributeArray_Encode(PRArenaPool *poolp, NSSCMSAttribute ***attrs, SECItem *dest);








extern SECStatus
NSS_CMSAttributeArray_Reorder(NSSCMSAttribute **attrs);









extern NSSCMSAttribute *
NSS_CMSAttributeArray_FindAttrByOidTag(NSSCMSAttribute **attrs, SECOidTag oidtag, PRBool only);





extern SECStatus
NSS_CMSAttributeArray_AddAttr(PLArenaPool *poolp, NSSCMSAttribute ***attrs, NSSCMSAttribute *attr);




extern SECStatus
NSS_CMSAttributeArray_SetAttr(PLArenaPool *poolp, NSSCMSAttribute ***attrs, SECOidTag type, SECItem *value, PRBool encoded);





extern SECStatus
NSS_CMSSignedData_AddTempCertificate(NSSCMSSignedData *sigd, CERTCertificate *cert);


SEC_END_PROTOS

#endif 
