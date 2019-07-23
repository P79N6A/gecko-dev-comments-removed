



































#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: session.c,v $ $Revision: 1.13 $ $Date: 2009/02/09 07:55:53 $";
#endif 







#ifndef CK_T
#include "ck.h"
#endif 

















































struct NSSCKFWSessionStr {
  NSSArena *arena;
  NSSCKMDSession *mdSession;
  NSSCKFWToken *fwToken;
  NSSCKMDToken *mdToken;
  NSSCKFWInstance *fwInstance;
  NSSCKMDInstance *mdInstance;
  CK_VOID_PTR pApplication;
  CK_NOTIFY Notify;

  





  CK_BBOOL rw;
  NSSCKFWFindObjects *fwFindObjects;
  NSSCKFWCryptoOperation *fwOperationArray[NSSCKFWCryptoOperationState_Max];
  nssCKFWHash *sessionObjectHash;
  CK_SESSION_HANDLE hSession;
};

#ifdef DEBUG











static CK_RV
session_add_pointer
(
  const NSSCKFWSession *fwSession
)
{
  return CKR_OK;
}

static CK_RV
session_remove_pointer
(
  const NSSCKFWSession *fwSession
)
{
  return CKR_OK;
}

NSS_IMPLEMENT CK_RV
nssCKFWSession_verifyPointer
(
  const NSSCKFWSession *fwSession
)
{
  return CKR_OK;
}

#endif 





NSS_IMPLEMENT NSSCKFWSession *
nssCKFWSession_Create
(
  NSSCKFWToken *fwToken,
  CK_BBOOL rw,
  CK_VOID_PTR pApplication,
  CK_NOTIFY Notify,
  CK_RV *pError
)
{
  NSSArena *arena = (NSSArena *)NULL;
  NSSCKFWSession *fwSession;
  NSSCKFWSlot *fwSlot;

#ifdef NSSDEBUG
  if (!pError) {
    return (NSSCKFWSession *)NULL;
  }

  *pError = nssCKFWToken_verifyPointer(fwToken);
  if( CKR_OK != *pError ) {
    return (NSSCKFWSession *)NULL;
  }
#endif 

  arena = NSSArena_Create();
  if (!arena) {
    *pError = CKR_HOST_MEMORY;
    return (NSSCKFWSession *)NULL;
  }

  fwSession = nss_ZNEW(arena, NSSCKFWSession);
  if (!fwSession) {
    *pError = CKR_HOST_MEMORY;
    goto loser;
  }

  fwSession->arena = arena;
  fwSession->mdSession = (NSSCKMDSession *)NULL; 
  fwSession->fwToken = fwToken;
  fwSession->mdToken = nssCKFWToken_GetMDToken(fwToken);

  fwSlot = nssCKFWToken_GetFWSlot(fwToken);
  fwSession->fwInstance = nssCKFWSlot_GetFWInstance(fwSlot);
  fwSession->mdInstance = nssCKFWSlot_GetMDInstance(fwSlot);

  fwSession->rw = rw;
  fwSession->pApplication = pApplication;
  fwSession->Notify = Notify;

  fwSession->fwFindObjects = (NSSCKFWFindObjects *)NULL;

  fwSession->sessionObjectHash = nssCKFWHash_Create(fwSession->fwInstance, arena, pError);
  if (!fwSession->sessionObjectHash) {
    if( CKR_OK == *pError ) {
      *pError = CKR_GENERAL_ERROR;
    }
    goto loser;
  }

#ifdef DEBUG
  *pError = session_add_pointer(fwSession);
  if( CKR_OK != *pError ) {
    goto loser;
  }
#endif 

  return fwSession;

 loser:
  if (arena) {
    if (fwSession &&   fwSession->sessionObjectHash) {
      (void)nssCKFWHash_Destroy(fwSession->sessionObjectHash);
    }
    NSSArena_Destroy(arena);
  }

  return (NSSCKFWSession *)NULL;
}

static void
nss_ckfw_session_object_destroy_iterator
(
  const void *key,
  void *value,
  void *closure
)
{
  NSSCKFWObject *fwObject = (NSSCKFWObject *)value;
  nssCKFWObject_Finalize(fwObject, PR_TRUE);
}





NSS_IMPLEMENT CK_RV
nssCKFWSession_Destroy
(
  NSSCKFWSession *fwSession,
  CK_BBOOL removeFromTokenHash
)
{
  CK_RV error = CKR_OK;
  nssCKFWHash *sessionObjectHash;
  NSSCKFWCryptoOperationState i;

#ifdef NSSDEBUG
  error = nssCKFWSession_verifyPointer(fwSession);
  if( CKR_OK != error ) {
    return error;
  }
#endif 

  if( removeFromTokenHash ) {
    error = nssCKFWToken_RemoveSession(fwSession->fwToken, fwSession);
  }

  



  sessionObjectHash = fwSession->sessionObjectHash;
  fwSession->sessionObjectHash = (nssCKFWHash *)NULL;

  nssCKFWHash_Iterate(sessionObjectHash, 
                      nss_ckfw_session_object_destroy_iterator, 
                      (void *)NULL);

  for (i=0; i < NSSCKFWCryptoOperationState_Max; i++) {
    if (fwSession->fwOperationArray[i]) {
      nssCKFWCryptoOperation_Destroy(fwSession->fwOperationArray[i]);
    }
  }

#ifdef DEBUG
  (void)session_remove_pointer(fwSession);
#endif 
  (void)nssCKFWHash_Destroy(sessionObjectHash);
  NSSArena_Destroy(fwSession->arena);

  return error;
}





NSS_IMPLEMENT NSSCKMDSession *
nssCKFWSession_GetMDSession
(
  NSSCKFWSession *fwSession
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWSession_verifyPointer(fwSession) ) {
    return (NSSCKMDSession *)NULL;
  }
#endif 

  return fwSession->mdSession;
}





NSS_IMPLEMENT NSSArena *
nssCKFWSession_GetArena
(
  NSSCKFWSession *fwSession,
  CK_RV *pError
)
{
#ifdef NSSDEBUG
  if (!pError) {
    return (NSSArena *)NULL;
  }

  *pError = nssCKFWSession_verifyPointer(fwSession);
  if( CKR_OK != *pError ) {
    return (NSSArena *)NULL;
  }
#endif 

  return fwSession->arena;
}





NSS_IMPLEMENT CK_RV
nssCKFWSession_CallNotification
(
  NSSCKFWSession *fwSession,
  CK_NOTIFICATION event
)
{
  CK_RV error = CKR_OK;
  CK_SESSION_HANDLE handle;

#ifdef NSSDEBUG
  error = nssCKFWSession_verifyPointer(fwSession);
  if( CKR_OK != error ) {
    return error;
  }
#endif 

  if( (CK_NOTIFY)NULL == fwSession->Notify ) {
    return CKR_OK;
  }

  handle = nssCKFWInstance_FindSessionHandle(fwSession->fwInstance, fwSession);
  if( (CK_SESSION_HANDLE)0 == handle ) {
    return CKR_GENERAL_ERROR;
  }

  error = fwSession->Notify(handle, event, fwSession->pApplication);

  return error;
}





NSS_IMPLEMENT CK_BBOOL
nssCKFWSession_IsRWSession
(
  NSSCKFWSession *fwSession
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWSession_verifyPointer(fwSession) ) {
    return CK_FALSE;
  }
#endif 

  return fwSession->rw;
}





NSS_IMPLEMENT CK_BBOOL
nssCKFWSession_IsSO
(
  NSSCKFWSession *fwSession
)
{
  CK_STATE state;

#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWSession_verifyPointer(fwSession) ) {
    return CK_FALSE;
  }
