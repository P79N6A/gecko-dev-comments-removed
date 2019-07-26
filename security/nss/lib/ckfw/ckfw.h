



#ifndef CKFW_H
#define CKFW_H

#ifdef DEBUG
static const char CKFW_CVS_ID[] = "@(#) $RCSfile: ckfw.h,v $ $Revision: 1.11 $ $Date: 2012/04/25 14:49:28 $";
#endif 







#ifndef NSSBASET_H
#include "nssbaset.h"
#endif 

#ifndef NSSCKT_H
#include "nssckt.h"
#endif 

#ifndef NSSCKFWT_H
#include "nssckfwt.h"
#endif 

#ifndef NSSCKMDT_H
#include "nssckmdt.h"
#endif 













































NSS_EXTERN NSSCKFWInstance *
nssCKFWInstance_Create
(
  CK_C_INITIALIZE_ARGS_PTR pInitArgs,
  CryptokiLockingState LockingState,
  NSSCKMDInstance *mdInstance,
  CK_RV *pError
);





NSS_EXTERN CK_RV
nssCKFWInstance_Destroy
(
  NSSCKFWInstance *fwInstance
);





NSS_EXTERN NSSCKMDInstance *
nssCKFWInstance_GetMDInstance
(
  NSSCKFWInstance *fwInstance
);





NSS_EXTERN NSSArena *
nssCKFWInstance_GetArena
(
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
);





NSS_EXTERN CK_BBOOL
nssCKFWInstance_MayCreatePthreads
(
  NSSCKFWInstance *fwInstance
);





NSS_EXTERN NSSCKFWMutex *
nssCKFWInstance_CreateMutex
(
  NSSCKFWInstance *fwInstance,
  NSSArena *arena,
  CK_RV *pError
);





NSS_EXTERN NSSUTF8 *
nssCKFWInstance_GetConfigurationData
(
  NSSCKFWInstance *fwInstance
);





NSS_EXTERN CK_C_INITIALIZE_ARGS_PTR
nssCKFWInstance_GetInitArgs
(
  NSSCKFWInstance *fwInstance
);





NSS_EXTERN CK_SESSION_HANDLE
nssCKFWInstance_CreateSessionHandle
(
  NSSCKFWInstance *fwInstance,
  NSSCKFWSession *fwSession,
  CK_RV *pError
);





NSS_EXTERN NSSCKFWSession *
nssCKFWInstance_ResolveSessionHandle
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession
);





NSS_EXTERN void
nssCKFWInstance_DestroySessionHandle
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession
);





NSS_EXTERN CK_SESSION_HANDLE
nssCKFWInstance_FindSessionHandle
(
  NSSCKFWInstance *fwInstance,
  NSSCKFWSession *fwSession
);





NSS_EXTERN CK_OBJECT_HANDLE
nssCKFWInstance_CreateObjectHandle
(
  NSSCKFWInstance *fwInstance,
  NSSCKFWObject *fwObject,
  CK_RV *pError
);





NSS_EXTERN NSSCKFWObject *
nssCKFWInstance_ResolveObjectHandle
(
  NSSCKFWInstance *fwInstance,
  CK_OBJECT_HANDLE hObject
);





NSS_EXTERN CK_RV
nssCKFWInstance_ReassignObjectHandle
(
  NSSCKFWInstance *fwInstance,
  CK_OBJECT_HANDLE hObject,
  NSSCKFWObject *fwObject
);





NSS_EXTERN void
nssCKFWInstance_DestroyObjectHandle
(
  NSSCKFWInstance *fwInstance,
  CK_OBJECT_HANDLE hObject
);





NSS_EXTERN CK_OBJECT_HANDLE
nssCKFWInstance_FindObjectHandle
(
  NSSCKFWInstance *fwInstance,
  NSSCKFWObject *fwObject
);





NSS_EXTERN CK_ULONG
nssCKFWInstance_GetNSlots
(
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
);





NSS_EXTERN CK_VERSION
nssCKFWInstance_GetCryptokiVersion
(
  NSSCKFWInstance *fwInstance
);





NSS_EXTERN CK_RV
nssCKFWInstance_GetManufacturerID
(
  NSSCKFWInstance *fwInstance,
  CK_CHAR manufacturerID[32]
);





NSS_EXTERN CK_ULONG
nssCKFWInstance_GetFlags
(
  NSSCKFWInstance *fwInstance
);





NSS_EXTERN CK_RV
nssCKFWInstance_GetLibraryDescription
(
  NSSCKFWInstance *fwInstance,
  CK_CHAR libraryDescription[32]
);





