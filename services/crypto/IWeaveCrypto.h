



#ifndef __gen_IWeaveCrypto_h__
#define __gen_IWeaveCrypto_h__


#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif


#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif


#define IWEAVECRYPTO_IID_STR "f4463043-315e-41f3-b779-82e900e6fffa"

#define IWEAVECRYPTO_IID \
  {0xf4463043, 0x315e, 0x41f3, \
    { 0xb7, 0x79, 0x82, 0xe9, 0x00, 0xe6, 0xff, 0xfa }}

class NS_NO_VTABLE NS_SCRIPTABLE IWeaveCrypto : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(IWEAVECRYPTO_IID)

  



  enum { DES_EDE3_CBC = 156U };

  enum { AES_128_CBC = 184U };

  enum { AES_192_CBC = 186U };

  enum { AES_256_CBC = 188U };

  





  
  NS_SCRIPTABLE NS_IMETHOD GetAlgorithm(PRUint32 *aAlgorithm) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetAlgorithm(PRUint32 aAlgorithm) = 0;

  




  
  NS_SCRIPTABLE NS_IMETHOD GetKeypairBits(PRUint32 *aKeypairBits) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetKeypairBits(PRUint32 aKeypairBits) = 0;

  











  
  NS_SCRIPTABLE NS_IMETHOD Encrypt(const nsACString & clearText, const nsACString & symmetricKey, const nsACString & iv, nsACString & _retval NS_OUTPARAM) = 0;

  











  
  NS_SCRIPTABLE NS_IMETHOD Decrypt(const nsACString & cipherText, const nsACString & symmetricKey, const nsACString & iv, nsACString & _retval NS_OUTPARAM) = 0;

  














  
  NS_SCRIPTABLE NS_IMETHOD GenerateKeypair(const nsACString & aPassphrase, const nsACString & aSalt, const nsACString & aIV, nsACString & aEncodedPublicKey NS_OUTPARAM, nsACString & aWrappedPrivateKey NS_OUTPARAM) = 0;

  
  NS_SCRIPTABLE NS_IMETHOD GenerateRandomKey(nsACString & _retval NS_OUTPARAM) = 0;

  
  NS_SCRIPTABLE NS_IMETHOD GenerateRandomIV(nsACString & _retval NS_OUTPARAM) = 0;

  
  NS_SCRIPTABLE NS_IMETHOD GenerateRandomBytes(PRUint32 aByteCount, nsACString & _retval NS_OUTPARAM) = 0;

  










  
  NS_SCRIPTABLE NS_IMETHOD WrapSymmetricKey(const nsACString & aSymmetricKey, const nsACString & aEncodedPublicKey, nsACString & _retval NS_OUTPARAM) = 0;

  
















  
  NS_SCRIPTABLE NS_IMETHOD UnwrapSymmetricKey(const nsACString & aWrappedSymmetricKey, const nsACString & aWrappedPrivateKey, const nsACString & aPassphrase, const nsACString & aSalt, const nsACString & aIV, nsACString & _retval NS_OUTPARAM) = 0;

  















  
  NS_SCRIPTABLE NS_IMETHOD RewrapPrivateKey(const nsACString & aWrappedPrivateKey, const nsACString & aPassphrase, const nsACString & aSalt, const nsACString & aIV, const nsACString & aNewPassphrase, nsACString & _retval NS_OUTPARAM) = 0;

  













  
  NS_SCRIPTABLE NS_IMETHOD VerifyPassphrase(const nsACString & aWrappedPrivateKey, const nsACString & aPassphrase, const nsACString & aSalt, const nsACString & aIV, PRBool *_retval NS_OUTPARAM) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(IWeaveCrypto, IWEAVECRYPTO_IID)


