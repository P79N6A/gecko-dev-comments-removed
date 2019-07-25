







































#include "WeaveCrypto.h"

#include "nsStringAPI.h"
#include "nsAutoPtr.h"
#include "plbase64.h"
#include "prmem.h"
#include "secerr.h"

#include "pk11func.h"
#include "keyhi.h"
#include "nss.h"






#define STACK_BUFFER_SIZE 4096

NS_IMPL_ISUPPORTS1(WeaveCrypto, IWeaveCrypto)

WeaveCrypto::WeaveCrypto() :
  mAlgorithm(SEC_OID_AES_256_CBC),
  mKeypairBits(2048)
{
  
  nsCOMPtr<nsISupports> psm(nsGetServiceByContractID("@mozilla.org/psm;1"));
}

WeaveCrypto::~WeaveCrypto()
{
}





nsresult
WeaveCrypto::EncodeBase64(const char *aData, PRUint32 aLength, nsACString& retval)
{
  
  if (!aLength) {
    retval.Assign(EmptyCString());
    return NS_OK;
  }

  PRUint32 encodedLength = (aLength + 2) / 3 * 4;
  char *encoded = (char *)PR_Malloc(encodedLength);
  if (!encoded)
    return NS_ERROR_OUT_OF_MEMORY;

  PL_Base64Encode(aData, aLength, encoded);

  retval.Assign(encoded, encodedLength);

  PR_Free(encoded);
  return NS_OK;
}

nsresult
WeaveCrypto::EncodeBase64(const nsACString& binary, nsACString& retval)
{
  PromiseFlatCString fBinary(binary);
  return EncodeBase64(fBinary.get(), fBinary.Length(), retval);
}

nsresult 
WeaveCrypto::DecodeBase64(const nsACString& base64,
                          char *decoded, PRUint32 *decodedSize)
{
  PromiseFlatCString fBase64(base64);

  if (fBase64.Length() == 0) {
    *decodedSize = 0;
    return NS_OK;
  }

  
  if (fBase64.Length() < 4)
    return NS_ERROR_FAILURE;

  PRUint32 size = (fBase64.Length() * 3) / 4;
  
  if (*(fBase64.get() + fBase64.Length() - 1) == '=')
    size--;
  if (*(fBase64.get() + fBase64.Length() - 2) == '=')
    size--;

  
  if (*decodedSize < size)
    return NS_ERROR_FAILURE;
  *decodedSize = size;

  if (!PL_Base64Decode(fBase64.get(), fBase64.Length(), decoded))
    return NS_ERROR_ILLEGAL_VALUE;

  return NS_OK;
}

nsresult 
WeaveCrypto::DecodeBase64(const nsACString& base64, nsACString& retval)
{
  PRUint32 decodedLength = base64.Length();
  char *decoded = (char *)PR_Malloc(decodedLength);
  if (!decoded)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = DecodeBase64(base64, decoded, &decodedLength);
  if (NS_FAILED(rv)) {
    PR_Free(decoded);
    return rv;
  }

  retval.Assign(decoded, decodedLength);

  PR_Free(decoded);
  return NS_OK;
}




NS_IMETHODIMP
WeaveCrypto::GetAlgorithm(PRUint32 *aAlgorithm)
{
  *aAlgorithm = mAlgorithm;
  return NS_OK;
}

NS_IMETHODIMP
WeaveCrypto::SetAlgorithm(PRUint32 aAlgorithm)
{
  mAlgorithm = (SECOidTag)aAlgorithm;
  return NS_OK;
}

NS_IMETHODIMP
WeaveCrypto::GetKeypairBits(PRUint32 *aBits)
{
  *aBits = mKeypairBits;
  return NS_OK;
}

NS_IMETHODIMP
WeaveCrypto::SetKeypairBits(PRUint32 aBits)
{
  mKeypairBits = aBits;
  return NS_OK;
}





