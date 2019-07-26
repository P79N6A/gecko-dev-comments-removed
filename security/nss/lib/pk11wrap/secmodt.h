


#ifndef _SECMODT_H_
#define _SECMODT_H_ 1

#include "nssrwlkt.h"
#include "nssilckt.h"
#include "secoid.h"
#include "secasn1.h"
#include "pkcs11t.h"
#include "utilmodt.h"

SEC_BEGIN_PROTOS


extern const SEC_ASN1Template SECKEY_PointerToEncryptedPrivateKeyInfoTemplate[];
SEC_ASN1_CHOOSER_DECLARE(SECKEY_PointerToEncryptedPrivateKeyInfoTemplate)
extern const SEC_ASN1Template SECKEY_EncryptedPrivateKeyInfoTemplate[];
SEC_ASN1_CHOOSER_DECLARE(SECKEY_EncryptedPrivateKeyInfoTemplate)
extern const SEC_ASN1Template SECKEY_PrivateKeyInfoTemplate[];
SEC_ASN1_CHOOSER_DECLARE(SECKEY_PrivateKeyInfoTemplate)
extern const SEC_ASN1Template SECKEY_PointerToPrivateKeyInfoTemplate[];
SEC_ASN1_CHOOSER_DECLARE(SECKEY_PointerToPrivateKeyInfoTemplate)

SEC_END_PROTOS


typedef struct SECMODModuleStr SECMODModule;
typedef struct SECMODModuleListStr SECMODModuleList;
typedef NSSRWLock SECMODListLock;
typedef struct PK11SlotInfoStr PK11SlotInfo; 
typedef struct NSSUTILPreSlotInfoStr PK11PreSlotInfo; 
typedef struct PK11SymKeyStr PK11SymKey; 
typedef struct PK11ContextStr PK11Context; 
typedef struct PK11SlotListStr PK11SlotList;
typedef struct PK11SlotListElementStr PK11SlotListElement;
typedef struct PK11RSAGenParamsStr PK11RSAGenParams;
typedef unsigned long SECMODModuleID;
typedef struct PK11DefaultArrayEntryStr PK11DefaultArrayEntry;
typedef struct PK11GenericObjectStr PK11GenericObject;
typedef void (*PK11FreeDataFunc)(void *);

struct SECMODModuleStr {
    PLArenaPool	*arena;
    PRBool	internal;	

    PRBool	loaded;		
    PRBool	isFIPS;		
    char	*dllName;	

    char	*commonName;	
    void	*library;	

    void	*functionList; 
    PZLock	*refLock;	
    int		refCount;	
    PK11SlotInfo **slots;	
    int		slotCount;	
    PK11PreSlotInfo *slotInfo;	
    int		slotInfoCount;  
    SECMODModuleID moduleID;	
    PRBool	isThreadSafe;
    unsigned long ssl[2];	
    char	*libraryParams;  
    void *moduleDBFunc; 
    SECMODModule *parent;	
    PRBool	isCritical;	
    PRBool	isModuleDB;	
    PRBool	moduleDBOnly;	
    int		trustOrder;	
    int		cipherOrder;	
    unsigned long evControlMask; 

    CK_VERSION  cryptokiVersion; 
};












 
#define SECMOD_END_WAIT 	    0x01
#define SECMOD_WAIT_SIMULATED_EVENT 0x02 
#define SECMOD_WAIT_PKCS11_EVENT    0x04

struct SECMODModuleListStr {
    SECMODModuleList	*next;
    SECMODModule	*module;
};

struct PK11SlotListStr {
    PK11SlotListElement *head;
    PK11SlotListElement *tail;
    PZLock *lock;
};

struct PK11SlotListElementStr {
    PK11SlotListElement *next;
    PK11SlotListElement *prev;
    PK11SlotInfo *slot;
    int refCount;
};

struct PK11RSAGenParamsStr {
    int keySizeInBits;
    unsigned long pe;
};

typedef enum {
     PK11CertListUnique = 0,     
     PK11CertListUser = 1,       
     PK11CertListRootUnique = 2, 


     PK11CertListCA = 3,         
     PK11CertListCAUnique = 4,   
     PK11CertListUserUnique = 5, 
     PK11CertListAll = 6         
} PK11CertListType;






struct PK11DefaultArrayEntryStr {
    const char *name;
    unsigned long flag;
    unsigned long mechanism; 

};






typedef PRUint32 PK11AttrFlags;











































#define PK11_ATTR_TOKEN         0x00000001L
#define PK11_ATTR_SESSION       0x00000002L




















#define PK11_ATTR_PRIVATE       0x00000004L
#define PK11_ATTR_PUBLIC        0x00000008L

















#define PK11_ATTR_MODIFIABLE    0x00000010L
#define PK11_ATTR_UNMODIFIABLE  0x00000020L























#define PK11_ATTR_SENSITIVE     0x00000040L
#define PK11_ATTR_INSENSITIVE   0x00000080L
















