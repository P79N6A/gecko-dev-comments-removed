





#ifndef mozilla_dom_WebCryptoTask_h
#define mozilla_dom_WebCryptoTask_h

#include "CryptoTask.h"

#include "nsIGlobalObject.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/DOMException.h"
#include "mozilla/dom/SubtleCryptoBinding.h"
#include "mozilla/dom/Key.h"

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
  static WebCryptoTask* EncryptDecryptTask(JSContext* aCx,
                           const ObjectOrString& aAlgorithm,
                           Key& aKey,
                           const CryptoOperationData& aData,
                           bool aEncrypt);

  static WebCryptoTask* SignVerifyTask(JSContext* aCx,
                          const ObjectOrString& aAlgorithm,
                          Key& aKey,
                          const CryptoOperationData& aSignature,
                          const CryptoOperationData& aData,
                          bool aSign);

public:
  static WebCryptoTask* EncryptTask(JSContext* aCx,
                          const ObjectOrString& aAlgorithm,
                          Key& aKey,
                          const CryptoOperationData& aData)
  {
    return EncryptDecryptTask(aCx, aAlgorithm, aKey, aData, true);
  }

  static WebCryptoTask* DecryptTask(JSContext* aCx,
                          const ObjectOrString& aAlgorithm,
                          Key& aKey,
                          const CryptoOperationData& aData)
  {
    return EncryptDecryptTask(aCx, aAlgorithm, aKey, aData, false);
  }

  static WebCryptoTask* SignTask(JSContext* aCx,
                          const ObjectOrString& aAlgorithm,
                          Key& aKey,
                          const CryptoOperationData& aData)
  {
    CryptoOperationData dummy;
    return SignVerifyTask(aCx, aAlgorithm, aKey, dummy, aData, true);
  }

  static WebCryptoTask* VerifyTask(JSContext* aCx,
                          const ObjectOrString& aAlgorithm,
                          Key& aKey,
                          const CryptoOperationData& aSignature,
                          const CryptoOperationData& aData)
  {
    return SignVerifyTask(aCx, aAlgorithm, aKey, aSignature, aData, false);
  }

  static WebCryptoTask* DigestTask(JSContext* aCx,
                          const ObjectOrString& aAlgorithm,
                          const CryptoOperationData& aData);

  static WebCryptoTask* ImportKeyTask(JSContext* aCx,
                          const nsAString& aFormat,
                          const KeyData& aKeyData,
                          const ObjectOrString& aAlgorithm,
                          bool aExtractable,
                          const Sequence<nsString>& aKeyUsages);
  static WebCryptoTask* ExportKeyTask(const nsAString& aFormat,
                          Key& aKey);
  static WebCryptoTask* GenerateKeyTask(JSContext* aCx,
                          const ObjectOrString& aAlgorithm,
                          bool aExtractable,
                          const Sequence<nsString>& aKeyUsages);

  static WebCryptoTask* DeriveKeyTask(JSContext* aCx,
                          const ObjectOrString& aAlgorithm,
                          Key& aBaseKey,
                          const ObjectOrString& aDerivedKeyType,
                          bool extractable,
                          const Sequence<nsString>& aKeyUsages);
  static WebCryptoTask* DeriveBitsTask(JSContext* aCx,
                          const ObjectOrString& aAlgorithm,
                          Key& aKey,
                          uint32_t aLength);

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

  void FailWithError(nsresult aRv)
  {
    MOZ_ASSERT(NS_IsMainThread());

    
    
    mResultPromise->MaybeReject(aRv);
    
    mResultPromise = nullptr;
    Cleanup();
  }

  
  
  virtual void ReleaseNSSResources() MOZ_OVERRIDE {}

  virtual nsresult CalculateResult() MOZ_OVERRIDE MOZ_FINAL
  {
    MOZ_ASSERT(!NS_IsMainThread());

    if (NS_FAILED(mEarlyRv)) {
      return mEarlyRv;
    }

    if (isAlreadyShutDown()) {
      return NS_ERROR_DOM_UNKNOWN_ERR;
    }

    return DoCrypto();
  }

  virtual void CallCallback(nsresult rv) MOZ_OVERRIDE MOZ_FINAL
  {
    MOZ_ASSERT(NS_IsMainThread());
    if (NS_FAILED(rv)) {
      FailWithError(rv);
      return;
    }

    nsresult rv2 = AfterCrypto();
    if (NS_FAILED(rv2)) {
      FailWithError(rv2);
      return;
    }

    Resolve();

    
    mResultPromise = nullptr;
    Cleanup();
  }
};

} 
} 

#endif 