NS_IMETHODIMP
WeaveCrypto::Encrypt(const nsACString& aClearText,
                     const nsACString& aSymmetricKey,
                     const nsACString& aIV,
                     nsACString& aCipherText)
{
  nsresult rv = NS_OK;

  
  CK_MECHANISM_TYPE mech = PK11_AlgtagToMechanism(mAlgorithm);
  PRUint32 blockSize = PK11_GetBlockSize(mech, nsnull);

  PRUint32 outputBufferSize = aClearText.Length() + blockSize;
  char *outputBuffer = (char *)PR_Malloc(outputBufferSize);
  if (!outputBuffer)
    return NS_ERROR_OUT_OF_MEMORY;

  PromiseFlatCString input(aClearText);

  rv = CommonCrypt(input.get(), input.Length(),
                   outputBuffer, &outputBufferSize,
                   aSymmetricKey, aIV, CKA_ENCRYPT);
  if (NS_FAILED(rv))
    goto encrypt_done;

  rv = EncodeBase64(outputBuffer, outputBufferSize, aCipherText);
  if (NS_FAILED(rv))
    goto encrypt_done;

encrypt_done:
  PR_Free(outputBuffer);
  return rv;
}





NS_IMETHODIMP
WeaveCrypto::Decrypt(const nsACString& aCipherText,
                     const nsACString& aSymmetricKey,
                     const nsACString& aIV,
                     nsACString& aClearText)
{
  nsresult rv = NS_OK;

  PRUint32 inputBufferSize  = aCipherText.Length();
  PRUint32 outputBufferSize = aCipherText.Length();
  char *outputBuffer = (char *)PR_Malloc(outputBufferSize);
  char *inputBuffer  = (char *)PR_Malloc(inputBufferSize);
  if (!inputBuffer || !outputBuffer)
    return NS_ERROR_OUT_OF_MEMORY;

  rv = DecodeBase64(aCipherText, inputBuffer, &inputBufferSize);
  if (NS_FAILED(rv))
    goto decrypt_done;

  rv = CommonCrypt(inputBuffer, inputBufferSize,
                   outputBuffer, &outputBufferSize,
                   aSymmetricKey, aIV, CKA_DECRYPT);
  if (NS_FAILED(rv))
    goto decrypt_done;

  aClearText.Assign(outputBuffer, outputBufferSize);

decrypt_done:
  PR_Free(outputBuffer);
  PR_Free(inputBuffer);
  return rv;
}





nsresult
WeaveCrypto::CommonCrypt(const char *input, PRUint32 inputSize,
                         char *output, PRUint32 *outputSize,
                         const nsACString& aSymmetricKey,
                         const nsACString& aIV,
                         CK_ATTRIBUTE_TYPE aOperation)
{
  nsresult rv = NS_OK;
  PK11SymKey   *symKey  = nsnull;
  PK11Context  *ctx     = nsnull;
  PK11SlotInfo *slot    = nsnull;
  SECItem      *ivParam = nsnull;
  PRUint32 maxOutputSize;

  char keyData[STACK_BUFFER_SIZE];
  PRUint32 keyDataSize = sizeof(keyData);
  char ivData[STACK_BUFFER_SIZE];
  PRUint32 ivDataSize = sizeof(ivData);

  rv = DecodeBase64(aSymmetricKey, keyData, &keyDataSize);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = DecodeBase64(aIV, ivData, &ivDataSize);
  NS_ENSURE_SUCCESS(rv, rv);

  SECItem keyItem = {siBuffer, (unsigned char*)keyData, keyDataSize}; 
  SECItem ivItem  = {siBuffer, (unsigned char*)ivData,  ivDataSize}; 


  
  CK_MECHANISM_TYPE mechanism = PK11_AlgtagToMechanism(mAlgorithm);
  mechanism = PK11_GetPadMechanism(mechanism);
  if (mechanism == CKM_INVALID_MECHANISM) {
    NS_WARNING("Unknown key mechanism");
    rv = NS_ERROR_FAILURE;
    goto crypt_done;
  }

  ivParam = PK11_ParamFromIV(mechanism, &ivItem);
  if (!ivParam) {
    NS_WARNING("Couldn't create IV param");
    rv = NS_ERROR_FAILURE;
    goto crypt_done;
  }

  slot = PK11_GetInternalKeySlot();
  if (!slot) {
    NS_WARNING("PK11_GetInternalKeySlot failed");
    rv = NS_ERROR_FAILURE;
    goto crypt_done;
  }

  symKey = PK11_ImportSymKey(slot, mechanism, PK11_OriginUnwrap, aOperation, &keyItem, NULL);
  if (!symKey) {
    NS_WARNING("PK11_ImportSymKey failed");
    rv = NS_ERROR_FAILURE;
    goto crypt_done;
  }

  ctx = PK11_CreateContextBySymKey(mechanism, aOperation, symKey, ivParam);
  if (!ctx) {
    NS_WARNING("PK11_CreateContextBySymKey failed");
    rv = NS_ERROR_FAILURE;
    goto crypt_done;
  }

  int tmpOutSize; 
  maxOutputSize = *outputSize;
  rv = PK11_CipherOp(ctx,
                     (unsigned char *)output, &tmpOutSize, maxOutputSize,
                     (unsigned char *)input, inputSize);
  if (NS_FAILED(rv)) {
    NS_WARNING("PK11_CipherOp failed");
    rv = NS_ERROR_FAILURE;
    goto crypt_done;
  }

  *outputSize = tmpOutSize;
  output += tmpOutSize;
  maxOutputSize -= tmpOutSize; 

  
  
  
  PRUint32 tmpOutSize2; 
  rv = PK11_DigestFinal(ctx, (unsigned char *)output, &tmpOutSize2, maxOutputSize);
  if (NS_FAILED(rv)) {
    NS_WARNING("PK11_DigestFinal failed");
    rv = NS_ERROR_FAILURE;
    goto crypt_done;
  }

  *outputSize += tmpOutSize2;

crypt_done:
  if (ctx)
    PK11_DestroyContext(ctx, PR_TRUE); 
  if (symKey)
    PK11_FreeSymKey(symKey);
  if (slot)
    PK11_FreeSlot(slot);
  if (ivParam)
    SECITEM_FreeItem(ivParam, PR_TRUE);

  return rv;
}