#define PK11_ATTR_EXTRACTABLE   0x00000100L
#define PK11_ATTR_UNEXTRACTABLE 0x00000200L


#define SECMOD_EXTERNAL	0	/* external module */
#define SECMOD_INTERNAL 1	/* internal default module */
#define SECMOD_FIPS	2	/* internal fips module */


#define SECMOD_SLOT_FLAGS "slotFlags=[RSA,DSA,DH,RC2,RC4,DES,RANDOM,SHA1,MD5,MD2,SSL,TLS,AES,Camellia,SEED,SHA256,SHA512]"

#define SECMOD_MAKE_NSS_FLAGS(fips,slot) \
"Flags=internal,critical" fips " slotparams=(" #slot "={" SECMOD_SLOT_FLAGS "})"

#define SECMOD_INT_NAME "NSS Internal PKCS #11 Module"
#define SECMOD_INT_FLAGS SECMOD_MAKE_NSS_FLAGS("",1)
#define SECMOD_FIPS_NAME "NSS Internal FIPS PKCS #11 Module"
#define SECMOD_FIPS_FLAGS SECMOD_MAKE_NSS_FLAGS(",fips",3)






typedef enum {
    PK11_OriginNULL = 0,	
    PK11_OriginDerive = 1,	
    PK11_OriginGenerated = 2,	
    PK11_OriginFortezzaHack = 3,
    PK11_OriginUnwrap = 4	
} PK11Origin;


typedef enum {
    PK11_DIS_NONE = 0,
    PK11_DIS_USER_SELECTED = 1,
    PK11_DIS_COULD_NOT_INIT_TOKEN = 2,
    PK11_DIS_TOKEN_VERIFY_FAILED = 3,
    PK11_DIS_TOKEN_NOT_PRESENT = 4
} PK11DisableReasons;










typedef enum {
   PK11_TypeGeneric = 0,
   PK11_TypePrivKey = 1,
   PK11_TypePubKey = 2,
   PK11_TypeCert = 3,
   PK11_TypeSymKey = 4
} PK11ObjectType;






typedef char *(PR_CALLBACK *PK11PasswordFunc)(PK11SlotInfo *slot, PRBool retry, void *arg);
typedef PRBool (PR_CALLBACK *PK11VerifyPasswordFunc)(PK11SlotInfo *slot, void *arg);
typedef PRBool (PR_CALLBACK *PK11IsLoggedInFunc)(PK11SlotInfo *slot, void *arg);




 
#define PK11_PW_RETRY		"RETRY"	/* an failed attempt to authenticate
					 * has already been made, just retry
					 * the operation */
#define PK11_PW_AUTHENTICATED	"AUTH"  /* a successful attempt to authenticate
					 * has completed. Continue without
					 * another call to C_Login */



#define PK11_PW_TRY		"TRY"   /* Default: a prompt has been presented
					 * to the user, initiate a C_Login
					 * to authenticate the token */








struct SECKEYAttributeStr {
    SECItem attrType;
    SECItem **attrValue;
};
typedef struct SECKEYAttributeStr SECKEYAttribute;




struct SECKEYPrivateKeyInfoStr {
    PLArenaPool *arena;
    SECItem version;
    SECAlgorithmID algorithm;
    SECItem privateKey;
    SECKEYAttribute **attributes;
};
typedef struct SECKEYPrivateKeyInfoStr SECKEYPrivateKeyInfo;




struct SECKEYEncryptedPrivateKeyInfoStr {
    PLArenaPool *arena;
    SECAlgorithmID algorithm;
    SECItem encryptedData;
};
typedef struct SECKEYEncryptedPrivateKeyInfoStr SECKEYEncryptedPrivateKeyInfo;




typedef enum {
   PK11TokenNotRemovable = 0,
   PK11TokenPresent = 1,
   PK11TokenChanged = 2,
   PK11TokenRemoved = 3
} PK11TokenStatus;

typedef enum {
   PK11TokenRemovedOrChangedEvent = 0,
   PK11TokenPresentEvent = 1
} PK11TokenEvent;




#define CRL_IMPORT_DEFAULT_OPTIONS 0x00000000
#define CRL_IMPORT_BYPASS_CHECKS   0x00000001





typedef struct PK11MergeLogStr PK11MergeLog;
typedef struct PK11MergeLogNodeStr PK11MergeLogNode;



struct PK11MergeLogNodeStr {
    PK11MergeLogNode *next;   
    PK11MergeLogNode *prev;   
    PK11GenericObject *object; 
    int	error;		       
    CK_RV reserved1;
    unsigned long reserved2; 
    unsigned long reserved3; 
    void *reserved4; 	      
    void *reserved5;	      
};

struct PK11MergeLogStr {
    PK11MergeLogNode *head;
    PK11MergeLogNode *tail;
    PLArenaPool *arena;
    int version;
    unsigned long reserved1;
    unsigned long reserved2;
    unsigned long reserved3;
    void *reserverd4;
    void *reserverd5;
};
    

#endif 
