




#pragma once

#include "nsWrapperCache.h"
#include "nsSVGElement.h"
#include "mozilla/Attributes.h"
#include "nsSVGBoolean.h"

namespace mozilla {
namespace dom {

class SVGAnimatedBoolean MOZ_FINAL : public nsISupports,
                                     public nsWrapperCache
{
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(SVGAnimatedBoolean)

  SVGAnimatedBoolean(nsSVGBoolean* aVal, nsSVGElement *aSVGElement)
    : mVal(aVal), mSVGElement(aSVGElement)
  {
    SetIsDOMBinding();
  }
  ~SVGAnimatedBoolean();

 
  nsSVGElement* GetParentObject() const { return mSVGElement; }
  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;
  bool BaseVal() const { return mVal->GetBaseValue(); }
  void SetBaseVal(bool aValue) { mVal->SetBaseValue(aValue, mSVGElement); }
  bool AnimVal() const { mSVGElement->FlushAnimations(); return mVal->GetAnimValue(); }

protected:
  nsSVGBoolean* mVal; 
  nsRefPtr<nsSVGElement> mSVGElement;
};

} 
} 
