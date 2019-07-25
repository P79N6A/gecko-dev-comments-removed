




































#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: crsa.c,v $ $Revision: 1.4 $ $Date: 2010/04/25 23:37:40 $";
#endif 

#include "ckcapi.h"
#include "secdert.h"

#define SSL3_SHAMD5_HASH_SIZE  36 /* LEN_MD5 (16) + LEN_SHA1 (20) */












static char *
putDecimalString(char *cstr, unsigned long value)
{
  unsigned long tenpower;
  int first = 1;

  for (tenpower=10000000; tenpower; tenpower /= 10) {
    unsigned char digit = (unsigned char )(value/tenpower);
    value = value % tenpower;

    
    if (first && (0 == digit)) {
      continue;
    }
    first = 0;
    *cstr++ = digit + '0';
  }

  
  if (first) {
    *cstr++ = '0';
  }
  return cstr;
}





static char *
nss_ckcapi_GetOidString
(
  unsigned char *oidTag,
  int oidTagSize,
  CK_RV *pError
)
{
  unsigned char *oid;
  char *oidStr;
  char *cstr;
  unsigned long value;
  int oidSize;

  if (DER_OBJECT_ID != *oidTag) {
    
    *pError = CKR_DATA_INVALID;
    return NULL;
  }
  oid = nss_ckcapi_DERUnwrap(oidTag, oidTagSize, &oidSize, NULL);

  if (oidSize < 2) {
    *pError = CKR_DATA_INVALID;
    return NULL;
  }

  oidStr = nss_ZNEWARRAY( NULL, char, oidSize*4 );
  if ((char *)NULL == oidStr) {
    *pError = CKR_HOST_MEMORY;
    return NULL;
  }
  cstr = oidStr;
  cstr = putDecimalString(cstr, (*oid) / 40);
  *cstr++ = '.';
  cstr = putDecimalString(cstr, (*oid) % 40);
  oidSize--;

  value = 0;
  while (oidSize--) {
    oid++;
    value = (value << 7) + (*oid & 0x7f);
    if (0 == (*oid & 0x80)) {
      *cstr++ = '.';
      cstr = putDecimalString(cstr, value);
      value = 0;
    }
  }

  *cstr = 0; 

  if (value != 0) {
    nss_ZFreeIf(oidStr);
    *pError = CKR_DATA_INVALID;
    return NULL;
  }
  return oidStr;
}









static CK_RV
ckcapi_GetRawHash
(
  const NSSItem *input,
  NSSItem *hash, 
  ALG_ID *hashAlg
)
{
   unsigned char *current;
   unsigned char *algid;
   unsigned char *oid;
   unsigned char *hashData;
   char *oidStr;
   CK_RV error;
   int oidSize;
   int size;
   








  if (SSL3_SHAMD5_HASH_SIZE == input->size) {
    hash->data = input->data;
    hash->size = input->size;
    *hashAlg = CALG_SSL3_SHAMD5;
    return CKR_OK;
  }

  current = (unsigned char *)input->data;

  
  if ((DER_SEQUENCE|DER_CONSTRUCTED) != *current) {
    return CKR_DATA_INVALID;
  }

  












  
  algid = nss_ckcapi_DERUnwrap(current,input->size, &size, NULL);
  
  if (algid+size != current+input->size) {
    
    return CKR_DATA_INVALID;
  }

  if ((DER_SEQUENCE|DER_CONSTRUCTED) != *algid) {
    
    return CKR_DATA_INVALID;
  }
  oid = nss_ckcapi_DERUnwrap(algid, size, &oidSize, &hashData);

  if (DER_OCTET_STRING != *hashData) {
    
    return CKR_DATA_INVALID;
  }

  
  current = hashData;
  size = size - (hashData-algid);
  hash->data = nss_ckcapi_DERUnwrap(current, size, &hash->size, NULL);

  

  oidStr = nss_ckcapi_GetOidString(oid, oidSize, &error);
  if ((char *)NULL == oidStr ) {
    return error;
  }

   
  *hashAlg = CertOIDToAlgId(oidStr);
  nss_ZFreeIf(oidStr);
  if (0 == *hashAlg) {
    return CKR_HOST_MEMORY;
  }

  
  return CKR_OK;
}





