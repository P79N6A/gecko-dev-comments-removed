




#pragma once

#include "nsIDOMSVGAnimatedBoolean.h"
#include "nsWrapperCache.h"
#include "nsSVGElement.h"
#include "mozilla/Attributes.h"
#include "nsSVGBoolean.h"

namespace mozilla {
namespace dom {

class SVGAnimatedBoolean MOZ_FINAL : public nsIDOMSVGAnimatedBoolean,
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

  NS_IMETHOD GetBaseVal(bool* aResult)
    { *aResult = BaseVal(); return NS_OK; }
  NS_IMETHOD SetBaseVal(bool aValue)
    { mVal->SetBaseValue(aValue, mSVGElement); return NS_OK; }

  
  
  NS_IMETHOD GetAnimVal(bool* aResult)
  {
    *aResult = AnimVal();
    return NS_OK;
  }

  
  nsSVGElement* GetParentObject() const { return mSVGElement; }
  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap);
  bool BaseVal() const { return mVal->GetBaseValue(); }
  bool AnimVal() const { mSVGElement->FlushAnimations(); return mVal->GetAnimValue(); }

protected:
  nsSVGBoolean* mVal; 
  nsRefPtr<nsSVGElement> mSVGElement;
};

} 
} 
