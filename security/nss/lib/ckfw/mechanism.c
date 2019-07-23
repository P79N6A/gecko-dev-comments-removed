



































#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: mechanism.c,v $ $Revision: 1.7 $ $Date: 2009/04/29 00:25:32 $";
#endif 







#ifndef CK_T
#include "ck.h"
#endif 













































struct NSSCKFWMechanismStr {
   NSSCKMDMechanism *mdMechanism;
   NSSCKMDToken *mdToken;
   NSSCKFWToken *fwToken;
   NSSCKMDInstance *mdInstance;
   NSSCKFWInstance *fwInstance;
};





NSS_IMPLEMENT NSSCKFWMechanism *
nssCKFWMechanism_Create
(
  NSSCKMDMechanism *mdMechanism,
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance
)
{
  NSSCKFWMechanism *fwMechanism;


  fwMechanism = nss_ZNEW(NULL, NSSCKFWMechanism);
  if (!fwMechanism) {
    return (NSSCKFWMechanism *)NULL;
  }
  fwMechanism->mdMechanism = mdMechanism;
  fwMechanism->mdToken = mdToken;
  fwMechanism->fwToken = fwToken;
  fwMechanism->mdInstance = mdInstance;
  fwMechanism->fwInstance = fwInstance;
  return fwMechanism;
}





NSS_IMPLEMENT void
nssCKFWMechanism_Destroy
(
  NSSCKFWMechanism *fwMechanism
)
{
  

  if (!fwMechanism->mdMechanism->Destroy) {
    
    fwMechanism->mdMechanism->Destroy(
        fwMechanism->mdMechanism, 
        fwMechanism,
        fwMechanism->mdInstance,
        fwMechanism->fwInstance);
  }
  

  return;
}





NSS_IMPLEMENT NSSCKMDMechanism *
nssCKFWMechanism_GetMDMechanism
(
  NSSCKFWMechanism *fwMechanism
)
{
  return fwMechanism->mdMechanism;
}





NSS_IMPLEMENT CK_ULONG
nssCKFWMechanism_GetMinKeySize
(
  NSSCKFWMechanism *fwMechanism,
  CK_RV *pError
)
{
  if (!fwMechanism->mdMechanism->GetMinKeySize) {
    return 0;
  }

  return fwMechanism->mdMechanism->GetMinKeySize(fwMechanism->mdMechanism,
    fwMechanism, fwMechanism->mdToken, fwMechanism->fwToken, 
    fwMechanism->mdInstance, fwMechanism->fwInstance, pError);
}





NSS_IMPLEMENT CK_ULONG
nssCKFWMechanism_GetMaxKeySize
(
  NSSCKFWMechanism *fwMechanism,
  CK_RV *pError
)
{
  if (!fwMechanism->mdMechanism->GetMaxKeySize) {
    return 0;
  }

  return fwMechanism->mdMechanism->GetMaxKeySize(fwMechanism->mdMechanism,
    fwMechanism, fwMechanism->mdToken, fwMechanism->fwToken, 
    fwMechanism->mdInstance, fwMechanism->fwInstance, pError);
}





NSS_IMPLEMENT CK_BBOOL
nssCKFWMechanism_GetInHardware
(
  NSSCKFWMechanism *fwMechanism,
  CK_RV *pError
)
{
  if (!fwMechanism->mdMechanism->GetInHardware) {
    return CK_FALSE;
  }

  return fwMechanism->mdMechanism->GetInHardware(fwMechanism->mdMechanism,
    fwMechanism, fwMechanism->mdToken, fwMechanism->fwToken, 
    fwMechanism->mdInstance, fwMechanism->fwInstance, pError);
}










NSS_EXTERN CK_BBOOL
nssCKFWMechanism_GetCanEncrypt
(
  NSSCKFWMechanism *fwMechanism,
  CK_RV *pError
)
{
  if (!fwMechanism->mdMechanism->EncryptInit) {
    return CK_FALSE;
  }
  return CK_TRUE;
}