#endif 

  state = nssCKFWToken_GetSessionState(fwSession->fwToken);
  switch( state ) {
  case CKS_RO_PUBLIC_SESSION:
  case CKS_RO_USER_FUNCTIONS:
  case CKS_RW_PUBLIC_SESSION:
  case CKS_RW_USER_FUNCTIONS:
    return CK_FALSE;
  case CKS_RW_SO_FUNCTIONS:
    return CK_TRUE;
  default:
    return CK_FALSE;
  }
}





NSS_IMPLEMENT NSSCKFWSlot *
nssCKFWSession_GetFWSlot
(
  NSSCKFWSession *fwSession
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWSession_verifyPointer(fwSession) ) {
    return (NSSCKFWSlot *)NULL;
  }
#endif 

  return nssCKFWToken_GetFWSlot(fwSession->fwToken);
}





NSS_IMPLEMENT CK_STATE
nssCKFWSession_GetSessionState
(
  NSSCKFWSession *fwSession
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWSession_verifyPointer(fwSession) ) {
    return CKS_RO_PUBLIC_SESSION; 
  }
#endif 

  return nssCKFWToken_GetSessionState(fwSession->fwToken);
}





NSS_IMPLEMENT CK_RV
nssCKFWSession_SetFWFindObjects
(
  NSSCKFWSession *fwSession,
  NSSCKFWFindObjects *fwFindObjects
)
{
#ifdef NSSDEBUG
  CK_RV error = CKR_OK;
#endif 

#ifdef NSSDEBUG
  error = nssCKFWSession_verifyPointer(fwSession);
  if( CKR_OK != error ) {
    return error;
  }

  
#endif 

  if ((fwSession->fwFindObjects) &&
      (fwFindObjects)) {
    return CKR_OPERATION_ACTIVE;
  }

  fwSession->fwFindObjects = fwFindObjects;

  return CKR_OK;
}





NSS_IMPLEMENT NSSCKFWFindObjects *
nssCKFWSession_GetFWFindObjects
(
  NSSCKFWSession *fwSession,
  CK_RV *pError
)
{
#ifdef NSSDEBUG
  if (!pError) {
    return (NSSCKFWFindObjects *)NULL;
  }

  *pError = nssCKFWSession_verifyPointer(fwSession);
  if( CKR_OK != *pError ) {
    return (NSSCKFWFindObjects *)NULL;
  }
#endif 

  if (!fwSession->fwFindObjects) {
    *pError = CKR_OPERATION_NOT_INITIALIZED;
    return (NSSCKFWFindObjects *)NULL;
  }

  return fwSession->fwFindObjects;
}





NSS_IMPLEMENT CK_RV
nssCKFWSession_SetMDSession
(
  NSSCKFWSession *fwSession,
  NSSCKMDSession *mdSession
)
{
#ifdef NSSDEBUG
  CK_RV error = CKR_OK;
#endif 

#ifdef NSSDEBUG
  error = nssCKFWSession_verifyPointer(fwSession);
  if( CKR_OK != error ) {
    return error;
  }

  if (!mdSession) {
    return CKR_ARGUMENTS_BAD;
  }
#endif 

  if (fwSession->mdSession) {
    return CKR_GENERAL_ERROR;
  }

  fwSession->mdSession = mdSession;

  return CKR_OK;
}





NSS_IMPLEMENT CK_RV
nssCKFWSession_SetHandle
(
  NSSCKFWSession *fwSession,
  CK_SESSION_HANDLE hSession
)
{
#ifdef NSSDEBUG
  CK_RV error = CKR_OK;
#endif 

#ifdef NSSDEBUG
  error = nssCKFWSession_verifyPointer(fwSession);
  if( CKR_OK != error ) {
    return error;
  }
#endif 

  if( (CK_SESSION_HANDLE)0 != fwSession->hSession ) {
    return CKR_GENERAL_ERROR;
  }

  fwSession->hSession = hSession;

  return CKR_OK;
}





NSS_IMPLEMENT CK_SESSION_HANDLE
nssCKFWSession_GetHandle
(
  NSSCKFWSession *fwSession
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWSession_verifyPointer(fwSession) ) {
    return NULL;
  }
#endif 

  return fwSession->hSession;
}





NSS_IMPLEMENT CK_RV
nssCKFWSession_RegisterSessionObject
(
  NSSCKFWSession *fwSession,
  NSSCKFWObject *fwObject
)
{
  CK_RV rv = CKR_OK;

#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWSession_verifyPointer(fwSession) ) {
    return CKR_GENERAL_ERROR;
  }
#endif 

  if (fwSession->sessionObjectHash) {
    rv = nssCKFWHash_Add(fwSession->sessionObjectHash, fwObject, fwObject);
  }

  return rv;
}





NSS_IMPLEMENT CK_RV
nssCKFWSession_DeregisterSessionObject
(
  NSSCKFWSession *fwSession,
  NSSCKFWObject *fwObject
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWSession_verifyPointer(fwSession) ) {
    return CKR_GENERAL_ERROR;
  }
#endif 

  if (fwSession->sessionObjectHash) {
    nssCKFWHash_Remove(fwSession->sessionObjectHash, fwObject);
  }

  return CKR_OK;
}





NSS_IMPLEMENT CK_ULONG
nssCKFWSession_GetDeviceError
(
  NSSCKFWSession *fwSession
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWSession_verifyPointer(fwSession) ) {
    return (CK_ULONG)0;
  }

  if (!fwSession->mdSession) {
    return (CK_ULONG)0;
  }
#endif 

  if (!fwSession->mdSession->GetDeviceError) {
    return (CK_ULONG)0;
  }

  return fwSession->mdSession->GetDeviceError(fwSession->mdSession, 
    fwSession, fwSession->mdToken, fwSession->fwToken, 
    fwSession->mdInstance, fwSession->fwInstance);
}





NSS_IMPLEMENT CK_RV
nssCKFWSession_Login
(
  NSSCKFWSession *fwSession,
  CK_USER_TYPE userType,
  NSSItem *pin
)
{
  CK_RV error = CKR_OK;
  CK_STATE oldState;
  CK_STATE newState;

#ifdef NSSDEBUG
  error = nssCKFWSession_verifyPointer(fwSession);
  if( CKR_OK != error ) {
    return error;
  }

  switch( userType ) {
  case CKU_SO:
  case CKU_USER:
    break;
  default:
    return CKR_USER_TYPE_INVALID;
  }

  if (!pin) {
    if( CK_TRUE != nssCKFWToken_GetHasProtectedAuthenticationPath(fwSession->fwToken) ) {
      return CKR_ARGUMENTS_BAD;
    }
  }

  if (!fwSession->mdSession) {
    return CKR_GENERAL_ERROR;
  }
#endif 

  oldState = nssCKFWToken_GetSessionState(fwSession->fwToken);

  





  if( CKU_SO == userType ) {
    switch( oldState ) {
    case CKS_RO_PUBLIC_SESSION:      
      





      return CKR_SESSION_READ_ONLY_EXISTS;
    case CKS_RO_USER_FUNCTIONS:
      return CKR_USER_ANOTHER_ALREADY_LOGGED_IN;
    case CKS_RW_PUBLIC_SESSION:
      newState = CKS_RW_SO_FUNCTIONS;
      break;
    case CKS_RW_USER_FUNCTIONS:
      return CKR_USER_ANOTHER_ALREADY_LOGGED_IN;
    case CKS_RW_SO_FUNCTIONS:
      return CKR_USER_ALREADY_LOGGED_IN;
    default:
      return CKR_GENERAL_ERROR;
    }
  } else  {
    switch( oldState ) {
    case CKS_RO_PUBLIC_SESSION:      
      newState = CKS_RO_USER_FUNCTIONS;
      break;
    case CKS_RO_USER_FUNCTIONS:
      return CKR_USER_ALREADY_LOGGED_IN;
    case CKS_RW_PUBLIC_SESSION:
      newState = CKS_RW_USER_FUNCTIONS;
      break;
    case CKS_RW_USER_FUNCTIONS:
      return CKR_USER_ALREADY_LOGGED_IN;
    case CKS_RW_SO_FUNCTIONS:
      return CKR_USER_ANOTHER_ALREADY_LOGGED_IN;
    default:
      return CKR_GENERAL_ERROR;
    }
  }

  







  if (!fwSession->mdSession->Login) {
    



    ;
  } else {
    error = fwSession->mdSession->Login(fwSession->mdSession, fwSession,
      fwSession->mdToken, fwSession->fwToken, fwSession->mdInstance,
      fwSession->fwInstance, userType, pin, oldState, newState);
    if( CKR_OK != error ) {
      return error;
    }
  }

  (void)nssCKFWToken_SetSessionState(fwSession->fwToken, newState);
  return CKR_OK;
}





