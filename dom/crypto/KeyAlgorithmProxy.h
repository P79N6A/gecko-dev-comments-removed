





#ifndef mozilla_dom_KeyAlgorithmProxy_h
#define mozilla_dom_KeyAlgorithmProxy_h

#include "pk11pub.h"
#include "js/StructuredClone.h"
#include "mozilla/dom/KeyAlgorithmBinding.h"
#include "mozilla/dom/WebCryptoCommon.h"

#define KEY_ALGORITHM_SC_VERSION 0x00000001

namespace mozilla {
namespace dom {



struct RsaHashedKeyAlgorithmStorage {
  nsString mName;
  KeyAlgorithm mHash;
  uint16_t mModulusLength;
  CryptoBuffer mPublicExponent;

  void
  ToKeyAlgorithm(JSContext* aCx, RsaHashedKeyAlgorithm& aRsa) const
  {
    aRsa.mName = mName;
    aRsa.mModulusLength = mModulusLength;
    aRsa.mHash.mName = mHash.mName;
    aRsa.mPublicExponent.Init(mPublicExponent.ToUint8Array(aCx));
    aRsa.mPublicExponent.ComputeLengthAndData();
  }
};



struct DhKeyAlgorithmStorage {
  nsString mName;
  CryptoBuffer mPrime;
  CryptoBuffer mGenerator;

  void
  ToKeyAlgorithm(JSContext* aCx, DhKeyAlgorithm& aDh) const
  {
    aDh.mName = mName;
    aDh.mPrime.Init(mPrime.ToUint8Array(aCx));
    aDh.mPrime.ComputeLengthAndData();
    aDh.mGenerator.Init(mGenerator.ToUint8Array(aCx));
    aDh.mGenerator.ComputeLengthAndData();
  }
};



struct KeyAlgorithmProxy
{
  enum KeyAlgorithmType {
    AES,
    HMAC,
    RSA,
    EC,
    DH,
  };
  KeyAlgorithmType mType;

  
  
  nsString mName;
  AesKeyAlgorithm mAes;
  HmacKeyAlgorithm mHmac;
  RsaHashedKeyAlgorithmStorage mRsa;
  EcKeyAlgorithm mEc;
  DhKeyAlgorithmStorage mDh;

  
  bool WriteStructuredClone(JSStructuredCloneWriter* aWriter) const;
  bool ReadStructuredClone(JSStructuredCloneReader* aReader);

  
  CK_MECHANISM_TYPE Mechanism() const;
  nsString JwkAlg() const;

  
  static CK_MECHANISM_TYPE GetMechanism(const KeyAlgorithm& aAlgorithm);
  static CK_MECHANISM_TYPE GetMechanism(const HmacKeyAlgorithm& aAlgorithm);
  static nsString GetJwkAlg(const KeyAlgorithm& aAlgorithm);

  
  void
  MakeAes(const nsString& aName, uint32_t aLength)
  {
    mType = AES;
    mName = aName;
    mAes.mName = aName;
    mAes.mLength = aLength;
  }

  void
  MakeHmac(uint32_t aLength, const nsString& aHashName)
  {
    mType = HMAC;
    mName = NS_LITERAL_STRING(WEBCRYPTO_ALG_HMAC);
    mHmac.mName = NS_LITERAL_STRING(WEBCRYPTO_ALG_HMAC);
    mHmac.mLength = aLength;
    mHmac.mHash.mName = aHashName;
  }

  bool
  MakeRsa(const nsString& aName, uint32_t aModulusLength,
         const CryptoBuffer& aPublicExponent, const nsString& aHashName)
  {
    mType = RSA;
    mName = aName;
    mRsa.mName = aName;
    mRsa.mModulusLength = aModulusLength;
    mRsa.mHash.mName = aHashName;
    if (!mRsa.mPublicExponent.Assign(aPublicExponent)) {
      return false;
    }
    return true;
  }

  void
  MakeEc(const nsString& aName, const nsString& aNamedCurve)
  {
    mType = EC;
    mName = aName;
    mEc.mName = aName;
    mEc.mNamedCurve = aNamedCurve;
  }

  bool
  MakeDh(const nsString& aName, const CryptoBuffer& aPrime,
         const CryptoBuffer& aGenerator)
  {
    mType = DH;
    mName = aName;
    mDh.mName = aName;
    if (!mDh.mPrime.Assign(aPrime)) {
      return false;
    }
    if (!mDh.mGenerator.Assign(aGenerator)) {
      return false;
    }
    return true;
  }
};

} 
} 

#endif 
