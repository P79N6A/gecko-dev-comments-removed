





#ifndef mozilla_dom_WebCryptoTask_h
#define mozilla_dom_WebCryptoTask_h

#include "CryptoTask.h"

#include "nsIGlobalObject.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/DOMException.h"
#include "mozilla/dom/SubtleCryptoBinding.h"
#include "mozilla/dom/CryptoKey.h"

namespace mozilla {
namespace dom {

typedef ArrayBufferViewOrArrayBuffer CryptoOperationData;
typedef ArrayBufferViewOrArrayBuffer KeyData;































#define MAYBE_EARLY_FAIL(rv) \
if (NS_FAILED(rv)) { \
  FailWithError(rv); \
  Skip(); \
  return; \
}

class WebCryptoTask : public CryptoTask
{
public:
  virtual void DispatchWithPromise(Promise* aResultPromise)
  {
    MOZ_ASSERT(NS_IsMainThread());
    mResultPromise = aResultPromise;

    
    MAYBE_EARLY_FAIL(mEarlyRv)

    
    mEarlyRv = BeforeCrypto();
    MAYBE_EARLY_FAIL(mEarlyRv)

    
    if (mEarlyComplete) {
      CallCallback(mEarlyRv);
      Skip();
      return;
    }

     mEarlyRv = Dispatch("SubtleCrypto");
     MAYBE_EARLY_FAIL(mEarlyRv)
  }

protected:
  static WebCryptoTask* CreateEncryptDecryptTask(JSContext* aCx,
                           const ObjectOrString& aAlgorithm,
                           CryptoKey& aKey,
                           const CryptoOperationData& aData,
                           bool aEncrypt);

  static WebCryptoTask* CreateSignVerifyTask(JSContext* aCx,
                          const ObjectOrString& aAlgorithm,
                          CryptoKey& aKey,
                          const CryptoOperationData& aSignature,
                          const CryptoOperationData& aData,
                          bool aSign);

public:
  static WebCryptoTask* CreateEncryptTask(JSContext* aCx,
                          const ObjectOrString& aAlgorithm,
                          CryptoKey& aKey,
                          const CryptoOperationData& aData)
  {
    return CreateEncryptDecryptTask(aCx, aAlgorithm, aKey, aData, true);
  }

  static WebCryptoTask* CreateDecryptTask(JSContext* aCx,
                          const ObjectOrString& aAlgorithm,
                          CryptoKey& aKey,
                          const CryptoOperationData& aData)
  {
    return CreateEncryptDecryptTask(aCx, aAlgorithm, aKey, aData, false);
  }

  static WebCryptoTask* CreateSignTask(JSContext* aCx,
                          const ObjectOrString& aAlgorithm,
                          CryptoKey& aKey,
                          const CryptoOperationData& aData)
  {
    CryptoOperationData dummy;
    dummy.SetAsArrayBuffer(aCx);
    return CreateSignVerifyTask(aCx, aAlgorithm, aKey, dummy, aData, true);
  }

  static WebCryptoTask* CreateVerifyTask(JSContext* aCx,
                          const ObjectOrString& aAlgorithm,
                          CryptoKey& aKey,
                          const CryptoOperationData& aSignature,
                          const CryptoOperationData& aData)
  {
    return CreateSignVerifyTask(aCx, aAlgorithm, aKey, aSignature, aData, false);
  }

  static WebCryptoTask* CreateDigestTask(JSContext* aCx,
                          const ObjectOrString& aAlgorithm,
                          const CryptoOperationData& aData);

  static WebCryptoTask* CreateImportKeyTask(JSContext* aCx,
                          const nsAString& aFormat,
                          JS::Handle<JSObject*> aKeyData,
                          const ObjectOrString& aAlgorithm,
                          bool aExtractable,
                          const Sequence<nsString>& aKeyUsages);
  static WebCryptoTask* CreateExportKeyTask(const nsAString& aFormat,
                          CryptoKey& aKey);
  static WebCryptoTask* CreateGenerateKeyTask(JSContext* aCx,
                          const ObjectOrString& aAlgorithm,
                          bool aExtractable,
                          const Sequence<nsString>& aKeyUsages);

  static WebCryptoTask* CreateDeriveKeyTask(JSContext* aCx,
                          const ObjectOrString& aAlgorithm,
                          CryptoKey& aBaseKey,
                          const ObjectOrString& aDerivedKeyType,
                          bool extractable,
                          const Sequence<nsString>& aKeyUsages);
  static WebCryptoTask* CreateDeriveBitsTask(JSContext* aCx,
                          const ObjectOrString& aAlgorithm,
                          CryptoKey& aKey,
                          uint32_t aLength);

  static WebCryptoTask* CreateWrapKeyTask(JSContext* aCx,
                          const nsAString& aFormat,
                          CryptoKey& aKey,
                          CryptoKey& aWrappingKey,
                          const ObjectOrString& aWrapAlgorithm);
  static WebCryptoTask* CreateUnwrapKeyTask(JSContext* aCx,
                          const nsAString& aFormat,
                          const ArrayBufferViewOrArrayBuffer& aWrappedKey,
                          CryptoKey& aUnwrappingKey,
                          const ObjectOrString& aUnwrapAlgorithm,
                          const ObjectOrString& aUnwrappedKeyAlgorithm,
                          bool aExtractable,
                          const Sequence<nsString>& aKeyUsages);

protected:
  nsRefPtr<Promise> mResultPromise;
  nsresult mEarlyRv;
  bool mEarlyComplete;

  WebCryptoTask()
    : mEarlyRv(NS_OK)
    , mEarlyComplete(false)
  {}

  
  
  virtual nsresult BeforeCrypto() { return NS_OK; }
  virtual nsresult DoCrypto() { return NS_OK; }
  virtual nsresult AfterCrypto() { return NS_OK; }
  virtual void Resolve() {}
  virtual void Cleanup() {}

  void FailWithError(nsresult aRv);

  
  
  virtual void ReleaseNSSResources() override {}

  virtual nsresult CalculateResult() override final;

  virtual void CallCallback(nsresult rv) override final;
};


class GenerateAsymmetricKeyTask : public WebCryptoTask
{
public:
  GenerateAsymmetricKeyTask(JSContext* aCx,
                            const ObjectOrString& aAlgorithm, bool aExtractable,
                            const Sequence<nsString>& aKeyUsages);
protected:
  ScopedPLArenaPool mArena;
  CryptoKeyPair mKeyPair;
  nsString mAlgName;
  CK_MECHANISM_TYPE mMechanism;
  PK11RSAGenParams mRsaParams;
  SECKEYDHParams mDhParams;
  nsString mNamedCurve;

  virtual void ReleaseNSSResources() override;
  virtual nsresult DoCrypto() override;
  virtual void Resolve() override;

private:
  ScopedSECKEYPublicKey mPublicKey;
  ScopedSECKEYPrivateKey mPrivateKey;
};

} 
} 

#endif 