NSS_EXTERN CK_BBOOL
nssCKFWMechanism_GetCanDecrypt
(
  NSSCKFWMechanism *fwMechanism,
  CK_RV *pError
)
{
  if (!fwMechanism->mdMechanism->DecryptInit) {
    return CK_FALSE;
  }
  return CK_TRUE;
}





NSS_EXTERN CK_BBOOL
nssCKFWMechanism_GetCanDigest
(
  NSSCKFWMechanism *fwMechanism,
  CK_RV *pError
)
{
  if (!fwMechanism->mdMechanism->DigestInit) {
    return CK_FALSE;
  }
  return CK_TRUE;
}





NSS_EXTERN CK_BBOOL
nssCKFWMechanism_GetCanSign
(
  NSSCKFWMechanism *fwMechanism,
  CK_RV *pError
)
{
  if (!fwMechanism->mdMechanism->SignInit) {
    return CK_FALSE;
  }
  return CK_TRUE;
}





NSS_EXTERN CK_BBOOL
nssCKFWMechanism_GetCanSignRecover
(
  NSSCKFWMechanism *fwMechanism,
  CK_RV *pError
)
{
  if (!fwMechanism->mdMechanism->SignRecoverInit) {
    return CK_FALSE;
  }
  return CK_TRUE;
}





NSS_EXTERN CK_BBOOL
nssCKFWMechanism_GetCanVerify
(
  NSSCKFWMechanism *fwMechanism,
  CK_RV *pError
)
{
  if (!fwMechanism->mdMechanism->VerifyInit) {
    return CK_FALSE;
  }
  return CK_TRUE;
}





NSS_EXTERN CK_BBOOL
nssCKFWMechanism_GetCanVerifyRecover
(
  NSSCKFWMechanism *fwMechanism,
  CK_RV *pError
)
{
  if (!fwMechanism->mdMechanism->VerifyRecoverInit) {
    return CK_FALSE;
  }
  return CK_TRUE;
}





NSS_EXTERN CK_BBOOL
nssCKFWMechanism_GetCanGenerate
(
  NSSCKFWMechanism *fwMechanism,
  CK_RV *pError
)
{
  if (!fwMechanism->mdMechanism->GenerateKey) {
    return CK_FALSE;
  }
  return CK_TRUE;
}





NSS_EXTERN CK_BBOOL
nssCKFWMechanism_GetCanGenerateKeyPair
(
  NSSCKFWMechanism *fwMechanism,
  CK_RV *pError
)
{
  if (!fwMechanism->mdMechanism->GenerateKeyPair) {
    return CK_FALSE;
  }
  return CK_TRUE;
}





NSS_EXTERN CK_BBOOL
nssCKFWMechanism_GetCanUnwrap
(
  NSSCKFWMechanism *fwMechanism,
  CK_RV *pError
)
{
  if (!fwMechanism->mdMechanism->UnwrapKey) {
    return CK_FALSE;
  }
  return CK_TRUE;
}





NSS_EXTERN CK_BBOOL
nssCKFWMechanism_GetCanWrap
(
  NSSCKFWMechanism *fwMechanism,
  CK_RV *pError
)
{
  if (!fwMechanism->mdMechanism->WrapKey) {
    return CK_FALSE;
  }
  return CK_TRUE;
}





NSS_EXTERN CK_BBOOL
nssCKFWMechanism_GetCanDerive
(
  NSSCKFWMechanism *fwMechanism,
  CK_RV *pError
)
{
  if (!fwMechanism->mdMechanism->DeriveKey) {
    return CK_FALSE;
  }
  return CK_TRUE;
}









