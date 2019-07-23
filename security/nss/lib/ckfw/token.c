



































#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: token.c,v $ $Revision: 1.13 $ $Date: 2009/02/09 07:55:53 $";
#endif 







#ifndef CK_T
#include "ck.h"
#endif 































































struct NSSCKFWTokenStr {
  NSSCKFWMutex *mutex;
  NSSArena *arena;
  NSSCKMDToken *mdToken;
  NSSCKFWSlot *fwSlot;
  NSSCKMDSlot *mdSlot;
  NSSCKFWInstance *fwInstance;
  NSSCKMDInstance *mdInstance;

  



















  NSSUTF8 *label;
  NSSUTF8 *manufacturerID;
  NSSUTF8 *model;
  NSSUTF8 *serialNumber;
  CK_VERSION hardwareVersion;
  CK_VERSION firmwareVersion;

  CK_ULONG sessionCount;
  CK_ULONG rwSessionCount;
  nssCKFWHash *sessions;
  nssCKFWHash *sessionObjectHash;
  nssCKFWHash *mdObjectHash;
  nssCKFWHash *mdMechanismHash;

  CK_STATE state;
};

#ifdef DEBUG











static CK_RV
token_add_pointer
(
  const NSSCKFWToken *fwToken
)
{
  return CKR_OK;
}

static CK_RV
token_remove_pointer
(
  const NSSCKFWToken *fwToken
)
{
  return CKR_OK;
}

NSS_IMPLEMENT CK_RV
nssCKFWToken_verifyPointer
(
  const NSSCKFWToken *fwToken
)
{
  return CKR_OK;
}

#endif 





NSS_IMPLEMENT NSSCKFWToken *
nssCKFWToken_Create
(
  NSSCKFWSlot *fwSlot,
  NSSCKMDToken *mdToken,
  CK_RV *pError
)
{
  NSSArena *arena = (NSSArena *)NULL;
  NSSCKFWToken *fwToken = (NSSCKFWToken *)NULL;
  CK_BBOOL called_setup = CK_FALSE;

  



  arena = NSSArena_Create();
  if (!arena) {
    *pError = CKR_HOST_MEMORY;
    goto loser;
  }

  fwToken = nss_ZNEW(arena, NSSCKFWToken);
  if (!fwToken) {
    *pError = CKR_HOST_MEMORY;
    goto loser;
  }    

  fwToken->arena = arena;
  fwToken->mdToken = mdToken;
  fwToken->fwSlot = fwSlot;
  fwToken->fwInstance = nssCKFWSlot_GetFWInstance(fwSlot);
  fwToken->mdInstance = nssCKFWSlot_GetMDInstance(fwSlot);
  fwToken->state = CKS_RO_PUBLIC_SESSION; 
  fwToken->sessionCount = 0;
  fwToken->rwSessionCount = 0;

  fwToken->mutex = nssCKFWInstance_CreateMutex(fwToken->fwInstance, arena, pError);
  if (!fwToken->mutex) {
    if( CKR_OK == *pError ) {
      *pError = CKR_GENERAL_ERROR;
    }
    goto loser;
  }

  fwToken->sessions = nssCKFWHash_Create(fwToken->fwInstance, arena, pError);
  if (!fwToken->sessions) {
    if( CKR_OK == *pError ) {
      *pError = CKR_GENERAL_ERROR;
    }
    goto loser;
  }

  if( CK_TRUE != nssCKFWInstance_GetModuleHandlesSessionObjects(
                   fwToken->fwInstance) ) {
    fwToken->sessionObjectHash = nssCKFWHash_Create(fwToken->fwInstance, 
                                   arena, pError);
    if (!fwToken->sessionObjectHash) {
      if( CKR_OK == *pError ) {
        *pError = CKR_GENERAL_ERROR;
      }
      goto loser;
    }
  }

  fwToken->mdObjectHash = nssCKFWHash_Create(fwToken->fwInstance, 
                            arena, pError);
  if (!fwToken->mdObjectHash) {
    if( CKR_OK == *pError ) {
      *pError = CKR_GENERAL_ERROR;
    }
    goto loser;
  }

  fwToken->mdMechanismHash = nssCKFWHash_Create(fwToken->fwInstance, 
                            arena, pError);
  if (!fwToken->mdMechanismHash) {
    if( CKR_OK == *pError ) {
      *pError = CKR_GENERAL_ERROR;
    }
    goto loser;
  }

  

  if (mdToken->Setup) {
    *pError = mdToken->Setup(mdToken, fwToken, fwToken->mdInstance, fwToken->fwInstance);
    if( CKR_OK != *pError ) {
      goto loser;
    }
  }

  called_setup = CK_TRUE;

#ifdef DEBUG
  *pError = token_add_pointer(fwToken);
  if( CKR_OK != *pError ) {
    goto loser;
  }
#endif 

  *pError = CKR_OK;
  return fwToken;

 loser:

  if( CK_TRUE == called_setup ) {
    if (mdToken->Invalidate) {
      mdToken->Invalidate(mdToken, fwToken, fwToken->mdInstance, fwToken->fwInstance);
    }
  }

  if (arena) {
    (void)NSSArena_Destroy(arena);
  }

  return (NSSCKFWToken *)NULL;
}

static void
nss_ckfwtoken_session_iterator
(
  const void *key,
  void *value,
  void *closure
)
{
  


  NSSCKFWSession *fwSession = (NSSCKFWSession *)value;
  (void)nssCKFWSession_Destroy(fwSession, CK_FALSE);
  return;
}

