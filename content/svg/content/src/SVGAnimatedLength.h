




#ifndef mozilla_dom_SVGAnimatedLength_h
#define mozilla_dom_SVGAnimatedLength_h

#include "nsSVGElement.h"
#include "nsIDOMSVGAnimatedLength.h"

class nsSVGLength2;
class nsIDOMSVGLength;

namespace mozilla {
namespace dom {

class SVGAnimatedLength MOZ_FINAL : public nsIDOMSVGAnimatedLength,
                                    public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(SVGAnimatedLength)

  SVGAnimatedLength(nsSVGLength2* aVal, nsSVGElement *aSVGElement)
    : mVal(aVal), mSVGElement(aSVGElement)
  { SetIsDOMBinding(); }

  ~SVGAnimatedLength();

  NS_IMETHOD GetBaseVal(nsIDOMSVGLength **aBaseVal)
    { *aBaseVal = BaseVal().get(); return NS_OK; }

  NS_IMETHOD GetAnimVal(nsIDOMSVGLength **aAnimVal)
    { *aAnimVal = AnimVal().get(); return NS_OK; }

  
  nsSVGElement* GetParentObject() { return mSVGElement; }
  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap);
  already_AddRefed<nsIDOMSVGLength> BaseVal();
  already_AddRefed<nsIDOMSVGLength> AnimVal();

protected:
  nsSVGLength2* mVal; 
  nsRefPtr<nsSVGElement> mSVGElement;
};

} 
} 

#endif 
