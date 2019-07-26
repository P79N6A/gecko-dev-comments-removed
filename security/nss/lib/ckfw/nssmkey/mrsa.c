



#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: mrsa.c,v $ $Revision: 1.3 $ $Date: 2012/04/25 14:49:40 $";
#endif 

#include "ckmk.h"






OSStatus 
SecKeyGetCredentials
(
  SecKeyRef keyRef,
  CSSM_ACL_AUTHORIZATION_TAG authTag,
  int type,
  const CSSM_ACCESS_CREDENTIALS **creds
);



OSStatus 
SecKeyGetCSPHandle
(
  SecKeyRef keyRef,
  CSSM_CSP_HANDLE *cspHandle
);


typedef struct ckmkInternalCryptoOperationRSAPrivStr 
               ckmkInternalCryptoOperationRSAPriv;
struct ckmkInternalCryptoOperationRSAPrivStr
{
  NSSCKMDCryptoOperation mdOperation;
  NSSCKMDMechanism     *mdMechanism;
  ckmkInternalObject *iKey;
  NSSItem  *buffer;
  CSSM_CC_HANDLE cssmContext;
};

typedef enum {
  CKMK_DECRYPT,
  CKMK_SIGN
} ckmkRSAOpType;




static NSSCKMDCryptoOperation *
ckmk_mdCryptoOperationRSAPriv_Create
(
  const NSSCKMDCryptoOperation *proto,
  NSSCKMDMechanism *mdMechanism,
  NSSCKMDObject *mdKey,
  ckmkRSAOpType type,
  CK_RV *pError
)
{
  ckmkInternalObject *iKey = (ckmkInternalObject *)mdKey->etc;
  const NSSItem *classItem = nss_ckmk_FetchAttribute(iKey, CKA_CLASS, pError);
  const NSSItem *keyType = nss_ckmk_FetchAttribute(iKey, CKA_KEY_TYPE, pError);
  ckmkInternalCryptoOperationRSAPriv *iOperation;
  SecKeyRef privateKey;
  OSStatus macErr;
  CSSM_RETURN cssmErr;
  const CSSM_KEY *cssmKey;
  CSSM_CSP_HANDLE cspHandle;
  const CSSM_ACCESS_CREDENTIALS *creds = NULL;
  CSSM_CC_HANDLE cssmContext;
  CSSM_ACL_AUTHORIZATION_TAG authType;

  
  if (((const NSSItem *)NULL == classItem) ||
      (sizeof(CK_OBJECT_CLASS) != classItem->size) ||
      (CKO_PRIVATE_KEY != *(CK_OBJECT_CLASS *)classItem->data) ||
      ((const NSSItem *)NULL == keyType) ||
      (sizeof(CK_KEY_TYPE) != keyType->size) ||
      (CKK_RSA != *(CK_KEY_TYPE *)keyType->data)) {
    *pError =  CKR_KEY_TYPE_INCONSISTENT;
    return (NSSCKMDCryptoOperation *)NULL;
  }

  privateKey = (SecKeyRef) iKey->u.item.itemRef;
  macErr = SecKeyGetCSSMKey(privateKey, &cssmKey);
  if (noErr != macErr) {
    CKMK_MACERR("Getting CSSM Key", macErr);
    *pError = CKR_KEY_HANDLE_INVALID;
    return (NSSCKMDCryptoOperation *)NULL;
  }
  macErr = SecKeyGetCSPHandle(privateKey, &cspHandle);
  if (noErr != macErr) {
    CKMK_MACERR("Getting CSP for Key", macErr);
    *pError = CKR_KEY_HANDLE_INVALID;
    return (NSSCKMDCryptoOperation *)NULL;
  }
  switch (type) {
  case CKMK_DECRYPT:
    authType = CSSM_ACL_AUTHORIZATION_DECRYPT;
    break;
  case CKMK_SIGN:
    authType = CSSM_ACL_AUTHORIZATION_SIGN;
    break;
  default:
    *pError = CKR_GENERAL_ERROR;
#ifdef DEBUG
    fprintf(stderr,"RSAPriv_Create: bad type = %d\n", type);
#endif
    return (NSSCKMDCryptoOperation *)NULL;
  }

  macErr = SecKeyGetCredentials(privateKey, authType, 0, &creds);
  if (noErr != macErr) {
    CKMK_MACERR("Getting Credentials for Key", macErr);
    *pError = CKR_KEY_HANDLE_INVALID;
    return (NSSCKMDCryptoOperation *)NULL;
  }
  
  switch (type) {
  case CKMK_DECRYPT:
    cssmErr = CSSM_CSP_CreateAsymmetricContext(cspHandle, CSSM_ALGID_RSA,
                        creds, cssmKey, CSSM_PADDING_PKCS1, &cssmContext);
    break;
  case CKMK_SIGN:
    cssmErr = CSSM_CSP_CreateSignatureContext(cspHandle, CSSM_ALGID_RSA,
                                              creds, cssmKey, &cssmContext);
    break;
  default:
    *pError = CKR_GENERAL_ERROR;
#ifdef DEBUG
    fprintf(stderr,"RSAPriv_Create: bad type = %d\n", type);
#endif
    return (NSSCKMDCryptoOperation *)NULL;
  }
  if (noErr != cssmErr) {
    CKMK_MACERR("Getting Context for Key", cssmErr);
    *pError = CKR_GENERAL_ERROR;
    return (NSSCKMDCryptoOperation *)NULL;
  }

  iOperation = nss_ZNEW(NULL, ckmkInternalCryptoOperationRSAPriv);
  if ((ckmkInternalCryptoOperationRSAPriv *)NULL == iOperation) {
    *pError = CKR_HOST_MEMORY;
    return (NSSCKMDCryptoOperation *)NULL;
  }
  iOperation->mdMechanism = mdMechanism;
  iOperation->iKey = iKey;
  iOperation->cssmContext = cssmContext;

  nsslibc_memcpy(&iOperation->mdOperation, 
                 proto, sizeof(NSSCKMDCryptoOperation));
  iOperation->mdOperation.etc = iOperation;

  return &iOperation->mdOperation;
}