static void
nss_ckfwtoken_object_iterator
(
  const void *key,
  void *value,
  void *closure
)
{
  


  NSSCKFWObject *fwObject = (NSSCKFWObject *)value;
  (void)nssCKFWObject_Finalize(fwObject, CK_FALSE);
  return;
}





NSS_IMPLEMENT CK_RV
nssCKFWToken_Destroy
(
  NSSCKFWToken *fwToken
)
{
  CK_RV error = CKR_OK;

#ifdef NSSDEBUG
  error = nssCKFWToken_verifyPointer(fwToken);
  if( CKR_OK != error ) {
    return error;
  }
#endif 

  (void)nssCKFWMutex_Destroy(fwToken->mutex);
  
  if (fwToken->mdToken->Invalidate) {
    fwToken->mdToken->Invalidate(fwToken->mdToken, fwToken,
      fwToken->mdInstance, fwToken->fwInstance);
  }
  


  nssCKFWHash_Iterate(fwToken->sessions, nss_ckfwtoken_session_iterator, 
                                                                (void *)NULL);
  nssCKFWHash_Destroy(fwToken->sessions);

  
  if (fwToken->sessionObjectHash) {
    nssCKFWHash_Destroy(fwToken->sessionObjectHash);
  }

  
  nssCKFWHash_Iterate(fwToken->mdObjectHash, nss_ckfwtoken_object_iterator, 
                                                                (void *)NULL);
  if (fwToken->mdObjectHash) {
    nssCKFWHash_Destroy(fwToken->mdObjectHash);
  }
  if (fwToken->mdMechanismHash) {
    nssCKFWHash_Destroy(fwToken->mdMechanismHash);
  }

  nssCKFWSlot_ClearToken(fwToken->fwSlot);
  
#ifdef DEBUG
  error = token_remove_pointer(fwToken);
#endif 

  (void)NSSArena_Destroy(fwToken->arena);
  return error;
}





NSS_IMPLEMENT NSSCKMDToken *
nssCKFWToken_GetMDToken
(
  NSSCKFWToken *fwToken
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWToken_verifyPointer(fwToken) ) {
    return (NSSCKMDToken *)NULL;
  }
#endif 

  return fwToken->mdToken;
}





NSS_IMPLEMENT NSSArena *
nssCKFWToken_GetArena
(
  NSSCKFWToken *fwToken,
  CK_RV *pError
)
{
#ifdef NSSDEBUG
  if (!pError) {
    return (NSSArena *)NULL;
  }

  *pError = nssCKFWToken_verifyPointer(fwToken);
  if( CKR_OK != *pError ) {
    return (NSSArena *)NULL;
  }
#endif 

  return fwToken->arena;
}





NSS_IMPLEMENT NSSCKFWSlot *
nssCKFWToken_GetFWSlot
(
  NSSCKFWToken *fwToken
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWToken_verifyPointer(fwToken) ) {
    return (NSSCKFWSlot *)NULL;
  }
#endif 

  return fwToken->fwSlot;
}





NSS_IMPLEMENT NSSCKMDSlot *
nssCKFWToken_GetMDSlot
(
  NSSCKFWToken *fwToken
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWToken_verifyPointer(fwToken) ) {
    return (NSSCKMDSlot *)NULL;
  }
#endif 

  return fwToken->mdSlot;
}





NSS_IMPLEMENT CK_STATE
nssCKFWToken_GetSessionState
(
  NSSCKFWToken *fwToken
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWToken_verifyPointer(fwToken) ) {
    return CKS_RO_PUBLIC_SESSION; 
  }
#endif 

  



  






  return fwToken->state;
}





NSS_IMPLEMENT CK_RV
nssCKFWToken_InitToken
(
  NSSCKFWToken *fwToken,
  NSSItem *pin,
  NSSUTF8 *label
)
{
  CK_RV error;

#ifdef NSSDEBUG
  error = nssCKFWToken_verifyPointer(fwToken);
  if( CKR_OK != error ) {
    return CKR_ARGUMENTS_BAD;
  }
#endif 

  error = nssCKFWMutex_Lock(fwToken->mutex);
  if( CKR_OK != error ) {
    return error;
  }

  if( fwToken->sessionCount > 0 ) {
    error = CKR_SESSION_EXISTS;
    goto done;
  }

  if (!fwToken->mdToken->InitToken) {
    error = CKR_DEVICE_ERROR;
    goto done;
  }

  if (!pin) {
    if( nssCKFWToken_GetHasProtectedAuthenticationPath(fwToken) ) {
      ; 
    } else {
      error = CKR_PIN_INCORRECT;
      goto done;
    }
  }

  if (!label) {
    label = (NSSUTF8 *) "";
  }

  error = fwToken->mdToken->InitToken(fwToken->mdToken, fwToken,
            fwToken->mdInstance, fwToken->fwInstance, pin, label);

 done:
  (void)nssCKFWMutex_Unlock(fwToken->mutex);
  return error;
}