NS_IMETHODIMP
WeaveCrypto::GenerateKeypair(const nsACString& aPassphrase,
                             const nsACString& aSalt,
                             const nsACString& aIV,
                             nsACString& aEncodedPublicKey,
                             nsACString& aWrappedPrivateKey)
{
  nsresult rv;
  SECStatus s;
  SECKEYPrivateKey *privKey = nsnull;
  SECKEYPublicKey  *pubKey  = nsnull;
  PK11SlotInfo     *slot    = nsnull;
  PK11RSAGenParams rsaParams;
  
  
  
  PK11AttrFlags attrFlags = (PK11_ATTR_SESSION |
                             PK11_ATTR_PUBLIC |
                             PK11_ATTR_SENSITIVE);


  rsaParams.keySizeInBits = mKeypairBits; 
  rsaParams.pe = 65537;            

  slot = PK11_GetInternalKeySlot();
  if (!slot) {
    NS_WARNING("PK11_GetInternalKeySlot failed");
    rv = NS_ERROR_FAILURE;
    goto keygen_done;
  }

  
  privKey = PK11_GenerateKeyPairWithFlags(slot,
                                CKM_RSA_PKCS_KEY_PAIR_GEN,
                                &rsaParams, &pubKey,
                                attrFlags, nsnull);

  if (!privKey) {
    NS_WARNING("PK11_GenerateKeyPair failed");
    rv = NS_ERROR_FAILURE;
    goto keygen_done;
  }

  s = PK11_SetPrivateKeyNickname(privKey, "Weave User PrivKey");
  if (s != SECSuccess) {
    NS_WARNING("PK11_SetPrivateKeyNickname failed");
    rv = NS_ERROR_FAILURE;
    goto keygen_done;
  }


  rv = WrapPrivateKey(privKey, aPassphrase, aSalt, aIV, aWrappedPrivateKey);
  if (NS_FAILED(rv)) {
    NS_WARNING("WrapPrivateKey failed");
    rv = NS_ERROR_FAILURE;
    goto keygen_done;
  }

  rv = EncodePublicKey(pubKey, aEncodedPublicKey);
  if (NS_FAILED(rv)) {
    NS_WARNING("EncodePublicKey failed");
    rv = NS_ERROR_FAILURE;
    goto keygen_done;
  }

keygen_done:
  
  if (pubKey)
    SECKEY_DestroyPublicKey(pubKey);
  if (privKey)
    SECKEY_DestroyPrivateKey(privKey);
  if (slot)
    PK11_FreeSlot(slot);

  return rv;
}







