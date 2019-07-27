







#ifndef MOZILLA_SVGMOTIONSMILATTR_H_
#define MOZILLA_SVGMOTIONSMILATTR_H_

#include "mozilla/Attributes.h"
#include "nsISMILAttr.h"

class nsIContent;
class nsSMILValue;
class nsSVGElement;

namespace mozilla {

namespace dom {
class SVGAnimationElement;
} 








class SVGMotionSMILAttr : public nsISMILAttr
{
public:
  explicit SVGMotionSMILAttr(nsSVGElement* aSVGElement)
    : mSVGElement(aSVGElement) {}

  
  virtual nsresult ValueFromString(const nsAString& aStr,
                                   const dom::SVGAnimationElement* aSrcElement,
                                   nsSMILValue& aValue,
                                   bool& aPreventCachingOfSandwich) const override;
  virtual nsSMILValue GetBaseValue() const override;
  virtual nsresult    SetAnimValue(const nsSMILValue& aValue) override;
  virtual void        ClearAnimValue() override;
  virtual const nsIContent* GetTargetNode() const override;

protected:
  
  
  
  nsSVGElement* mSVGElement;
};

} 

#endif 
