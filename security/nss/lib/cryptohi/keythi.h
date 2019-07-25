



































#ifndef _KEYTHI_H_
#define _KEYTHI_H_ 1

#include "plarena.h"
#include "pkcs11t.h"
#include "secmodt.h"
#include "prclist.h"













 

typedef enum { 
    nullKey = 0, 
    rsaKey = 1, 
    dsaKey = 2, 
    fortezzaKey = 3, 
    dhKey = 4, 
    keaKey = 5, 
    ecKey = 6,
    rsaPssKey = 7,
    rsaOaepKey = 8
} KeyType;





SEC_BEGIN_PROTOS
extern const SEC_ASN1Template SECKEY_RSAPublicKeyTemplate[];
extern const SEC_ASN1Template SECKEY_RSAPSSParamsTemplate[];
extern const SEC_ASN1Template SECKEY_DSAPublicKeyTemplate[];
extern const SEC_ASN1Template SECKEY_DHPublicKeyTemplate[];
extern const SEC_ASN1Template SECKEY_DHParamKeyTemplate[];
extern const SEC_ASN1Template SECKEY_PQGParamsTemplate[];
extern const SEC_ASN1Template SECKEY_DSAPrivateKeyExportTemplate[];


SEC_ASN1_CHOOSER_DECLARE(SECKEY_DSAPublicKeyTemplate)
SEC_ASN1_CHOOSER_DECLARE(SECKEY_RSAPublicKeyTemplate)
SEC_ASN1_CHOOSER_DECLARE(SECKEY_RSAPSSParamsTemplate)
SEC_END_PROTOS







struct SECKEYRSAPublicKeyStr {
    PLArenaPool * arena;
    SECItem modulus;
    SECItem publicExponent;
};
typedef struct SECKEYRSAPublicKeyStr SECKEYRSAPublicKey;




struct SECKEYRSAPSSParamsStr {
    SECAlgorithmID *hashAlg;
    SECAlgorithmID *maskAlg;
    SECItem saltLength;
    SECItem trailerField;
};
typedef struct SECKEYRSAPSSParamsStr SECKEYRSAPSSParams;





struct SECKEYPQGParamsStr {
    PLArenaPool *arena;
    SECItem prime;    
    SECItem subPrime; 
    SECItem base;     
    
};
typedef struct SECKEYPQGParamsStr SECKEYPQGParams;

struct SECKEYDSAPublicKeyStr {
    SECKEYPQGParams params;
    SECItem publicValue;
};
typedef struct SECKEYDSAPublicKeyStr SECKEYDSAPublicKey;






struct SECKEYDHParamsStr {
    PLArenaPool * arena;
    SECItem prime; 
    SECItem base; 
};
typedef struct SECKEYDHParamsStr SECKEYDHParams;

struct SECKEYDHPublicKeyStr {
    PLArenaPool * arena;
    SECItem prime;
    SECItem base;
    SECItem publicValue;
};
typedef struct SECKEYDHPublicKeyStr SECKEYDHPublicKey;






typedef SECItem SECKEYECParams;

struct SECKEYECPublicKeyStr {
    SECKEYECParams DEREncodedParams;
    int     size;             
    SECItem publicValue;      
    





};
typedef struct SECKEYECPublicKeyStr SECKEYECPublicKey;




struct SECKEYFortezzaPublicKeyStr {
    int      KEAversion;
    int      DSSversion;
    unsigned char    KMID[8];
    SECItem clearance;
    SECItem KEApriviledge;
    SECItem DSSpriviledge;
    SECItem KEAKey;
    SECItem DSSKey;
    SECKEYPQGParams params;
    SECKEYPQGParams keaParams;
};
typedef struct SECKEYFortezzaPublicKeyStr SECKEYFortezzaPublicKey;
#define KEAprivilege KEApriviledge /* corrected spelling */
#define DSSprivilege DSSpriviledge /* corrected spelling */

struct SECKEYDiffPQGParamsStr {
    SECKEYPQGParams DiffKEAParams;
    SECKEYPQGParams DiffDSAParams;
};
typedef struct SECKEYDiffPQGParamsStr SECKEYDiffPQGParams;

struct SECKEYPQGDualParamsStr {
    SECKEYPQGParams CommParams;
    SECKEYDiffPQGParams DiffParams;
};
typedef struct SECKEYPQGDualParamsStr SECKEYPQGDualParams;

struct SECKEYKEAParamsStr {
    PLArenaPool *arena;
    SECItem hash;
};
typedef struct SECKEYKEAParamsStr SECKEYKEAParams;
 
struct SECKEYKEAPublicKeyStr {
    SECKEYKEAParams params;
    SECItem publicValue;
};
typedef struct SECKEYKEAPublicKeyStr SECKEYKEAPublicKey;




struct SECKEYPublicKeyStr {
    PLArenaPool *arena;
    KeyType keyType;
    PK11SlotInfo *pkcs11Slot;
    CK_OBJECT_HANDLE pkcs11ID;
    union {
        SECKEYRSAPublicKey rsa;
	SECKEYDSAPublicKey dsa;
	SECKEYDHPublicKey  dh;
        SECKEYKEAPublicKey kea;
        SECKEYFortezzaPublicKey fortezza;
	SECKEYECPublicKey  ec;
    } u;
};
typedef struct SECKEYPublicKeyStr SECKEYPublicKey;


#define SECKEY_Attributes_Cached 0x1    /* bit 0 states
                                           whether attributes are cached */
#define SECKEY_CKA_PRIVATE (1U << 1)    /* bit 1 is the value of CKA_PRIVATE */

#define SECKEY_ATTRIBUTES_CACHED(key) \
     (0 != (key->staticflags & SECKEY_Attributes_Cached))

#define SECKEY_ATTRIBUTE_VALUE(key,attribute) \
     (0 != (key->staticflags & SECKEY_##attribute))

#define SECKEY_HAS_ATTRIBUTE_SET(key,attribute) \
    (0 != (key->staticflags & SECKEY_Attributes_Cached)) ? \
    (0 != (key->staticflags & SECKEY_##attribute)) : \
    PK11_HasAttributeSet(key->pkcs11Slot,key->pkcs11ID,attribute)



 
struct SECKEYPrivateKeyStr {
    PLArenaPool *arena;
    KeyType keyType;
    PK11SlotInfo *pkcs11Slot;	
    CK_OBJECT_HANDLE pkcs11ID;  
    PRBool pkcs11IsTemp;	
    void *wincx;		
    PRUint32 staticflags;       
};
typedef struct SECKEYPrivateKeyStr SECKEYPrivateKey;

typedef struct {
    PRCList links;
    SECKEYPrivateKey *key;
} SECKEYPrivateKeyListNode;

typedef struct {
    PRCList list;
    PLArenaPool *arena;
} SECKEYPrivateKeyList;

typedef struct {
    PRCList links;
    SECKEYPublicKey *key;
} SECKEYPublicKeyListNode;

typedef struct {
    PRCList list;
    PLArenaPool *arena;
} SECKEYPublicKeyList;
#endif 