NSS_EXTERN CK_RV
nssCKFWMechanism_EncryptInit
(
  NSSCKFWMechanism *fwMechanism,
  CK_MECHANISM     *pMechanism,
  NSSCKFWSession   *fwSession,
  NSSCKFWObject    *fwObject
)
{
  NSSCKFWCryptoOperation *fwOperation;
  NSSCKMDCryptoOperation *mdOperation;
  NSSCKMDSession *mdSession;
  NSSCKMDObject  *mdObject;
  CK_RV  error = CKR_OK;


  fwOperation = nssCKFWSession_GetCurrentCryptoOperation(fwSession, 
                        NSSCKFWCryptoOperationState_EncryptDecrypt);
  if (fwOperation) {
    return CKR_OPERATION_ACTIVE;
  }

  if (!fwMechanism->mdMechanism->EncryptInit) {
    return CKR_FUNCTION_FAILED;
  }

  mdSession = nssCKFWSession_GetMDSession(fwSession);
  mdObject = nssCKFWObject_GetMDObject(fwObject);
  mdOperation = fwMechanism->mdMechanism->EncryptInit(
        fwMechanism->mdMechanism,
        fwMechanism,
        pMechanism,
        mdSession,
        fwSession,
        fwMechanism->mdToken,
        fwMechanism->fwToken,
        fwMechanism->mdInstance,
        fwMechanism->fwInstance,
        mdObject,
        fwObject,
        &error
  );
  if (!mdOperation) {
    goto loser;
  }

  fwOperation = nssCKFWCryptoOperation_Create(mdOperation, 
        mdSession, fwSession, fwMechanism->mdToken, fwMechanism->fwToken,
        fwMechanism->mdInstance, fwMechanism->fwInstance,
        NSSCKFWCryptoOperationType_Encrypt, &error);
  if (fwOperation) {
    nssCKFWSession_SetCurrentCryptoOperation(fwSession, fwOperation,
                NSSCKFWCryptoOperationState_EncryptDecrypt);
  }

loser:
  return error;
}





NSS_EXTERN CK_RV
nssCKFWMechanism_DecryptInit
(
  NSSCKFWMechanism *fwMechanism,
  CK_MECHANISM     *pMechanism,
  NSSCKFWSession   *fwSession,
  NSSCKFWObject    *fwObject
)
{
  NSSCKFWCryptoOperation *fwOperation;
  NSSCKMDCryptoOperation *mdOperation;
  NSSCKMDSession *mdSession;
  NSSCKMDObject  *mdObject;
  CK_RV  error = CKR_OK;


  fwOperation = nssCKFWSession_GetCurrentCryptoOperation(fwSession, 
                        NSSCKFWCryptoOperationState_EncryptDecrypt);
  if (fwOperation) {
    return CKR_OPERATION_ACTIVE;
  }

  if (!fwMechanism->mdMechanism->DecryptInit) {
    return CKR_FUNCTION_FAILED;
  }

  mdSession = nssCKFWSession_GetMDSession(fwSession);
  mdObject = nssCKFWObject_GetMDObject(fwObject);
  mdOperation = fwMechanism->mdMechanism->DecryptInit(
        fwMechanism->mdMechanism,
        fwMechanism,
        pMechanism,
        mdSession,
        fwSession,
        fwMechanism->mdToken,
        fwMechanism->fwToken,
        fwMechanism->mdInstance,
        fwMechanism->fwInstance,
        mdObject,
        fwObject,
        &error
  );
  if (!mdOperation) {
    goto loser;
  }

  fwOperation = nssCKFWCryptoOperation_Create(mdOperation, 
        mdSession, fwSession, fwMechanism->mdToken, fwMechanism->fwToken,
        fwMechanism->mdInstance, fwMechanism->fwInstance,
        NSSCKFWCryptoOperationType_Decrypt, &error);
  if (fwOperation) {
    nssCKFWSession_SetCurrentCryptoOperation(fwSession, fwOperation,
                NSSCKFWCryptoOperationState_EncryptDecrypt);
  }

loser:
  return error;
}





