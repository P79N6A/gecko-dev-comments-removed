




#ifndef NS_ISMILATTR_H_
#define NS_ISMILATTR_H_

#include "nscore.h"

class nsSMILValue;
class nsISMILType;
class nsISMILAnimationElement;
class nsIContent;
class nsAString;










class nsISMILAttr
{
public:
  


















  virtual nsresult ValueFromString(const nsAString& aStr,
                                   const nsISMILAnimationElement* aSrcElement,
                                   nsSMILValue& aValue,
                                   bool& aPreventCachingOfSandwich) const = 0;

  





  virtual nsSMILValue GetBaseValue() const = 0;

  





  virtual void ClearAnimValue() = 0;

  





  virtual nsresult SetAnimValue(const nsSMILValue& aValue) = 0;

  







  virtual const nsIContent* GetTargetNode() const { return nullptr; }

  


  virtual ~nsISMILAttr() {}
};

#endif 
