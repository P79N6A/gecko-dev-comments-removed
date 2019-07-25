









































#ifndef _BLAPIT_H_
#define _BLAPIT_H_

#include "seccomon.h"
#include "prlink.h"
#include "plarena.h"
#include "ecl-exp.h"



#define NSS_RC2			0
#define NSS_RC2_CBC		1


#define NSS_RC5                 0
#define NSS_RC5_CBC             1


#define NSS_DES			0
#define NSS_DES_CBC		1
#define NSS_DES_EDE3		2
#define NSS_DES_EDE3_CBC	3

#define DES_KEY_LENGTH		8	/* Bytes */


#define NSS_AES                 0
#define NSS_AES_CBC             1


#define NSS_CAMELLIA                 0
#define NSS_CAMELLIA_CBC             1


#define NSS_SEED		0
#define NSS_SEED_CBC		1

#define DSA_SIGNATURE_LEN 	40	/* Bytes */
#define DSA_SUBPRIME_LEN	20	/* Bytes */







#define MAX_ECKEY_LEN 	        72	/* Bytes */




#define MD2_LENGTH		16	/* Bytes */
#define MD5_LENGTH		16	/* Bytes */
#define SHA1_LENGTH		20	/* Bytes */
#define SHA256_LENGTH 		32 	/* bytes */
#define SHA384_LENGTH 		48 	/* bytes */
#define SHA512_LENGTH 		64 	/* bytes */
#define HASH_LENGTH_MAX         SHA512_LENGTH





#define MD2_BLOCK_LENGTH 	 64 	/* bytes */
#define MD5_BLOCK_LENGTH 	 64 	/* bytes */
#define SHA1_BLOCK_LENGTH 	 64 	/* bytes */
#define SHA256_BLOCK_LENGTH 	 64 	/* bytes */
#define SHA384_BLOCK_LENGTH 	128 	/* bytes */
#define SHA512_BLOCK_LENGTH 	128 	/* bytes */
#define HASH_BLOCK_LENGTH_MAX 	SHA512_BLOCK_LENGTH

#define AES_KEY_WRAP_IV_BYTES    8
#define AES_KEY_WRAP_BLOCK_SIZE  8  /* bytes */
#define AES_BLOCK_SIZE          16  /* bytes */

#define AES_128_KEY_LENGTH      16  /* bytes */
#define AES_192_KEY_LENGTH      24  /* bytes */
#define AES_256_KEY_LENGTH      32  /* bytes */

#define CAMELLIA_BLOCK_SIZE          16  /* bytes */

#define SEED_BLOCK_SIZE 16              /* bytes */
#define SEED_KEY_LENGTH 16              /* bytes */

#define NSS_FREEBL_DEFAULT_CHUNKSIZE 2048





#define RSA_MIN_MODULUS_BITS   128
#define RSA_MAX_MODULUS_BITS  8192
#define RSA_MAX_EXPONENT_BITS   64
#define DH_MIN_P_BITS	       128
#define DH_MAX_P_BITS         2236
























#define DSA_Q_BITS       160
#define DSA_MAX_P_BITS	1024
#define DSA_MIN_P_BITS	 512





#define PQG_PBITS_TO_INDEX(bits) \
    (((bits) < 512 || (bits) > 1024 || (bits) % 64) ? \
    -1 : (int)((bits)-512)/64)





#define PQG_INDEX_TO_PBITS(j) (((unsigned)(j) > 8) ? -1 : (512 + 64 * (j)))






struct DESContextStr        ;
struct RC2ContextStr        ;
struct RC4ContextStr        ;
struct RC5ContextStr        ;
struct AESContextStr        ;
struct CamelliaContextStr   ;
struct MD2ContextStr        ;
struct MD5ContextStr        ;
struct SHA1ContextStr       ;
struct SHA256ContextStr     ;
struct SHA512ContextStr     ;
struct AESKeyWrapContextStr ;
struct SEEDContextStr       ;	

typedef struct DESContextStr        DESContext;
typedef struct RC2ContextStr        RC2Context;
typedef struct RC4ContextStr        RC4Context;
typedef struct RC5ContextStr        RC5Context;
typedef struct AESContextStr        AESContext;
typedef struct CamelliaContextStr   CamelliaContext;
typedef struct MD2ContextStr        MD2Context;
typedef struct MD5ContextStr        MD5Context;
typedef struct SHA1ContextStr       SHA1Context;
typedef struct SHA256ContextStr     SHA256Context;
typedef struct SHA512ContextStr     SHA512Context;