NSS_IMPLEMENT CK_RV
nssCKFWSession_Logout
(
  NSSCKFWSession *fwSession
)
{
  CK_RV error = CKR_OK;
  CK_STATE oldState;
  CK_STATE newState;

#ifdef NSSDEBUG
  error = nssCKFWSession_verifyPointer(fwSession);
  if( CKR_OK != error ) {
    return error;
  }

  if (!fwSession->mdSession) {
    return CKR_GENERAL_ERROR;
  }
#endif 

  oldState = nssCKFWToken_GetSessionState(fwSession->fwToken);

  switch( oldState ) {
  case CKS_RO_PUBLIC_SESSION:
    return CKR_USER_NOT_LOGGED_IN;
  case CKS_RO_USER_FUNCTIONS:
    newState = CKS_RO_PUBLIC_SESSION;
    break;
  case CKS_RW_PUBLIC_SESSION:
    return CKR_USER_NOT_LOGGED_IN;
  case CKS_RW_USER_FUNCTIONS:
    newState = CKS_RW_PUBLIC_SESSION;
    break;
  case CKS_RW_SO_FUNCTIONS:
    newState = CKS_RW_PUBLIC_SESSION;
    break;
  default:
    return CKR_GENERAL_ERROR;
  }

  







  if (!fwSession->mdSession->Logout) {
    


    ;
  } else {
    error = fwSession->mdSession->Logout(fwSession->mdSession, fwSession,
      fwSession->mdToken, fwSession->fwToken, fwSession->mdInstance,
      fwSession->fwInstance, oldState, newState);
    if( CKR_OK != error ) {
      



      ;
    }
  }

  (void)nssCKFWToken_SetSessionState(fwSession->fwToken, newState);
  return error;
}





NSS_IMPLEMENT CK_RV
nssCKFWSession_InitPIN
(
  NSSCKFWSession *fwSession,
  NSSItem *pin
)
{
  CK_RV error = CKR_OK;
  CK_STATE state;

#ifdef NSSDEBUG
  error = nssCKFWSession_verifyPointer(fwSession);
  if( CKR_OK != error ) {
    return error;
  }

  if (!fwSession->mdSession) {
    return CKR_GENERAL_ERROR;
  }
#endif 

  state = nssCKFWToken_GetSessionState(fwSession->fwToken);
  if( CKS_RW_SO_FUNCTIONS != state ) {
    return CKR_USER_NOT_LOGGED_IN;
  }

  if (!pin) {
    CK_BBOOL has = nssCKFWToken_GetHasProtectedAuthenticationPath(fwSession->fwToken);
    if( CK_TRUE != has ) {
      return CKR_ARGUMENTS_BAD;
    }
  }

  if (!fwSession->mdSession->InitPIN) {
    return CKR_TOKEN_WRITE_PROTECTED;
  }

  error = fwSession->mdSession->InitPIN(fwSession->mdSession, fwSession,
    fwSession->mdToken, fwSession->fwToken, fwSession->mdInstance,
    fwSession->fwInstance, pin);

  return error;
}





NSS_IMPLEMENT CK_RV
nssCKFWSession_SetPIN
(
  NSSCKFWSession *fwSession,
  NSSItem *newPin,
  NSSItem *oldPin
)
{
  CK_RV error = CKR_OK;

#ifdef NSSDEBUG
  error = nssCKFWSession_verifyPointer(fwSession);
  if( CKR_OK != error ) {
    return error;
  }

  if (!fwSession->mdSession) {
    return CKR_GENERAL_ERROR;
  }
#endif 

  if (!newPin) {
    CK_BBOOL has = nssCKFWToken_GetHasProtectedAuthenticationPath(fwSession->fwToken);
    if( CK_TRUE != has ) {
      return CKR_ARGUMENTS_BAD;
    }
  }

  if (!oldPin) {
    CK_BBOOL has = nssCKFWToken_GetHasProtectedAuthenticationPath(fwSession->fwToken);
    if( CK_TRUE != has ) {
      return CKR_ARGUMENTS_BAD;
    }
  }

  if (!fwSession->mdSession->SetPIN) {
    return CKR_TOKEN_WRITE_PROTECTED;
  }

  error = fwSession->mdSession->SetPIN(fwSession->mdSession, fwSession,
    fwSession->mdToken, fwSession->fwToken, fwSession->mdInstance,
    fwSession->fwInstance, newPin, oldPin);

  return error;
}





NSS_IMPLEMENT CK_ULONG
nssCKFWSession_GetOperationStateLen
(
  NSSCKFWSession *fwSession,
  CK_RV *pError
)
{
  CK_ULONG mdAmt;
  CK_ULONG fwAmt;

#ifdef NSSDEBUG
  if (!pError) {
    return (CK_ULONG)0;
  }

  *pError = nssCKFWSession_verifyPointer(fwSession);
  if( CKR_OK != *pError ) {
    return (CK_ULONG)0;
  }

  if (!fwSession->mdSession) {
    *pError = CKR_GENERAL_ERROR;
    return (CK_ULONG)0;
  }
#endif 

  if (!fwSession->mdSession->GetOperationStateLen) {
    *pError = CKR_STATE_UNSAVEABLE;
    return (CK_ULONG)0;
  }

  



  mdAmt = fwSession->mdSession->GetOperationStateLen(fwSession->mdSession,
    fwSession, fwSession->mdToken, fwSession->fwToken, fwSession->mdInstance,
    fwSession->fwInstance, pError);

  if( ((CK_ULONG)0 == mdAmt) && (CKR_OK != *pError) ) {
    return (CK_ULONG)0;
  }

  


  fwAmt = mdAmt + 2*sizeof(CK_ULONG);

  return fwAmt;
}





NSS_IMPLEMENT CK_RV
nssCKFWSession_GetOperationState
(
  NSSCKFWSession *fwSession,
  NSSItem *buffer
)
{
  CK_RV error = CKR_OK;
  CK_ULONG fwAmt;
  CK_ULONG *ulBuffer;
  NSSItem i2;
  CK_ULONG n, i;

#ifdef NSSDEBUG
  error = nssCKFWSession_verifyPointer(fwSession);
  if( CKR_OK != error ) {
    return error;
  }

  if (!buffer) {
    return CKR_ARGUMENTS_BAD;
  }

  if (!buffer->data) {
    return CKR_ARGUMENTS_BAD;
  }

  if (!fwSession->mdSession) {
    return CKR_GENERAL_ERROR;
  }
#endif 

  if (!fwSession->mdSession->GetOperationState) {
    return CKR_STATE_UNSAVEABLE;
  }

  



  error = CKR_OK;
  fwAmt = nssCKFWSession_GetOperationStateLen(fwSession, &error);
  if( ((CK_ULONG)0 == fwAmt) && (CKR_OK != error) ) {
    return error;
  }

  if( buffer->size < fwAmt ) {
    return CKR_BUFFER_TOO_SMALL;
  }

  ulBuffer = (CK_ULONG *)buffer->data;

  i2.size = buffer->size - 2*sizeof(CK_ULONG);
  i2.data = (void *)&ulBuffer[2];

  error = fwSession->mdSession->GetOperationState(fwSession->mdSession,
    fwSession, fwSession->mdToken, fwSession->fwToken, 
    fwSession->mdInstance, fwSession->fwInstance, &i2);

  if( CKR_OK != error ) {
    return error;
  }

  





  ulBuffer[0] = 0x434b4657; 
  ulBuffer[1] = 0;
  n = i2.size/sizeof(CK_ULONG);
  for( i = 0; i < n; i++ ) {
    ulBuffer[1] ^= ulBuffer[2+i];
  }

  return CKR_OK;
}





