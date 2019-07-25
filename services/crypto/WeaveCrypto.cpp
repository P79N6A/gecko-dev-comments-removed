






































#include "WeaveCrypto.h"

#include "nsStringAPI.h"
#include "nsAutoPtr.h"
#include "plbase64.h"
#include "secerr.h"
#include "secpkcs7.h"

NS_IMPL_ISUPPORTS1(WeaveCrypto, IWeaveCrypto)

WeaveCrypto::WeaveCrypto()
: mAlgorithm(SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_3KEY_TRIPLE_DES_CBC)
{
}

WeaveCrypto::~WeaveCrypto()
{
}

nsresult 
WeaveCrypto::EncodeBase64(const nsACString& binary, nsACString& retval)
{
  PRUint32 encodedLength = (binary.Length() * 4 + 2) / 3;

  nsAutoArrayPtr<char> encoded;
  encoded = new char[encodedLength + 2];
  NS_ENSURE_TRUE(encoded, NS_ERROR_OUT_OF_MEMORY);

  PromiseFlatCString fBinary(binary);
  PL_Base64Encode(fBinary.get(), fBinary.Length(), encoded);

  retval.Assign(encoded, encodedLength);
  return NS_OK;
}

nsresult 
WeaveCrypto::DecodeBase64(const nsACString& base64, nsACString& retval)
{
  PromiseFlatCString flat(base64);

  PRUint32 decodedLength = (flat.Length() * 3) / 4;

  nsAutoArrayPtr<char> decoded;
  decoded = new char[decodedLength];
  NS_ENSURE_TRUE(decoded, NS_ERROR_OUT_OF_MEMORY);

  if (!PL_Base64Decode(flat.get(), flat.Length(), decoded))
    return NS_ERROR_ILLEGAL_VALUE;

  retval.Assign(decoded, decodedLength);
  return NS_OK;
}


void
WeaveCrypto::StoreToStringCallback(void *arg, const char *buf, unsigned long len)
{
  nsACString* aText = (nsACString*)arg;
  aText->Append(buf, len);
}


PK11SymKey * 
WeaveCrypto::GetSymmetricKeyCallback(void *arg, SECAlgorithmID *algid)
{
  SECItem *pwitem = (SECItem *)arg;

  PK11SlotInfo *slot = PK11_GetInternalSlot();
  if (!slot)
    return nsnull;

  PK11SymKey *key = PK11_PBEKeyGen(slot, algid, pwitem, PR_FALSE, nsnull);
  PK11_FreeSlot(slot);

  if (key)
    PK11_SetSymKeyUserData(key, pwitem, nsnull);

  return key;
}


PRBool 
WeaveCrypto::DecryptionAllowedCallback(SECAlgorithmID *algid, PK11SymKey *key)
{
  return PR_TRUE;
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
WeaveCrypto::Encrypt(const nsACString& aPass,
                     const nsACString& aClearText,
                     nsACString& aCipherText)
{
  nsresult rv = NS_ERROR_FAILURE;

  SEC_PKCS7ContentInfo *cinfo = SEC_PKCS7CreateEncryptedData(mAlgorithm, 0, nsnull, nsnull);
  NS_ENSURE_TRUE(cinfo, NS_ERROR_FAILURE);
  
  SECAlgorithmID* encalgid = SEC_PKCS7GetEncryptionAlgorithm(cinfo);
  if (encalgid)
  {
    PromiseFlatCString fPass(aPass);
    SECItem pwitem = {siBuffer, (unsigned char*)fPass.get(), fPass.Length()}; 
    PK11SymKey *key = GetSymmetricKeyCallback(&pwitem, encalgid);
    if (key)
    {
      nsCString result;
      SEC_PKCS7EncoderContext *ecx = SEC_PKCS7EncoderStart(
          cinfo, StoreToStringCallback, &result, key);
      PK11_FreeSymKey(key);

      if (ecx) 
      {
        SECStatus srv;
        
        PromiseFlatCString fClearText(aClearText);
        srv = SEC_PKCS7EncoderUpdate(ecx, fClearText.get(), fClearText.Length());

        SEC_PKCS7EncoderFinish(ecx, nsnull, nsnull);

        if (SECSuccess == srv)
          rv = EncodeBase64(result, aCipherText);
      }
      else
      {
        NS_WARNING("Could not create PKCS#7 encoder context");
      } 
    }
  }

  SEC_PKCS7DestroyContentInfo(cinfo);
  return rv;
}

NS_IMETHODIMP
WeaveCrypto::Decrypt(const nsACString& aPass,
                     const nsACString& aCipherText,
                     nsACString& aClearText)
{
  nsresult rv;

  nsCString fCipherText;
  rv = DecodeBase64(aCipherText, fCipherText);
  NS_ENSURE_SUCCESS(rv, rv);  

  aClearText.Truncate();

  PromiseFlatCString fPass(aPass);
  SECItem pwitem = {siBuffer, (unsigned char*)fPass.get(), fPass.Length()}; 
  SEC_PKCS7DecoderContext *dcx = SEC_PKCS7DecoderStart(
      StoreToStringCallback, &aClearText, nsnull, nsnull,
      GetSymmetricKeyCallback, &pwitem, 
      DecryptionAllowedCallback);
  NS_ENSURE_TRUE(dcx, NS_ERROR_FAILURE);

  SECStatus srv = SEC_PKCS7DecoderUpdate(dcx, fCipherText.get(), fCipherText.Length());
  rv = (SECSuccess == srv) ? NS_OK : NS_ERROR_FAILURE;

  SEC_PKCS7ContentInfo *cinfo = SEC_PKCS7DecoderFinish(dcx);
  if (cinfo)
    SEC_PKCS7DestroyContentInfo(cinfo);

  return rv;
}