void
ckcapi_ReverseData(NSSItem *item)
{
  int end = (item->size)-1;
  int middle = (item->size)/2;
  unsigned char *buf = item->data;
  int i;

  for (i=0; i < middle; i++) {
    unsigned char  tmp = buf[i];
    buf[i] = buf[end-i];
    buf[end-i] = tmp;
  }
  return;
}

typedef struct ckcapiInternalCryptoOperationRSAPrivStr 
               ckcapiInternalCryptoOperationRSAPriv;
struct ckcapiInternalCryptoOperationRSAPrivStr
{
  NSSCKMDCryptoOperation mdOperation;
  NSSCKMDMechanism     *mdMechanism;
  ckcapiInternalObject *iKey;
  HCRYPTPROV           hProv;
  DWORD                keySpec;
  HCRYPTKEY            hKey;
  NSSItem	       *buffer;
};




static NSSCKMDCryptoOperation *
ckcapi_mdCryptoOperationRSAPriv_Create
(
  const NSSCKMDCryptoOperation *proto,
  NSSCKMDMechanism *mdMechanism,
  NSSCKMDObject *mdKey,
  CK_RV *pError
)
{
  ckcapiInternalObject *iKey = (ckcapiInternalObject *)mdKey->etc;
  const NSSItem *classItem = nss_ckcapi_FetchAttribute(iKey, CKA_CLASS);
  const NSSItem *keyType = nss_ckcapi_FetchAttribute(iKey, CKA_KEY_TYPE);
  ckcapiInternalCryptoOperationRSAPriv *iOperation;
  CK_RV   error;
  HCRYPTPROV  hProv;
  DWORD       keySpec;
  HCRYPTKEY   hKey;

  
  if (((const NSSItem *)NULL == classItem) ||
      (sizeof(CK_OBJECT_CLASS) != classItem->size) ||
      (CKO_PRIVATE_KEY != *(CK_OBJECT_CLASS *)classItem->data) ||
      ((const NSSItem *)NULL == keyType) ||
      (sizeof(CK_KEY_TYPE) != keyType->size) ||
      (CKK_RSA != *(CK_KEY_TYPE *)keyType->data)) {
    *pError =  CKR_KEY_TYPE_INCONSISTENT;
    return (NSSCKMDCryptoOperation *)NULL;
  }

  error = nss_ckcapi_FetchKeyContainer(iKey, &hProv, &keySpec, &hKey);
  if (error != CKR_OK) {
    *pError = error;
    return (NSSCKMDCryptoOperation *)NULL;
  }

  iOperation = nss_ZNEW(NULL, ckcapiInternalCryptoOperationRSAPriv);
  if ((ckcapiInternalCryptoOperationRSAPriv *)NULL == iOperation) {
    *pError = CKR_HOST_MEMORY;
    return (NSSCKMDCryptoOperation *)NULL;
  }
  iOperation->mdMechanism = mdMechanism;
  iOperation->iKey = iKey;
  iOperation->hProv = hProv;
  iOperation->keySpec = keySpec;
  iOperation->hKey = hKey;

  nsslibc_memcpy(&iOperation->mdOperation, 
                 proto, sizeof(NSSCKMDCryptoOperation));
  iOperation->mdOperation.etc = iOperation;

  return &iOperation->mdOperation;
}

