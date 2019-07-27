




#ifndef mozilla_dom_SVGAnimatedLength_h
#define mozilla_dom_SVGAnimatedLength_h

#include "mozilla/Attributes.h"
#include "nsSVGElement.h"

class nsSVGLength2;

namespace mozilla {

class DOMSVGLength;

namespace dom {

class SVGAnimatedLength MOZ_FINAL : public nsWrapperCache
{
public:
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(SVGAnimatedLength)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(SVGAnimatedLength)

  SVGAnimatedLength(nsSVGLength2* aVal, nsSVGElement *aSVGElement)
    : mVal(aVal), mSVGElement(aSVGElement)
  { SetIsDOMBinding(); }

  
  nsSVGElement* GetParentObject() { return mSVGElement; }
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;
  already_AddRefed<DOMSVGLength> BaseVal();
  already_AddRefed<DOMSVGLength> AnimVal();

protected:
  ~SVGAnimatedLength();

  nsSVGLength2* mVal; 
  nsRefPtr<nsSVGElement> mSVGElement;
};

} 
} 

#endif 