#define NS_DECL_IWEAVECRYPTO \
  NS_SCRIPTABLE NS_IMETHOD GetAlgorithm(PRUint32 *aAlgorithm); \
  NS_SCRIPTABLE NS_IMETHOD SetAlgorithm(PRUint32 aAlgorithm); \
  NS_SCRIPTABLE NS_IMETHOD GetKeypairBits(PRUint32 *aKeypairBits); \
  NS_SCRIPTABLE NS_IMETHOD SetKeypairBits(PRUint32 aKeypairBits); \
  NS_SCRIPTABLE NS_IMETHOD Encrypt(const nsACString & clearText, const nsACString & symmetricKey, const nsACString & iv, nsACString & _retval NS_OUTPARAM); \
  NS_SCRIPTABLE NS_IMETHOD Decrypt(const nsACString & cipherText, const nsACString & symmetricKey, const nsACString & iv, nsACString & _retval NS_OUTPARAM); \
  NS_SCRIPTABLE NS_IMETHOD GenerateKeypair(const nsACString & aPassphrase, const nsACString & aSalt, const nsACString & aIV, nsACString & aEncodedPublicKey NS_OUTPARAM, nsACString & aWrappedPrivateKey NS_OUTPARAM); \
  NS_SCRIPTABLE NS_IMETHOD GenerateRandomKey(nsACString & _retval NS_OUTPARAM); \
  NS_SCRIPTABLE NS_IMETHOD GenerateRandomIV(nsACString & _retval NS_OUTPARAM); \
  NS_SCRIPTABLE NS_IMETHOD GenerateRandomBytes(PRUint32 aByteCount, nsACString & _retval NS_OUTPARAM); \
  NS_SCRIPTABLE NS_IMETHOD WrapSymmetricKey(const nsACString & aSymmetricKey, const nsACString & aEncodedPublicKey, nsACString & _retval NS_OUTPARAM); \
  NS_SCRIPTABLE NS_IMETHOD UnwrapSymmetricKey(const nsACString & aWrappedSymmetricKey, const nsACString & aWrappedPrivateKey, const nsACString & aPassphrase, const nsACString & aSalt, const nsACString & aIV, nsACString & _retval NS_OUTPARAM); \
  NS_SCRIPTABLE NS_IMETHOD RewrapPrivateKey(const nsACString & aWrappedPrivateKey, const nsACString & aPassphrase, const nsACString & aSalt, const nsACString & aIV, const nsACString & aNewPassphrase, nsACString & _retval NS_OUTPARAM); \
  NS_SCRIPTABLE NS_IMETHOD VerifyPassphrase(const nsACString & aWrappedPrivateKey, const nsACString & aPassphrase, const nsACString & aSalt, const nsACString & aIV, PRBool *_retval NS_OUTPARAM); 


