





#ifndef mozilla_dom_SVGAnimatedInteger_h
#define mozilla_dom_SVGAnimatedInteger_h

#include "nsWrapperCache.h"

#include "nsSVGElement.h"

namespace mozilla {
namespace dom {

class SVGAnimatedInteger : public nsISupports
                         , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(SVGAnimatedInteger)

  nsSVGElement* GetParentObject() const
  {
    return mSVGElement;
  }

  virtual JSObject* WrapObject(JSContext* aCx)
    MOZ_OVERRIDE MOZ_FINAL;

  virtual int32_t BaseVal() = 0;
  virtual void SetBaseVal(int32_t aBaseVal) = 0;
  virtual int32_t AnimVal() = 0;

protected:
  explicit SVGAnimatedInteger(nsSVGElement* aSVGElement)
    : mSVGElement(aSVGElement)
  {
    SetIsDOMBinding();
  }
  virtual ~SVGAnimatedInteger() {};

  nsRefPtr<nsSVGElement> mSVGElement;
};

} 
} 

#endif 
