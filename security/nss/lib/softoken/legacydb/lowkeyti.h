


#ifndef _LOWKEYTI_H_
#define _LOWKEYTI_H_ 1

#include "blapit.h"
#include "prtypes.h"
#include "plarena.h"
#include "secitem.h"
#include "secasn1t.h"
#include "secoidt.h"





struct NSSLOWKEYDBKeyStr {
    PLArenaPool *arena;
    int version;
    char *nickname;
    SECItem salt;
    SECItem derPK;
};
typedef struct NSSLOWKEYDBKeyStr NSSLOWKEYDBKey;

typedef struct NSSLOWKEYDBHandleStr NSSLOWKEYDBHandle;

#ifdef NSS_USE_KEY4_DB
#define NSSLOWKEY_DB_FILE_VERSION 4
#else
#define NSSLOWKEY_DB_FILE_VERSION 3
#endif

#define NSSLOWKEY_VERSION	    0	/* what we *create* */




extern const SEC_ASN1Template lg_nsslowkey_PQGParamsTemplate[];
extern const SEC_ASN1Template lg_nsslowkey_RSAPrivateKeyTemplate[];
extern const SEC_ASN1Template lg_nsslowkey_RSAPrivateKeyTemplate2[];
extern const SEC_ASN1Template lg_nsslowkey_DSAPrivateKeyTemplate[];
extern const SEC_ASN1Template lg_nsslowkey_DHPrivateKeyTemplate[];
extern const SEC_ASN1Template lg_nsslowkey_DHPrivateKeyExportTemplate[];
#ifdef NSS_ENABLE_ECC
#define NSSLOWKEY_EC_PRIVATE_KEY_VERSION   1  /* as per SECG 1 C.4 */
extern const SEC_ASN1Template lg_nsslowkey_ECParamsTemplate[];
extern const SEC_ASN1Template lg_nsslowkey_ECPrivateKeyTemplate[];
#endif 

extern const SEC_ASN1Template lg_nsslowkey_PrivateKeyInfoTemplate[];
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




struct NSSLOWKEYEncryptedPrivateKeyInfoStr {
    PLArenaPool *arena;
    SECAlgorithmID algorithm;
    SECItem encryptedData;
};
typedef struct NSSLOWKEYEncryptedPrivateKeyInfoStr NSSLOWKEYEncryptedPrivateKeyInfo;


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


typedef struct NSSLOWKEYPasswordEntryStr NSSLOWKEYPasswordEntry;
struct NSSLOWKEYPasswordEntryStr {
    SECItem salt;
    SECItem value;
    unsigned char data[128];
};


#endif	