nsresult
WeaveCrypto::DeriveKeyFromPassphrase(const nsACString& aPassphrase,
                                     const nsACString& aSalt,
                                     PK11SymKey **aSymKey)
{
  nsresult rv;

  PromiseFlatCString fPass(aPassphrase);
  SECItem passphrase = {siBuffer, (unsigned char *)fPass.get(), fPass.Length()}; 

  char saltBytes[STACK_BUFFER_SIZE];
  PRUint32 saltBytesLength = sizeof(saltBytes);
  rv = DecodeBase64(aSalt, saltBytes, &saltBytesLength);
  NS_ENSURE_SUCCESS(rv, rv);
  SECItem salt = {siBuffer, (unsigned char*)saltBytes, saltBytesLength}; 

  

  
  SECOidTag pbeAlg = mAlgorithm;
  SECOidTag cipherAlg = mAlgorithm; 
  SECOidTag prfAlg =  SEC_OID_HMAC_SHA1; 

  PRInt32 keyLength  = 0;    
  PRInt32 iterations = 4096; 

  SECAlgorithmID *algid = PK11_CreatePBEV2AlgorithmID(pbeAlg, cipherAlg, prfAlg,
                                                      keyLength, iterations, &salt);
  if (!algid)
    return NS_ERROR_FAILURE;

  PK11SlotInfo *slot = PK11_GetInternalSlot();
  if (!slot)
    return NS_ERROR_FAILURE;

  *aSymKey = PK11_PBEKeyGen(slot, algid, &passphrase, PR_FALSE, nsnull);

  SECOID_DestroyAlgorithmID(algid, PR_TRUE);
  PK11_FreeSlot(slot);

  return (*aSymKey ? NS_OK : NS_ERROR_FAILURE);
}















nsresult
WeaveCrypto::WrapPrivateKey(SECKEYPrivateKey *aPrivateKey,
                            const nsACString& aPassphrase,
                            const nsACString& aSalt,
                            const nsACString& aIV,
                            nsACString& aWrappedPrivateKey)

{
  nsresult rv;
  SECStatus s;
  PK11SymKey *pbeKey = nsnull;

  
  rv = DeriveKeyFromPassphrase(aPassphrase, aSalt, &pbeKey);
  NS_ENSURE_SUCCESS(rv, rv);

  char ivData[STACK_BUFFER_SIZE];
  PRUint32 ivDataSize = sizeof(ivData);
  rv = DecodeBase64(aIV, ivData, &ivDataSize);
  NS_ENSURE_SUCCESS(rv, rv);
  SECItem ivItem  = {siBuffer, (unsigned char*)ivData,  ivDataSize}; 

  
  CK_MECHANISM_TYPE wrapMech = PK11_AlgtagToMechanism(mAlgorithm);
  wrapMech = PK11_GetPadMechanism(wrapMech);
  if (wrapMech == CKM_INVALID_MECHANISM) {
    NS_WARNING("Unknown key mechanism");
    return NS_ERROR_FAILURE;
  }

  SECItem *ivParam = PK11_ParamFromIV(wrapMech, &ivItem);
  if (!ivParam) {
    NS_WARNING("Couldn't create IV param");
    return NS_ERROR_FAILURE;
  }


  
  
  unsigned char stackBuffer[STACK_BUFFER_SIZE];
  SECItem wrappedKey = {siBuffer, stackBuffer, sizeof(stackBuffer)}; 

  s = PK11_WrapPrivKey(aPrivateKey->pkcs11Slot,
                       pbeKey, aPrivateKey,
                       wrapMech, ivParam,
                       &wrappedKey, nsnull);

  SECITEM_FreeItem(ivParam, PR_TRUE);
  PK11_FreeSymKey(pbeKey);

  if (s != SECSuccess) {
    NS_WARNING("PK11_WrapPrivKey failed");
    return(NS_ERROR_FAILURE);
  }
    
  rv = EncodeBase64((char *)wrappedKey.data, wrappedKey.len, aWrappedPrivateKey);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}







nsresult
WeaveCrypto::EncodePublicKey(SECKEYPublicKey *aPublicKey,
                             nsACString& aEncodedPublicKey)
{
  
  SECItem *derKey = SECKEY_EncodeDERSubjectPublicKeyInfo(aPublicKey);
  if (!derKey)
    return NS_ERROR_FAILURE;
    
  nsresult rv = EncodeBase64((char *)derKey->data, derKey->len, aEncodedPublicKey);
  NS_ENSURE_SUCCESS(rv, rv);

  

  return NS_OK;
}





