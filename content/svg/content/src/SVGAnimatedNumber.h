





#ifndef mozilla_dom_SVGAnimatedNumber_h
#define mozilla_dom_SVGAnimatedNumber_h

#include "nsISupports.h"
#include "nsWrapperCache.h"

#include "nsSVGElement.h"

namespace mozilla {
namespace dom {

class SVGAnimatedNumber : public nsISupports
                        , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(SVGAnimatedNumber)

  nsSVGElement* GetParentObject() const
  {
    return mSVGElement;
  }

  virtual JSObject* WrapObject(JSContext* aCx)
    MOZ_OVERRIDE MOZ_FINAL;

  virtual float BaseVal() = 0;
  virtual void SetBaseVal(float aBaseVal) = 0;
  virtual float AnimVal() = 0;

protected:
  explicit SVGAnimatedNumber(nsSVGElement* aSVGElement)
    : mSVGElement(aSVGElement)
  {
    SetIsDOMBinding();
  }
  virtual ~SVGAnimatedNumber() {};

  nsRefPtr<nsSVGElement> mSVGElement;
};

} 
} 

#endif 