static void
ckmk_mdCryptoOperationRSAPriv_Destroy
(
  NSSCKMDCryptoOperation *mdOperation,
  NSSCKFWCryptoOperation *fwOperation,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance
)
{
  ckmkInternalCryptoOperationRSAPriv *iOperation =
       (ckmkInternalCryptoOperationRSAPriv *)mdOperation->etc;

  if (iOperation->buffer) {
    nssItem_Destroy(iOperation->buffer);
  }
  if (iOperation->cssmContext) {
    CSSM_DeleteContext(iOperation->cssmContext);
  }
  nss_ZFreeIf(iOperation);
  return;
}

static CK_ULONG
ckmk_mdCryptoOperationRSA_GetFinalLength
(
  NSSCKMDCryptoOperation *mdOperation,
  NSSCKFWCryptoOperation *fwOperation,
  NSSCKMDSession *mdSession,
  NSSCKFWSession *fwSession,
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  ckmkInternalCryptoOperationRSAPriv *iOperation =
       (ckmkInternalCryptoOperationRSAPriv *)mdOperation->etc;
  const NSSItem *modulus = 
       nss_ckmk_FetchAttribute(iOperation->iKey, CKA_MODULUS, pError);

  return modulus->size;
}








static CK_ULONG
ckmk_mdCryptoOperationRSADecrypt_GetOperationLength
(
  NSSCKMDCryptoOperation *mdOperation,
  NSSCKFWCryptoOperation *fwOperation,
  NSSCKMDSession *mdSession,
  NSSCKFWSession *fwSession,
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  const NSSItem *input,
  CK_RV *pError
)
{
  ckmkInternalCryptoOperationRSAPriv *iOperation =
       (ckmkInternalCryptoOperationRSAPriv *)mdOperation->etc; 
  CSSM_DATA cssmInput;
  CSSM_DATA cssmOutput = { 0, NULL };
  PRUint32  bytesDecrypted;
  CSSM_DATA remainder = { 0, NULL };
  NSSItem output;
  CSSM_RETURN cssmErr;

  if (iOperation->buffer) {
    return iOperation->buffer->size;
  }

  cssmInput.Data = input->data;
  cssmInput.Length = input->size;

  cssmErr = CSSM_DecryptData(iOperation->cssmContext, 
			     &cssmInput, 1, &cssmOutput, 1,
			     &bytesDecrypted, &remainder);
  if (CSSM_OK != cssmErr) {
    CKMK_MACERR("Decrypt Failed", cssmErr);
    *pError = CKR_DATA_INVALID;
    return 0;
  }
  
  output.data = nss_ZNEWARRAY(NULL, char, bytesDecrypted + remainder.Length);
  if (NULL == output.data) {
    free(cssmOutput.Data);
    free(remainder.Data);
    *pError = CKR_HOST_MEMORY;
    return 0;
  }
  output.size = bytesDecrypted + remainder.Length;

  if (0 != bytesDecrypted) {
    nsslibc_memcpy(output.data, cssmOutput.Data, bytesDecrypted);
    free(cssmOutput.Data);
  }
  if (0 != remainder.Length) {
    nsslibc_memcpy(((char *)output.data)+bytesDecrypted, 
	           remainder.Data, remainder.Length);
    free(remainder.Data);
  }
  
  iOperation->buffer = nssItem_Duplicate(&output, NULL, NULL);
  nss_ZFreeIf(output.data);
  if ((NSSItem *) NULL == iOperation->buffer) {
    *pError = CKR_HOST_MEMORY;
    return 0;
  }

  return iOperation->buffer->size;
}







