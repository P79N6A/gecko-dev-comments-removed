




#ifndef mozilla_dom_SVGAnimatedString_h
#define mozilla_dom_SVGAnimatedString_h

#include "nsSVGElement.h"

namespace mozilla {
namespace dom {

class SVGAnimatedString : public nsISupports,
                          public nsWrapperCache
{
public:
  explicit SVGAnimatedString(nsSVGElement* aSVGElement)
    : mSVGElement(aSVGElement)
  {
    SetIsDOMBinding();
  }

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  
  nsSVGElement* GetParentObject() const
  {
    return mSVGElement;
  }

  virtual void GetBaseVal(nsAString& aResult) = 0;
  virtual void SetBaseVal(const nsAString& aValue) = 0;
  virtual void GetAnimVal(nsAString& aResult) = 0;

  nsRefPtr<nsSVGElement> mSVGElement;
};

} 
} 

#endif 
