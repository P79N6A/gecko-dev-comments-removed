



#ifndef NSSCKFWT_H
#define NSSCKFWT_H












struct NSSCKFWInstanceStr;
typedef struct NSSCKFWInstanceStr NSSCKFWInstance;






struct NSSCKFWSlotStr;
typedef struct NSSCKFWSlotStr NSSCKFWSlot;






struct NSSCKFWTokenStr;
typedef struct NSSCKFWTokenStr NSSCKFWToken;






struct NSSCKFWMechanismStr;
typedef struct NSSCKFWMechanismStr NSSCKFWMechanism;






struct NSSCKFWCryptoOperationStr;
typedef struct NSSCKFWCryptoOperationStr NSSCKFWCryptoOperation;







struct NSSCKFWSessionStr;
typedef struct NSSCKFWSessionStr NSSCKFWSession;






struct NSSCKFWObjectStr;
typedef struct NSSCKFWObjectStr NSSCKFWObject;






struct NSSCKFWFindObjectsStr;
typedef struct NSSCKFWFindObjectsStr NSSCKFWFindObjects;






struct NSSCKFWMutexStr;
typedef struct NSSCKFWMutexStr NSSCKFWMutex;

typedef enum {
    SingleThreaded,
    MultiThreaded
} CryptokiLockingState ;


typedef enum {
    NSSCKFWCryptoOperationState_EncryptDecrypt = 0,
    NSSCKFWCryptoOperationState_SignVerify,
    NSSCKFWCryptoOperationState_Digest,
    NSSCKFWCryptoOperationState_Max
} NSSCKFWCryptoOperationState;

typedef enum {
    NSSCKFWCryptoOperationType_Encrypt,
    NSSCKFWCryptoOperationType_Decrypt,
    NSSCKFWCryptoOperationType_Digest,
    NSSCKFWCryptoOperationType_Sign,
    NSSCKFWCryptoOperationType_Verify,
    NSSCKFWCryptoOperationType_SignRecover,
    NSSCKFWCryptoOperationType_VerifyRecover
} NSSCKFWCryptoOperationType;

#endif 
