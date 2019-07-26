




#ifndef mozilla_dom_SVGAnimatedAngle_h
#define mozilla_dom_SVGAnimatedAngle_h

#include "nsWrapperCache.h"
#include "nsSVGElement.h"
#include "mozilla/Attributes.h"

class nsSVGAngle;

namespace mozilla {
namespace dom {

class SVGAngle;

class SVGAnimatedAngle MOZ_FINAL : public nsISupports,
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

#endif 
