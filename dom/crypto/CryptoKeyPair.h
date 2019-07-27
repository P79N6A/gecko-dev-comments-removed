





#ifndef mozilla_dom_CryptoKeyPair_h
#define mozilla_dom_CryptoKeyPair_h

#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "nsIGlobalObject.h"
#include "mozilla/dom/CryptoKey.h"
#include "js/TypeDecls.h"

namespace mozilla {
namespace dom {

class CryptoKeyPair MOZ_FINAL : public nsISupports,
                                public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(CryptoKeyPair)

public:
  explicit CryptoKeyPair(nsIGlobalObject* aGlobal)
    : mGlobal(aGlobal)
    , mPublicKey(new CryptoKey(aGlobal))
    , mPrivateKey(new CryptoKey(aGlobal))
  {
    SetIsDOMBinding();
  }

  nsIGlobalObject* GetParentObject() const
  {
    return mGlobal;
  }

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  CryptoKey* PublicKey() const
  {
    return mPublicKey;
  }

  CryptoKey* PrivateKey() const
  {
    return mPrivateKey;
  }

private:
  ~CryptoKeyPair() {}

  nsRefPtr<nsIGlobalObject> mGlobal;
  nsRefPtr<CryptoKey> mPublicKey;
  nsRefPtr<CryptoKey> mPrivateKey;
};

} 
} 

#endif 