NSS_IMPLEMENT CK_RV
nssCKFWToken_GetLabel
(
  NSSCKFWToken *fwToken,
  CK_CHAR label[32]
)
{
  CK_RV error = CKR_OK;

#ifdef NSSDEBUG
  if( (CK_CHAR_PTR)NULL == label ) {
    return CKR_ARGUMENTS_BAD;
  }

  error = nssCKFWToken_verifyPointer(fwToken);
  if( CKR_OK != error ) {
    return error;
  }
#endif 

  error = nssCKFWMutex_Lock(fwToken->mutex);
  if( CKR_OK != error ) {
    return error;
  }

  if (!fwToken->label) {
    if (fwToken->mdToken->GetLabel) {
      fwToken->label = fwToken->mdToken->GetLabel(fwToken->mdToken, fwToken,
        fwToken->mdInstance, fwToken->fwInstance, &error);
      if ((!fwToken->label) && (CKR_OK != error)) {
        goto done;
      }
    } else {
      fwToken->label = (NSSUTF8 *) "";
    }
  }

  (void)nssUTF8_CopyIntoFixedBuffer(fwToken->label, (char *)label, 32, ' ');
  error = CKR_OK;

 done:
  (void)nssCKFWMutex_Unlock(fwToken->mutex);
  return error;
}





NSS_IMPLEMENT CK_RV
nssCKFWToken_GetManufacturerID
(
  NSSCKFWToken *fwToken,
  CK_CHAR manufacturerID[32]
)
{
  CK_RV error = CKR_OK;

#ifdef NSSDEBUG
  if( (CK_CHAR_PTR)NULL == manufacturerID ) {
    return CKR_ARGUMENTS_BAD;
  }

  error = nssCKFWToken_verifyPointer(fwToken);
  if( CKR_OK != error ) {
    return error;
  }
#endif 

  error = nssCKFWMutex_Lock(fwToken->mutex);
  if( CKR_OK != error ) {
    return error;
  }

  if (!fwToken->manufacturerID) {
    if (fwToken->mdToken->GetManufacturerID) {
      fwToken->manufacturerID = fwToken->mdToken->GetManufacturerID(fwToken->mdToken,
        fwToken, fwToken->mdInstance, fwToken->fwInstance, &error);
      if ((!fwToken->manufacturerID) && (CKR_OK != error)) {
        goto done;
      }
    } else {
      fwToken->manufacturerID = (NSSUTF8 *)"";
    }
  }

  (void)nssUTF8_CopyIntoFixedBuffer(fwToken->manufacturerID, (char *)manufacturerID, 32, ' ');
  error = CKR_OK;

 done:
  (void)nssCKFWMutex_Unlock(fwToken->mutex);
  return error;
}





NSS_IMPLEMENT CK_RV
nssCKFWToken_GetModel
(
  NSSCKFWToken *fwToken,
  CK_CHAR model[16]
)
{
  CK_RV error = CKR_OK;

#ifdef NSSDEBUG
  if( (CK_CHAR_PTR)NULL == model ) {
    return CKR_ARGUMENTS_BAD;
  }

  error = nssCKFWToken_verifyPointer(fwToken);
  if( CKR_OK != error ) {
    return error;
  }
#endif 

  error = nssCKFWMutex_Lock(fwToken->mutex);
  if( CKR_OK != error ) {
    return error;
  }

  if (!fwToken->model) {
    if (fwToken->mdToken->GetModel) {
      fwToken->model = fwToken->mdToken->GetModel(fwToken->mdToken, fwToken,
        fwToken->mdInstance, fwToken->fwInstance, &error);
      if ((!fwToken->model) && (CKR_OK != error)) {
        goto done;
      }
    } else {
      fwToken->model = (NSSUTF8 *)"";
    }
  }

  (void)nssUTF8_CopyIntoFixedBuffer(fwToken->model, (char *)model, 16, ' ');
  error = CKR_OK;

 done:
  (void)nssCKFWMutex_Unlock(fwToken->mutex);
  return error;
}





NSS_IMPLEMENT CK_RV
nssCKFWToken_GetSerialNumber
(
  NSSCKFWToken *fwToken,
  CK_CHAR serialNumber[16]
)
{
  CK_RV error = CKR_OK;

#ifdef NSSDEBUG
  if( (CK_CHAR_PTR)NULL == serialNumber ) {
    return CKR_ARGUMENTS_BAD;
  }

  error = nssCKFWToken_verifyPointer(fwToken);
  if( CKR_OK != error ) {
    return error;
  }
#endif 

  error = nssCKFWMutex_Lock(fwToken->mutex);
  if( CKR_OK != error ) {
    return error;
  }

  if (!fwToken->serialNumber) {
    if (fwToken->mdToken->GetSerialNumber) {
      fwToken->serialNumber = fwToken->mdToken->GetSerialNumber(fwToken->mdToken, 
        fwToken, fwToken->mdInstance, fwToken->fwInstance, &error);
      if ((!fwToken->serialNumber) && (CKR_OK != error)) {
        goto done;
      }
    } else {
      fwToken->serialNumber = (NSSUTF8 *)"";
    }
  }

  (void)nssUTF8_CopyIntoFixedBuffer(fwToken->serialNumber, (char *)serialNumber, 16, ' ');
  error = CKR_OK;

 done:
  (void)nssCKFWMutex_Unlock(fwToken->mutex);
  return error;
}






NSS_IMPLEMENT CK_BBOOL
nssCKFWToken_GetHasRNG
(
  NSSCKFWToken *fwToken
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWToken_verifyPointer(fwToken) ) {
    return CK_FALSE;
  }
#endif 

  if (!fwToken->mdToken->GetHasRNG) {
    return CK_FALSE;
  }

  return fwToken->mdToken->GetHasRNG(fwToken->mdToken, fwToken, 
    fwToken->mdInstance, fwToken->fwInstance);
}