NSS_EXTERN CK_VERSION
nssCKFWInstance_GetLibraryVersion
(
  NSSCKFWInstance *fwInstance
);





NSS_EXTERN CK_BBOOL
nssCKFWInstance_GetModuleHandlesSessionObjects
(
  NSSCKFWInstance *fwInstance
);





NSS_EXTERN NSSCKFWSlot **
nssCKFWInstance_GetSlots
(
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
);





NSS_EXTERN NSSCKFWSlot *
nssCKFWInstance_WaitForSlotEvent
(
  NSSCKFWInstance *fwInstance,
  CK_BBOOL block,
  CK_RV *pError
);





NSS_EXTERN CK_RV
nssCKFWInstance_verifyPointer
(
  const NSSCKFWInstance *fwInstance
);
































NSS_EXTERN NSSCKFWSlot *
nssCKFWSlot_Create
(
  NSSCKFWInstance *fwInstance,
  NSSCKMDSlot *mdSlot,
  CK_SLOT_ID slotID,
  CK_RV *pError
);





NSS_EXTERN CK_RV
nssCKFWSlot_Destroy
(
  NSSCKFWSlot *fwSlot
);





NSS_EXTERN NSSCKMDSlot *
nssCKFWSlot_GetMDSlot
(
  NSSCKFWSlot *fwSlot
);






NSS_EXTERN NSSCKFWInstance *
nssCKFWSlot_GetFWInstance
(
  NSSCKFWSlot *fwSlot
);






NSS_EXTERN NSSCKMDInstance *
nssCKFWSlot_GetMDInstance
(
  NSSCKFWSlot *fwSlot
);





NSS_EXTERN CK_SLOT_ID
nssCKFWSlot_GetSlotID
(
  NSSCKFWSlot *fwSlot
);





NSS_EXTERN CK_RV
nssCKFWSlot_GetSlotDescription
(
  NSSCKFWSlot *fwSlot,
  CK_CHAR slotDescription[64]
);





NSS_EXTERN CK_RV
nssCKFWSlot_GetManufacturerID
(
  NSSCKFWSlot *fwSlot,
  CK_CHAR manufacturerID[32]
);





NSS_EXTERN CK_BBOOL
nssCKFWSlot_GetTokenPresent
(
  NSSCKFWSlot *fwSlot
);





NSS_EXTERN CK_BBOOL
nssCKFWSlot_GetRemovableDevice
(
  NSSCKFWSlot *fwSlot
);





NSS_EXTERN CK_BBOOL
nssCKFWSlot_GetHardwareSlot
(
  NSSCKFWSlot *fwSlot
);





NSS_EXTERN CK_VERSION
nssCKFWSlot_GetHardwareVersion
(
  NSSCKFWSlot *fwSlot
);





NSS_EXTERN CK_VERSION
nssCKFWSlot_GetFirmwareVersion
(
  NSSCKFWSlot *fwSlot
);





NSS_EXTERN NSSCKFWToken *
nssCKFWSlot_GetToken
(
  NSSCKFWSlot *fwSlot,
  CK_RV *pError
);





NSS_EXTERN void
nssCKFWSlot_ClearToken
(
  NSSCKFWSlot *fwSlot
);




























































NSS_EXTERN NSSCKFWToken *
nssCKFWToken_Create
(
  NSSCKFWSlot *fwSlot,
  NSSCKMDToken *mdToken,
  CK_RV *pError
);





NSS_EXTERN CK_RV
nssCKFWToken_Destroy
(
  NSSCKFWToken *fwToken
);





NSS_EXTERN NSSCKMDToken *
nssCKFWToken_GetMDToken
(
  NSSCKFWToken *fwToken
);





NSS_EXTERN NSSArena *
nssCKFWToken_GetArena
(
  NSSCKFWToken *fwToken,
  CK_RV *pError
);





NSS_EXTERN NSSCKFWSlot *
nssCKFWToken_GetFWSlot
(
  NSSCKFWToken *fwToken
);





NSS_EXTERN NSSCKMDSlot *
nssCKFWToken_GetMDSlot
(
  NSSCKFWToken *fwToken
);





NSS_EXTERN CK_STATE
nssCKFWToken_GetSessionState
(
  NSSCKFWToken *fwToken
);





NSS_EXTERN CK_RV
nssCKFWToken_InitToken
(
  NSSCKFWToken *fwToken,
  NSSItem *pin,
  NSSUTF8 *label
);





NSS_EXTERN CK_RV
nssCKFWToken_GetLabel
(
  NSSCKFWToken *fwToken,
  CK_CHAR label[32]
);





NSS_EXTERN CK_RV
nssCKFWToken_GetManufacturerID
(
  NSSCKFWToken *fwToken,
  CK_CHAR manufacturerID[32]
);