static CK_RV
ckmk_mdCryptoOperationRSADecrypt_UpdateFinal
(
  NSSCKMDCryptoOperation *mdOperation,
  NSSCKFWCryptoOperation *fwOperation,
  NSSCKMDSession *mdSession,
  NSSCKFWSession *fwSession,
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  const NSSItem *input,
  NSSItem *output
)
{
  ckmkInternalCryptoOperationRSAPriv *iOperation =
       (ckmkInternalCryptoOperationRSAPriv *)mdOperation->etc; 
  NSSItem *buffer = iOperation->buffer;

  if ((NSSItem *)NULL == buffer) {
    return CKR_GENERAL_ERROR;
  }
  nsslibc_memcpy(output->data, buffer->data, buffer->size);
  output->size = buffer->size;
  return CKR_OK;
}





static CK_RV
ckmk_mdCryptoOperationRSASign_UpdateFinal
(
  NSSCKMDCryptoOperation *mdOperation,
  NSSCKFWCryptoOperation *fwOperation,
  NSSCKMDSession *mdSession,
  NSSCKFWSession *fwSession,
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  const NSSItem *input,
  NSSItem *output
)
{
  ckmkInternalCryptoOperationRSAPriv *iOperation =
       (ckmkInternalCryptoOperationRSAPriv *)mdOperation->etc;
  CSSM_DATA cssmInput;
  CSSM_DATA cssmOutput = { 0, NULL };
  CSSM_RETURN cssmErr;

  cssmInput.Data = input->data;
  cssmInput.Length = input->size; 

  cssmErr = CSSM_SignData(iOperation->cssmContext, &cssmInput, 1,
                          CSSM_ALGID_NONE, &cssmOutput);
  if (CSSM_OK != cssmErr) {
    CKMK_MACERR("Signed Failed", cssmErr);
    return CKR_FUNCTION_FAILED;
  }
  if (cssmOutput.Length > output->size) {
    free(cssmOutput.Data);
    return CKR_BUFFER_TOO_SMALL;
  }
  nsslibc_memcpy(output->data, cssmOutput.Data, cssmOutput.Length);
  free(cssmOutput.Data);
  output->size = cssmOutput.Length;

  return CKR_OK;
}
  

