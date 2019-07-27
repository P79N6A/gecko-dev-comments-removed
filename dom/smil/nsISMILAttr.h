




#ifndef NS_ISMILATTR_H_
#define NS_ISMILATTR_H_

#include "nscore.h"

class nsSMILValue;
class nsIContent;
class nsAString;

namespace mozilla {
namespace dom {
class SVGAnimationElement;
}
}










class nsISMILAttr
{
public:
  


















  virtual nsresult ValueFromString(const nsAString& aStr,
                                   const mozilla::dom::SVGAnimationElement* aSrcElement,
                                   nsSMILValue& aValue,
                                   bool& aPreventCachingOfSandwich) const = 0;

  





  virtual nsSMILValue GetBaseValue() const = 0;

  





  virtual void ClearAnimValue() = 0;

  





  virtual nsresult SetAnimValue(const nsSMILValue& aValue) = 0;

  







  virtual const nsIContent* GetTargetNode() const { return nullptr; }

  


  virtual ~nsISMILAttr() {}
};

#endif 