NSS_EXTERN CK_RV
nssCKFWMechanism_DigestInit
(
  NSSCKFWMechanism *fwMechanism,
  CK_MECHANISM     *pMechanism,
  NSSCKFWSession   *fwSession
)
{
  NSSCKFWCryptoOperation *fwOperation;
  NSSCKMDCryptoOperation *mdOperation;
  NSSCKMDSession *mdSession;
  CK_RV  error = CKR_OK;


  fwOperation = nssCKFWSession_GetCurrentCryptoOperation(fwSession, 
                        NSSCKFWCryptoOperationState_Digest);
  if (fwOperation) {
    return CKR_OPERATION_ACTIVE;
  }

  if (!fwMechanism->mdMechanism->DigestInit) {
    return CKR_FUNCTION_FAILED;
  }

  mdSession = nssCKFWSession_GetMDSession(fwSession);
  mdOperation = fwMechanism->mdMechanism->DigestInit(
        fwMechanism->mdMechanism,
        fwMechanism,
        pMechanism,
        mdSession,
        fwSession,
        fwMechanism->mdToken,
        fwMechanism->fwToken,
        fwMechanism->mdInstance,
        fwMechanism->fwInstance,
        &error
  );
  if (!mdOperation) {
    goto loser;
  }

  fwOperation = nssCKFWCryptoOperation_Create(mdOperation, 
        mdSession, fwSession, fwMechanism->mdToken, fwMechanism->fwToken,
        fwMechanism->mdInstance, fwMechanism->fwInstance,
        NSSCKFWCryptoOperationType_Digest, &error);
  if (fwOperation) {
    nssCKFWSession_SetCurrentCryptoOperation(fwSession, fwOperation,
                NSSCKFWCryptoOperationState_Digest);
  }

loser:
  return error;
}





NSS_EXTERN CK_RV
nssCKFWMechanism_SignInit
(
  NSSCKFWMechanism *fwMechanism,
  CK_MECHANISM     *pMechanism,
  NSSCKFWSession   *fwSession,
  NSSCKFWObject    *fwObject
)
{
  NSSCKFWCryptoOperation *fwOperation;
  NSSCKMDCryptoOperation *mdOperation;
  NSSCKMDSession *mdSession;
  NSSCKMDObject  *mdObject;
  CK_RV  error = CKR_OK;


  fwOperation = nssCKFWSession_GetCurrentCryptoOperation(fwSession, 
                        NSSCKFWCryptoOperationState_SignVerify);
  if (fwOperation) {
    return CKR_OPERATION_ACTIVE;
  }

  if (!fwMechanism->mdMechanism->SignInit) {
    return CKR_FUNCTION_FAILED;
  }

  mdSession = nssCKFWSession_GetMDSession(fwSession);
  mdObject = nssCKFWObject_GetMDObject(fwObject);
  mdOperation = fwMechanism->mdMechanism->SignInit(
        fwMechanism->mdMechanism,
        fwMechanism,
        pMechanism,
        mdSession,
        fwSession,
        fwMechanism->mdToken,
        fwMechanism->fwToken,
        fwMechanism->mdInstance,
        fwMechanism->fwInstance,
        mdObject,
        fwObject,
        &error
  );
  if (!mdOperation) {
    goto loser;
  }

  fwOperation = nssCKFWCryptoOperation_Create(mdOperation, 
        mdSession, fwSession, fwMechanism->mdToken, fwMechanism->fwToken,
        fwMechanism->mdInstance, fwMechanism->fwInstance,
        NSSCKFWCryptoOperationType_Sign, &error);
  if (fwOperation) {
    nssCKFWSession_SetCurrentCryptoOperation(fwSession, fwOperation,
                NSSCKFWCryptoOperationState_SignVerify);
  }

loser:
  return error;
}