NSS_EXTERN CK_RV
nssCKFWToken_GetModel
(
  NSSCKFWToken *fwToken,
  CK_CHAR model[16]
);





NSS_EXTERN CK_RV
nssCKFWToken_GetSerialNumber
(
  NSSCKFWToken *fwToken,
  CK_CHAR serialNumber[16]
);





NSS_EXTERN CK_BBOOL
nssCKFWToken_GetHasRNG
(
  NSSCKFWToken *fwToken
);





NSS_EXTERN CK_BBOOL
nssCKFWToken_GetIsWriteProtected
(
  NSSCKFWToken *fwToken
);





NSS_EXTERN CK_BBOOL
nssCKFWToken_GetLoginRequired
(
  NSSCKFWToken *fwToken
);





NSS_EXTERN CK_BBOOL
nssCKFWToken_GetUserPinInitialized
(
  NSSCKFWToken *fwToken
);





NSS_EXTERN CK_BBOOL
nssCKFWToken_GetRestoreKeyNotNeeded
(
  NSSCKFWToken *fwToken
);





NSS_EXTERN CK_BBOOL
nssCKFWToken_GetHasClockOnToken
(
  NSSCKFWToken *fwToken
);





NSS_EXTERN CK_BBOOL
nssCKFWToken_GetHasProtectedAuthenticationPath
(
  NSSCKFWToken *fwToken
);





NSS_EXTERN CK_BBOOL
nssCKFWToken_GetSupportsDualCryptoOperations
(
  NSSCKFWToken *fwToken
);





NSS_EXTERN CK_ULONG
nssCKFWToken_GetMaxSessionCount
(
  NSSCKFWToken *fwToken
);





NSS_EXTERN CK_ULONG
nssCKFWToken_GetMaxRwSessionCount
(
  NSSCKFWToken *fwToken
);





NSS_EXTERN CK_ULONG
nssCKFWToken_GetMaxPinLen
(
  NSSCKFWToken *fwToken
);





NSS_EXTERN CK_ULONG
nssCKFWToken_GetMinPinLen
(
  NSSCKFWToken *fwToken
);





NSS_EXTERN CK_ULONG
nssCKFWToken_GetTotalPublicMemory
(
  NSSCKFWToken *fwToken
);





NSS_EXTERN CK_ULONG
nssCKFWToken_GetFreePublicMemory
(
  NSSCKFWToken *fwToken
);





NSS_EXTERN CK_ULONG
nssCKFWToken_GetTotalPrivateMemory
(
  NSSCKFWToken *fwToken
);





NSS_EXTERN CK_ULONG
nssCKFWToken_GetFreePrivateMemory
(
  NSSCKFWToken *fwToken
);





NSS_EXTERN CK_VERSION
nssCKFWToken_GetHardwareVersion
(
  NSSCKFWToken *fwToken
);





NSS_EXTERN CK_VERSION
nssCKFWToken_GetFirmwareVersion
(
  NSSCKFWToken *fwToken
);





NSS_EXTERN CK_RV
nssCKFWToken_GetUTCTime
(
  NSSCKFWToken *fwToken,
  CK_CHAR utcTime[16]
);





NSS_EXTERN NSSCKFWSession *
nssCKFWToken_OpenSession
(
  NSSCKFWToken *fwToken,
  CK_BBOOL rw,
  CK_VOID_PTR pApplication,
  CK_NOTIFY Notify,
  CK_RV *pError
);





NSS_EXTERN CK_ULONG
nssCKFWToken_GetMechanismCount
(
  NSSCKFWToken *fwToken
);





NSS_EXTERN CK_RV
nssCKFWToken_GetMechanismTypes
(
  NSSCKFWToken *fwToken,
  CK_MECHANISM_TYPE types[]
);





NSS_EXTERN NSSCKFWMechanism *
nssCKFWToken_GetMechanism
(
  NSSCKFWToken *fwToken,
  CK_MECHANISM_TYPE which,
  CK_RV *pError
);





NSS_EXTERN CK_RV
nssCKFWToken_SetSessionState
(
  NSSCKFWToken *fwToken,
  CK_STATE newState
);





NSS_EXTERN CK_RV
nssCKFWToken_RemoveSession
(
  NSSCKFWToken *fwToken,
  NSSCKFWSession *fwSession
);





NSS_EXTERN CK_RV
nssCKFWToken_CloseAllSessions
(
  NSSCKFWToken *fwToken
);





NSS_EXTERN CK_ULONG
nssCKFWToken_GetSessionCount
(
  NSSCKFWToken *fwToken
);