NSS_IMPLEMENT CK_RV
nssCKFWSession_SetOperationState
(
  NSSCKFWSession *fwSession,
  NSSItem *state,
  NSSCKFWObject *encryptionKey,
  NSSCKFWObject *authenticationKey
)
{
  CK_RV error = CKR_OK;
  CK_ULONG *ulBuffer;
  CK_ULONG n, i;
  CK_ULONG x;
  NSSItem s;
  NSSCKMDObject *mdek;
  NSSCKMDObject *mdak;

#ifdef NSSDEBUG
  error = nssCKFWSession_verifyPointer(fwSession);
  if( CKR_OK != error ) {
    return error;
  }

  if (!state) {
    return CKR_ARGUMENTS_BAD;
  }

  if (!state->data) {
    return CKR_ARGUMENTS_BAD;
  }

  if (encryptionKey) {
    error = nssCKFWObject_verifyPointer(encryptionKey);
    if( CKR_OK != error ) {
      return error;
    }
  }

  if (authenticationKey) {
    error = nssCKFWObject_verifyPointer(authenticationKey);
    if( CKR_OK != error ) {
      return error;
    }
  }

  if (!fwSession->mdSession) {
    return CKR_GENERAL_ERROR;
  }
#endif 

  ulBuffer = (CK_ULONG *)state->data;
  if( 0x43b4657 != ulBuffer[0] ) {
    return CKR_SAVED_STATE_INVALID;
  }
  n = (state->size / sizeof(CK_ULONG)) - 2;
  x = (CK_ULONG)0;
  for( i = 0; i < n; i++ ) {
    x ^= ulBuffer[2+i];
  }

  if( x != ulBuffer[1] ) {
    return CKR_SAVED_STATE_INVALID;
  }

  if (!fwSession->mdSession->SetOperationState) {
    return CKR_GENERAL_ERROR;
  }

  s.size = state->size - 2*sizeof(CK_ULONG);
  s.data = (void *)&ulBuffer[2];

  if (encryptionKey) {
    mdek = nssCKFWObject_GetMDObject(encryptionKey);
  } else {
    mdek = (NSSCKMDObject *)NULL;
  }

  if (authenticationKey) {
    mdak = nssCKFWObject_GetMDObject(authenticationKey);
  } else {
    mdak = (NSSCKMDObject *)NULL;
  }

  error = fwSession->mdSession->SetOperationState(fwSession->mdSession, 
    fwSession, fwSession->mdToken, fwSession->fwToken, fwSession->mdInstance,
    fwSession->fwInstance, &s, mdek, encryptionKey, mdak, authenticationKey);

  if( CKR_OK != error ) {
    return error;
  }

  


  
  return CKR_OK;
}

static CK_BBOOL
nss_attributes_form_token_object
(
  CK_ATTRIBUTE_PTR pTemplate,
  CK_ULONG ulAttributeCount
)
{
  CK_ULONG i;
  CK_BBOOL rv;

  for( i = 0; i < ulAttributeCount; i++ ) {
    if( CKA_TOKEN == pTemplate[i].type ) {
      
      if( sizeof(CK_BBOOL) == pTemplate[i].ulValueLen ) {
        (void)nsslibc_memcpy(&rv, pTemplate[i].pValue, sizeof(CK_BBOOL));
        return rv;
      } else {
        return CK_FALSE;
      }
    }
  }

  return CK_FALSE;
}





NSS_IMPLEMENT NSSCKFWObject *
nssCKFWSession_CreateObject
(
  NSSCKFWSession *fwSession,
  CK_ATTRIBUTE_PTR pTemplate,
  CK_ULONG ulAttributeCount,
  CK_RV *pError
)
{
  NSSArena *arena;
  NSSCKMDObject *mdObject;
  NSSCKFWObject *fwObject;
  CK_BBOOL isTokenObject;

#ifdef NSSDEBUG
  if (!pError) {
    return (NSSCKFWObject *)NULL;
  }

  *pError = nssCKFWSession_verifyPointer(fwSession);
  if( CKR_OK != pError ) {
    return (NSSCKFWObject *)NULL;
  }

  if( (CK_ATTRIBUTE_PTR)NULL == pTemplate ) {
    *pError = CKR_ARGUMENTS_BAD;
    return (NSSCKFWObject *)NULL;
  }

  if (!fwSession->mdSession) {
    *pError = CKR_GENERAL_ERROR;
    return (NSSCKFWObject *)NULL;
  }
#endif 

  



  isTokenObject = nss_attributes_form_token_object(pTemplate, ulAttributeCount);
  if( CK_TRUE == isTokenObject ) {
    

    if (!fwSession->mdSession->CreateObject) {
      *pError = CKR_TOKEN_WRITE_PROTECTED;
      return (NSSCKFWObject *)NULL;
    }

    arena = nssCKFWToken_GetArena(fwSession->fwToken, pError);
    if (!arena) {
      if( CKR_OK == *pError ) {
        *pError = CKR_GENERAL_ERROR;
      }
      return (NSSCKFWObject *)NULL;
    }

    goto callmdcreateobject;
  } else {
    

    arena = nssCKFWSession_GetArena(fwSession, pError);
    if (!arena) {
      if( CKR_OK == *pError ) {
        *pError = CKR_GENERAL_ERROR;
      }
      return (NSSCKFWObject *)NULL;
    }

    if( CK_TRUE == nssCKFWInstance_GetModuleHandlesSessionObjects(
                     fwSession->fwInstance) ) {
      

      if (!fwSession->mdSession->CreateObject) {
        *pError = CKR_GENERAL_ERROR;
        return (NSSCKFWObject *)NULL;
      }
      
      goto callmdcreateobject;
    } else {
      
      mdObject = nssCKMDSessionObject_Create(fwSession->fwToken, 
        arena, pTemplate, ulAttributeCount, pError);
      goto gotmdobject;
    }
  }

 callmdcreateobject:
  mdObject = fwSession->mdSession->CreateObject(fwSession->mdSession,
    fwSession, fwSession->mdToken, fwSession->fwToken,
    fwSession->mdInstance, fwSession->fwInstance, arena, pTemplate,
    ulAttributeCount, pError);

 gotmdobject:
  if (!mdObject) {
    if( CKR_OK == *pError ) {
      *pError = CKR_GENERAL_ERROR;
    }
    return (NSSCKFWObject *)NULL;
  }

  fwObject = nssCKFWObject_Create(arena, mdObject, 
    isTokenObject ? NULL : fwSession, 
    fwSession->fwToken, fwSession->fwInstance, pError);
  if (!fwObject) {
    if( CKR_OK == *pError ) {
      *pError = CKR_GENERAL_ERROR;
    }
    
    if (mdObject->Destroy) {
      (void)mdObject->Destroy(mdObject, (NSSCKFWObject *)NULL,
        fwSession->mdSession, fwSession, fwSession->mdToken,
        fwSession->fwToken, fwSession->mdInstance, fwSession->fwInstance);
    }
    
    return (NSSCKFWObject *)NULL;
  }

  if( CK_FALSE == isTokenObject ) {
    if( CK_FALSE == nssCKFWHash_Exists(fwSession->sessionObjectHash, fwObject) ) {
      *pError = nssCKFWHash_Add(fwSession->sessionObjectHash, fwObject, fwObject);
      if( CKR_OK != *pError ) {
        nssCKFWObject_Finalize(fwObject, PR_TRUE);
        return (NSSCKFWObject *)NULL;
      }
    }
  }
  
  return fwObject;
}





