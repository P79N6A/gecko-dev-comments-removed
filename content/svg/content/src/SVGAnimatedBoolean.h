




#ifndef mozilla_dom_SVGAnimatedBoolean_h
#define mozilla_dom_SVGAnimatedBoolean_h

#include "nsWrapperCache.h"
#include "nsSVGElement.h"
#include "mozilla/Attributes.h"
#include "nsSVGBoolean.h"

namespace mozilla {
namespace dom {

class SVGAnimatedBoolean MOZ_FINAL : public nsWrapperCache
{
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(SVGAnimatedBoolean)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(SVGAnimatedBoolean)

  SVGAnimatedBoolean(nsSVGBoolean* aVal, nsSVGElement *aSVGElement)
    : mVal(aVal), mSVGElement(aSVGElement)
  {
    SetIsDOMBinding();
  }

  
  nsSVGElement* GetParentObject() const { return mSVGElement; }
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;
  bool BaseVal() const { return mVal->GetBaseValue(); }
  void SetBaseVal(bool aValue) { mVal->SetBaseValue(aValue, mSVGElement); }
  bool AnimVal() const { mSVGElement->FlushAnimations(); return mVal->GetAnimValue(); }

protected:
  ~SVGAnimatedBoolean();

  nsSVGBoolean* mVal; 
  nsRefPtr<nsSVGElement> mSVGElement;
};

} 
} 

#endif 
