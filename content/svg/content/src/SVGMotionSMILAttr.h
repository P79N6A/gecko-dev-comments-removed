






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
                                   bool& aPreventCachingOfSandwich) const MOZ_OVERRIDE;
  virtual nsSMILValue GetBaseValue() const MOZ_OVERRIDE;
  virtual nsresult    SetAnimValue(const nsSMILValue& aValue) MOZ_OVERRIDE;
  virtual void        ClearAnimValue() MOZ_OVERRIDE;
  virtual const nsIContent* GetTargetNode() const MOZ_OVERRIDE;

protected:
  
  
  
  nsSVGElement* mSVGElement;
};

} 

#endif 