#define NS_FORWARD_IWEAVECRYPTO(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetAlgorithm(PRUint32 *aAlgorithm) { return _to GetAlgorithm(aAlgorithm); } \
  NS_SCRIPTABLE NS_IMETHOD SetAlgorithm(PRUint32 aAlgorithm) { return _to SetAlgorithm(aAlgorithm); } \
  NS_SCRIPTABLE NS_IMETHOD GetKeypairBits(PRUint32 *aKeypairBits) { return _to GetKeypairBits(aKeypairBits); } \
  NS_SCRIPTABLE NS_IMETHOD SetKeypairBits(PRUint32 aKeypairBits) { return _to SetKeypairBits(aKeypairBits); } \
  NS_SCRIPTABLE NS_IMETHOD Encrypt(const nsACString & clearText, const nsACString & symmetricKey, const nsACString & iv, nsACString & _retval NS_OUTPARAM) { return _to Encrypt(clearText, symmetricKey, iv, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD Decrypt(const nsACString & cipherText, const nsACString & symmetricKey, const nsACString & iv, nsACString & _retval NS_OUTPARAM) { return _to Decrypt(cipherText, symmetricKey, iv, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD GenerateKeypair(const nsACString & aPassphrase, const nsACString & aSalt, const nsACString & aIV, nsACString & aEncodedPublicKey NS_OUTPARAM, nsACString & aWrappedPrivateKey NS_OUTPARAM) { return _to GenerateKeypair(aPassphrase, aSalt, aIV, aEncodedPublicKey, aWrappedPrivateKey); } \
  NS_SCRIPTABLE NS_IMETHOD GenerateRandomKey(nsACString & _retval NS_OUTPARAM) { return _to GenerateRandomKey(_retval); } \
  NS_SCRIPTABLE NS_IMETHOD GenerateRandomIV(nsACString & _retval NS_OUTPARAM) { return _to GenerateRandomIV(_retval); } \
  NS_SCRIPTABLE NS_IMETHOD GenerateRandomBytes(PRUint32 aByteCount, nsACString & _retval NS_OUTPARAM) { return _to GenerateRandomBytes(aByteCount, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD WrapSymmetricKey(const nsACString & aSymmetricKey, const nsACString & aEncodedPublicKey, nsACString & _retval NS_OUTPARAM) { return _to WrapSymmetricKey(aSymmetricKey, aEncodedPublicKey, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD UnwrapSymmetricKey(const nsACString & aWrappedSymmetricKey, const nsACString & aWrappedPrivateKey, const nsACString & aPassphrase, const nsACString & aSalt, const nsACString & aIV, nsACString & _retval NS_OUTPARAM) { return _to UnwrapSymmetricKey(aWrappedSymmetricKey, aWrappedPrivateKey, aPassphrase, aSalt, aIV, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD RewrapPrivateKey(const nsACString & aWrappedPrivateKey, const nsACString & aPassphrase, const nsACString & aSalt, const nsACString & aIV, const nsACString & aNewPassphrase, nsACString & _retval NS_OUTPARAM) { return _to RewrapPrivateKey(aWrappedPrivateKey, aPassphrase, aSalt, aIV, aNewPassphrase, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD VerifyPassphrase(const nsACString & aWrappedPrivateKey, const nsACString & aPassphrase, const nsACString & aSalt, const nsACString & aIV, PRBool *_retval NS_OUTPARAM) { return _to VerifyPassphrase(aWrappedPrivateKey, aPassphrase, aSalt, aIV, _retval); } 


#define NS_FORWARD_SAFE_IWEAVECRYPTO(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetAlgorithm(PRUint32 *aAlgorithm) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetAlgorithm(aAlgorithm); } \
  NS_SCRIPTABLE NS_IMETHOD SetAlgorithm(PRUint32 aAlgorithm) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetAlgorithm(aAlgorithm); } \
  NS_SCRIPTABLE NS_IMETHOD GetKeypairBits(PRUint32 *aKeypairBits) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetKeypairBits(aKeypairBits); } \
  NS_SCRIPTABLE NS_IMETHOD SetKeypairBits(PRUint32 aKeypairBits) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetKeypairBits(aKeypairBits); } \
  NS_SCRIPTABLE NS_IMETHOD Encrypt(const nsACString & clearText, const nsACString & symmetricKey, const nsACString & iv, nsACString & _retval NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->Encrypt(clearText, symmetricKey, iv, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD Decrypt(const nsACString & cipherText, const nsACString & symmetricKey, const nsACString & iv, nsACString & _retval NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->Decrypt(cipherText, symmetricKey, iv, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD GenerateKeypair(const nsACString & aPassphrase, const nsACString & aSalt, const nsACString & aIV, nsACString & aEncodedPublicKey NS_OUTPARAM, nsACString & aWrappedPrivateKey NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->GenerateKeypair(aPassphrase, aSalt, aIV, aEncodedPublicKey, aWrappedPrivateKey); } \
  NS_SCRIPTABLE NS_IMETHOD GenerateRandomKey(nsACString & _retval NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->GenerateRandomKey(_retval); } \
  NS_SCRIPTABLE NS_IMETHOD GenerateRandomIV(nsACString & _retval NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->GenerateRandomIV(_retval); } \
  NS_SCRIPTABLE NS_IMETHOD GenerateRandomBytes(PRUint32 aByteCount, nsACString & _retval NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->GenerateRandomBytes(aByteCount, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD WrapSymmetricKey(const nsACString & aSymmetricKey, const nsACString & aEncodedPublicKey, nsACString & _retval NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->WrapSymmetricKey(aSymmetricKey, aEncodedPublicKey, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD UnwrapSymmetricKey(const nsACString & aWrappedSymmetricKey, const nsACString & aWrappedPrivateKey, const nsACString & aPassphrase, const nsACString & aSalt, const nsACString & aIV, nsACString & _retval NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->UnwrapSymmetricKey(aWrappedSymmetricKey, aWrappedPrivateKey, aPassphrase, aSalt, aIV, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD RewrapPrivateKey(const nsACString & aWrappedPrivateKey, const nsACString & aPassphrase, const nsACString & aSalt, const nsACString & aIV, const nsACString & aNewPassphrase, nsACString & _retval NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->RewrapPrivateKey(aWrappedPrivateKey, aPassphrase, aSalt, aIV, aNewPassphrase, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD VerifyPassphrase(const nsACString & aWrappedPrivateKey, const nsACString & aPassphrase, const nsACString & aSalt, const nsACString & aIV, PRBool *_retval NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->VerifyPassphrase(aWrappedPrivateKey, aPassphrase, aSalt, aIV, _retval); } 

#if 0



class _MYCLASS_ : public IWeaveCrypto
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IWEAVECRYPTO

  _MYCLASS_();

private:
  ~_MYCLASS_();

protected:
  
};


NS_IMPL_ISUPPORTS1(_MYCLASS_, IWeaveCrypto)

_MYCLASS_::_MYCLASS_()
{
  
}

_MYCLASS_::~_MYCLASS_()
{
  
}


NS_IMETHODIMP _MYCLASS_::GetAlgorithm(PRUint32 *aAlgorithm)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP _MYCLASS_::SetAlgorithm(PRUint32 aAlgorithm)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP _MYCLASS_::GetKeypairBits(PRUint32 *aKeypairBits)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP _MYCLASS_::SetKeypairBits(PRUint32 aKeypairBits)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP _MYCLASS_::Encrypt(const nsACString & clearText, const nsACString & symmetricKey, const nsACString & iv, nsACString & _retval NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP _MYCLASS_::Decrypt(const nsACString & cipherText, const nsACString & symmetricKey, const nsACString & iv, nsACString & _retval NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP _MYCLASS_::GenerateKeypair(const nsACString & aPassphrase, const nsACString & aSalt, const nsACString & aIV, nsACString & aEncodedPublicKey NS_OUTPARAM, nsACString & aWrappedPrivateKey NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP _MYCLASS_::GenerateRandomKey(nsACString & _retval NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP _MYCLASS_::GenerateRandomIV(nsACString & _retval NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP _MYCLASS_::GenerateRandomBytes(PRUint32 aByteCount, nsACString & _retval NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP _MYCLASS_::WrapSymmetricKey(const nsACString & aSymmetricKey, const nsACString & aEncodedPublicKey, nsACString & _retval NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP _MYCLASS_::UnwrapSymmetricKey(const nsACString & aWrappedSymmetricKey, const nsACString & aWrappedPrivateKey, const nsACString & aPassphrase, const nsACString & aSalt, const nsACString & aIV, nsACString & _retval NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP _MYCLASS_::RewrapPrivateKey(const nsACString & aWrappedPrivateKey, const nsACString & aPassphrase, const nsACString & aSalt, const nsACString & aIV, const nsACString & aNewPassphrase, nsACString & _retval NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP _MYCLASS_::VerifyPassphrase(const nsACString & aWrappedPrivateKey, const nsACString & aPassphrase, const nsACString & aSalt, const nsACString & aIV, PRBool *_retval NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


#endif


#endif