NSS_EXTERN CK_ULONG
nssCKFWToken_GetRwSessionCount
(
  NSSCKFWToken *fwToken
);





NSS_EXTERN CK_ULONG
nssCKFWToken_GetRoSessionCount
(
  NSSCKFWToken *fwToken
);





NSS_EXTERN nssCKFWHash *
nssCKFWToken_GetSessionObjectHash
(
  NSSCKFWToken *fwToken
);





NSS_EXTERN nssCKFWHash *
nssCKFWToken_GetMDObjectHash
(
  NSSCKFWToken *fwToken
);





NSS_EXTERN nssCKFWHash *
nssCKFWToken_GetObjectHandleHash
(
  NSSCKFWToken *fwToken
);















































NSS_EXTERN NSSCKFWMechanism *
nssCKFWMechanism_Create
(
  NSSCKMDMechanism *mdMechanism,
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance
);





NSS_EXTERN void
nssCKFWMechanism_Destroy
(
  NSSCKFWMechanism *fwMechanism
);






NSS_EXTERN NSSCKMDMechanism *
nssCKFWMechanism_GetMDMechanism
(
  NSSCKFWMechanism *fwMechanism
);





NSS_EXTERN CK_ULONG
nssCKFWMechanism_GetMinKeySize
(
  NSSCKFWMechanism *fwMechanism,
  CK_RV *pError
);





NSS_EXTERN CK_ULONG
nssCKFWMechanism_GetMaxKeySize
(
  NSSCKFWMechanism *fwMechanism,
  CK_RV *pError
);





NSS_EXTERN CK_BBOOL
nssCKFWMechanism_GetInHardware
(
  NSSCKFWMechanism *fwMechanism,
  CK_RV *pError
);









NSS_EXTERN CK_BBOOL
nssCKFWMechanism_GetCanEncrypt
(
  NSSCKFWMechanism *fwMechanism,
  CK_RV *pError
);





NSS_EXTERN CK_BBOOL
nssCKFWMechanism_GetCanDecrypt
(
  NSSCKFWMechanism *fwMechanism,
  CK_RV *pError
);





NSS_EXTERN CK_BBOOL
nssCKFWMechanism_GetCanDigest
(
  NSSCKFWMechanism *fwMechanism,
  CK_RV *pError
);





NSS_EXTERN CK_BBOOL
nssCKFWMechanism_GetCanSign
(
  NSSCKFWMechanism *fwMechanism,
  CK_RV *pError
);





NSS_EXTERN CK_BBOOL
nssCKFWMechanism_GetCanSignRecover
(
  NSSCKFWMechanism *fwMechanism,
  CK_RV *pError
);





NSS_EXTERN CK_BBOOL
nssCKFWMechanism_GetCanVerify
(
  NSSCKFWMechanism *fwMechanism,
  CK_RV *pError
);





NSS_EXTERN CK_BBOOL
nssCKFWMechanism_GetCanVerifyRecover
(
  NSSCKFWMechanism *fwMechanism,
  CK_RV *pError
);





NSS_EXTERN CK_BBOOL
nssCKFWMechanism_GetCanGenerate
(
  NSSCKFWMechanism *fwMechanism,
  CK_RV *pError
);





NSS_EXTERN CK_BBOOL
nssCKFWMechanism_GetCanGenerateKeyPair
(
  NSSCKFWMechanism *fwMechanism,
  CK_RV *pError
);





NSS_EXTERN CK_BBOOL
nssCKFWMechanism_GetCanWrap
(
  NSSCKFWMechanism *fwMechanism,
  CK_RV *pError
);





NSS_EXTERN CK_BBOOL
nssCKFWMechanism_GetCanUnwrap
(
  NSSCKFWMechanism *fwMechanism,
  CK_RV *pError
);





NSS_EXTERN CK_BBOOL
nssCKFWMechanism_GetCanDerive
(
  NSSCKFWMechanism *fwMechanism,
  CK_RV *pError
);




NSS_EXTERN CK_RV
nssCKFWMechanism_EncryptInit
(
  NSSCKFWMechanism *fwMechanism,
  CK_MECHANISM      *pMechanism,
  NSSCKFWSession    *fwSession,
  NSSCKFWObject     *fwObject
);




NSS_EXTERN CK_RV
nssCKFWMechanism_DecryptInit
(
  NSSCKFWMechanism *fwMechanism,
  CK_MECHANISM      *pMechanism,
  NSSCKFWSession    *fwSession,
  NSSCKFWObject     *fwObject
);




NSS_EXTERN CK_RV
nssCKFWMechanism_DigestInit
(
  NSSCKFWMechanism *fwMechanism,
  CK_MECHANISM      *pMechanism,
  NSSCKFWSession    *fwSession
);