NS_IMETHODIMP
WeaveCrypto::GenerateRandomBytes(PRUint32 aByteCount,
                                 nsACString& aEncodedBytes)
{
  nsresult rv;
  char random[STACK_BUFFER_SIZE];

  if (aByteCount > STACK_BUFFER_SIZE)
    return NS_ERROR_OUT_OF_MEMORY;

  rv = PK11_GenerateRandom((unsigned char *)random, aByteCount);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = EncodeBase64(random, aByteCount, aEncodedBytes);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}





NS_IMETHODIMP
WeaveCrypto::GenerateRandomIV(nsACString& aEncodedBytes)
{
  nsresult rv;

  CK_MECHANISM_TYPE mech = PK11_AlgtagToMechanism(mAlgorithm);
  PRUint32 size = PK11_GetIVLength(mech);

  char random[STACK_BUFFER_SIZE];

  if (size > STACK_BUFFER_SIZE)
    return NS_ERROR_OUT_OF_MEMORY;
 
  rv = PK11_GenerateRandom((unsigned char *)random, size);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = EncodeBase64(random, size, aEncodedBytes);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}












NS_IMETHODIMP
WeaveCrypto::GenerateRandomKey(nsACString& aEncodedKey)
{
  nsresult rv = NS_OK;
  PRUint32 keySize;
  CK_MECHANISM_TYPE keygenMech;
  SECItem *keydata = nsnull;

  
  switch (mAlgorithm) {
    case AES_128_CBC:
        keygenMech = CKM_AES_KEY_GEN;
        keySize = 16;
        break;

    case AES_192_CBC:
        keygenMech = CKM_AES_KEY_GEN;
        keySize = 24;
        break;

    case AES_256_CBC:
        keygenMech = CKM_AES_KEY_GEN;
        keySize = 32;
        break;

    default:
        NS_WARNING("Unknown random keygen algorithm");
        return NS_ERROR_FAILURE;
  }

  PK11SlotInfo *slot = PK11_GetInternalSlot();
  if (!slot)
    return NS_ERROR_FAILURE;

  PK11SymKey* randKey = PK11_KeyGen(slot, keygenMech, NULL, keySize, NULL);
  if (!randKey) {
    NS_WARNING("PK11_KeyGen failed");
    rv = NS_ERROR_FAILURE;
    goto keygen_done;
  }

  if (PK11_ExtractKeyValue(randKey)) {
    NS_WARNING("PK11_ExtractKeyValue failed");
    rv = NS_ERROR_FAILURE;
    goto keygen_done;
  }

  keydata = PK11_GetKeyData(randKey);
  if (!keydata) {
    NS_WARNING("PK11_GetKeyData failed");
    rv = NS_ERROR_FAILURE;
    goto keygen_done;
  }

  rv = EncodeBase64((char *)keydata->data, keydata->len, aEncodedKey);
  if (NS_FAILED(rv)) {
    NS_WARNING("EncodeBase64 failed");
    goto keygen_done;
  }

keygen_done:
  
  if (randKey)
    PK11_FreeSymKey(randKey);
  if (slot)
    PK11_FreeSlot(slot);

  return rv;
}