NSS_EXTERN CK_RV
nssCKFWMechanism_VerifyInit
(
  NSSCKFWMechanism *fwMechanism,
  CK_MECHANISM     *pMechanism,
  NSSCKFWSession   *fwSession,
  NSSCKFWObject    *fwObject
)
{
  NSSCKFWCryptoOperation *fwOperation;
  NSSCKMDCryptoOperation *mdOperation;
  NSSCKMDSession *mdSession;
  NSSCKMDObject  *mdObject;
  CK_RV  error = CKR_OK;


  fwOperation = nssCKFWSession_GetCurrentCryptoOperation(fwSession, 
                        NSSCKFWCryptoOperationState_SignVerify);
  if (fwOperation) {
    return CKR_OPERATION_ACTIVE;
  }

  if (!fwMechanism->mdMechanism->VerifyInit) {
    return CKR_FUNCTION_FAILED;
  }

  mdSession = nssCKFWSession_GetMDSession(fwSession);
  mdObject = nssCKFWObject_GetMDObject(fwObject);
  mdOperation = fwMechanism->mdMechanism->VerifyInit(
        fwMechanism->mdMechanism,
        fwMechanism,
        pMechanism,
        mdSession,
        fwSession,
        fwMechanism->mdToken,
        fwMechanism->fwToken,
        fwMechanism->mdInstance,
        fwMechanism->fwInstance,
        mdObject,
        fwObject,
        &error
  );
  if (!mdOperation) {
    goto loser;
  }

  fwOperation = nssCKFWCryptoOperation_Create(mdOperation, 
        mdSession, fwSession, fwMechanism->mdToken, fwMechanism->fwToken,
        fwMechanism->mdInstance, fwMechanism->fwInstance,
        NSSCKFWCryptoOperationType_Verify, &error);
  if (fwOperation) {
    nssCKFWSession_SetCurrentCryptoOperation(fwSession, fwOperation,
                NSSCKFWCryptoOperationState_SignVerify);
  }

loser:
  return error;
}





NSS_EXTERN CK_RV
nssCKFWMechanism_SignRecoverInit
(
  NSSCKFWMechanism *fwMechanism,
  CK_MECHANISM     *pMechanism,
  NSSCKFWSession   *fwSession,
  NSSCKFWObject    *fwObject
)
{
  NSSCKFWCryptoOperation *fwOperation;
  NSSCKMDCryptoOperation *mdOperation;
  NSSCKMDSession *mdSession;
  NSSCKMDObject  *mdObject;
  CK_RV  error = CKR_OK;


  fwOperation = nssCKFWSession_GetCurrentCryptoOperation(fwSession, 
                        NSSCKFWCryptoOperationState_SignVerify);
  if (fwOperation) {
    return CKR_OPERATION_ACTIVE;
  }

  if (!fwMechanism->mdMechanism->SignRecoverInit) {
    return CKR_FUNCTION_FAILED;
  }

  mdSession = nssCKFWSession_GetMDSession(fwSession);
  mdObject = nssCKFWObject_GetMDObject(fwObject);
  mdOperation = fwMechanism->mdMechanism->SignRecoverInit(
        fwMechanism->mdMechanism,
        fwMechanism,
        pMechanism,
        mdSession,
        fwSession,
        fwMechanism->mdToken,
        fwMechanism->fwToken,
        fwMechanism->mdInstance,
        fwMechanism->fwInstance,
        mdObject,
        fwObject,
        &error
  );
  if (!mdOperation) {
    goto loser;
  }

  fwOperation = nssCKFWCryptoOperation_Create(mdOperation, 
        mdSession, fwSession, fwMechanism->mdToken, fwMechanism->fwToken,
        fwMechanism->mdInstance, fwMechanism->fwInstance,
        NSSCKFWCryptoOperationType_SignRecover, &error);
  if (fwOperation) {
    nssCKFWSession_SetCurrentCryptoOperation(fwSession, fwOperation,
                NSSCKFWCryptoOperationState_SignVerify);
  }

loser:
  return error;
}