NSS_IMPLEMENT NSSCKFWObject *
nssCKFWSession_CopyObject
(
  NSSCKFWSession *fwSession,
  NSSCKFWObject *fwObject,
  CK_ATTRIBUTE_PTR pTemplate,
  CK_ULONG ulAttributeCount,
  CK_RV *pError
)
{
  CK_BBOOL oldIsToken;
  CK_BBOOL newIsToken;
  CK_ULONG i;
  NSSCKFWObject *rv;

#ifdef NSSDEBUG
  if (!pError) {
    return (NSSCKFWObject *)NULL;
  }

  *pError = nssCKFWSession_verifyPointer(fwSession);
  if( CKR_OK != *pError ) {
    return (NSSCKFWObject *)NULL;
  }

  *pError = nssCKFWObject_verifyPointer(fwObject);
  if( CKR_OK != *pError ) {
    return (NSSCKFWObject *)NULL;
  }

  if (!fwSession->mdSession) {
    *pError = CKR_GENERAL_ERROR;
    return (NSSCKFWObject *)NULL;
  }
#endif 

  



  if (!fwObject) {
    *pError = CKR_ARGUMENTS_BAD;
    return (NSSCKFWObject *)NULL;
  }

  oldIsToken = nssCKFWObject_IsTokenObject(fwObject);

  newIsToken = oldIsToken;
  for( i = 0; i < ulAttributeCount; i++ ) {
    if( CKA_TOKEN == pTemplate[i].type ) {
      
      (void)nsslibc_memcpy(&newIsToken, pTemplate[i].pValue, sizeof(CK_BBOOL));
      break;
    }
  }

  




  if ((fwSession->mdSession->CopyObject) &&
      (((CK_TRUE == oldIsToken) && (CK_TRUE == newIsToken)) ||
       (CK_TRUE == nssCKFWInstance_GetModuleHandlesSessionObjects(
                     fwSession->fwInstance))) ) {
    
    NSSArena *arena;
    NSSCKMDObject *mdOldObject;
    NSSCKMDObject *mdObject;

    mdOldObject = nssCKFWObject_GetMDObject(fwObject);

    if( CK_TRUE == newIsToken ) {
      arena = nssCKFWToken_GetArena(fwSession->fwToken, pError);
    } else {
      arena = nssCKFWSession_GetArena(fwSession, pError);
    }
    if (!arena) {
      if( CKR_OK == *pError ) {
        *pError = CKR_GENERAL_ERROR;
      }
      return (NSSCKFWObject *)NULL;
    }

    mdObject = fwSession->mdSession->CopyObject(fwSession->mdSession,
      fwSession, fwSession->mdToken, fwSession->fwToken,
      fwSession->mdInstance, fwSession->fwInstance, mdOldObject,
      fwObject, arena, pTemplate, ulAttributeCount, pError);
    if (!mdObject) {
      if( CKR_OK == *pError ) {
        *pError = CKR_GENERAL_ERROR;
      }
      return (NSSCKFWObject *)NULL;
    }

    rv = nssCKFWObject_Create(arena, mdObject, 
      newIsToken ? NULL : fwSession,
      fwSession->fwToken, fwSession->fwInstance, pError);

    if( CK_FALSE == newIsToken ) {
      if( CK_FALSE == nssCKFWHash_Exists(fwSession->sessionObjectHash, rv) ) {
        *pError = nssCKFWHash_Add(fwSession->sessionObjectHash, rv, rv);
        if( CKR_OK != *pError ) {
          nssCKFWObject_Finalize(rv, PR_TRUE);
          return (NSSCKFWObject *)NULL;
        }
      }
    }

    return rv;
  } else {
    
    NSSArena *tmpArena;
    CK_ATTRIBUTE_PTR newTemplate;
    CK_ULONG i, j, n, newLength, k;
    CK_ATTRIBUTE_TYPE_PTR oldTypes;
    NSSCKFWObject *rv;
    
    tmpArena = NSSArena_Create();
    if (!tmpArena) {
      *pError = CKR_HOST_MEMORY;
      return (NSSCKFWObject *)NULL;
    }

    n = nssCKFWObject_GetAttributeCount(fwObject, pError);
    if( (0 == n) && (CKR_OK != *pError) ) {
      return (NSSCKFWObject *)NULL;
    }

    oldTypes = nss_ZNEWARRAY(tmpArena, CK_ATTRIBUTE_TYPE, n);
    if( (CK_ATTRIBUTE_TYPE_PTR)NULL == oldTypes ) {
      NSSArena_Destroy(tmpArena);
      *pError = CKR_HOST_MEMORY;
      return (NSSCKFWObject *)NULL;
    }

    *pError = nssCKFWObject_GetAttributeTypes(fwObject, oldTypes, n);
    if( CKR_OK != *pError ) {
      NSSArena_Destroy(tmpArena);
      return (NSSCKFWObject *)NULL;
    }

    newLength = n;
    for( i = 0; i < ulAttributeCount; i++ ) {
      for( j = 0; j < n; j++ ) {
        if( oldTypes[j] == pTemplate[i].type ) {
          if( (CK_VOID_PTR)NULL == pTemplate[i].pValue ) {
            
            newLength--;
          }
          break;
        }
      }
      if( j == n ) {
        
        newLength++;
      }
    }

    newTemplate = nss_ZNEWARRAY(tmpArena, CK_ATTRIBUTE, newLength);
    if( (CK_ATTRIBUTE_PTR)NULL == newTemplate ) {
      NSSArena_Destroy(tmpArena);
      *pError = CKR_HOST_MEMORY;
      return (NSSCKFWObject *)NULL;
    }

    k = 0;
    for( j = 0; j < n; j++ ) {
      for( i = 0; i < ulAttributeCount; i++ ) {
        if( oldTypes[j] == pTemplate[i].type ) {
          if( (CK_VOID_PTR)NULL == pTemplate[i].pValue ) {
            
            ;
          } else {
            
            newTemplate[k].type = pTemplate[i].type;
            newTemplate[k].pValue = pTemplate[i].pValue;
            newTemplate[k].ulValueLen = pTemplate[i].ulValueLen;
            k++;
          }
          break;
        }
      }
      if( i == ulAttributeCount ) {
        
        NSSItem item, *it;
        item.size = 0;
        item.data = (void *)NULL;
        it = nssCKFWObject_GetAttribute(fwObject, oldTypes[j],
          &item, tmpArena, pError);
        if (!it) {
          if( CKR_OK == *pError ) {
            *pError = CKR_GENERAL_ERROR;
          }
          NSSArena_Destroy(tmpArena);
          return (NSSCKFWObject *)NULL;
        }
        newTemplate[k].type = oldTypes[j];
        newTemplate[k].pValue = it->data;
        newTemplate[k].ulValueLen = it->size;
        k++;
      }
    }
    

    rv = nssCKFWSession_CreateObject(fwSession, newTemplate, newLength, pError);
    if (!rv) {
      if( CKR_OK == *pError ) {
        *pError = CKR_GENERAL_ERROR;
      }
      NSSArena_Destroy(tmpArena);
      return (NSSCKFWObject *)NULL;
    }

    NSSArena_Destroy(tmpArena);
    return rv;
  }
}