static CK_RV
ckcapi_mdCryptoOperationRSAPriv_Destroy
(
  NSSCKMDCryptoOperation *mdOperation,
  NSSCKFWCryptoOperation *fwOperation,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance
)
{
  ckcapiInternalCryptoOperationRSAPriv *iOperation =
       (ckcapiInternalCryptoOperationRSAPriv *)mdOperation->etc;

  if (iOperation->hKey) {
    CryptDestroyKey(iOperation->hKey);
  }
  if (iOperation->buffer) {
    nssItem_Destroy(iOperation->buffer);
  }
  nss_ZFreeIf(iOperation);
  return CKR_OK;
}

static CK_ULONG
ckcapi_mdCryptoOperationRSA_GetFinalLength
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
  ckcapiInternalCryptoOperationRSAPriv *iOperation =
       (ckcapiInternalCryptoOperationRSAPriv *)mdOperation->etc;
  const NSSItem *modulus = 
       nss_ckcapi_FetchAttribute(iOperation->iKey, CKA_MODULUS);

  return modulus->size;
}








static CK_ULONG
ckcapi_mdCryptoOperationRSADecrypt_GetOperationLength
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
  ckcapiInternalCryptoOperationRSAPriv *iOperation =
       (ckcapiInternalCryptoOperationRSAPriv *)mdOperation->etc;
  BOOL rc;

  

  iOperation->buffer = nssItem_Duplicate((NSSItem *)input, NULL, NULL);
  if ((NSSItem *) NULL == iOperation->buffer) {
    *pError = CKR_HOST_MEMORY;
    return 0;
  }
  
  ckcapi_ReverseData(iOperation->buffer);
  
  rc = CryptDecrypt(iOperation->hKey, 0, TRUE, 0, 
		    iOperation->buffer->data, &iOperation->buffer->size);
  if (!rc) {
    DWORD msError = GetLastError();
    switch (msError) {
    case NTE_BAD_DATA:
      *pError = CKR_ENCRYPTED_DATA_INVALID;
      break;
    case NTE_FAIL:
    case NTE_BAD_UID:
      *pError = CKR_DEVICE_ERROR;
      break;
    default:
      *pError = CKR_GENERAL_ERROR; 
    }
    return 0;
  }

  return iOperation->buffer->size;
}







static CK_RV
ckcapi_mdCryptoOperationRSADecrypt_UpdateFinal
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
  ckcapiInternalCryptoOperationRSAPriv *iOperation =
       (ckcapiInternalCryptoOperationRSAPriv *)mdOperation->etc;
  NSSItem *buffer = iOperation->buffer;

  if ((NSSItem *)NULL == buffer) {
    return CKR_GENERAL_ERROR;
  }
  nsslibc_memcpy(output->data, buffer->data, buffer->size);
  output->size = buffer->size;
  return CKR_OK;
}





static CK_RV
ckcapi_mdCryptoOperationRSASign_UpdateFinal
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
  ckcapiInternalCryptoOperationRSAPriv *iOperation =
       (ckcapiInternalCryptoOperationRSAPriv *)mdOperation->etc;
  CK_RV error = CKR_OK;
  DWORD msError;
  NSSItem hash;
  HCRYPTHASH hHash = 0;
  ALG_ID  hashAlg;
  DWORD  hashSize;
  DWORD  len; 
  BOOL   rc;

  






  error = ckcapi_GetRawHash(input, &hash, &hashAlg);
  if (CKR_OK != error) {
    goto loser;
  }

  rc = CryptCreateHash(iOperation->hProv, hashAlg, 0, 0, &hHash);
  if (!rc) {
    goto loser;
  }

  
  len = sizeof(DWORD);
  rc = CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE *)&hashSize, &len, 0);
  if (!rc) {
    goto loser;
  }

  if (hash.size != hashSize) {
    
    error = CKR_DATA_INVALID;
    goto loser;
  }

  

  rc = CryptSetHashParam(hHash, HP_HASHVAL, hash.data, 0);
  if (!rc) {
    goto loser;
  }

  
  rc = CryptSignHash(hHash, iOperation->keySpec, NULL, 0,
                     output->data, &output->size);
  if (!rc) {
    goto loser;
  }

  

  rc = CryptVerifySignature(hHash, output->data, output->size, 
                            iOperation->hKey, NULL, 0);
  if (!rc) {
    goto loser;
  }

  

  ckcapi_ReverseData(output);
  CryptDestroyHash(hHash);
  return CKR_OK;