NS_IMETHODIMP
WeaveCrypto::WrapSymmetricKey(const nsACString& aSymmetricKey,
                              const nsACString& aPublicKey,
                              nsACString& aWrappedKey)
{
  nsresult rv = NS_OK;
  SECStatus s;
  PK11SlotInfo *slot = nsnull;
  PK11SymKey *symKey = nsnull;
  SECKEYPublicKey *pubKey = nsnull;
  CERTSubjectPublicKeyInfo *pubKeyInfo = nsnull;
  CK_MECHANISM_TYPE keyMech, wrapMech;

  

  char publicKeyBuffer[STACK_BUFFER_SIZE];
  PRUint32 publicKeyBufferSize = sizeof(publicKeyBuffer);
  rv = DecodeBase64(aPublicKey, publicKeyBuffer, &publicKeyBufferSize);
  NS_ENSURE_SUCCESS(rv, rv);
  SECItem pubKeyData = {siBuffer, (unsigned char *)publicKeyBuffer, publicKeyBufferSize};

  char symKeyBuffer[STACK_BUFFER_SIZE];
  PRUint32 symKeyBufferSize = sizeof(symKeyBuffer);
  rv = DecodeBase64(aSymmetricKey, symKeyBuffer, &symKeyBufferSize);
  NS_ENSURE_SUCCESS(rv, rv);
  SECItem symKeyData = {siBuffer, (unsigned char *)symKeyBuffer, symKeyBufferSize};

  char wrappedBuffer[STACK_BUFFER_SIZE];
  SECItem wrappedKey = {siBuffer, (unsigned char *)wrappedBuffer, sizeof(wrappedBuffer)};


  

  slot = PK11_GetInternalSlot();
  if (!slot) {
    NS_WARNING("Can't get internal PK11 slot");
    rv = NS_ERROR_FAILURE;
    goto wrap_done;
  }

  
  keyMech = PK11_AlgtagToMechanism(mAlgorithm);
  if (keyMech == CKM_INVALID_MECHANISM) {
    NS_WARNING("Unknown key mechanism");
    rv = NS_ERROR_FAILURE;
    goto wrap_done;
  }

  
  
  symKey = PK11_ImportSymKey(slot,
                             keyMech,
                             PK11_OriginUnwrap,
                             CKA_ENCRYPT,
                             &symKeyData,
                             NULL);
  if (!symKey) {
    NS_WARNING("PK11_ImportSymKey failed");
    rv = NS_ERROR_FAILURE;
    goto wrap_done;
  }


  

  
  
  pubKeyInfo = SECKEY_DecodeDERSubjectPublicKeyInfo(&pubKeyData);
  if (!pubKeyInfo) {
    NS_WARNING("SECKEY_DecodeDERSubjectPublicKeyInfo failed");
    rv = NS_ERROR_FAILURE;
    goto wrap_done;
  }

  pubKey = SECKEY_ExtractPublicKey(pubKeyInfo);
  if (!pubKey) {
    NS_WARNING("SECKEY_ExtractPublicKey failed");
    rv = NS_ERROR_FAILURE;
    goto wrap_done;
  }


  

  wrapMech = PK11_AlgtagToMechanism(SEC_OID_PKCS1_RSA_ENCRYPTION);

  s = PK11_PubWrapSymKey(wrapMech, pubKey, symKey, &wrappedKey);
  if (s != SECSuccess) {
    NS_WARNING("PK11_PubWrapSymKey failed");
    rv = NS_ERROR_FAILURE;
    goto wrap_done;
  }


  

  rv = EncodeBase64((char *)wrappedKey.data, wrappedKey.len, aWrappedKey);
  if (NS_FAILED(rv)) {
    NS_WARNING("EncodeBase64 failed");
    goto wrap_done;
  }

wrap_done:
  if (pubKey)
    SECKEY_DestroyPublicKey(pubKey);
  if (pubKeyInfo)
    SECKEY_DestroySubjectPublicKeyInfo(pubKeyInfo);
  if (symKey)
    PK11_FreeSymKey(symKey);
  if (slot)
    PK11_FreeSlot(slot);

  return rv;
}