NSS_IMPLEMENT_DATA const NSSCKMDCryptoOperation
ckmk_mdCryptoOperationRSADecrypt_proto = {
  NULL, 
  ckmk_mdCryptoOperationRSAPriv_Destroy,
  NULL, 
  ckmk_mdCryptoOperationRSADecrypt_GetOperationLength,
  NULL, 
  NULL, 
  NULL, 
  ckmk_mdCryptoOperationRSADecrypt_UpdateFinal,
  NULL, 
  NULL, 
  (void *)NULL 
};

NSS_IMPLEMENT_DATA const NSSCKMDCryptoOperation
ckmk_mdCryptoOperationRSASign_proto = {
  NULL, 
  ckmk_mdCryptoOperationRSAPriv_Destroy,
  ckmk_mdCryptoOperationRSA_GetFinalLength,
  NULL, 
  NULL, 
  NULL, 
  NULL, 
  ckmk_mdCryptoOperationRSASign_UpdateFinal,
  NULL, 
  NULL, 
  (void *)NULL 
};





static void
ckmk_mdMechanismRSA_Destroy
(
  NSSCKMDMechanism *mdMechanism,
  NSSCKFWMechanism *fwMechanism,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance
)
{
  nss_ZFreeIf(fwMechanism);
}




static CK_ULONG
ckmk_mdMechanismRSA_GetMinKeySize
(
  NSSCKMDMechanism *mdMechanism,
  NSSCKFWMechanism *fwMechanism,
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return 384;
}




static CK_ULONG
ckmk_mdMechanismRSA_GetMaxKeySize
(
  NSSCKMDMechanism *mdMechanism,
  NSSCKFWMechanism *fwMechanism,
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return 16384;
}




static NSSCKMDCryptoOperation * 
ckmk_mdMechanismRSA_DecryptInit
(
  NSSCKMDMechanism *mdMechanism,
  NSSCKFWMechanism *fwMechanism,
  CK_MECHANISM     *pMechanism,
  NSSCKMDSession *mdSession,
  NSSCKFWSession *fwSession,
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  NSSCKMDObject *mdKey,
  NSSCKFWObject *fwKey,
  CK_RV *pError
)
{
  return ckmk_mdCryptoOperationRSAPriv_Create(
		&ckmk_mdCryptoOperationRSADecrypt_proto,
		mdMechanism, mdKey, CKMK_DECRYPT, pError);
}




static NSSCKMDCryptoOperation * 
ckmk_mdMechanismRSA_SignInit
(
  NSSCKMDMechanism *mdMechanism,
  NSSCKFWMechanism *fwMechanism,
  CK_MECHANISM     *pMechanism,
  NSSCKMDSession *mdSession,
  NSSCKFWSession *fwSession,
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  NSSCKMDObject *mdKey,
  NSSCKFWObject *fwKey,
  CK_RV *pError
)
{
  return ckmk_mdCryptoOperationRSAPriv_Create(
		&ckmk_mdCryptoOperationRSASign_proto,
		mdMechanism, mdKey, CKMK_SIGN, pError);
}


NSS_IMPLEMENT_DATA const NSSCKMDMechanism
nss_ckmk_mdMechanismRSA = {
  (void *)NULL, 
  ckmk_mdMechanismRSA_Destroy,
  ckmk_mdMechanismRSA_GetMinKeySize,
  ckmk_mdMechanismRSA_GetMaxKeySize,
  NULL, 
  NULL, 
  ckmk_mdMechanismRSA_DecryptInit,
  NULL, 
  ckmk_mdMechanismRSA_SignInit,
  NULL, 
  ckmk_mdMechanismRSA_SignInit,  
  NULL, 
  NULL, 
  NULL, 
  NULL, 
  NULL, 
  NULL, 
  NULL, 
  (void *)NULL 
};