NSS_EXTERN CK_RV
nssCKFWMechanism_SignInit
(
  NSSCKFWMechanism *fwMechanism,
  CK_MECHANISM      *pMechanism,
  NSSCKFWSession    *fwSession,
  NSSCKFWObject     *fwObject
);




NSS_EXTERN CK_RV
nssCKFWMechanism_SignRecoverInit
(
  NSSCKFWMechanism *fwMechanism,
  CK_MECHANISM      *pMechanism,
  NSSCKFWSession    *fwSession,
  NSSCKFWObject     *fwObject
);




NSS_EXTERN CK_RV
nssCKFWMechanism_VerifyInit
(
  NSSCKFWMechanism *fwMechanism,
  CK_MECHANISM      *pMechanism,
  NSSCKFWSession    *fwSession,
  NSSCKFWObject     *fwObject
);




NSS_EXTERN CK_RV
nssCKFWMechanism_VerifyRecoverInit
(
  NSSCKFWMechanism *fwMechanism,
  CK_MECHANISM      *pMechanism,
  NSSCKFWSession    *fwSession,
  NSSCKFWObject     *fwObject
);




NSS_EXTERN NSSCKFWObject *
nssCKFWMechanism_GenerateKey
(
  NSSCKFWMechanism *fwMechanism,
  CK_MECHANISM_PTR pMechanism,
  NSSCKFWSession   *fwSession,
  CK_ATTRIBUTE_PTR pTemplate,
  CK_ULONG         ulAttributeCount,
  CK_RV            *pError
);




NSS_EXTERN CK_RV
nssCKFWMechanism_GenerateKeyPair
(
  NSSCKFWMechanism *fwMechanism,
  CK_MECHANISM_PTR pMechanism,
  NSSCKFWSession   *fwSession,
  CK_ATTRIBUTE_PTR pPublicKeyTemplate,
  CK_ULONG         ulPublicKeyAttributeCount,
  CK_ATTRIBUTE_PTR pPrivateKeyTemplate,
  CK_ULONG         ulPrivateKeyAttributeCount,
  NSSCKFWObject    **fwPublicKeyObject,
  NSSCKFWObject    **fwPrivateKeyObject
);




NSS_EXTERN CK_ULONG
nssCKFWMechanism_GetWrapKeyLength
(
  NSSCKFWMechanism *fwMechanism,
  CK_MECHANISM_PTR pMechanism,
  NSSCKFWSession   *fwSession,
  NSSCKFWObject    *fwWrappingKeyObject,
  NSSCKFWObject    *fwObject,
  CK_RV		   *pError
);




NSS_EXTERN CK_RV
nssCKFWMechanism_WrapKey
(
  NSSCKFWMechanism *fwMechanism,
  CK_MECHANISM_PTR pMechanism,
  NSSCKFWSession   *fwSession,
  NSSCKFWObject    *fwWrappingKeyObject,
  NSSCKFWObject    *fwObject,
  NSSItem          *wrappedKey
);




NSS_EXTERN NSSCKFWObject *
nssCKFWMechanism_UnwrapKey
(
  NSSCKFWMechanism *fwMechanism,
  CK_MECHANISM_PTR pMechanism,
  NSSCKFWSession   *fwSession,
  NSSCKFWObject    *fwWrappingKeyObject,
  NSSItem          *wrappedKey,
  CK_ATTRIBUTE_PTR pTemplate,
  CK_ULONG         ulAttributeCount,
  CK_RV            *pError
);




NSS_EXTERN NSSCKFWObject *
nssCKFWMechanism_DeriveKey
(
  NSSCKFWMechanism *fwMechanism,
  CK_MECHANISM_PTR pMechanism,
  NSSCKFWSession   *fwSession,
  NSSCKFWObject    *fwBaseKeyObject,
  CK_ATTRIBUTE_PTR pTemplate,
  CK_ULONG         ulAttributeCount,
  CK_RV            *pError
);



























NSS_EXTERN NSSCKFWCryptoOperation *
nssCKFWCryptoOperation_Create
(
  NSSCKMDCryptoOperation *mdOperation,
  NSSCKMDSession *mdSession,
  NSSCKFWSession *fwSession,
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  NSSCKFWCryptoOperationType type,
  CK_RV *pError
);




NSS_EXTERN void
nssCKFWCryptoOperation_Destroy
(
  NSSCKFWCryptoOperation *fwOperation
);




NSS_EXTERN NSSCKMDCryptoOperation *
nssCKFWCryptoOperation_GetMDCryptoOperation
(
  NSSCKFWCryptoOperation *fwOperation
);