NSS_IMPLEMENT NSSCKFWFindObjects *
nssCKFWSession_FindObjectsInit
(
  NSSCKFWSession *fwSession,
  CK_ATTRIBUTE_PTR pTemplate,
  CK_ULONG ulAttributeCount,
  CK_RV *pError
)
{
  NSSCKMDFindObjects *mdfo1 = (NSSCKMDFindObjects *)NULL;
  NSSCKMDFindObjects *mdfo2 = (NSSCKMDFindObjects *)NULL;

#ifdef NSSDEBUG
  if (!pError) {
    return (NSSCKFWFindObjects *)NULL;
  }

  *pError = nssCKFWSession_verifyPointer(fwSession);
  if( CKR_OK != *pError ) {
    return (NSSCKFWFindObjects *)NULL;
  }

  if( ((CK_ATTRIBUTE_PTR)NULL == pTemplate) && (ulAttributeCount != 0) ) {
    *pError = CKR_ARGUMENTS_BAD;
    return (NSSCKFWFindObjects *)NULL;
  }

  if (!fwSession->mdSession) {
    *pError = CKR_GENERAL_ERROR;
    return (NSSCKFWFindObjects *)NULL;
  }
#endif 

  if( CK_TRUE != nssCKFWInstance_GetModuleHandlesSessionObjects(
                   fwSession->fwInstance) ) {
    CK_ULONG i;

    




    for( i = 0; i < ulAttributeCount; i++ ) {
      if( CKA_TOKEN == pTemplate[i].type ) {
        
        CK_BBOOL isToken;
        if( sizeof(CK_BBOOL) != pTemplate[i].ulValueLen ) {
          *pError = CKR_ATTRIBUTE_VALUE_INVALID;
          return (NSSCKFWFindObjects *)NULL;
        }
        (void)nsslibc_memcpy(&isToken, pTemplate[i].pValue, sizeof(CK_BBOOL));

        if( CK_TRUE == isToken ) {
          
          if (!fwSession->mdSession->FindObjectsInit) {
            goto wrap;
          }

          mdfo1 = fwSession->mdSession->FindObjectsInit(fwSession->mdSession,
                    fwSession, fwSession->mdToken, fwSession->fwToken,
                    fwSession->mdInstance, fwSession->fwInstance, 
                    pTemplate, ulAttributeCount, pError);
        } else {
          
          mdfo1 = nssCKMDFindSessionObjects_Create(fwSession->fwToken, 
                    pTemplate, ulAttributeCount, pError);
        }

        if (!mdfo1) {
          if( CKR_OK == *pError ) {
            *pError = CKR_GENERAL_ERROR;
          }
          return (NSSCKFWFindObjects *)NULL;
        }
        
        goto wrap;
      }
    }

    if( i == ulAttributeCount ) {
      
      mdfo1 = fwSession->mdSession->FindObjectsInit(fwSession->mdSession,
                fwSession, fwSession->mdToken, fwSession->fwToken,
                fwSession->mdInstance, fwSession->fwInstance, 
                pTemplate, ulAttributeCount, pError);

      if (!mdfo1) {
        if( CKR_OK == *pError ) {
          *pError = CKR_GENERAL_ERROR;
        }
        return (NSSCKFWFindObjects *)NULL;
      }

      mdfo2 = nssCKMDFindSessionObjects_Create(fwSession->fwToken,
                pTemplate, ulAttributeCount, pError);
      if (!mdfo2) {
        if( CKR_OK == *pError ) {
          *pError = CKR_GENERAL_ERROR;
        }
        if (mdfo1->Final) {
          mdfo1->Final(mdfo1, (NSSCKFWFindObjects *)NULL, fwSession->mdSession,
            fwSession, fwSession->mdToken, fwSession->fwToken, 
            fwSession->mdInstance, fwSession->fwInstance);
        }
        return (NSSCKFWFindObjects *)NULL;
      }

      goto wrap;
    }
    
  } else {
    
    mdfo1 = fwSession->mdSession->FindObjectsInit(fwSession->mdSession,
              fwSession, fwSession->mdToken, fwSession->fwToken,
              fwSession->mdInstance, fwSession->fwInstance, 
              pTemplate, ulAttributeCount, pError);

    if (!mdfo1) {
      if( CKR_OK == *pError ) {
        *pError = CKR_GENERAL_ERROR;
      }
      return (NSSCKFWFindObjects *)NULL;
    }

    goto wrap;
  }

 wrap:
  return nssCKFWFindObjects_Create(fwSession, fwSession->fwToken,
           fwSession->fwInstance, mdfo1, mdfo2, pError);
}





NSS_IMPLEMENT CK_RV
nssCKFWSession_SeedRandom
(
  NSSCKFWSession *fwSession,
  NSSItem *seed
)
{
  CK_RV error = CKR_OK;

#ifdef NSSDEBUG
  error = nssCKFWSession_verifyPointer(fwSession);
  if( CKR_OK != error ) {
    return error;
  }

  if (!seed) {
    return CKR_ARGUMENTS_BAD;
  }

  if (!seed->data) {
    return CKR_ARGUMENTS_BAD;
  }

  if( 0 == seed->size ) {
    return CKR_ARGUMENTS_BAD;
  }

  if (!fwSession->mdSession) {
    return CKR_GENERAL_ERROR;
  }
#endif 

  if (!fwSession->mdSession->SeedRandom) {
    return CKR_RANDOM_SEED_NOT_SUPPORTED;
  }

  error = fwSession->mdSession->SeedRandom(fwSession->mdSession, fwSession,
    fwSession->mdToken, fwSession->fwToken, fwSession->mdInstance,
    fwSession->fwInstance, seed);

  return error;
}





NSS_IMPLEMENT CK_RV
nssCKFWSession_GetRandom
(
  NSSCKFWSession *fwSession,
  NSSItem *buffer
)
{
  CK_RV error = CKR_OK;

#ifdef NSSDEBUG
  error = nssCKFWSession_verifyPointer(fwSession);
  if( CKR_OK != error ) {
    return error;
  }

  if (!buffer) {
    return CKR_ARGUMENTS_BAD;
  }

  if (!buffer->data) {
    return CKR_ARGUMENTS_BAD;
  }

  if (!fwSession->mdSession) {
    return CKR_GENERAL_ERROR;
  }
#endif 

  if (!fwSession->mdSession->GetRandom) {
    if( CK_TRUE == nssCKFWToken_GetHasRNG(fwSession->fwToken) ) {
      return CKR_GENERAL_ERROR;
    } else {
      return CKR_RANDOM_NO_RNG;
    }
  }

  if( 0 == buffer->size ) {
    return CKR_OK;
  }

  error = fwSession->mdSession->GetRandom(fwSession->mdSession, fwSession,
    fwSession->mdToken, fwSession->fwToken, fwSession->mdInstance,
    fwSession->fwInstance, buffer);

  return error;
}





NSS_IMPLEMENT void
nssCKFWSession_SetCurrentCryptoOperation
(
  NSSCKFWSession *fwSession,
  NSSCKFWCryptoOperation * fwOperation,
  NSSCKFWCryptoOperationState state
)
{
#ifdef NSSDEBUG
  CK_RV error = CKR_OK;
  error = nssCKFWSession_verifyPointer(fwSession);
  if( CKR_OK != error ) {
    return;
  }

  if ( state >= NSSCKFWCryptoOperationState_Max) {
    return;
  }

  if (!fwSession->mdSession) {
    return;
  }
#endif 
  fwSession->fwOperationArray[state] = fwOperation;
  return;
}




