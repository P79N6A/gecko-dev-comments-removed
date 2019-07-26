



#ifndef NSSCKFWC_H
#define NSSCKFWC_H

#ifdef DEBUG
static const char NSSCKFWC_CVS_ID[] = "@(#) $RCSfile: nssckfwc.h,v $ $Revision: 1.4 $ $Date: 2012/04/25 14:49:28 $";
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












































































NSS_EXTERN CK_RV
NSSCKFWC_Initialize
(
  NSSCKFWInstance **pFwInstance,
  NSSCKMDInstance *mdInstance,
  CK_VOID_PTR pInitArgs
);





NSS_EXTERN CK_RV
NSSCKFWC_Finalize
(
  NSSCKFWInstance **pFwInstance
);





NSS_EXTERN CK_RV
NSSCKFWC_GetInfo
(
  NSSCKFWInstance *fwInstance,
  CK_INFO_PTR pInfo
);
  










NSS_EXTERN CK_RV
NSSCKFWC_GetSlotList
(
  NSSCKFWInstance *fwInstance,
  CK_BBOOL tokenPresent,
  CK_SLOT_ID_PTR pSlotList,
  CK_ULONG_PTR pulCount
);
 




NSS_EXTERN CK_RV
NSSCKFWC_GetSlotInfo
(
  NSSCKFWInstance *fwInstance,
  CK_SLOT_ID slotID,
  CK_SLOT_INFO_PTR pInfo
);





NSS_EXTERN CK_RV
NSSCKFWC_GetTokenInfo
(
  NSSCKFWInstance *fwInstance,
  CK_SLOT_ID slotID,
  CK_TOKEN_INFO_PTR pInfo
);





NSS_EXTERN CK_RV
NSSCKFWC_WaitForSlotEvent
(
  NSSCKFWInstance *fwInstance,
  CK_FLAGS flags,
  CK_SLOT_ID_PTR pSlot,
  CK_VOID_PTR pReserved
);





NSS_EXTERN CK_RV
NSSCKFWC_GetMechanismList
(
  NSSCKFWInstance *fwInstance,
  CK_SLOT_ID slotID,
  CK_MECHANISM_TYPE_PTR pMechanismList,
  CK_ULONG_PTR pulCount
);





NSS_EXTERN CK_RV
NSSCKFWC_GetMechanismInfo
(
  NSSCKFWInstance *fwInstance,
  CK_SLOT_ID slotID,
  CK_MECHANISM_TYPE type,
  CK_MECHANISM_INFO_PTR pInfo
);





NSS_EXTERN CK_RV
NSSCKFWC_InitToken
(
  NSSCKFWInstance *fwInstance,
  CK_SLOT_ID slotID,
  CK_CHAR_PTR pPin,
  CK_ULONG ulPinLen,
  CK_CHAR_PTR pLabel
);





NSS_EXTERN CK_RV
NSSCKFWC_InitPIN
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_CHAR_PTR pPin,
  CK_ULONG ulPinLen
);





NSS_EXTERN CK_RV
NSSCKFWC_SetPIN
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_CHAR_PTR pOldPin,
  CK_ULONG ulOldLen,
  CK_CHAR_PTR pNewPin,
  CK_ULONG ulNewLen
);





NSS_EXTERN CK_RV
NSSCKFWC_OpenSession
(
  NSSCKFWInstance *fwInstance,
  CK_SLOT_ID slotID,
  CK_FLAGS flags,
  CK_VOID_PTR pApplication,
  CK_NOTIFY Notify,
  CK_SESSION_HANDLE_PTR phSession
);





NSS_EXTERN CK_RV
NSSCKFWC_CloseSession
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession
);





NSS_EXTERN CK_RV
NSSCKFWC_CloseAllSessions
(
  NSSCKFWInstance *fwInstance,
  CK_SLOT_ID slotID
);





NSS_EXTERN CK_RV
NSSCKFWC_GetSessionInfo
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_SESSION_INFO_PTR pInfo
);





NSS_EXTERN CK_RV
NSSCKFWC_GetOperationState
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_BYTE_PTR pOperationState,
  CK_ULONG_PTR pulOperationStateLen
);





NSS_EXTERN CK_RV
NSSCKFWC_SetOperationState
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_BYTE_PTR pOperationState,
  CK_ULONG ulOperationStateLen,
  CK_OBJECT_HANDLE hEncryptionKey,
  CK_OBJECT_HANDLE hAuthenticationKey
);





NSS_EXTERN CK_RV
NSSCKFWC_Login
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_USER_TYPE userType,
  CK_CHAR_PTR pPin,
  CK_ULONG ulPinLen
);





NSS_EXTERN CK_RV
NSSCKFWC_Logout
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession
);





NSS_EXTERN CK_RV
NSSCKFWC_CreateObject
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_ATTRIBUTE_PTR pTemplate,
  CK_ULONG ulCount,
  CK_OBJECT_HANDLE_PTR phObject
);





NSS_EXTERN CK_RV
NSSCKFWC_CopyObject
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_OBJECT_HANDLE hObject,
  CK_ATTRIBUTE_PTR pTemplate,
  CK_ULONG ulCount,
  CK_OBJECT_HANDLE_PTR phNewObject
);