NSS_EXTERN NSSCKFWCryptoOperationType
nssCKFWCryptoOperation_GetType
(
  NSSCKFWCryptoOperation *fwOperation
);




NSS_EXTERN CK_ULONG
nssCKFWCryptoOperation_GetFinalLength
(
  NSSCKFWCryptoOperation *fwOperation,
  CK_RV *pError
);




NSS_EXTERN CK_ULONG
nssCKFWCryptoOperation_GetOperationLength
(
  NSSCKFWCryptoOperation *fwOperation,
  NSSItem *inputBuffer,
  CK_RV *pError
);




NSS_EXTERN CK_RV
nssCKFWCryptoOperation_Final
(
  NSSCKFWCryptoOperation *fwOperation,
  NSSItem *outputBuffer
);




NSS_EXTERN CK_RV
nssCKFWCryptoOperation_Update
(
  NSSCKFWCryptoOperation *fwOperation,
  NSSItem *inputBuffer,
  NSSItem *outputBuffer
);




NSS_EXTERN CK_RV
nssCKFWCryptoOperation_DigestUpdate
(
  NSSCKFWCryptoOperation *fwOperation,
  NSSItem *inputBuffer
);




NSS_EXTERN CK_RV
nssCKFWCryptoOperation_DigestKey
(
  NSSCKFWCryptoOperation *fwOperation,
  NSSCKFWObject *fwKey
);




NSS_EXTERN CK_RV
nssCKFWCryptoOperation_UpdateFinal
(
  NSSCKFWCryptoOperation *fwOperation,
  NSSItem *inputBuffer,
  NSSItem *outputBuffer
);




NSS_EXTERN CK_RV
nssCKFWCryptoOperation_UpdateCombo
(
  NSSCKFWCryptoOperation *fwOperation,
  NSSCKFWCryptoOperation *fwPeerOperation,
  NSSItem *inputBuffer,
  NSSItem *outputBuffer
);






















































NSS_EXTERN NSSCKFWSession *
nssCKFWSession_Create
(
  NSSCKFWToken *fwToken,
  CK_BBOOL rw,
  CK_VOID_PTR pApplication,
  CK_NOTIFY Notify,
  CK_RV *pError
);





NSS_EXTERN CK_RV
nssCKFWSession_Destroy
(
  NSSCKFWSession *fwSession,
  CK_BBOOL removeFromTokenHash
);





NSS_EXTERN NSSCKMDSession *
nssCKFWSession_GetMDSession
(
  NSSCKFWSession *fwSession
);





NSS_EXTERN NSSArena *
nssCKFWSession_GetArena
(
  NSSCKFWSession *fwSession,
  CK_RV *pError
);





NSS_EXTERN CK_RV
nssCKFWSession_CallNotification
(
  NSSCKFWSession *fwSession,
  CK_NOTIFICATION event
);





NSS_EXTERN CK_BBOOL
nssCKFWSession_IsRWSession
(
  NSSCKFWSession *fwSession
);





NSS_EXTERN CK_BBOOL
nssCKFWSession_IsSO
(
  NSSCKFWSession *fwSession
);





NSS_EXTERN NSSCKFWSlot *
nssCKFWSession_GetFWSlot
(
  NSSCKFWSession *fwSession
);





NSS_EXTERN CK_STATE
nssCKFWSession_GetSessionState
(
  NSSCKFWSession *fwSession
);





NSS_EXTERN CK_RV
nssCKFWSession_SetFWFindObjects
(
  NSSCKFWSession *fwSession,
  NSSCKFWFindObjects *fwFindObjects
);





NSS_EXTERN NSSCKFWFindObjects *
nssCKFWSession_GetFWFindObjects
(
  NSSCKFWSession *fwSesssion,
  CK_RV *pError
);





NSS_EXTERN CK_RV
nssCKFWSession_SetMDSession
(
  NSSCKFWSession *fwSession,
  NSSCKMDSession *mdSession
);





NSS_EXTERN CK_RV
nssCKFWSession_SetHandle
(
  NSSCKFWSession *fwSession,
  CK_SESSION_HANDLE hSession
);





NSS_EXTERN CK_SESSION_HANDLE
nssCKFWSession_GetHandle
(
  NSSCKFWSession *fwSession
);





NSS_EXTERN CK_RV
nssCKFWSession_RegisterSessionObject
(
  NSSCKFWSession *fwSession,
  NSSCKFWObject *fwObject
);





NSS_EXTERN CK_RV
nssCKFWSession_DeregisterSessionObject
(
  NSSCKFWSession *fwSession,
  NSSCKFWObject *fwObject
);





