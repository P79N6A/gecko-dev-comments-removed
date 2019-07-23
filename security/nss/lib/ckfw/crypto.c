




































#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: crypto.c,v $ $Revision: 1.4 $ $Date: 2009/02/09 07:55:51 $";
#endif 







#ifndef CK_T
#include "ck.h"
#endif 























struct NSSCKFWCryptoOperationStr {
  
  NSSCKMDCryptoOperation *mdOperation;
  NSSCKMDSession *mdSession;
  NSSCKFWSession *fwSession;
  NSSCKMDToken *mdToken;
  NSSCKFWToken *fwToken;
  NSSCKMDInstance *mdInstance;
  NSSCKFWInstance *fwInstance;
  NSSCKFWCryptoOperationType type;
};




NSS_EXTERN NSSCKFWCryptoOperation *
nssCKFWCryptoOperation_Create(
  NSSCKMDCryptoOperation *mdOperation,
  NSSCKMDSession *mdSession,
  NSSCKFWSession *fwSession,
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  NSSCKFWCryptoOperationType type,
  CK_RV *pError
)
{
  NSSCKFWCryptoOperation *fwOperation;
  fwOperation = nss_ZNEW(NULL, NSSCKFWCryptoOperation);
  if (!fwOperation) {
    *pError = CKR_HOST_MEMORY;
    return (NSSCKFWCryptoOperation *)NULL;
  }
  fwOperation->mdOperation = mdOperation; 
  fwOperation->mdSession = mdSession; 
  fwOperation->fwSession = fwSession; 
  fwOperation->mdToken = mdToken; 
  fwOperation->fwToken = fwToken; 
  fwOperation->mdInstance = mdInstance; 
  fwOperation->fwInstance = fwInstance; 
  fwOperation->type = type; 
  return fwOperation;
}




NSS_EXTERN void
nssCKFWCryptoOperation_Destroy
(
  NSSCKFWCryptoOperation *fwOperation
)
{
  if ((NSSCKMDCryptoOperation *) NULL != fwOperation->mdOperation) {
    if (fwOperation->mdOperation->Destroy) {
      fwOperation->mdOperation->Destroy(
                                fwOperation->mdOperation,
                                fwOperation,
                                fwOperation->mdInstance,
                                fwOperation->fwInstance);
    }
  }
  nss_ZFreeIf(fwOperation);
}




NSS_EXTERN NSSCKMDCryptoOperation *
nssCKFWCryptoOperation_GetMDCryptoOperation
(
  NSSCKFWCryptoOperation *fwOperation
)
{
  return fwOperation->mdOperation;
}




NSS_EXTERN NSSCKFWCryptoOperationType
nssCKFWCryptoOperation_GetType
(
  NSSCKFWCryptoOperation *fwOperation
)
{
  return fwOperation->type;
}




NSS_EXTERN CK_ULONG
nssCKFWCryptoOperation_GetFinalLength
(
  NSSCKFWCryptoOperation *fwOperation,
  CK_RV *pError
)
{
  if (!fwOperation->mdOperation->GetFinalLength) {
    *pError = CKR_FUNCTION_FAILED;
    return 0;
  }
  return fwOperation->mdOperation->GetFinalLength(
                fwOperation->mdOperation,
                fwOperation,
                fwOperation->mdSession,
                fwOperation->fwSession,
                fwOperation->mdToken,
                fwOperation->fwToken,
                fwOperation->mdInstance,
                fwOperation->fwInstance,
                pError);
}




NSS_EXTERN CK_ULONG
nssCKFWCryptoOperation_GetOperationLength
(
  NSSCKFWCryptoOperation *fwOperation,
  NSSItem *inputBuffer,
  CK_RV *pError
)
{
  if (!fwOperation->mdOperation->GetOperationLength) {
    *pError = CKR_FUNCTION_FAILED;
    return 0;
  }
  return fwOperation->mdOperation->GetOperationLength(
                fwOperation->mdOperation,
                fwOperation,
                fwOperation->mdSession,
                fwOperation->fwSession,
                fwOperation->mdToken,
                fwOperation->fwToken,
                fwOperation->mdInstance,
                fwOperation->fwInstance,
                inputBuffer,
                pError);
}




