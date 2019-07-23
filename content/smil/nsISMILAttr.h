




































#ifndef NS_ISMILATTR_H_
#define NS_ISMILATTR_H_

#include "nsStringFwd.h"

class nsSMILValue;
class nsISMILType;
class nsISMILAnimationElement;










class nsISMILAttr
{
public:
  













  virtual nsresult ValueFromString(const nsAString& aStr,
                                   const nsISMILAnimationElement* aSrcElement,
                                   nsSMILValue& aValue) const = 0;

  





  virtual nsSMILValue GetBaseValue() const = 0;

  





  virtual void ClearAnimValue() = 0;

  





  virtual nsresult SetAnimValue(const nsSMILValue& aValue) = 0;

  


  virtual ~nsISMILAttr() {};
};

#endif 