NSS_EXTERN CK_RV
NSSCKFWC_DestroyObject
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_OBJECT_HANDLE hObject
);





NSS_EXTERN CK_RV
NSSCKFWC_GetObjectSize
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_OBJECT_HANDLE hObject,
  CK_ULONG_PTR pulSize
);





NSS_EXTERN CK_RV
NSSCKFWC_GetAttributeValue
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_OBJECT_HANDLE hObject,
  CK_ATTRIBUTE_PTR pTemplate,
  CK_ULONG ulCount
);
  




NSS_EXTERN CK_RV
NSSCKFWC_SetAttributeValue
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_OBJECT_HANDLE hObject,
  CK_ATTRIBUTE_PTR pTemplate,
  CK_ULONG ulCount
);





NSS_EXTERN CK_RV
NSSCKFWC_FindObjectsInit
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_ATTRIBUTE_PTR pTemplate,
  CK_ULONG ulCount
);





NSS_EXTERN CK_RV
NSSCKFWC_FindObjects
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_OBJECT_HANDLE_PTR phObject,
  CK_ULONG ulMaxObjectCount,
  CK_ULONG_PTR pulObjectCount
);





NSS_EXTERN CK_RV
NSSCKFWC_FindObjectsFinal
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession
);





NSS_EXTERN CK_RV
NSSCKFWC_EncryptInit
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_MECHANISM_PTR pMechanism,
  CK_OBJECT_HANDLE hKey
);





NSS_EXTERN CK_RV
NSSCKFWC_Encrypt
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_BYTE_PTR pData,
  CK_ULONG ulDataLen,
  CK_BYTE_PTR pEncryptedData,
  CK_ULONG_PTR pulEncryptedDataLen
);





NSS_EXTERN CK_RV
NSSCKFWC_EncryptUpdate
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_BYTE_PTR pPart,
  CK_ULONG ulPartLen,
  CK_BYTE_PTR pEncryptedPart,
  CK_ULONG_PTR pulEncryptedPartLen
);





NSS_EXTERN CK_RV
NSSCKFWC_EncryptFinal
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_BYTE_PTR pLastEncryptedPart,
  CK_ULONG_PTR pulLastEncryptedPartLen
);





NSS_EXTERN CK_RV
NSSCKFWC_DecryptInit
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_MECHANISM_PTR pMechanism,
  CK_OBJECT_HANDLE hKey
);





NSS_EXTERN CK_RV
NSSCKFWC_Decrypt
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_BYTE_PTR pEncryptedData,
  CK_ULONG ulEncryptedDataLen,
  CK_BYTE_PTR pData,
  CK_ULONG_PTR pulDataLen
);





NSS_EXTERN CK_RV
NSSCKFWC_DecryptUpdate
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_BYTE_PTR pEncryptedPart,
  CK_ULONG ulEncryptedPartLen,
  CK_BYTE_PTR pPart,
  CK_ULONG_PTR pulPartLen
);





NSS_EXTERN CK_RV
NSSCKFWC_DecryptFinal
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_BYTE_PTR pLastPart,
  CK_ULONG_PTR pulLastPartLen
);





NSS_EXTERN CK_RV
NSSCKFWC_DigestInit
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_MECHANISM_PTR pMechanism
);





NSS_EXTERN CK_RV
NSSCKFWC_Digest
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_BYTE_PTR pData,
  CK_ULONG ulDataLen,
  CK_BYTE_PTR pDigest,
  CK_ULONG_PTR pulDigestLen
);





NSS_EXTERN CK_RV
NSSCKFWC_DigestUpdate
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_BYTE_PTR pData,
  CK_ULONG ulDataLen
);





NSS_EXTERN CK_RV
NSSCKFWC_DigestKey
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_OBJECT_HANDLE hKey
);





NSS_EXTERN CK_RV
NSSCKFWC_DigestFinal
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_BYTE_PTR pDigest,
  CK_ULONG_PTR pulDigestLen
);





NSS_EXTERN CK_RV
NSSCKFWC_SignInit
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_MECHANISM_PTR pMechanism,
  CK_OBJECT_HANDLE hKey
);





NSS_EXTERN CK_RV
NSSCKFWC_Sign
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_BYTE_PTR pData,
  CK_ULONG ulDataLen,
  CK_BYTE_PTR pSignature,
  CK_ULONG_PTR pulSignatureLen
);





NSS_EXTERN CK_RV
NSSCKFWC_SignUpdate
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_BYTE_PTR pPart,
  CK_ULONG ulPartLen
);





NSS_EXTERN CK_RV
NSSCKFWC_SignFinal
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_BYTE_PTR pSignature,
  CK_ULONG_PTR pulSignatureLen
);





NSS_EXTERN CK_RV
NSSCKFWC_SignRecoverInit
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_MECHANISM_PTR pMechanism,
  CK_OBJECT_HANDLE hKey
);





NSS_EXTERN CK_RV
NSSCKFWC_SignRecover
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_BYTE_PTR pData,
  CK_ULONG ulDataLen,
  CK_BYTE_PTR pSignature,
  CK_ULONG_PTR pulSignatureLen
);