NSS_EXTERN CK_RV
nssCKFWMechanism_VerifyRecoverInit
(
  NSSCKFWMechanism *fwMechanism,
  CK_MECHANISM     *pMechanism,
  NSSCKFWSession   *fwSession,
  NSSCKFWObject    *fwObject
)
{
  NSSCKFWCryptoOperation *fwOperation;
  NSSCKMDCryptoOperation *mdOperation;
  NSSCKMDSession *mdSession;
  NSSCKMDObject  *mdObject;
  CK_RV  error = CKR_OK;


  fwOperation = nssCKFWSession_GetCurrentCryptoOperation(fwSession, 
                        NSSCKFWCryptoOperationState_SignVerify);
  if (fwOperation) {
    return CKR_OPERATION_ACTIVE;
  }

  if (!fwMechanism->mdMechanism->VerifyRecoverInit) {
    return CKR_FUNCTION_FAILED;
  }

  mdSession = nssCKFWSession_GetMDSession(fwSession);
  mdObject = nssCKFWObject_GetMDObject(fwObject);
  mdOperation = fwMechanism->mdMechanism->VerifyRecoverInit(
        fwMechanism->mdMechanism,
        fwMechanism,
        pMechanism,
        mdSession,
        fwSession,
        fwMechanism->mdToken,
        fwMechanism->fwToken,
        fwMechanism->mdInstance,
        fwMechanism->fwInstance,
        mdObject,
        fwObject,
        &error
  );
  if (!mdOperation) {
    goto loser;
  }

  fwOperation = nssCKFWCryptoOperation_Create(mdOperation, 
        mdSession, fwSession, fwMechanism->mdToken, fwMechanism->fwToken,
        fwMechanism->mdInstance, fwMechanism->fwInstance,
        NSSCKFWCryptoOperationType_VerifyRecover, &error);
  if (fwOperation) {
    nssCKFWSession_SetCurrentCryptoOperation(fwSession, fwOperation,
                NSSCKFWCryptoOperationState_SignVerify);
  }

loser:
  return error;
}




NSS_EXTERN NSSCKFWObject *
nssCKFWMechanism_GenerateKey
(
  NSSCKFWMechanism *fwMechanism,
  CK_MECHANISM_PTR pMechanism,
  NSSCKFWSession   *fwSession,
  CK_ATTRIBUTE_PTR pTemplate,
  CK_ULONG         ulAttributeCount,
  CK_RV            *pError
)
{
  NSSCKMDSession *mdSession;
  NSSCKMDObject  *mdObject;
  NSSCKFWObject  *fwObject = NULL;
  NSSArena       *arena;

  if (!fwMechanism->mdMechanism->GenerateKey) {
    *pError = CKR_FUNCTION_FAILED;
    return (NSSCKFWObject *)NULL;
  }

  arena = nssCKFWToken_GetArena(fwMechanism->fwToken, pError);
  if (!arena) {
    if (CKR_OK == *pError) {
      *pError = CKR_GENERAL_ERROR;
    }
    return (NSSCKFWObject *)NULL;
  }

  mdSession = nssCKFWSession_GetMDSession(fwSession);
  mdObject = fwMechanism->mdMechanism->GenerateKey(
        fwMechanism->mdMechanism,
        fwMechanism,
        pMechanism,
        mdSession,
        fwSession,
        fwMechanism->mdToken,
        fwMechanism->fwToken,
        fwMechanism->mdInstance,
        fwMechanism->fwInstance,
        pTemplate,
        ulAttributeCount,
        pError);

  if (!mdObject) {
    return (NSSCKFWObject *)NULL;
  }

  fwObject = nssCKFWObject_Create(arena, mdObject, 
        fwSession, fwMechanism->fwToken, fwMechanism->fwInstance, pError);

  return fwObject;
}




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
)
{
  NSSCKMDSession *mdSession;
  NSSCKMDObject  *mdPublicKeyObject;
  NSSCKMDObject  *mdPrivateKeyObject;
  NSSArena       *arena;
  CK_RV         error = CKR_OK;

  if (!fwMechanism->mdMechanism->GenerateKeyPair) {
    return CKR_FUNCTION_FAILED;
  }

  arena = nssCKFWToken_GetArena(fwMechanism->fwToken, &error);
  if (!arena) {
    if (CKR_OK == error) {
      error = CKR_GENERAL_ERROR;
    }
    return error;
  }

  mdSession = nssCKFWSession_GetMDSession(fwSession);
  error = fwMechanism->mdMechanism->GenerateKeyPair(
        fwMechanism->mdMechanism,
        fwMechanism,
        pMechanism,
        mdSession,
        fwSession,
        fwMechanism->mdToken,
        fwMechanism->fwToken,
        fwMechanism->mdInstance,
        fwMechanism->fwInstance,
        pPublicKeyTemplate,
        ulPublicKeyAttributeCount,
        pPrivateKeyTemplate,
        ulPrivateKeyAttributeCount,
        &mdPublicKeyObject,
        &mdPrivateKeyObject);

  if (CKR_OK != error) {
    return error;
  }

  *fwPublicKeyObject = nssCKFWObject_Create(arena, mdPublicKeyObject, 
        fwSession, fwMechanism->fwToken, fwMechanism->fwInstance, &error);
  if (!*fwPublicKeyObject) {
    return error;
  }
  *fwPrivateKeyObject = nssCKFWObject_Create(arena, mdPrivateKeyObject, 
        fwSession, fwMechanism->fwToken, fwMechanism->fwInstance, &error);

  return error;
}




