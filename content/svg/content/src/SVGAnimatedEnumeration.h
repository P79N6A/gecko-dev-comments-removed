





#ifndef mozilla_dom_SVGAnimatedEnumeration_h
#define mozilla_dom_SVGAnimatedEnumeration_h

#include "nsWrapperCache.h"

#include "nsSVGElement.h"

namespace mozilla {
namespace dom {

class SVGAnimatedEnumeration : public nsISupports
                             , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(SVGAnimatedEnumeration)

  nsSVGElement* GetParentObject() const
  {
    return mSVGElement;
  }

  virtual JSObject* WrapObject(JSContext* aCx)
    MOZ_OVERRIDE MOZ_FINAL;

  virtual uint16_t BaseVal() = 0;
  virtual void SetBaseVal(uint16_t aBaseVal, ErrorResult& aRv) = 0;
  virtual uint16_t AnimVal() = 0;

protected:
  explicit SVGAnimatedEnumeration(nsSVGElement* aSVGElement)
    : mSVGElement(aSVGElement)
  {
    SetIsDOMBinding();
  }
  virtual ~SVGAnimatedEnumeration() {};

  nsRefPtr<nsSVGElement> mSVGElement;
};

} 
} 

#endif 
