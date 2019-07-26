



#include "nsComponentManagerUtils.h"
#include "nsCOMPtr.h"
#include "nsKeyModule.h"
#include "nsString.h"

NS_IMPL_ISUPPORTS1(nsKeyObject, nsIKeyObject)

nsKeyObject::nsKeyObject()
  : mKeyType(0), mSymKey(nullptr), mPrivateKey(nullptr),
    mPublicKey(nullptr)
{
}

nsKeyObject::~nsKeyObject()
{
  CleanUp();
}

void
nsKeyObject::CleanUp()
{
  switch (mKeyType) {
    case nsIKeyObject::SYM_KEY:
      PK11_FreeSymKey(mSymKey);
      break;
    
    case nsIKeyObject::PRIVATE_KEY:
      PK11_DeleteTokenPrivateKey(mPrivateKey, true );
      break;

    case nsIKeyObject::PUBLIC_KEY:
      PK11_DeleteTokenPublicKey(mPublicKey);
      break;
    
    default:
      
      break;
  }
  mKeyType = 0;
}





NS_IMETHODIMP
nsKeyObject::InitKey(int16_t aAlgorithm, void * aKey)
{
  
  CleanUp();

  switch (aAlgorithm) {
    case nsIKeyObject::RC4:
    case nsIKeyObject::HMAC:
      mSymKey = reinterpret_cast<PK11SymKey*>(aKey);

      if (!mSymKey) {
        NS_ERROR("no symkey");
        break;
      }
      mKeyType = nsIKeyObject::SYM_KEY;
      break;

    case nsIKeyObject::AES_CBC:
      return NS_ERROR_NOT_IMPLEMENTED;

    default:
      return NS_ERROR_INVALID_ARG;
  }

  
  if (!mSymKey && !mPrivateKey && !mPublicKey)
    return NS_ERROR_FAILURE;

  return NS_OK;
}


NS_IMETHODIMP
nsKeyObject::GetKeyObj(void * *_retval)
{
  if (mKeyType == 0)
    return NS_ERROR_NOT_INITIALIZED;

  switch (mKeyType) {
    case nsIKeyObject::SYM_KEY:
      *_retval = (void*)mSymKey;
      break;

    case nsIKeyObject::PRIVATE_KEY:
      *_retval = (void*)mPublicKey;
      break;

    case nsIKeyObject::PUBLIC_KEY:
      *_retval = (void*)mPrivateKey;
      break;

    default:
      
      return NS_ERROR_FAILURE;
  }
  return NS_OK;
}


NS_IMETHODIMP
nsKeyObject::GetType(int16_t *_retval)
{
  if (mKeyType == 0)
    return NS_ERROR_NOT_INITIALIZED;

  *_retval = mKeyType;
  return NS_OK;
}




NS_IMPL_THREADSAFE_ISUPPORTS1(nsKeyObjectFactory, nsIKeyObjectFactory)

nsKeyObjectFactory::nsKeyObjectFactory()
{
}


NS_IMETHODIMP
nsKeyObjectFactory::LookupKeyByName(const nsACString & aName,
                                    nsIKeyObject **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
 
NS_IMETHODIMP
nsKeyObjectFactory::UnwrapKey(int16_t aAlgorithm, const uint8_t *aWrappedKey,
                              uint32_t aWrappedKeyLen, nsIKeyObject **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsKeyObjectFactory::KeyFromString(int16_t aAlgorithm, const nsACString & aKey,
                                  nsIKeyObject **_retval)
{
  CK_MECHANISM_TYPE cipherMech;
  CK_ATTRIBUTE_TYPE cipherOperation;
  switch (aAlgorithm)
  {
  case nsIKeyObject::HMAC:
    cipherMech = CKM_GENERIC_SECRET_KEY_GEN;
    cipherOperation = CKA_SIGN;
    break;

  case nsIKeyObject::RC4:
    cipherMech = CKM_RC4;
    cipherOperation = CKA_ENCRYPT;
    break;

  default:
    return NS_ERROR_INVALID_ARG;
  }

  nsresult rv;
  nsCOMPtr<nsIKeyObject> key =
      do_CreateInstance(NS_KEYMODULEOBJECT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  const nsCString& flatKey = PromiseFlatCString(aKey);
  SECItem keyItem;
  keyItem.data = (unsigned char*)flatKey.get();
  keyItem.len = flatKey.Length();

  PK11SlotInfo *slot = nullptr;
  slot = PK11_GetBestSlot(cipherMech, nullptr);
  if (!slot) {
    NS_ERROR("no slot");
    return NS_ERROR_FAILURE;
  }

  PK11SymKey* symKey = PK11_ImportSymKey(slot, cipherMech, PK11_OriginUnwrap,
                                         cipherOperation, &keyItem, nullptr);
  
  if (slot)
    PK11_FreeSlot(slot);

  if (!symKey) {
    return NS_ERROR_FAILURE;
  }
  
  rv = key->InitKey(aAlgorithm, (void*)symKey);
  NS_ENSURE_SUCCESS(rv, rv);

  key.swap(*_retval);
  return NS_OK;
}
