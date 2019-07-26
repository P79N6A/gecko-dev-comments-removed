




#ifndef mozilla_dom_SVGAnimatedAngle_h
#define mozilla_dom_SVGAnimatedAngle_h

#include "nsWrapperCache.h"
#include "nsSVGElement.h"
#include "mozilla/Attributes.h"

class nsSVGAngle;

namespace mozilla {
namespace dom {

class SVGAngle;

class SVGAnimatedAngle MOZ_FINAL : public nsWrapperCache
{
public:
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(SVGAnimatedAngle)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(SVGAnimatedAngle)

  SVGAnimatedAngle(nsSVGAngle* aVal, nsSVGElement *aSVGElement)
    : mVal(aVal), mSVGElement(aSVGElement)
  {
    SetIsDOMBinding();
  }

  
  nsSVGElement* GetParentObject() { return mSVGElement; }
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;
  already_AddRefed<SVGAngle> BaseVal();
  already_AddRefed<SVGAngle> AnimVal();

protected:
  ~SVGAnimatedAngle();

  nsSVGAngle* mVal; 
  nsRefPtr<nsSVGElement> mSVGElement;
};

} 
} 

#endif 