loser:
  
  if (CKR_OK == error) {
    msError = GetLastError();
    switch (msError) {
    case ERROR_NOT_ENOUGH_MEMORY:
      error = CKR_HOST_MEMORY;
      break;
    case NTE_NO_MEMORY:
      error = CKR_DEVICE_MEMORY;
      break;
    case ERROR_MORE_DATA:
      return CKR_BUFFER_TOO_SMALL;
    case ERROR_INVALID_PARAMETER: 
    case ERROR_INVALID_HANDLE:     
    case NTE_BAD_ALGID:           
    case NTE_BAD_HASH:
      error = CKR_DATA_INVALID;
      break;
    case ERROR_BUSY:
    case NTE_FAIL:
    case NTE_BAD_UID:
      error = CKR_DEVICE_ERROR;
      break;
    default:
      error = CKR_GENERAL_ERROR;
      break;
    }
  }
  if (hHash) {
    CryptDestroyHash(hHash);
  }
  return error;
}
  

NSS_IMPLEMENT_DATA const NSSCKMDCryptoOperation
ckcapi_mdCryptoOperationRSADecrypt_proto = {
  NULL, 
  ckcapi_mdCryptoOperationRSAPriv_Destroy,
  NULL, 
  ckcapi_mdCryptoOperationRSADecrypt_GetOperationLength,
  NULL, 
  NULL, 
  NULL, 
  ckcapi_mdCryptoOperationRSADecrypt_UpdateFinal,
  NULL, 
  NULL, 
  (void *)NULL 
};

NSS_IMPLEMENT_DATA const NSSCKMDCryptoOperation
ckcapi_mdCryptoOperationRSASign_proto = {
  NULL, 
  ckcapi_mdCryptoOperationRSAPriv_Destroy,
  ckcapi_mdCryptoOperationRSA_GetFinalLength,
  NULL, 
  NULL, 
  NULL, 
  NULL, 
  ckcapi_mdCryptoOperationRSASign_UpdateFinal,
  NULL, 
  NULL, 
  (void *)NULL 
};





static void
ckcapi_mdMechanismRSA_Destroy
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
ckcapi_mdMechanismRSA_GetMinKeySize
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
ckcapi_mdMechanismRSA_GetMaxKeySize
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
ckcapi_mdMechanismRSA_DecryptInit
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
  return ckcapi_mdCryptoOperationRSAPriv_Create(
		&ckcapi_mdCryptoOperationRSADecrypt_proto,
		mdMechanism, mdKey, pError);
}




static NSSCKMDCryptoOperation * 
ckcapi_mdMechanismRSA_SignInit
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
  return ckcapi_mdCryptoOperationRSAPriv_Create(
		&ckcapi_mdCryptoOperationRSASign_proto,
		mdMechanism, mdKey, pError);
}


NSS_IMPLEMENT_DATA const NSSCKMDMechanism
nss_ckcapi_mdMechanismRSA = {
  (void *)NULL, 
  ckcapi_mdMechanismRSA_Destroy,
  ckcapi_mdMechanismRSA_GetMinKeySize,
  ckcapi_mdMechanismRSA_GetMaxKeySize,
  NULL, 
  NULL, 
  ckcapi_mdMechanismRSA_DecryptInit,
  NULL, 
  ckcapi_mdMechanismRSA_SignInit,
  NULL, 
  ckcapi_mdMechanismRSA_SignInit,  
  NULL, 
  NULL, 
  NULL, 
  NULL, 
  NULL, 
  NULL, 
  NULL, 
  (void *)NULL 
};