NSS_EXTERN CK_ULONG
nssCKFWSession_GetDeviceError
(
  NSSCKFWSession *fwSession
);





NSS_EXTERN CK_RV
nssCKFWSession_Login
(
  NSSCKFWSession *fwSession,
  CK_USER_TYPE userType,
  NSSItem *pin
);





NSS_EXTERN CK_RV
nssCKFWSession_Logout
(
  NSSCKFWSession *fwSession
);





NSS_EXTERN CK_RV
nssCKFWSession_InitPIN
(
  NSSCKFWSession *fwSession,
  NSSItem *pin
);





NSS_EXTERN CK_RV
nssCKFWSession_SetPIN
(
  NSSCKFWSession *fwSession,
  NSSItem *newPin,
  NSSItem *oldPin
);





NSS_EXTERN CK_ULONG
nssCKFWSession_GetOperationStateLen
(
  NSSCKFWSession *fwSession,
  CK_RV *pError
);





NSS_EXTERN CK_RV
nssCKFWSession_GetOperationState
(
  NSSCKFWSession *fwSession,
  NSSItem *buffer
);





NSS_EXTERN CK_RV
nssCKFWSession_SetOperationState
(
  NSSCKFWSession *fwSession,
  NSSItem *state,
  NSSCKFWObject *encryptionKey,
  NSSCKFWObject *authenticationKey
);





NSS_EXTERN NSSCKFWObject *
nssCKFWSession_CreateObject
(
  NSSCKFWSession *fwSession,
  CK_ATTRIBUTE_PTR pTemplate,
  CK_ULONG ulAttributeCount,
  CK_RV *pError
);





NSS_EXTERN NSSCKFWObject *
nssCKFWSession_CopyObject
(
  NSSCKFWSession *fwSession,
  NSSCKFWObject *object,
  CK_ATTRIBUTE_PTR pTemplate,
  CK_ULONG ulAttributeCount,
  CK_RV *pError
);





NSS_EXTERN NSSCKFWFindObjects *
nssCKFWSession_FindObjectsInit
(
  NSSCKFWSession *fwSession,
  CK_ATTRIBUTE_PTR pTemplate,
  CK_ULONG ulAttributeCount,
  CK_RV *pError
);




NSS_IMPLEMENT void
nssCKFWSession_SetCurrentCryptoOperation
(
  NSSCKFWSession *fwSession,
  NSSCKFWCryptoOperation * fwOperation,
  NSSCKFWCryptoOperationState state
);




NSS_IMPLEMENT NSSCKFWCryptoOperation *
nssCKFWSession_GetCurrentCryptoOperation
(
  NSSCKFWSession *fwSession,
  NSSCKFWCryptoOperationState state
);





NSS_IMPLEMENT CK_RV
nssCKFWSession_Final
(
  NSSCKFWSession *fwSession,
  NSSCKFWCryptoOperationType type,
  NSSCKFWCryptoOperationState state,
  CK_BYTE_PTR  outBuf,
  CK_ULONG_PTR outBufLen
);





NSS_IMPLEMENT CK_RV
nssCKFWSession_Update
(
  NSSCKFWSession *fwSession,
  NSSCKFWCryptoOperationType type,
  NSSCKFWCryptoOperationState state,
  CK_BYTE_PTR  inBuf,
  CK_ULONG     inBufLen,
  CK_BYTE_PTR  outBuf,
  CK_ULONG_PTR outBufLen
);





NSS_IMPLEMENT CK_RV
nssCKFWSession_DigestUpdate
(
  NSSCKFWSession *fwSession,
  NSSCKFWCryptoOperationType type,
  NSSCKFWCryptoOperationState state,
  CK_BYTE_PTR  inBuf,
  CK_ULONG     inBufLen
);





NSS_IMPLEMENT CK_RV
nssCKFWSession_DigestKey
(
  NSSCKFWSession *fwSession,
  NSSCKFWObject *fwKey
);





NSS_IMPLEMENT CK_RV
nssCKFWSession_UpdateFinal
(
  NSSCKFWSession *fwSession,
  NSSCKFWCryptoOperationType type,
  NSSCKFWCryptoOperationState state,
  CK_BYTE_PTR  inBuf,
  CK_ULONG     inBufLen,
  CK_BYTE_PTR  outBuf,
  CK_ULONG_PTR outBufLen
);





NSS_IMPLEMENT CK_RV
nssCKFWSession_UpdateCombo
(
  NSSCKFWSession *fwSession,
  NSSCKFWCryptoOperationType encryptType,
  NSSCKFWCryptoOperationType digestType,
  NSSCKFWCryptoOperationState digestState,
  CK_BYTE_PTR  inBuf,
  CK_ULONG     inBufLen,
  CK_BYTE_PTR  outBuf,
  CK_ULONG_PTR outBufLen
);





