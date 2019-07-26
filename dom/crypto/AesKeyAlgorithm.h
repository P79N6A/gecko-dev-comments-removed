





#ifndef mozilla_dom_AesKeyAlgorithm_h
#define mozilla_dom_AesKeyAlgorithm_h

#include "mozilla/dom/KeyAlgorithm.h"
#include "js/TypeDecls.h"

namespace mozilla {
namespace dom {

class AesKeyAlgorithm MOZ_FINAL : public KeyAlgorithm
{
public:
  AesKeyAlgorithm(nsIGlobalObject* aGlobal, const nsString& aName, uint16_t aLength)
    : KeyAlgorithm(aGlobal, aName)
    , mLength(aLength)
  {}

  ~AesKeyAlgorithm()
  {}

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  uint16_t Length() const
  {
    return mLength;
  }

  virtual bool WriteStructuredClone(JSStructuredCloneWriter* aWriter) const MOZ_OVERRIDE;
  static KeyAlgorithm* Create(nsIGlobalObject* aGlobal,
                              JSStructuredCloneReader* aReader);

protected:
  uint16_t mLength;
};

} 
} 

#endif 