NS_IMETHODIMP
WeaveCrypto::UnwrapSymmetricKey(const nsACString& aWrappedSymmetricKey,
                                const nsACString& aWrappedPrivateKey,
                                const nsACString& aPassphrase,
                                const nsACString& aSalt,
                                const nsACString& aIV,
                                nsACString& aSymmetricKey)
{
  nsresult rv = NS_OK;
  PK11SlotInfo *slot = nsnull;
  PK11SymKey *pbeKey = nsnull;
  PK11SymKey *symKey = nsnull;
  SECKEYPrivateKey *privKey = nsnull;
  SECItem *ivParam = nsnull;
  SECItem *symKeyData = nsnull;
  SECItem *keyID = nsnull;

  CK_ATTRIBUTE_TYPE privKeyUsage[] = { CKA_UNWRAP };
  PRUint32 privKeyUsageLength = sizeof(privKeyUsage) / sizeof(CK_ATTRIBUTE_TYPE);


  

  char privateKeyBuffer[STACK_BUFFER_SIZE];
  PRUint32 privateKeyBufferSize = sizeof(privateKeyBuffer);
  rv = DecodeBase64(aWrappedPrivateKey, privateKeyBuffer, &privateKeyBufferSize);
  NS_ENSURE_SUCCESS(rv, rv);
  SECItem wrappedPrivKey = {siBuffer, (unsigned char *)privateKeyBuffer, privateKeyBufferSize};

  char wrappedKeyBuffer[STACK_BUFFER_SIZE];
  PRUint32 wrappedKeyBufferSize = sizeof(wrappedKeyBuffer);
  rv = DecodeBase64(aWrappedSymmetricKey, wrappedKeyBuffer, &wrappedKeyBufferSize);
  NS_ENSURE_SUCCESS(rv, rv);
  SECItem wrappedSymKey = {siBuffer, (unsigned char *)wrappedKeyBuffer, wrappedKeyBufferSize};


  
  rv = DeriveKeyFromPassphrase(aPassphrase, aSalt, &pbeKey);
  NS_ENSURE_SUCCESS(rv, rv);

  char ivData[STACK_BUFFER_SIZE];
  PRUint32 ivDataSize = sizeof(ivData);
  rv = DecodeBase64(aIV, ivData, &ivDataSize);
  NS_ENSURE_SUCCESS(rv, rv);
  SECItem ivItem  = {siBuffer, (unsigned char*)ivData,  ivDataSize}; 

  
  CK_MECHANISM_TYPE wrapMech = PK11_AlgtagToMechanism(mAlgorithm);
  wrapMech = PK11_GetPadMechanism(wrapMech);
  if (wrapMech == CKM_INVALID_MECHANISM) {
    NS_WARNING("Unknown key mechanism");
    rv = NS_ERROR_FAILURE;
    goto unwrap_done;
  }

  ivParam = PK11_ParamFromIV(wrapMech, &ivItem);
  if (!ivParam) {
    NS_WARNING("Couldn't create IV param");
    rv = NS_ERROR_FAILURE;
    goto unwrap_done;
  }


  

  slot = PK11_GetInternalSlot();
  if (!slot) {
    NS_WARNING("Can't get internal PK11 slot");
    rv = NS_ERROR_FAILURE;
    goto unwrap_done;
  }


  
  
  
  
  
  
  
  keyID = &ivItem;

  privKey = PK11_UnwrapPrivKey(slot,
                               pbeKey, wrapMech, ivParam, &wrappedPrivKey,
                               nsnull,   
                               keyID,
                               PR_FALSE, 
                               PR_TRUE,  
                               CKK_RSA,
                               privKeyUsage, privKeyUsageLength,
                               nsnull);  
  if (!privKey) {
    NS_WARNING("PK11_UnwrapPrivKey failed");
    rv = NS_ERROR_FAILURE;
    goto unwrap_done;
  }

  

  
  
  symKey = PK11_PubUnwrapSymKey(privKey, &wrappedSymKey, wrapMech,
                                CKA_DECRYPT, 0);
  if (!symKey) {
    NS_WARNING("PK11_PubUnwrapSymKey failed");
    rv = NS_ERROR_FAILURE;
    goto unwrap_done;
  }

  

  if (PK11_ExtractKeyValue(symKey)) {
    NS_WARNING("PK11_ExtractKeyValue failed");
    rv = NS_ERROR_FAILURE;
    goto unwrap_done;
  }

  
  symKeyData = PK11_GetKeyData(symKey);
  if (!symKeyData) {
    NS_WARNING("PK11_GetKeyData failed");
    rv = NS_ERROR_FAILURE;
    goto unwrap_done;
  }

  rv = EncodeBase64((char *)symKeyData->data, symKeyData->len, aSymmetricKey);
  if (NS_FAILED(rv)) {
    NS_WARNING("EncodeBase64 failed");
    goto unwrap_done;
  }

unwrap_done:
  if (privKey)
    SECKEY_DestroyPrivateKey(privKey);
  if (symKey)
    PK11_FreeSymKey(symKey);
  if (pbeKey)
    PK11_FreeSymKey(pbeKey);
  if (slot)
    PK11_FreeSlot(slot);
  if (ivParam)
    SECITEM_FreeItem(ivParam, PR_TRUE);

  return rv;
}
