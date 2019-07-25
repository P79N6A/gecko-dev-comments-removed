

















































#ifndef _P7LOCAL_H_
#define _P7LOCAL_H_

#include "secpkcs7.h"
#include "secasn1t.h"

extern const SEC_ASN1Template sec_PKCS7ContentInfoTemplate[];


typedef struct sec_pkcs7_cipher_object sec_PKCS7CipherObject;



SEC_BEGIN_PROTOS








extern SEC_PKCS7Attribute *sec_PKCS7FindAttribute (SEC_PKCS7Attribute **attrs,
						   SECOidTag oidtag,
						   PRBool only);





extern SECItem *sec_PKCS7AttributeValue (SEC_PKCS7Attribute *attr);




extern SECItem *sec_PKCS7EncodeAttributes (PRArenaPool *poolp,
					   SECItem *dest, void *src);






extern SECStatus sec_PKCS7ReorderAttributes (SEC_PKCS7Attribute **attrs);





extern sec_PKCS7CipherObject *
sec_PKCS7CreateDecryptObject (PK11SymKey *key, SECAlgorithmID *algid);





extern sec_PKCS7CipherObject *
sec_PKCS7CreateEncryptObject (PRArenaPool *poolp, PK11SymKey *key,
			      SECOidTag algtag, SECAlgorithmID *algid);




extern void sec_PKCS7DestroyDecryptObject (sec_PKCS7CipherObject *obj);
extern void sec_PKCS7DestroyEncryptObject (sec_PKCS7CipherObject *obj);

















extern unsigned int sec_PKCS7DecryptLength (sec_PKCS7CipherObject *obj,
					    unsigned int input_len,
					    PRBool final);
extern unsigned int sec_PKCS7EncryptLength (sec_PKCS7CipherObject *obj,
					    unsigned int input_len,
					    PRBool final);







 
extern SECStatus sec_PKCS7Decrypt (sec_PKCS7CipherObject *obj,
				   unsigned char *output,
				   unsigned int *output_len_p,
				   unsigned int max_output_len,
				   const unsigned char *input,
				   unsigned int input_len,
				   PRBool final);







 
extern SECStatus sec_PKCS7Encrypt (sec_PKCS7CipherObject *obj,
				   unsigned char *output,
				   unsigned int *output_len_p,
				   unsigned int max_output_len,
				   const unsigned char *input,
				   unsigned int input_len,
				   PRBool final);


SEC_END_PROTOS

#endif 
