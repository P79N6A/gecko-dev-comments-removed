




#ifndef mozilla_dom_SVGAnimatedLength_h
#define mozilla_dom_SVGAnimatedLength_h

#include "mozilla/Attributes.h"
#include "nsSVGElement.h"

class nsSVGLength2;
class nsIDOMSVGLength;

namespace mozilla {
namespace dom {

class SVGAnimatedLength MOZ_FINAL : public nsWrapperCache
{
public:
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(SVGAnimatedLength)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(SVGAnimatedLength)

  SVGAnimatedLength(nsSVGLength2* aVal, nsSVGElement *aSVGElement)
    : mVal(aVal), mSVGElement(aSVGElement)
  { SetIsDOMBinding(); }

  ~SVGAnimatedLength();

  
  nsSVGElement* GetParentObject() { return mSVGElement; }
  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;
  already_AddRefed<nsIDOMSVGLength> BaseVal();
  already_AddRefed<nsIDOMSVGLength> AnimVal();

protected:
  nsSVGLength2* mVal; 
  nsRefPtr<nsSVGElement> mSVGElement;
};

} 
} 

#endif 