NSS_IMPLEMENT CK_BBOOL
nssCKFWToken_GetIsWriteProtected
(
  NSSCKFWToken *fwToken
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWToken_verifyPointer(fwToken) ) {
    return CK_FALSE;
  }
#endif 

  if (!fwToken->mdToken->GetIsWriteProtected) {
    return CK_FALSE;
  }

  return fwToken->mdToken->GetIsWriteProtected(fwToken->mdToken, fwToken, 
    fwToken->mdInstance, fwToken->fwInstance);
}





NSS_IMPLEMENT CK_BBOOL
nssCKFWToken_GetLoginRequired
(
  NSSCKFWToken *fwToken
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWToken_verifyPointer(fwToken) ) {
    return CK_FALSE;
  }
#endif 

  if (!fwToken->mdToken->GetLoginRequired) {
    return CK_FALSE;
  }

  return fwToken->mdToken->GetLoginRequired(fwToken->mdToken, fwToken, 
    fwToken->mdInstance, fwToken->fwInstance);
}





NSS_IMPLEMENT CK_BBOOL
nssCKFWToken_GetUserPinInitialized
(
  NSSCKFWToken *fwToken
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWToken_verifyPointer(fwToken) ) {
    return CK_FALSE;
  }
#endif 

  if (!fwToken->mdToken->GetUserPinInitialized) {
    return CK_FALSE;
  }

  return fwToken->mdToken->GetUserPinInitialized(fwToken->mdToken, fwToken, 
    fwToken->mdInstance, fwToken->fwInstance);
}





NSS_IMPLEMENT CK_BBOOL
nssCKFWToken_GetRestoreKeyNotNeeded
(
  NSSCKFWToken *fwToken
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWToken_verifyPointer(fwToken) ) {
    return CK_FALSE;
  }
#endif 

  if (!fwToken->mdToken->GetRestoreKeyNotNeeded) {
    return CK_FALSE;
  }

  return fwToken->mdToken->GetRestoreKeyNotNeeded(fwToken->mdToken, fwToken, 
    fwToken->mdInstance, fwToken->fwInstance);
}





NSS_IMPLEMENT CK_BBOOL
nssCKFWToken_GetHasClockOnToken
(
  NSSCKFWToken *fwToken
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWToken_verifyPointer(fwToken) ) {
    return CK_FALSE;
  }
#endif 

  if (!fwToken->mdToken->GetHasClockOnToken) {
    return CK_FALSE;
  }

  return fwToken->mdToken->GetHasClockOnToken(fwToken->mdToken, fwToken, 
    fwToken->mdInstance, fwToken->fwInstance);
}





NSS_IMPLEMENT CK_BBOOL
nssCKFWToken_GetHasProtectedAuthenticationPath
(
  NSSCKFWToken *fwToken
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWToken_verifyPointer(fwToken) ) {
    return CK_FALSE;
  }
#endif 

  if (!fwToken->mdToken->GetHasProtectedAuthenticationPath) {
    return CK_FALSE;
  }

  return fwToken->mdToken->GetHasProtectedAuthenticationPath(fwToken->mdToken, 
    fwToken, fwToken->mdInstance, fwToken->fwInstance);
}





NSS_IMPLEMENT CK_BBOOL
nssCKFWToken_GetSupportsDualCryptoOperations
(
  NSSCKFWToken *fwToken
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWToken_verifyPointer(fwToken) ) {
    return CK_FALSE;
  }
#endif 

  if (!fwToken->mdToken->GetSupportsDualCryptoOperations) {
    return CK_FALSE;
  }

  return fwToken->mdToken->GetSupportsDualCryptoOperations(fwToken->mdToken, 
    fwToken, fwToken->mdInstance, fwToken->fwInstance);
}





NSS_IMPLEMENT CK_ULONG
nssCKFWToken_GetMaxSessionCount
(
  NSSCKFWToken *fwToken
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWToken_verifyPointer(fwToken) ) {
    return CK_UNAVAILABLE_INFORMATION;
  }
#endif 

  if (!fwToken->mdToken->GetMaxSessionCount) {
    return CK_UNAVAILABLE_INFORMATION;
  }

  return fwToken->mdToken->GetMaxSessionCount(fwToken->mdToken, fwToken, 
    fwToken->mdInstance, fwToken->fwInstance);
}





NSS_IMPLEMENT CK_ULONG
nssCKFWToken_GetMaxRwSessionCount
(
  NSSCKFWToken *fwToken
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWToken_verifyPointer(fwToken) ) {
    return CK_UNAVAILABLE_INFORMATION;
  }
#endif 

  if (!fwToken->mdToken->GetMaxRwSessionCount) {
    return CK_UNAVAILABLE_INFORMATION;
  }

  return fwToken->mdToken->GetMaxRwSessionCount(fwToken->mdToken, fwToken, 
    fwToken->mdInstance, fwToken->fwInstance);
}





NSS_IMPLEMENT CK_ULONG
nssCKFWToken_GetMaxPinLen
(
  NSSCKFWToken *fwToken
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWToken_verifyPointer(fwToken) ) {
    return CK_UNAVAILABLE_INFORMATION;
  }
#endif 

  if (!fwToken->mdToken->GetMaxPinLen) {
    return CK_UNAVAILABLE_INFORMATION;
  }

  return fwToken->mdToken->GetMaxPinLen(fwToken->mdToken, fwToken, 
    fwToken->mdInstance, fwToken->fwInstance);
}





