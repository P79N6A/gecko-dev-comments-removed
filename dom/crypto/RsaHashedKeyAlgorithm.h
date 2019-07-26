





#ifndef mozilla_dom_RsaHashedKeyAlgorithm_h
#define mozilla_dom_RsaHashedKeyAlgorithm_h

#include "nsAutoPtr.h"
#include "mozilla/dom/RsaKeyAlgorithm.h"

namespace mozilla {
namespace dom {

class RsaHashedKeyAlgorithm MOZ_FINAL : public RsaKeyAlgorithm
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(RsaHashedKeyAlgorithm, RsaKeyAlgorithm)

  RsaHashedKeyAlgorithm(nsIGlobalObject* aGlobal,
                        const nsString& aName,
                        uint32_t aModulusLength,
                        const CryptoBuffer& aPublicExponent,
                        const nsString& aHashName)
    : RsaKeyAlgorithm(aGlobal, aName, aModulusLength, aPublicExponent)
    , mHash(new KeyAlgorithm(aGlobal, aHashName))
  {}

  ~RsaHashedKeyAlgorithm() {}

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  KeyAlgorithm* Hash() const
  {
    return mHash;
  }

  virtual bool WriteStructuredClone(JSStructuredCloneWriter* aWriter) const MOZ_OVERRIDE;
  static KeyAlgorithm* Create(nsIGlobalObject* aGlobal,
                              JSStructuredCloneReader* aReader);

private:
  nsRefPtr<KeyAlgorithm> mHash;
};

} 
} 

#endif 
