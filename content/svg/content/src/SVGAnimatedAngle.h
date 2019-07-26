




#pragma once

#include "nsIDOMSVGAnimatedAngle.h"
#include "nsWrapperCache.h"
#include "nsSVGElement.h"
#include "SVGAngle.h"
#include "mozilla/Attributes.h"

class nsSVGAngle;

namespace mozilla {
namespace dom {

class SVGAnimatedAngle MOZ_FINAL : public nsIDOMSVGAnimatedAngle,
                                   public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(SVGAnimatedAngle)

  SVGAnimatedAngle(nsSVGAngle* aVal, nsSVGElement *aSVGElement)
    : mVal(aVal), mSVGElement(aSVGElement)
  {
    SetIsDOMBinding();
  }
  ~SVGAnimatedAngle();

  NS_IMETHOD GetBaseVal(nsIDOMSVGAngle **aBaseVal)
    { *aBaseVal = BaseVal().get(); return NS_OK; }

  NS_IMETHOD GetAnimVal(nsIDOMSVGAngle **aAnimVal)
    { *aAnimVal = AnimVal().get(); return NS_OK; }

  
  nsSVGElement* GetParentObject() { return mSVGElement; }
  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap);
  already_AddRefed<SVGAngle> BaseVal();
  already_AddRefed<SVGAngle> AnimVal();

protected:
  nsSVGAngle* mVal; 
  nsRefPtr<nsSVGElement> mSVGElement;
};

} 
} 

