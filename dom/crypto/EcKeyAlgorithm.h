





#ifndef mozilla_dom_EcKeyAlgorithm_h
#define mozilla_dom_EcKeyAlgorithm_h

#include "mozilla/ErrorResult.h"
#include "mozilla/dom/KeyAlgorithm.h"
#include "js/TypeDecls.h"

namespace mozilla {
namespace dom {

class EcKeyAlgorithm : public KeyAlgorithm
{
public:
  EcKeyAlgorithm(nsIGlobalObject* aGlobal,
                 const nsString& aName,
                 const nsString& aNamedCurve)
    : KeyAlgorithm(aGlobal, aName)
    , mNamedCurve(aNamedCurve)
  {}

  ~EcKeyAlgorithm()
  {}

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  void GetNamedCurve(nsString& aRetVal) const
  {
    aRetVal.Assign(mNamedCurve);
  }

  virtual bool WriteStructuredClone(JSStructuredCloneWriter* aWriter) const MOZ_OVERRIDE;
  static KeyAlgorithm* Create(nsIGlobalObject* aGlobal,
                              JSStructuredCloneReader* aReader);

protected:
  nsString mNamedCurve;
};

} 
} 

#endif 