NSS_EXTERN CK_RV
nssCKFWCryptoOperation_Final
(
  NSSCKFWCryptoOperation *fwOperation,
  NSSItem *outputBuffer
)
{
  if (!fwOperation->mdOperation->Final) {
    return CKR_FUNCTION_FAILED;
  }
  return fwOperation->mdOperation->Final(
                fwOperation->mdOperation,
                fwOperation,
                fwOperation->mdSession,
                fwOperation->fwSession,
                fwOperation->mdToken,
                fwOperation->fwToken,
                fwOperation->mdInstance,
                fwOperation->fwInstance,
                outputBuffer);
}




NSS_EXTERN CK_RV
nssCKFWCryptoOperation_Update
(
  NSSCKFWCryptoOperation *fwOperation,
  NSSItem *inputBuffer,
  NSSItem *outputBuffer
)
{
  if (!fwOperation->mdOperation->Update) {
    return CKR_FUNCTION_FAILED;
  }
  return fwOperation->mdOperation->Update(
                fwOperation->mdOperation,
                fwOperation,
                fwOperation->mdSession,
                fwOperation->fwSession,
                fwOperation->mdToken,
                fwOperation->fwToken,
                fwOperation->mdInstance,
                fwOperation->fwInstance,
                inputBuffer,
                outputBuffer);
}




NSS_EXTERN CK_RV
nssCKFWCryptoOperation_DigestUpdate
(
  NSSCKFWCryptoOperation *fwOperation,
  NSSItem *inputBuffer
)
{
  if (!fwOperation->mdOperation->DigestUpdate) {
    return CKR_FUNCTION_FAILED;
  }
  return fwOperation->mdOperation->DigestUpdate(
                fwOperation->mdOperation,
                fwOperation,
                fwOperation->mdSession,
                fwOperation->fwSession,
                fwOperation->mdToken,
                fwOperation->fwToken,
                fwOperation->mdInstance,
                fwOperation->fwInstance,
                inputBuffer);
}




NSS_EXTERN CK_RV
nssCKFWCryptoOperation_DigestKey
(
  NSSCKFWCryptoOperation *fwOperation,
  NSSCKFWObject *fwObject 
)
{
  NSSCKMDObject *mdObject;

  if (!fwOperation->mdOperation->DigestKey) {
    return CKR_FUNCTION_FAILED;
  }
  mdObject = nssCKFWObject_GetMDObject(fwObject);
  return fwOperation->mdOperation->DigestKey(
                fwOperation->mdOperation,
                fwOperation,
                fwOperation->mdToken,
                fwOperation->fwToken,
                fwOperation->mdInstance,
                fwOperation->fwInstance,
                mdObject,
                fwObject);
}




NSS_EXTERN CK_RV
nssCKFWCryptoOperation_UpdateFinal
(
  NSSCKFWCryptoOperation *fwOperation,
  NSSItem *inputBuffer,
  NSSItem *outputBuffer
)
{
  if (!fwOperation->mdOperation->UpdateFinal) {
    return CKR_FUNCTION_FAILED;
  }
  return fwOperation->mdOperation->UpdateFinal(
                fwOperation->mdOperation,
                fwOperation,
                fwOperation->mdSession,
                fwOperation->fwSession,
                fwOperation->mdToken,
                fwOperation->fwToken,
                fwOperation->mdInstance,
                fwOperation->fwInstance,
                inputBuffer,
                outputBuffer);
}




NSS_EXTERN CK_RV
nssCKFWCryptoOperation_UpdateCombo
(
  NSSCKFWCryptoOperation *fwOperation,
  NSSCKFWCryptoOperation *fwPeerOperation,
  NSSItem *inputBuffer,
  NSSItem *outputBuffer
)
{
  if (!fwOperation->mdOperation->UpdateCombo) {
    return CKR_FUNCTION_FAILED;
  }
  return fwOperation->mdOperation->UpdateCombo(
                fwOperation->mdOperation,
                fwOperation,
                fwPeerOperation->mdOperation,
                fwPeerOperation,
                fwOperation->mdSession,
                fwOperation->fwSession,
                fwOperation->mdToken,
                fwOperation->fwToken,
                fwOperation->mdInstance,
                fwOperation->fwInstance,
                inputBuffer,
                outputBuffer);
}