NSS_IMPLEMENT CK_ULONG
nssCKFWToken_GetMinPinLen
(
  NSSCKFWToken *fwToken
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWToken_verifyPointer(fwToken) ) {
    return CK_UNAVAILABLE_INFORMATION;
  }
#endif 

  if (!fwToken->mdToken->GetMinPinLen) {
    return CK_UNAVAILABLE_INFORMATION;
  }

  return fwToken->mdToken->GetMinPinLen(fwToken->mdToken, fwToken, 
    fwToken->mdInstance, fwToken->fwInstance);
}





NSS_IMPLEMENT CK_ULONG
nssCKFWToken_GetTotalPublicMemory
(
  NSSCKFWToken *fwToken
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWToken_verifyPointer(fwToken) ) {
    return CK_UNAVAILABLE_INFORMATION;
  }
#endif 

  if (!fwToken->mdToken->GetTotalPublicMemory) {
    return CK_UNAVAILABLE_INFORMATION;
  }

  return fwToken->mdToken->GetTotalPublicMemory(fwToken->mdToken, fwToken, 
    fwToken->mdInstance, fwToken->fwInstance);
}





NSS_IMPLEMENT CK_ULONG
nssCKFWToken_GetFreePublicMemory
(
  NSSCKFWToken *fwToken
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWToken_verifyPointer(fwToken) ) {
    return CK_UNAVAILABLE_INFORMATION;
  }
#endif 

  if (!fwToken->mdToken->GetFreePublicMemory) {
    return CK_UNAVAILABLE_INFORMATION;
  }

  return fwToken->mdToken->GetFreePublicMemory(fwToken->mdToken, fwToken, 
    fwToken->mdInstance, fwToken->fwInstance);
}





NSS_IMPLEMENT CK_ULONG
nssCKFWToken_GetTotalPrivateMemory
(
  NSSCKFWToken *fwToken
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWToken_verifyPointer(fwToken) ) {
    return CK_UNAVAILABLE_INFORMATION;
  }
#endif 

  if (!fwToken->mdToken->GetTotalPrivateMemory) {
    return CK_UNAVAILABLE_INFORMATION;
  }

  return fwToken->mdToken->GetTotalPrivateMemory(fwToken->mdToken, fwToken, 
    fwToken->mdInstance, fwToken->fwInstance);
}





NSS_IMPLEMENT CK_ULONG
nssCKFWToken_GetFreePrivateMemory
(
  NSSCKFWToken *fwToken
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWToken_verifyPointer(fwToken) ) {
    return CK_UNAVAILABLE_INFORMATION;
  }
#endif 

  if (!fwToken->mdToken->GetFreePrivateMemory) {
    return CK_UNAVAILABLE_INFORMATION;
  }

  return fwToken->mdToken->GetFreePrivateMemory(fwToken->mdToken, fwToken, 
    fwToken->mdInstance, fwToken->fwInstance);
}





NSS_IMPLEMENT CK_VERSION
nssCKFWToken_GetHardwareVersion
(
  NSSCKFWToken *fwToken
)
{
  CK_VERSION rv;

#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWToken_verifyPointer(fwToken) ) {
    rv.major = rv.minor = 0;
    return rv;
  }
#endif 

  if( CKR_OK != nssCKFWMutex_Lock(fwToken->mutex) ) {
    rv.major = rv.minor = 0;
    return rv;
  }

  if( (0 != fwToken->hardwareVersion.major) ||
      (0 != fwToken->hardwareVersion.minor) ) {
    rv = fwToken->hardwareVersion;
    goto done;
  }

  if (fwToken->mdToken->GetHardwareVersion) {
    fwToken->hardwareVersion = fwToken->mdToken->GetHardwareVersion(
      fwToken->mdToken, fwToken, fwToken->mdInstance, fwToken->fwInstance);
  } else {
    fwToken->hardwareVersion.major = 0;
    fwToken->hardwareVersion.minor = 1;
  }

  rv = fwToken->hardwareVersion;

 done:
  (void)nssCKFWMutex_Unlock(fwToken->mutex);
  return rv;
}





NSS_IMPLEMENT CK_VERSION
nssCKFWToken_GetFirmwareVersion
(
  NSSCKFWToken *fwToken
)
{
  CK_VERSION rv;

#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWToken_verifyPointer(fwToken) ) {
    rv.major = rv.minor = 0;
    return rv;
  }
#endif 

  if( CKR_OK != nssCKFWMutex_Lock(fwToken->mutex) ) {
    rv.major = rv.minor = 0;
    return rv;
  }

  if( (0 != fwToken->firmwareVersion.major) ||
      (0 != fwToken->firmwareVersion.minor) ) {
    rv = fwToken->firmwareVersion;
    goto done;
  }

  if (fwToken->mdToken->GetFirmwareVersion) {
    fwToken->firmwareVersion = fwToken->mdToken->GetFirmwareVersion(
      fwToken->mdToken, fwToken, fwToken->mdInstance, fwToken->fwInstance);
  } else {
    fwToken->firmwareVersion.major = 0;
    fwToken->firmwareVersion.minor = 1;
  }

  rv = fwToken->firmwareVersion;

 done:
  (void)nssCKFWMutex_Unlock(fwToken->mutex);
  return rv;
}





