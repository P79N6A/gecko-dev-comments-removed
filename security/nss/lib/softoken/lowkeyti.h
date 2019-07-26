


#ifndef _LOWKEYTI_H_
#define _LOWKEYTI_H_ 1

#include "blapit.h"
#include "prtypes.h"
#include "plarena.h"
#include "secitem.h"
#include "secasn1t.h"
#include "secoidt.h"




extern const SEC_ASN1Template nsslowkey_PQGParamsTemplate[];
extern const SEC_ASN1Template nsslowkey_RSAPrivateKeyTemplate[];
extern const SEC_ASN1Template nsslowkey_DSAPrivateKeyTemplate[];
extern const SEC_ASN1Template nsslowkey_DSAPrivateKeyExportTemplate[];
extern const SEC_ASN1Template nsslowkey_DHPrivateKeyTemplate[];
extern const SEC_ASN1Template nsslowkey_DHPrivateKeyExportTemplate[];
#ifdef NSS_ENABLE_ECC
#define NSSLOWKEY_EC_PRIVATE_KEY_VERSION   1  /* as per SECG 1 C.4 */
extern const SEC_ASN1Template nsslowkey_ECParamsTemplate[];
extern const SEC_ASN1Template nsslowkey_ECPrivateKeyTemplate[];
#endif 

extern const SEC_ASN1Template nsslowkey_PrivateKeyInfoTemplate[];
extern const SEC_ASN1Template nsslowkey_EncryptedPrivateKeyInfoTemplate[];




struct NSSLOWKEYAttributeStr {
    SECItem attrType;
    SECItem *attrValue;
};
typedef struct NSSLOWKEYAttributeStr NSSLOWKEYAttribute;




struct NSSLOWKEYPrivateKeyInfoStr {
    PLArenaPool *arena;
    SECItem version;
    SECAlgorithmID algorithm;
    SECItem privateKey;
    NSSLOWKEYAttribute **attributes;
};
typedef struct NSSLOWKEYPrivateKeyInfoStr NSSLOWKEYPrivateKeyInfo;
#define NSSLOWKEY_PRIVATE_KEY_INFO_VERSION	0	/* what we *create* */

typedef enum { 
    NSSLOWKEYNullKey = 0, 
    NSSLOWKEYRSAKey = 1, 
    NSSLOWKEYDSAKey = 2, 
    NSSLOWKEYDHKey = 4,
    NSSLOWKEYECKey = 5
} NSSLOWKEYType;




struct NSSLOWKEYPublicKeyStr {
    PLArenaPool *arena;
    NSSLOWKEYType keyType ;
    union {
        RSAPublicKey rsa;
	DSAPublicKey dsa;
	DHPublicKey  dh;
	ECPublicKey  ec;
    } u;
};
typedef struct NSSLOWKEYPublicKeyStr NSSLOWKEYPublicKey;






struct NSSLOWKEYPrivateKeyStr {
    PLArenaPool *arena;
    NSSLOWKEYType keyType;
    union {
        RSAPrivateKey rsa;
	DSAPrivateKey dsa;
	DHPrivateKey  dh;
	ECPrivateKey  ec;
    } u;
};
typedef struct NSSLOWKEYPrivateKeyStr NSSLOWKEYPrivateKey;

#endif	
