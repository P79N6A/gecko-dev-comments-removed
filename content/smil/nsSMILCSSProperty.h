






































#ifndef NS_SMILCSSPROPERTY_H_
#define NS_SMILCSSPROPERTY_H_

#include "nsISMILAttr.h"
#include "nsIAtom.h"
#include "nsCSSProperty.h"
#include "nsCSSValue.h"

class nsIContent;
class nsCSSDeclaration;






class nsSMILCSSProperty : public nsISMILAttr
{
public:
  




  nsSMILCSSProperty(nsCSSProperty aPropID, nsIContent* aElement);

  
  virtual nsresult ValueFromString(const nsAString& aStr,
                                   const nsISMILAnimationElement* aSrcElement,
                                   nsSMILValue& aValue) const;
  virtual nsSMILValue GetBaseValue() const;
  virtual nsresult    SetAnimValue(const nsSMILValue& aValue);
  virtual void        ClearAnimValue();

  







  static PRBool IsPropertyAnimatable(nsCSSProperty aPropID);

protected:
  nsCSSProperty mPropID;
  
  
  
  
  nsIContent*   mElement;
};

#endif 