NSS_EXTERN CK_ULONG
nssCKFWMechanism_GetWrapKeyLength
(
  NSSCKFWMechanism *fwMechanism,
  CK_MECHANISM_PTR pMechanism,
  NSSCKFWSession   *fwSession,
  NSSCKFWObject    *fwWrappingKeyObject,
  NSSCKFWObject    *fwKeyObject,
  CK_RV                   *pError
)
{
  NSSCKMDSession *mdSession;
  NSSCKMDObject  *mdWrappingKeyObject;
  NSSCKMDObject  *mdKeyObject;

  if (!fwMechanism->mdMechanism->WrapKey) {
    *pError = CKR_FUNCTION_FAILED;
    return (CK_ULONG) 0;
  }

  mdSession = nssCKFWSession_GetMDSession(fwSession);
  mdWrappingKeyObject = nssCKFWObject_GetMDObject(fwWrappingKeyObject);
  mdKeyObject = nssCKFWObject_GetMDObject(fwKeyObject);
  return fwMechanism->mdMechanism->GetWrapKeyLength(
        fwMechanism->mdMechanism,
        fwMechanism,
        pMechanism,
        mdSession,
        fwSession,
        fwMechanism->mdToken,
        fwMechanism->fwToken,
        fwMechanism->mdInstance,
        fwMechanism->fwInstance,
        mdWrappingKeyObject,
        fwWrappingKeyObject,
        mdKeyObject,
        fwKeyObject,
        pError);
}