NSS_IMPLEMENT NSSCKFWCryptoOperation *
nssCKFWSession_GetCurrentCryptoOperation
(
  NSSCKFWSession *fwSession,
  NSSCKFWCryptoOperationState state
)
{
#ifdef NSSDEBUG
  CK_RV error = CKR_OK;
  error = nssCKFWSession_verifyPointer(fwSession);
  if( CKR_OK != error ) {
    return (NSSCKFWCryptoOperation *)NULL;
  }

  if ( state >= NSSCKFWCryptoOperationState_Max) {
    return (NSSCKFWCryptoOperation *)NULL;
  }

  if (!fwSession->mdSession) {
    return (NSSCKFWCryptoOperation *)NULL;
  }
#endif 
  return fwSession->fwOperationArray[state];
}




NSS_IMPLEMENT CK_RV
nssCKFWSession_Final
(
  NSSCKFWSession *fwSession,
  NSSCKFWCryptoOperationType type,
  NSSCKFWCryptoOperationState state,
  CK_BYTE_PTR  outBuf,
  CK_ULONG_PTR outBufLen
)
{
  NSSCKFWCryptoOperation *fwOperation;
  NSSItem outputBuffer;
  CK_RV error = CKR_OK;

#ifdef NSSDEBUG
  error = nssCKFWSession_verifyPointer(fwSession);
  if( CKR_OK != error ) {
    return error;
  }

  if (!fwSession->mdSession) {
    return CKR_GENERAL_ERROR;
  }
#endif 

  
  fwOperation = nssCKFWSession_GetCurrentCryptoOperation(fwSession, state);
  if (!fwOperation) {
    return CKR_OPERATION_NOT_INITIALIZED;
  }

  
  if (type != nssCKFWCryptoOperation_GetType(fwOperation)) {
    return CKR_OPERATION_NOT_INITIALIZED;
  }

  
  if (NSSCKFWCryptoOperationType_Verify == type) {
    if ((CK_BYTE_PTR)NULL == outBuf) {
      error = CKR_ARGUMENTS_BAD;
      goto done;
    }
  } else {
    CK_ULONG len = nssCKFWCryptoOperation_GetFinalLength(fwOperation, &error);
    CK_ULONG maxBufLen = *outBufLen;

    if (CKR_OK != error) {
       goto done;
    }
    *outBufLen = len;
    if ((CK_BYTE_PTR)NULL == outBuf) {
      return CKR_OK;
    }

    if (len > maxBufLen) {
      return CKR_BUFFER_TOO_SMALL;
    }
  }
  outputBuffer.data = outBuf;
  outputBuffer.size = *outBufLen;

  error = nssCKFWCryptoOperation_Final(fwOperation, &outputBuffer);
done:
  if (CKR_BUFFER_TOO_SMALL == error) {
    return error;
  }
  
  nssCKFWCryptoOperation_Destroy(fwOperation);
  nssCKFWSession_SetCurrentCryptoOperation(fwSession, NULL, state);
  return error;
}




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
)
{
  NSSCKFWCryptoOperation *fwOperation;
  NSSItem inputBuffer;
  NSSItem outputBuffer;
  CK_ULONG len;
  CK_ULONG maxBufLen;
  CK_RV error = CKR_OK;

#ifdef NSSDEBUG
  error = nssCKFWSession_verifyPointer(fwSession);
  if( CKR_OK != error ) {
    return error;
  }

  if (!fwSession->mdSession) {
    return CKR_GENERAL_ERROR;
  }
#endif 

  
  fwOperation = nssCKFWSession_GetCurrentCryptoOperation(fwSession, state);
  if (!fwOperation) {
    return CKR_OPERATION_NOT_INITIALIZED;
  }

  
  if (type != nssCKFWCryptoOperation_GetType(fwOperation)) {
    return CKR_OPERATION_NOT_INITIALIZED;
  }

  inputBuffer.data = inBuf;
  inputBuffer.size = inBufLen;

  
  len = nssCKFWCryptoOperation_GetOperationLength(fwOperation, &inputBuffer, 
                                                  &error);
  if (CKR_OK != error) {
    return error;
  }
  maxBufLen = *outBufLen;

  *outBufLen = len;
  if ((CK_BYTE_PTR)NULL == outBuf) {
    return CKR_OK;
  }

  if (len > maxBufLen) {
    return CKR_BUFFER_TOO_SMALL;
  }
  outputBuffer.data = outBuf;
  outputBuffer.size = *outBufLen;

  return nssCKFWCryptoOperation_Update(fwOperation,
                                       &inputBuffer, &outputBuffer);
}




NSS_IMPLEMENT CK_RV
nssCKFWSession_DigestUpdate
(
  NSSCKFWSession *fwSession,
  NSSCKFWCryptoOperationType type,
  NSSCKFWCryptoOperationState state,
  CK_BYTE_PTR  inBuf,
  CK_ULONG     inBufLen
)
{
  NSSCKFWCryptoOperation *fwOperation;
  NSSItem inputBuffer;
  CK_RV error = CKR_OK;

#ifdef NSSDEBUG
  error = nssCKFWSession_verifyPointer(fwSession);
  if( CKR_OK != error ) {
    return error;
  }

  if (!fwSession->mdSession) {
    return CKR_GENERAL_ERROR;
  }
#endif 

  
  fwOperation = nssCKFWSession_GetCurrentCryptoOperation(fwSession, state);
  if (!fwOperation) {
    return CKR_OPERATION_NOT_INITIALIZED;
  }

  
  if (type != nssCKFWCryptoOperation_GetType(fwOperation)) {
    return CKR_OPERATION_NOT_INITIALIZED;
  }

  inputBuffer.data = inBuf;
  inputBuffer.size = inBufLen;


  error = nssCKFWCryptoOperation_DigestUpdate(fwOperation, &inputBuffer);
  return error;
}




