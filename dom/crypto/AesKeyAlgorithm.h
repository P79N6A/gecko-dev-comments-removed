





#ifndef mozilla_dom_AesKeyAlgorithm_h
#define mozilla_dom_AesKeyAlgorithm_h

#include "mozilla/dom/BasicSymmetricKeyAlgorithm.h"
#include "js/TypeDecls.h"

namespace mozilla {
namespace dom {

class AesKeyAlgorithm MOZ_FINAL : public BasicSymmetricKeyAlgorithm
{
public:
  AesKeyAlgorithm(nsIGlobalObject* aGlobal, const nsString& aName, uint16_t aLength)
    : BasicSymmetricKeyAlgorithm(aGlobal, aName, aLength)
  {}

  ~AesKeyAlgorithm()
  {}

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  virtual nsString ToJwkAlg() const MOZ_OVERRIDE;

  virtual bool WriteStructuredClone(JSStructuredCloneWriter* aWriter) const MOZ_OVERRIDE;
  static KeyAlgorithm* Create(nsIGlobalObject* aGlobal,
                              JSStructuredCloneReader* aReader);
};

} 
} 

#endif 