NSS_IMPLEMENT CK_RV
nssCKFWToken_GetUTCTime
(
  NSSCKFWToken *fwToken,
  CK_CHAR utcTime[16]
)
{
  CK_RV error = CKR_OK;

#ifdef NSSDEBUG
  error = nssCKFWToken_verifyPointer(fwToken);
  if( CKR_OK != error ) {
    return error;
  }

  if( (CK_CHAR_PTR)NULL == utcTime ) {
    return CKR_ARGUMENTS_BAD;
  }
#endif 

  if( CK_TRUE != nssCKFWToken_GetHasClockOnToken(fwToken) ) {
    
    (void)nssUTF8_CopyIntoFixedBuffer((NSSUTF8 *)NULL, (char *)utcTime, 16, ' ');
    return CKR_OK;
  }

  if (!fwToken->mdToken->GetUTCTime) {
    
    return CKR_GENERAL_ERROR;
  }

  error = fwToken->mdToken->GetUTCTime(fwToken->mdToken, fwToken, 
            fwToken->mdInstance, fwToken->fwInstance, utcTime);
  if( CKR_OK != error ) {
    return error;
  }

  
  {
    
    int i;
    int Y, M, D, h, m, s, z;
    static int dims[] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    for( i = 0; i < 16; i++ ) {
      if( (utcTime[i] < '0') || (utcTime[i] > '9') ) {
        goto badtime;
      }
    }

    Y = ((utcTime[ 0] - '0') * 1000) + ((utcTime[1] - '0') * 100) +
        ((utcTime[ 2] - '0') * 10) + (utcTime[ 3] - '0');
    M = ((utcTime[ 4] - '0') * 10) + (utcTime[ 5] - '0');
    D = ((utcTime[ 6] - '0') * 10) + (utcTime[ 7] - '0');
    h = ((utcTime[ 8] - '0') * 10) + (utcTime[ 9] - '0');
    m = ((utcTime[10] - '0') * 10) + (utcTime[11] - '0');
    s = ((utcTime[12] - '0') * 10) + (utcTime[13] - '0');
    z = ((utcTime[14] - '0') * 10) + (utcTime[15] - '0');

    if( (Y < 1990) || (Y > 3000) ) goto badtime; 
    if( (M < 1) || (M > 12) ) goto badtime;
    if( (D < 1) || (D > 31) ) goto badtime;

    if( D > dims[M-1] ) goto badtime; 
    if( (2 == M) && (((Y%4)||!(Y%100))&&(Y%400)) && (D > 28) ) goto badtime; 

    if( (h < 0) || (h > 23) ) goto badtime;
    if( (m < 0) || (m > 60) ) goto badtime;
    if( (s < 0) || (s > 61) ) goto badtime;

    
    if( (60 == m) || (s >= 60) ) {
      if( (23 != h) || (60 != m) || (s < 60) ) goto badtime;
      
      
    }
  }

  return CKR_OK;

 badtime:
  return CKR_GENERAL_ERROR;
}





NSS_IMPLEMENT NSSCKFWSession *
nssCKFWToken_OpenSession
(
  NSSCKFWToken *fwToken,
  CK_BBOOL rw,
  CK_VOID_PTR pApplication,
  CK_NOTIFY Notify,
  CK_RV *pError
)
{
  NSSCKFWSession *fwSession = (NSSCKFWSession *)NULL;
  NSSCKMDSession *mdSession;

#ifdef NSSDEBUG
  if (!pError) {
    return (NSSCKFWSession *)NULL;
  }

  *pError = nssCKFWToken_verifyPointer(fwToken);
  if( CKR_OK != *pError ) {
    return (NSSCKFWSession *)NULL;
  }

  switch( rw ) {
  case CK_TRUE:
  case CK_FALSE:
    break;
  default:
    *pError = CKR_ARGUMENTS_BAD;
    return (NSSCKFWSession *)NULL;
  }
#endif 

  *pError = nssCKFWMutex_Lock(fwToken->mutex);
  if( CKR_OK != *pError ) {
    return (NSSCKFWSession *)NULL;
  }

  if( CK_TRUE == rw ) {
    
    if( CK_TRUE == nssCKFWToken_GetIsWriteProtected(fwToken) ) {
      *pError = CKR_TOKEN_WRITE_PROTECTED;
      goto done;
    }
  } else {
    
    if( CKS_RW_SO_FUNCTIONS == nssCKFWToken_GetSessionState(fwToken) ) {
      *pError = CKR_SESSION_READ_WRITE_SO_EXISTS;
      goto done;
    }
  }

  

  if (!fwToken->mdToken->OpenSession) {
    




    *pError = CKR_GENERAL_ERROR;
    goto done;
  }

  fwSession = nssCKFWSession_Create(fwToken, rw, pApplication, Notify, pError);
  if (!fwSession) {
    if( CKR_OK == *pError ) {
      *pError = CKR_GENERAL_ERROR;
    }
    goto done;
  }

  mdSession = fwToken->mdToken->OpenSession(fwToken->mdToken, fwToken,
                fwToken->mdInstance, fwToken->fwInstance, fwSession,
                rw, pError);
  if (!mdSession) {
    (void)nssCKFWSession_Destroy(fwSession, CK_FALSE);
    if( CKR_OK == *pError ) {
      *pError = CKR_GENERAL_ERROR;
    }
    goto done;
  }

  *pError = nssCKFWSession_SetMDSession(fwSession, mdSession);
  if( CKR_OK != *pError ) {
    if (mdSession->Close) {
      mdSession->Close(mdSession, fwSession, fwToken->mdToken, fwToken,
      fwToken->mdInstance, fwToken->fwInstance);
    }
    (void)nssCKFWSession_Destroy(fwSession, CK_FALSE);
    goto done;
  }

  *pError = nssCKFWHash_Add(fwToken->sessions, fwSession, fwSession);
  if( CKR_OK != *pError ) {
    (void)nssCKFWSession_Destroy(fwSession, CK_FALSE);
    fwSession = (NSSCKFWSession *)NULL;
    goto done;
  }

 done:
  (void)nssCKFWMutex_Unlock(fwToken->mutex);
  return fwSession;
}





