






































#ifndef MOZILLA_SVGMOTIONSMILATTR_H_
#define MOZILLA_SVGMOTIONSMILATTR_H_

#include "nsISMILAttr.h"
class nsSVGElement;

namespace mozilla {








class SVGMotionSMILAttr : public nsISMILAttr
{
public:
  SVGMotionSMILAttr(nsSVGElement* aSVGElement)
    : mSVGElement(aSVGElement) {}

  
  virtual nsresult ValueFromString(const nsAString& aStr,
                                   const nsISMILAnimationElement* aSrcElement,
                                   nsSMILValue& aValue,
                                   bool& aPreventCachingOfSandwich) const;
  virtual nsSMILValue GetBaseValue() const;
  virtual nsresult    SetAnimValue(const nsSMILValue& aValue);
  virtual void        ClearAnimValue();
  virtual const nsIContent* GetTargetNode() const;

protected:
  
  
  
  nsSVGElement* mSVGElement;
};

} 

#endif 
