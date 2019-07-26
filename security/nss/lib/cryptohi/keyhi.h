



#ifndef _KEYHI_H_
#define _KEYHI_H_

#include "plarena.h"

#include "seccomon.h"
#include "secoidt.h"
#include "secdert.h"
#include "keythi.h"
#include "certt.h"


SEC_BEGIN_PROTOS





extern void SECKEY_DestroySubjectPublicKeyInfo(CERTSubjectPublicKeyInfo *spki);





extern SECStatus SECKEY_CopySubjectPublicKeyInfo(PLArenaPool *arena,
					     CERTSubjectPublicKeyInfo *dst,
					     CERTSubjectPublicKeyInfo *src);





extern SECStatus
SECKEY_UpdateCertPQG(CERTCertificate * subjectCert);





extern unsigned SECKEY_PublicKeyStrength(const SECKEYPublicKey *pubk);




extern unsigned SECKEY_PublicKeyStrengthInBits(const SECKEYPublicKey *pubk);




extern unsigned SECKEY_SignatureLen(const SECKEYPublicKey *pubk);




extern SECKEYPrivateKey *SECKEY_CopyPrivateKey(const SECKEYPrivateKey *privKey);




extern SECKEYPublicKey *SECKEY_CopyPublicKey(const SECKEYPublicKey *pubKey);




extern SECKEYPublicKey *SECKEY_ConvertToPublicKey(SECKEYPrivateKey *privateKey);




SECKEYPrivateKey *SECKEY_CreateRSAPrivateKey(int keySizeInBits,
					   SECKEYPublicKey **pubk, void *cx);
	



SECKEYPrivateKey *SECKEY_CreateDHPrivateKey(SECKEYDHParams *param,
					   SECKEYPublicKey **pubk, void *cx);




SECKEYPrivateKey *SECKEY_CreateECPrivateKey(SECKEYECParams *param,
                                           SECKEYPublicKey **pubk, void *cx);




extern CERTSubjectPublicKeyInfo *
SECKEY_CreateSubjectPublicKeyInfo(SECKEYPublicKey *k);




extern SECKEYPublicKey *SECKEY_DecodeDERPublicKey(const SECItem *pubkder);




extern SECKEYPublicKey *SECKEY_ConvertAndDecodePublicKey(const char *pubkstr);





extern CERTSubjectPublicKeyInfo *
SECKEY_ConvertAndDecodePublicKeyAndChallenge(char *pkacstr, char *challenge,
								void *cx);





SECItem *
SECKEY_EncodeDERSubjectPublicKeyInfo(SECKEYPublicKey *pubk);





extern CERTSubjectPublicKeyInfo *
SECKEY_DecodeDERSubjectPublicKeyInfo(const SECItem *spkider);





extern CERTSubjectPublicKeyInfo *
SECKEY_ConvertAndDecodeSubjectPublicKeyInfo(const char *spkistr);





extern SECKEYPublicKey *
SECKEY_ExtractPublicKey(const CERTSubjectPublicKeyInfo *);





extern void SECKEY_DestroyPrivateKey(SECKEYPrivateKey *key);






extern void SECKEY_DestroyPublicKey(SECKEYPublicKey *key);









extern void
SECKEY_DestroyPrivateKeyInfo(SECKEYPrivateKeyInfo *pvk, PRBool freeit);







extern void
SECKEY_DestroyEncryptedPrivateKeyInfo(SECKEYEncryptedPrivateKeyInfo *epki,
				      PRBool freeit);









extern SECStatus
SECKEY_CopyPrivateKeyInfo(PLArenaPool *poolp,
			  SECKEYPrivateKeyInfo *to,
			  const SECKEYPrivateKeyInfo *from);

extern SECStatus
SECKEY_CacheStaticFlags(SECKEYPrivateKey* key);









extern SECStatus
SECKEY_CopyEncryptedPrivateKeyInfo(PLArenaPool *poolp,
				   SECKEYEncryptedPrivateKeyInfo *to,
				   const SECKEYEncryptedPrivateKeyInfo *from);



KeyType SECKEY_GetPrivateKeyType(const SECKEYPrivateKey *privKey);
KeyType SECKEY_GetPublicKeyType(const SECKEYPublicKey *pubKey);





SECKEYPublicKey*
SECKEY_ImportDERPublicKey(const SECItem *derKey, CK_KEY_TYPE type);

SECKEYPrivateKeyList*
SECKEY_NewPrivateKeyList(void);

void
SECKEY_DestroyPrivateKeyList(SECKEYPrivateKeyList *keys);

void
SECKEY_RemovePrivateKeyListNode(SECKEYPrivateKeyListNode *node);

SECStatus
SECKEY_AddPrivateKeyToListTail( SECKEYPrivateKeyList *list,
                                SECKEYPrivateKey *key);

#define PRIVKEY_LIST_HEAD(l) ((SECKEYPrivateKeyListNode*)PR_LIST_HEAD(&l->list))
#define PRIVKEY_LIST_NEXT(n) ((SECKEYPrivateKeyListNode *)n->links.next)
#define PRIVKEY_LIST_END(n,l) (((void *)n) == ((void *)&l->list))

SECKEYPublicKeyList*
SECKEY_NewPublicKeyList(void);

void
SECKEY_DestroyPublicKeyList(SECKEYPublicKeyList *keys);

void
SECKEY_RemovePublicKeyListNode(SECKEYPublicKeyListNode *node);

SECStatus
SECKEY_AddPublicKeyToListTail( SECKEYPublicKeyList *list,
                                SECKEYPublicKey *key);

#define PUBKEY_LIST_HEAD(l) ((SECKEYPublicKeyListNode*)PR_LIST_HEAD(&l->list))
#define PUBKEY_LIST_NEXT(n) ((SECKEYPublicKeyListNode *)n->links.next)
#define PUBKEY_LIST_END(n,l) (((void *)n) == ((void *)&l->list))








extern int SECKEY_ECParamsToKeySize(const SECItem *params);








extern int SECKEY_ECParamsToBasePointOrderLen(const SECItem *params);

SEC_END_PROTOS

#endif 