NSS_EXTERN CK_RV
nssCKFWMechanism_WrapKey
(
  NSSCKFWMechanism *fwMechanism,
  CK_MECHANISM_PTR pMechanism,
  NSSCKFWSession   *fwSession,
  NSSCKFWObject    *fwWrappingKeyObject,
  NSSCKFWObject    *fwKeyObject,
  NSSItem          *wrappedKey
)
{
  NSSCKMDSession *mdSession;
  NSSCKMDObject  *mdWrappingKeyObject;
  NSSCKMDObject  *mdKeyObject;

  if (!fwMechanism->mdMechanism->WrapKey) {
    return CKR_FUNCTION_FAILED;
  }

  mdSession = nssCKFWSession_GetMDSession(fwSession);
  mdWrappingKeyObject = nssCKFWObject_GetMDObject(fwWrappingKeyObject);
  mdKeyObject = nssCKFWObject_GetMDObject(fwKeyObject);
  return fwMechanism->mdMechanism->WrapKey(
        fwMechanism->mdMechanism,
        fwMechanism,
        pMechanism,
        mdSession,
        fwSession,
        fwMechanism->mdToken,
        fwMechanism->fwToken,
        fwMechanism->mdInstance,
        fwMechanism->fwInstance,
        mdWrappingKeyObject,
        fwWrappingKeyObject,
        mdKeyObject,
        fwKeyObject,
        wrappedKey);
}




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
)
{
  NSSCKMDSession *mdSession;
  NSSCKMDObject  *mdObject;
  NSSCKMDObject  *mdWrappingKeyObject;
  NSSCKFWObject  *fwObject = NULL;
  NSSArena       *arena;

  if (!fwMechanism->mdMechanism->UnwrapKey) {
    



    *pError = CKR_FUNCTION_FAILED;
    return (NSSCKFWObject *)NULL;
  }

  arena = nssCKFWToken_GetArena(fwMechanism->fwToken, pError);
  if (!arena) {
    if (CKR_OK == *pError) {
      *pError = CKR_GENERAL_ERROR;
    }
    return (NSSCKFWObject *)NULL;
  }

  mdSession = nssCKFWSession_GetMDSession(fwSession);
  mdWrappingKeyObject = nssCKFWObject_GetMDObject(fwWrappingKeyObject);
  mdObject = fwMechanism->mdMechanism->UnwrapKey(
        fwMechanism->mdMechanism,
        fwMechanism,
        pMechanism,
        mdSession,
        fwSession,
        fwMechanism->mdToken,
        fwMechanism->fwToken,
        fwMechanism->mdInstance,
        fwMechanism->fwInstance,
        mdWrappingKeyObject,
        fwWrappingKeyObject,
        wrappedKey,
        pTemplate,
        ulAttributeCount,
        pError);

  if (!mdObject) {
    return (NSSCKFWObject *)NULL;
  }

  fwObject = nssCKFWObject_Create(arena, mdObject, 
        fwSession, fwMechanism->fwToken, fwMechanism->fwInstance, pError);

  return fwObject;
}




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
)
{
  NSSCKMDSession *mdSession;
  NSSCKMDObject  *mdObject;
  NSSCKMDObject  *mdBaseKeyObject;
  NSSCKFWObject  *fwObject = NULL;
  NSSArena       *arena;

  if (!fwMechanism->mdMechanism->DeriveKey) {
    *pError = CKR_FUNCTION_FAILED;
    return (NSSCKFWObject *)NULL;
  }

  arena = nssCKFWToken_GetArena(fwMechanism->fwToken, pError);
  if (!arena) {
    if (CKR_OK == *pError) {
      *pError = CKR_GENERAL_ERROR;
    }
    return (NSSCKFWObject *)NULL;
  }

  mdSession = nssCKFWSession_GetMDSession(fwSession);
  mdBaseKeyObject = nssCKFWObject_GetMDObject(fwBaseKeyObject);
  mdObject = fwMechanism->mdMechanism->DeriveKey(
        fwMechanism->mdMechanism,
        fwMechanism,
        pMechanism,
        mdSession,
        fwSession,
        fwMechanism->mdToken,
        fwMechanism->fwToken,
        fwMechanism->mdInstance,
        fwMechanism->fwInstance,
        mdBaseKeyObject,
        fwBaseKeyObject,
        pTemplate,
        ulAttributeCount,
        pError);

  if (!mdObject) {
    return (NSSCKFWObject *)NULL;
  }

  fwObject = nssCKFWObject_Create(arena, mdObject, 
        fwSession, fwMechanism->fwToken, fwMechanism->fwInstance, pError);

  return fwObject;
}