NSS_IMPLEMENT CK_ULONG
nssCKFWToken_GetMechanismCount
(
  NSSCKFWToken *fwToken
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWToken_verifyPointer(fwToken) ) {
    return 0;
  }
#endif 

  if (!fwToken->mdToken->GetMechanismCount) {
    return 0;
  }

  return fwToken->mdToken->GetMechanismCount(fwToken->mdToken, fwToken,
    fwToken->mdInstance, fwToken->fwInstance);
}





NSS_IMPLEMENT CK_RV
nssCKFWToken_GetMechanismTypes
(
  NSSCKFWToken *fwToken,
  CK_MECHANISM_TYPE types[]
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWToken_verifyPointer(fwToken) ) {
    return CKR_ARGUMENTS_BAD;
  }

  if (!types) {
    return CKR_ARGUMENTS_BAD;
  }
#endif 

  if (!fwToken->mdToken->GetMechanismTypes) {
    





    return CKR_GENERAL_ERROR;
  }

  return fwToken->mdToken->GetMechanismTypes(fwToken->mdToken, fwToken,
    fwToken->mdInstance, fwToken->fwInstance, types);
}






NSS_IMPLEMENT NSSCKFWMechanism *
nssCKFWToken_GetMechanism
(
  NSSCKFWToken *fwToken,
  CK_MECHANISM_TYPE which,
  CK_RV *pError
)
{
  NSSCKMDMechanism *mdMechanism;
  if (!fwToken->mdMechanismHash) {
    *pError = CKR_GENERAL_ERROR;
    return (NSSCKFWMechanism *)NULL;
  }
  
  if (!fwToken->mdToken->GetMechanism) {
    



    *pError = CKR_MECHANISM_INVALID;
    return (NSSCKFWMechanism *)NULL;
  }

  
  mdMechanism = fwToken->mdToken->GetMechanism(fwToken->mdToken, fwToken,
    fwToken->mdInstance, fwToken->fwInstance, which, pError);
  if (!mdMechanism) {
    return (NSSCKFWMechanism *) NULL;
  }
  
  return nssCKFWMechanism_Create(mdMechanism, fwToken->mdToken, fwToken,
    fwToken->mdInstance, fwToken->fwInstance);
}

NSS_IMPLEMENT CK_RV
nssCKFWToken_SetSessionState
(
  NSSCKFWToken *fwToken,
  CK_STATE newState
)
{
  CK_RV error = CKR_OK;

#ifdef NSSDEBUG
  error = nssCKFWToken_verifyPointer(fwToken);
  if( CKR_OK != error ) {
    return error;
  }

  switch( newState ) {
  case CKS_RO_PUBLIC_SESSION:
  case CKS_RO_USER_FUNCTIONS:
  case CKS_RW_PUBLIC_SESSION:
  case CKS_RW_USER_FUNCTIONS:
  case CKS_RW_SO_FUNCTIONS:
    break;
  default:
    return CKR_ARGUMENTS_BAD;
  }
#endif 

  error = nssCKFWMutex_Lock(fwToken->mutex);
  if( CKR_OK != error ) {
    return error;
  }

  fwToken->state = newState;
  (void)nssCKFWMutex_Unlock(fwToken->mutex);
  return CKR_OK;
}





NSS_IMPLEMENT CK_RV
nssCKFWToken_RemoveSession
(
  NSSCKFWToken *fwToken,
  NSSCKFWSession *fwSession
)
{
  CK_RV error = CKR_OK;

#ifdef NSSDEBUG
  error = nssCKFWToken_verifyPointer(fwToken);
  if( CKR_OK != error ) {
    return error;
  }

  error = nssCKFWSession_verifyPointer(fwSession);
  if( CKR_OK != error ) {
    return error;
  }
#endif 

  error = nssCKFWMutex_Lock(fwToken->mutex);
  if( CKR_OK != error ) {
    return error;
  }

  if( CK_TRUE != nssCKFWHash_Exists(fwToken->sessions, fwSession) ) {
    error = CKR_SESSION_HANDLE_INVALID;
    goto done;
  }

  nssCKFWHash_Remove(fwToken->sessions, fwSession);
  fwToken->sessionCount--;

  if( nssCKFWSession_IsRWSession(fwSession) ) {
    fwToken->rwSessionCount--;
  }

  if( 0 == fwToken->sessionCount ) {
    fwToken->rwSessionCount = 0; 
    fwToken->state = CKS_RO_PUBLIC_SESSION; 
  }

  error = CKR_OK;

 done:
  (void)nssCKFWMutex_Unlock(fwToken->mutex);
  return error;
}