typedef struct SHA512ContextStr     SHA384Context;
typedef struct AESKeyWrapContextStr AESKeyWrapContext;
typedef struct SEEDContextStr	    SEEDContext;	






struct RSAPublicKeyStr {
    PLArenaPool * arena;
    SECItem modulus;
    SECItem publicExponent;
};
typedef struct RSAPublicKeyStr RSAPublicKey;


struct RSAPrivateKeyStr {
    PLArenaPool * arena;
    SECItem version;
    SECItem modulus;
    SECItem publicExponent;
    SECItem privateExponent;
    SECItem prime1;
    SECItem prime2;
    SECItem exponent1;
    SECItem exponent2;
    SECItem coefficient;
};
typedef struct RSAPrivateKeyStr RSAPrivateKey;






struct PQGParamsStr {
    PLArenaPool *arena;
    SECItem prime;    
    SECItem subPrime; 
    SECItem base;     
    
};
typedef struct PQGParamsStr PQGParams;

struct PQGVerifyStr {
    PLArenaPool * arena;	
    unsigned int  counter;
    SECItem       seed;
    SECItem       h;
};
typedef struct PQGVerifyStr PQGVerify;

struct DSAPublicKeyStr {
    PQGParams params;
    SECItem publicValue;
};
typedef struct DSAPublicKeyStr DSAPublicKey;

struct DSAPrivateKeyStr {
    PQGParams params;
    SECItem publicValue;
    SECItem privateValue;
};
typedef struct DSAPrivateKeyStr DSAPrivateKey;






struct DHParamsStr {
    PLArenaPool * arena;
    SECItem prime; 
    SECItem base; 
};
typedef struct DHParamsStr DHParams;

struct DHPublicKeyStr {
    PLArenaPool * arena;
    SECItem prime;
    SECItem base;
    SECItem publicValue;
};
typedef struct DHPublicKeyStr DHPublicKey;

struct DHPrivateKeyStr {
    PLArenaPool * arena;
    SECItem prime;
    SECItem base;
    SECItem publicValue;
    SECItem privateValue;
};
typedef struct DHPrivateKeyStr DHPrivateKey;











typedef enum { ec_params_explicit,
	       ec_params_named
} ECParamsType;

typedef enum { ec_field_GFp = 1,
               ec_field_GF2m
} ECFieldType;

struct ECFieldIDStr {
    int         size;   
    ECFieldType type;
    union {
        SECItem  prime; 
        SECItem  poly;  
    } u;
    int         k1;     


    int         k2;     
    int         k3;
};
typedef struct ECFieldIDStr ECFieldID;

struct ECCurveStr {
    SECItem a;          


    SECItem b;
    SECItem seed;
};
typedef struct ECCurveStr ECCurve;

struct ECParamsStr {
    PLArenaPool * arena;
    ECParamsType  type;
    ECFieldID     fieldID;
    ECCurve       curve; 
    SECItem       base;
    SECItem       order; 
    int           cofactor;
    SECItem       DEREncoding;
    ECCurveName   name;
    SECItem       curveOID;
};
typedef struct ECParamsStr ECParams;

struct ECPublicKeyStr {
    ECParams ecParams;   
    SECItem publicValue;   


};
typedef struct ECPublicKeyStr ECPublicKey;

struct ECPrivateKeyStr {
    ECParams ecParams;   
    SECItem publicValue;   
    SECItem privateValue;  
    SECItem version;       
};
typedef struct ECPrivateKeyStr ECPrivateKey;

typedef void * (*BLapiAllocateFunc)(void);
typedef void (*BLapiDestroyContextFunc)(void *cx, PRBool freeit);
typedef SECStatus (*BLapiInitContextFunc)(void *cx, 
				   const unsigned char *key, 
				   unsigned int keylen,
				   const unsigned char *, 
				   int, 
				   unsigned int ,
				   unsigned int );
typedef SECStatus (*BLapiEncrypt)(void *cx, unsigned char *output,
				unsigned int *outputLen, 
				unsigned int maxOutputLen,
				const unsigned char *input, 
				unsigned int inputLen);

#endif 
