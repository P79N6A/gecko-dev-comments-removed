










#ifndef _SECMIME_H_
#define _SECMIME_H_ 1

#include "secpkcs7.h"



SEC_BEGIN_PROTOS




























extern SECStatus SECMIME_EnableCipher(long which, int on);



























extern SECStatus SECMIME_SetPolicy(long which, int on);





extern PRBool SECMIME_DecryptionAllowed(SECAlgorithmID *algid, PK11SymKey *key);



















extern PRBool SECMIME_EncryptionPossible(void);


















extern SEC_PKCS7ContentInfo *SECMIME_CreateEncrypted(CERTCertificate *scert,
						     CERTCertificate **rcerts,
						     CERTCertDBHandle *certdb,
						     SECKEYGetPasswordKey pwfn,
						     void *pwfn_arg);


























extern SEC_PKCS7ContentInfo *SECMIME_CreateSigned(CERTCertificate *scert,
						  CERTCertificate *ecert,
						  CERTCertDBHandle *certdb,
						  SECOidTag digestalg,
						  SECItem *digest,
						  SECKEYGetPasswordKey pwfn,
						  void *pwfn_arg);


SEC_END_PROTOS

#endif 