NSS_EXTERN CK_RV
NSSCKFWC_VerifyInit
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_MECHANISM_PTR pMechanism,
  CK_OBJECT_HANDLE hKey
);





NSS_EXTERN CK_RV
NSSCKFWC_Verify
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_BYTE_PTR pData,
  CK_ULONG ulDataLen,
  CK_BYTE_PTR pSignature,
  CK_ULONG ulSignatureLen
);





NSS_EXTERN CK_RV
NSSCKFWC_VerifyUpdate
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_BYTE_PTR pPart,
  CK_ULONG ulPartLen
);





NSS_EXTERN CK_RV
NSSCKFWC_VerifyFinal
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_BYTE_PTR pSignature,
  CK_ULONG ulSignatureLen
);





NSS_EXTERN CK_RV
NSSCKFWC_VerifyRecoverInit
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_MECHANISM_PTR pMechanism,
  CK_OBJECT_HANDLE hKey
);





NSS_EXTERN CK_RV
NSSCKFWC_VerifyRecover
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_BYTE_PTR pSignature,
  CK_ULONG ulSignatureLen,
  CK_BYTE_PTR pData,
  CK_ULONG_PTR pulDataLen
);





NSS_EXTERN CK_RV
NSSCKFWC_DigestEncryptUpdate
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_BYTE_PTR pPart,
  CK_ULONG ulPartLen,
  CK_BYTE_PTR pEncryptedPart,
  CK_ULONG_PTR pulEncryptedPartLen
);





NSS_EXTERN CK_RV
NSSCKFWC_DecryptDigestUpdate
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_BYTE_PTR pEncryptedPart,
  CK_ULONG ulEncryptedPartLen,
  CK_BYTE_PTR pPart,
  CK_ULONG_PTR pulPartLen
);





NSS_EXTERN CK_RV
NSSCKFWC_SignEncryptUpdate
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_BYTE_PTR pPart,
  CK_ULONG ulPartLen,
  CK_BYTE_PTR pEncryptedPart,
  CK_ULONG_PTR pulEncryptedPartLen
);





NSS_EXTERN CK_RV
NSSCKFWC_DecryptVerifyUpdate
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_BYTE_PTR pEncryptedPart,
  CK_ULONG ulEncryptedPartLen,
  CK_BYTE_PTR pPart,
  CK_ULONG_PTR pulPartLen
);





NSS_EXTERN CK_RV
NSSCKFWC_GenerateKey
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_MECHANISM_PTR pMechanism,
  CK_ATTRIBUTE_PTR pTemplate,
  CK_ULONG ulCount,
  CK_OBJECT_HANDLE_PTR phKey
);





NSS_EXTERN CK_RV
NSSCKFWC_GenerateKeyPair
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_MECHANISM_PTR pMechanism,
  CK_ATTRIBUTE_PTR pPublicKeyTemplate,
  CK_ULONG ulPublicKeyAttributeCount,
  CK_ATTRIBUTE_PTR pPrivateKeyTemplate,
  CK_ULONG ulPrivateKeyAttributeCount,
  CK_OBJECT_HANDLE_PTR phPublicKey,
  CK_OBJECT_HANDLE_PTR phPrivateKey
);





NSS_EXTERN CK_RV
NSSCKFWC_WrapKey
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_MECHANISM_PTR pMechanism,
  CK_OBJECT_HANDLE hWrappingKey,
  CK_OBJECT_HANDLE hKey,
  CK_BYTE_PTR pWrappedKey,
  CK_ULONG_PTR pulWrappedKeyLen
);





NSS_EXTERN CK_RV
NSSCKFWC_UnwrapKey
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_MECHANISM_PTR pMechanism,
  CK_OBJECT_HANDLE hUnwrappingKey,
  CK_BYTE_PTR pWrappedKey,
  CK_ULONG ulWrappedKeyLen,
  CK_ATTRIBUTE_PTR pTemplate,
  CK_ULONG ulAttributeCount,
  CK_OBJECT_HANDLE_PTR phKey
);





NSS_EXTERN CK_RV
NSSCKFWC_DeriveKey
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_MECHANISM_PTR pMechanism,
  CK_OBJECT_HANDLE hBaseKey,
  CK_ATTRIBUTE_PTR pTemplate,
  CK_ULONG ulAttributeCount,
  CK_OBJECT_HANDLE_PTR phKey
);





NSS_EXTERN CK_RV
NSSCKFWC_SeedRandom
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_BYTE_PTR pSeed,
  CK_ULONG ulSeedLen
);





NSS_EXTERN CK_RV
NSSCKFWC_GenerateRandom
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession,
  CK_BYTE_PTR pRandomData,
  CK_ULONG ulRandomLen
);





NSS_EXTERN CK_RV
NSSCKFWC_GetFunctionStatus
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession
);





NSS_EXTERN CK_RV
NSSCKFWC_CancelFunction
(
  NSSCKFWInstance *fwInstance,
  CK_SESSION_HANDLE hSession
);

#endif 