NSS_EXTERN CK_RV
nssCKFWSession_SeedRandom
(
  NSSCKFWSession *fwSession,
  NSSItem *seed
);





NSS_EXTERN CK_RV
nssCKFWSession_GetRandom
(
  NSSCKFWSession *fwSession,
  NSSItem *buffer
);































NSS_EXTERN NSSCKFWObject *
nssCKFWObject_Create
(
  NSSArena *arena,
  NSSCKMDObject *mdObject,
  NSSCKFWSession *fwSession,
  NSSCKFWToken *fwToken,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
);





NSS_EXTERN void
nssCKFWObject_Finalize
(
  NSSCKFWObject *fwObject,
  PRBool removeFromHash
);





NSS_EXTERN void
nssCKFWObject_Destroy
(
  NSSCKFWObject *fwObject
);





NSS_EXTERN NSSCKMDObject *
nssCKFWObject_GetMDObject
(
  NSSCKFWObject *fwObject
);





NSS_EXTERN NSSArena *
nssCKFWObject_GetArena
(
  NSSCKFWObject *fwObject,
  CK_RV *pError
);





NSS_EXTERN CK_RV
nssCKFWObject_SetHandle
(
  NSSCKFWObject *fwObject,
  CK_OBJECT_HANDLE hObject
);





NSS_EXTERN CK_OBJECT_HANDLE
nssCKFWObject_GetHandle
(
  NSSCKFWObject *fwObject
);





NSS_EXTERN CK_BBOOL
nssCKFWObject_IsTokenObject
(
  NSSCKFWObject *fwObject
);





NSS_EXTERN CK_ULONG
nssCKFWObject_GetAttributeCount
(
  NSSCKFWObject *fwObject,
  CK_RV *pError
);





NSS_EXTERN CK_RV
nssCKFWObject_GetAttributeTypes
(
  NSSCKFWObject *fwObject,
  CK_ATTRIBUTE_TYPE_PTR typeArray,
  CK_ULONG ulCount
);





NSS_EXTERN CK_ULONG
nssCKFWObject_GetAttributeSize
(
  NSSCKFWObject *fwObject,
  CK_ATTRIBUTE_TYPE attribute,
  CK_RV *pError
);











NSS_EXTERN NSSItem *
nssCKFWObject_GetAttribute
(
  NSSCKFWObject *fwObject,
  CK_ATTRIBUTE_TYPE attribute,
  NSSItem *itemOpt,
  NSSArena *arenaOpt,
  CK_RV *pError
);





NSS_EXTERN CK_RV
nssCKFWObject_SetAttribute
(
  NSSCKFWObject *fwObject,
  NSSCKFWSession *fwSession,
  CK_ATTRIBUTE_TYPE attribute,
  NSSItem *value
);





NSS_EXTERN CK_ULONG
nssCKFWObject_GetObjectSize
(
  NSSCKFWObject *fwObject,
  CK_RV *pError
);





















NSS_EXTERN NSSCKFWFindObjects *
nssCKFWFindObjects_Create
(
  NSSCKFWSession *fwSession,
  NSSCKFWToken *fwToken,
  NSSCKFWInstance *fwInstance,
  NSSCKMDFindObjects *mdFindObjects1,
  NSSCKMDFindObjects *mdFindObjects2,
  CK_RV *pError
);





NSS_EXTERN void
nssCKFWFindObjects_Destroy
(
  NSSCKFWFindObjects *fwFindObjects
);





NSS_EXTERN NSSCKMDFindObjects *
nssCKFWFindObjects_GetMDFindObjects
(
  NSSCKFWFindObjects *fwFindObjects
);





NSS_EXTERN NSSCKFWObject *
nssCKFWFindObjects_Next
(
  NSSCKFWFindObjects *fwFindObjects,
  NSSArena *arenaOpt,
  CK_RV *pError
);















NSS_EXTERN NSSCKFWMutex *
nssCKFWMutex_Create
(
  CK_C_INITIALIZE_ARGS_PTR pInitArgs,
  CryptokiLockingState LockingState,
  NSSArena *arena,
  CK_RV *pError
);





NSS_EXTERN CK_RV
nssCKFWMutex_Destroy
(
  NSSCKFWMutex *mutex
);





NSS_EXTERN CK_RV
nssCKFWMutex_Lock
(
  NSSCKFWMutex *mutex
);





NSS_EXTERN CK_RV
nssCKFWMutex_Unlock
(
  NSSCKFWMutex *mutex
);

#endif 