NSS_IMPLEMENT CK_RV
nssCKFWSession_DigestKey
(
  NSSCKFWSession *fwSession,
  NSSCKFWObject *fwKey
)
{
  NSSCKFWCryptoOperation *fwOperation;
  NSSItem *inputBuffer;
  CK_RV error = CKR_OK;

#ifdef NSSDEBUG
  error = nssCKFWSession_verifyPointer(fwSession);
  if( CKR_OK != error ) {
    return error;
  }

  if (!fwSession->mdSession) {
    return CKR_GENERAL_ERROR;
  }
#endif 

  
  fwOperation = nssCKFWSession_GetCurrentCryptoOperation(fwSession, 
                                 NSSCKFWCryptoOperationState_Digest);
  if (!fwOperation) {
    return CKR_OPERATION_NOT_INITIALIZED;
  }

  
  if (NSSCKFWCryptoOperationType_Digest != 
      nssCKFWCryptoOperation_GetType(fwOperation)) {
    return CKR_OPERATION_NOT_INITIALIZED;
  }

  error = nssCKFWCryptoOperation_DigestKey(fwOperation, fwKey);
  if (CKR_FUNCTION_FAILED != error) {
    return error;
  }

  
  inputBuffer=nssCKFWObject_GetAttribute(fwKey, CKA_VALUE, NULL, NULL, &error);
  if (!inputBuffer) {
    
    return error;
  }
  error = nssCKFWCryptoOperation_DigestUpdate(fwOperation, inputBuffer);
  nssItem_Destroy(inputBuffer);
  return error;
}




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
)
{
  NSSCKFWCryptoOperation *fwOperation;
  NSSItem inputBuffer;
  NSSItem outputBuffer;
  PRBool isEncryptDecrypt;
  CK_RV error = CKR_OK;

#ifdef NSSDEBUG
  error = nssCKFWSession_verifyPointer(fwSession);
  if( CKR_OK != error ) {
    return error;
  }

  if (!fwSession->mdSession) {
    return CKR_GENERAL_ERROR;
  }
#endif 

  
  fwOperation = nssCKFWSession_GetCurrentCryptoOperation(fwSession, state);
  if (!fwOperation) {
    return CKR_OPERATION_NOT_INITIALIZED;
  }

  
  if (type != nssCKFWCryptoOperation_GetType(fwOperation)) {
    return CKR_OPERATION_NOT_INITIALIZED;
  }

  inputBuffer.data = inBuf;
  inputBuffer.size = inBufLen;
  isEncryptDecrypt = (PRBool) ((NSSCKFWCryptoOperationType_Encrypt == type) || 
                               (NSSCKFWCryptoOperationType_Decrypt == type)) ;

  
  if (NSSCKFWCryptoOperationType_Verify == type) {
    if ((CK_BYTE_PTR)NULL == outBuf) {
      error = CKR_ARGUMENTS_BAD;
      goto done;
    }
  } else {
    CK_ULONG maxBufLen = *outBufLen;
    CK_ULONG len;

    len = (isEncryptDecrypt) ?
      nssCKFWCryptoOperation_GetOperationLength(fwOperation, 
                                                &inputBuffer, &error) :
      nssCKFWCryptoOperation_GetFinalLength(fwOperation, &error);

    if (CKR_OK != error) {
      goto done;
    }

    *outBufLen = len;
    if ((CK_BYTE_PTR)NULL == outBuf) {
      return CKR_OK;
    }

    if (len > maxBufLen) {
      return CKR_BUFFER_TOO_SMALL;
    }
  }
  outputBuffer.data = outBuf;
  outputBuffer.size = *outBufLen;

  error = nssCKFWCryptoOperation_UpdateFinal(fwOperation, 
                                             &inputBuffer, &outputBuffer);

  
  if (CKR_FUNCTION_FAILED == error) {
    error = isEncryptDecrypt ? 
      nssCKFWCryptoOperation_Update(fwOperation, &inputBuffer, &outputBuffer) :
      nssCKFWCryptoOperation_DigestUpdate(fwOperation, &inputBuffer);

    if (CKR_OK == error) {
      error = nssCKFWCryptoOperation_Final(fwOperation, &outputBuffer);
    }
  }


done:
  if (CKR_BUFFER_TOO_SMALL == error) {
    

    return error;
  }

  
  nssCKFWCryptoOperation_Destroy(fwOperation);
  nssCKFWSession_SetCurrentCryptoOperation(fwSession, NULL, state);
  return error;
}

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
)
{
  NSSCKFWCryptoOperation *fwOperation;
  NSSCKFWCryptoOperation *fwPeerOperation;
  NSSItem inputBuffer;
  NSSItem outputBuffer;
  CK_ULONG maxBufLen = *outBufLen;
  CK_ULONG len;
  CK_RV error = CKR_OK;

#ifdef NSSDEBUG
  error = nssCKFWSession_verifyPointer(fwSession);
  if( CKR_OK != error ) {
    return error;
  }

  if (!fwSession->mdSession) {
    return CKR_GENERAL_ERROR;
  }
#endif 

  
  fwOperation = nssCKFWSession_GetCurrentCryptoOperation(fwSession, 
                NSSCKFWCryptoOperationState_EncryptDecrypt);
  if (!fwOperation) {
    return CKR_OPERATION_NOT_INITIALIZED;
  }

  
  if (encryptType != nssCKFWCryptoOperation_GetType(fwOperation)) {
    return CKR_OPERATION_NOT_INITIALIZED;
  }
  
  fwPeerOperation = nssCKFWSession_GetCurrentCryptoOperation(fwSession, 
                  digestState);
  if (!fwPeerOperation) {
    return CKR_OPERATION_NOT_INITIALIZED;
  }

  
  if (digestType != nssCKFWCryptoOperation_GetType(fwOperation)) {
    return CKR_OPERATION_NOT_INITIALIZED;
  }

  inputBuffer.data = inBuf;
  inputBuffer.size = inBufLen;
  len = nssCKFWCryptoOperation_GetOperationLength(fwOperation, 
                                                &inputBuffer, &error);
  if (CKR_OK != error) {
    return error;
  }

  *outBufLen = len;
  if ((CK_BYTE_PTR)NULL == outBuf) {
    return CKR_OK;
  }

  if (len > maxBufLen) {
    return CKR_BUFFER_TOO_SMALL;
  }

  outputBuffer.data = outBuf;
  outputBuffer.size = *outBufLen;

  error = nssCKFWCryptoOperation_UpdateCombo(fwOperation, fwPeerOperation,
                                             &inputBuffer, &outputBuffer);
  if (CKR_FUNCTION_FAILED == error) {
    PRBool isEncrypt = 
           (PRBool) (NSSCKFWCryptoOperationType_Encrypt == encryptType);

    if (isEncrypt) {
      error = nssCKFWCryptoOperation_DigestUpdate(fwPeerOperation, 
                                                  &inputBuffer);
      if (CKR_OK != error) {
        return error;
      }
    }
    error = nssCKFWCryptoOperation_Update(fwOperation, 
                                          &inputBuffer, &outputBuffer);
    if (CKR_OK != error) {
      return error;
    }
    if (!isEncrypt) {
      error = nssCKFWCryptoOperation_DigestUpdate(fwPeerOperation,
                                                  &outputBuffer);
    }
  }
  return error;
}







NSS_IMPLEMENT NSSCKMDSession *
NSSCKFWSession_GetMDSession
(
  NSSCKFWSession *fwSession
)
{
#ifdef DEBUG
  if( CKR_OK != nssCKFWSession_verifyPointer(fwSession) ) {
    return (NSSCKMDSession *)NULL;
  }
#endif 

  return nssCKFWSession_GetMDSession(fwSession);
}






NSS_IMPLEMENT NSSArena *
NSSCKFWSession_GetArena
(
  NSSCKFWSession *fwSession,
  CK_RV *pError
)
{
#ifdef DEBUG
  if (!pError) {
    return (NSSArena *)NULL;
  }

  *pError = nssCKFWSession_verifyPointer(fwSession);
  if( CKR_OK != *pError ) {
    return (NSSArena *)NULL;
  }
#endif 

  return nssCKFWSession_GetArena(fwSession, pError);
}






NSS_IMPLEMENT CK_RV
NSSCKFWSession_CallNotification
(
  NSSCKFWSession *fwSession,
  CK_NOTIFICATION event
)
{
#ifdef DEBUG
  CK_RV error = CKR_OK;

  error = nssCKFWSession_verifyPointer(fwSession);
  if( CKR_OK != error ) {
    return error;
  }
#endif 

  return nssCKFWSession_CallNotification(fwSession, event);
}






NSS_IMPLEMENT CK_BBOOL
NSSCKFWSession_IsRWSession
(
  NSSCKFWSession *fwSession
)
{
#ifdef DEBUG
  if( CKR_OK != nssCKFWSession_verifyPointer(fwSession) ) {
    return CK_FALSE;
  }
#endif 

  return nssCKFWSession_IsRWSession(fwSession);
}






NSS_IMPLEMENT CK_BBOOL
NSSCKFWSession_IsSO
(
  NSSCKFWSession *fwSession
)
{
#ifdef DEBUG
  if( CKR_OK != nssCKFWSession_verifyPointer(fwSession) ) {
    return CK_FALSE;
  }
#endif 

  return nssCKFWSession_IsSO(fwSession);
}

NSS_IMPLEMENT NSSCKFWCryptoOperation *
NSSCKFWSession_GetCurrentCryptoOperation
(
  NSSCKFWSession *fwSession,
  NSSCKFWCryptoOperationState state
)
{
#ifdef DEBUG
  CK_RV error = CKR_OK;
  error = nssCKFWSession_verifyPointer(fwSession);
  if( CKR_OK != error ) {
    return (NSSCKFWCryptoOperation *)NULL;
  }

  if ( state >= NSSCKFWCryptoOperationState_Max) {
    return (NSSCKFWCryptoOperation *)NULL;
  }
#endif 
  return nssCKFWSession_GetCurrentCryptoOperation(fwSession, state);
}