NSS_IMPLEMENT CK_RV
nssCKFWToken_CloseAllSessions
(
  NSSCKFWToken *fwToken
)
{
  CK_RV error = CKR_OK;

#ifdef NSSDEBUG
  error = nssCKFWToken_verifyPointer(fwToken);
  if( CKR_OK != error ) {
    return error;
  }
#endif 

  error = nssCKFWMutex_Lock(fwToken->mutex);
  if( CKR_OK != error ) {
    return error;
  }

  nssCKFWHash_Iterate(fwToken->sessions, nss_ckfwtoken_session_iterator, (void *)NULL);

  nssCKFWHash_Destroy(fwToken->sessions);

  fwToken->sessions = nssCKFWHash_Create(fwToken->fwInstance, fwToken->arena, &error);
  if (!fwToken->sessions) {
    if( CKR_OK == error ) {
      error = CKR_GENERAL_ERROR;
    }
    goto done;
  }

  fwToken->state = CKS_RO_PUBLIC_SESSION; 
  fwToken->sessionCount = 0;
  fwToken->rwSessionCount = 0;

  error = CKR_OK;

 done:
  (void)nssCKFWMutex_Unlock(fwToken->mutex);
  return error;
}





NSS_IMPLEMENT CK_ULONG
nssCKFWToken_GetSessionCount
(
  NSSCKFWToken *fwToken
)
{
  CK_ULONG rv;

#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWToken_verifyPointer(fwToken) ) {
    return (CK_ULONG)0;
  }
#endif 

  if( CKR_OK != nssCKFWMutex_Lock(fwToken->mutex) ) {
    return (CK_ULONG)0;
  }

  rv = fwToken->sessionCount;
  (void)nssCKFWMutex_Unlock(fwToken->mutex);
  return rv;
}





NSS_IMPLEMENT CK_ULONG
nssCKFWToken_GetRwSessionCount
(
  NSSCKFWToken *fwToken
)
{
  CK_ULONG rv;

#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWToken_verifyPointer(fwToken) ) {
    return (CK_ULONG)0;
  }
#endif 

  if( CKR_OK != nssCKFWMutex_Lock(fwToken->mutex) ) {
    return (CK_ULONG)0;
  }

  rv = fwToken->rwSessionCount;
  (void)nssCKFWMutex_Unlock(fwToken->mutex);
  return rv;
}





NSS_IMPLEMENT CK_ULONG
nssCKFWToken_GetRoSessionCount
(
  NSSCKFWToken *fwToken
)
{
  CK_ULONG rv;

#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWToken_verifyPointer(fwToken) ) {
    return (CK_ULONG)0;
  }
#endif 

  if( CKR_OK != nssCKFWMutex_Lock(fwToken->mutex) ) {
    return (CK_ULONG)0;
  }

  rv = fwToken->sessionCount - fwToken->rwSessionCount;
  (void)nssCKFWMutex_Unlock(fwToken->mutex);
  return rv;
}





NSS_IMPLEMENT nssCKFWHash *
nssCKFWToken_GetSessionObjectHash
(
  NSSCKFWToken *fwToken
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWToken_verifyPointer(fwToken) ) {
    return (nssCKFWHash *)NULL;
  }
#endif 

  return fwToken->sessionObjectHash;
}





NSS_IMPLEMENT nssCKFWHash *
nssCKFWToken_GetMDObjectHash
(
  NSSCKFWToken *fwToken
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWToken_verifyPointer(fwToken) ) {
    return (nssCKFWHash *)NULL;
  }
#endif 

  return fwToken->mdObjectHash;
}





NSS_IMPLEMENT nssCKFWHash *
nssCKFWToken_GetObjectHandleHash
(
  NSSCKFWToken *fwToken
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWToken_verifyPointer(fwToken) ) {
    return (nssCKFWHash *)NULL;
  }
#endif 

  return fwToken->mdObjectHash;
}






NSS_IMPLEMENT NSSCKMDToken *
NSSCKFWToken_GetMDToken
(
  NSSCKFWToken *fwToken
)
{
#ifdef DEBUG
  if( CKR_OK != nssCKFWToken_verifyPointer(fwToken) ) {
    return (NSSCKMDToken *)NULL;
  }
#endif 

  return nssCKFWToken_GetMDToken(fwToken);
}






NSS_IMPLEMENT NSSArena *
NSSCKFWToken_GetArena
(
  NSSCKFWToken *fwToken,
  CK_RV *pError
)
{
#ifdef DEBUG
  if (!pError) {
    return (NSSArena *)NULL;
  }

  if( CKR_OK != nssCKFWToken_verifyPointer(fwToken) ) {
    *pError = CKR_ARGUMENTS_BAD;
    return (NSSArena *)NULL;
  }
#endif 

  return nssCKFWToken_GetArena(fwToken, pError);
}






NSS_IMPLEMENT NSSCKFWSlot *
NSSCKFWToken_GetFWSlot
(
  NSSCKFWToken *fwToken
)
{
#ifdef DEBUG
  if( CKR_OK != nssCKFWToken_verifyPointer(fwToken) ) {
    return (NSSCKFWSlot *)NULL;
  }
#endif 

  return nssCKFWToken_GetFWSlot(fwToken);
}






NSS_IMPLEMENT NSSCKMDSlot *
NSSCKFWToken_GetMDSlot
(
  NSSCKFWToken *fwToken
)
{
#ifdef DEBUG
  if( CKR_OK != nssCKFWToken_verifyPointer(fwToken) ) {
    return (NSSCKMDSlot *)NULL;
  }
#endif 

  return nssCKFWToken_GetMDSlot(fwToken);
}






NSS_IMPLEMENT CK_STATE
NSSCKFWSession_GetSessionState
(
  NSSCKFWToken *fwToken
)
{
#ifdef DEBUG
  if( CKR_OK != nssCKFWToken_verifyPointer(fwToken) ) {
    return CKS_RO_PUBLIC_SESSION;
  }
#endif 

  return nssCKFWToken_GetSessionState(fwToken);
}
