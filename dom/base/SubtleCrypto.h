





#ifndef mozilla_dom_SubtleCrypto_h
#define mozilla_dom_SubtleCrypto_h

#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "nsPIDOMWindow.h"
#include "mozilla/dom/UnionTypes.h"
#include "mozilla/dom/Key.h"
#include "js/TypeDecls.h"

namespace mozilla {
namespace dom {

class Promise;

typedef ArrayBufferViewOrArrayBuffer CryptoOperationData;
typedef ArrayBufferViewOrArrayBuffer KeyData;

class SubtleCrypto MOZ_FINAL : public nsISupports,
                               public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(SubtleCrypto)

public:
  SubtleCrypto(nsPIDOMWindow* aWindow);

  nsPIDOMWindow* GetParentObject() const
  {
    return mWindow;
  }

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  already_AddRefed<Promise> Encrypt(JSContext* cx,
                                    const ObjectOrString& algorithm,
                                    Key& key,
                                    const CryptoOperationData& data);

  already_AddRefed<Promise> Decrypt(JSContext* cx,
                                    const ObjectOrString& algorithm,
                                    Key& key,
                                    const CryptoOperationData& data);

  already_AddRefed<Promise> Sign(JSContext* cx,
                                 const ObjectOrString& algorithm,
                                 Key& key,
                                 const CryptoOperationData& data);

  already_AddRefed<Promise> Verify(JSContext* cx,
                                   const ObjectOrString& algorithm,
                                   Key& key,
                                   const CryptoOperationData& signature,
                                   const CryptoOperationData& data);

  already_AddRefed<Promise> Digest(JSContext* cx,
                                   const ObjectOrString& aAlgorithm,
                                   const CryptoOperationData& aData);

  already_AddRefed<Promise> ImportKey(JSContext* cx,
                                      const nsAString& format,
                                      const KeyData& keyData,
                                      const ObjectOrString& algorithm,
                                      bool extractable,
                                      const Sequence<nsString>& keyUsages);

  already_AddRefed<Promise> ExportKey(const nsAString& format, Key& key);

  already_AddRefed<Promise> GenerateKey(JSContext* cx,
                                        const ObjectOrString& algorithm,
                                        bool extractable,
                                        const Sequence<nsString>& keyUsages);

  already_AddRefed<Promise> DeriveKey(JSContext* cx,
                                      const ObjectOrString& algorithm,
                                      Key& baseKey,
                                      const ObjectOrString& derivedKeyType,
                                      bool extractable,
                                      const Sequence<nsString>& keyUsages);

  already_AddRefed<Promise> DeriveBits(JSContext* cx,
                                       const ObjectOrString& algorithm,
                                       Key& baseKey,
                                       uint32_t length);

private:
  nsCOMPtr<nsPIDOMWindow> mWindow;
};

} 
} 

#endif 
