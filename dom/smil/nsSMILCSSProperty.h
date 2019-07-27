






#ifndef NS_SMILCSSPROPERTY_H_
#define NS_SMILCSSPROPERTY_H_

#include "mozilla/Attributes.h"
#include "nsISMILAttr.h"
#include "nsIAtom.h"
#include "nsCSSProperty.h"
#include "nsCSSValue.h"

namespace mozilla {
namespace dom {
class Element;
} 
} 






class nsSMILCSSProperty : public nsISMILAttr
{
public:
  




  nsSMILCSSProperty(nsCSSProperty aPropID, mozilla::dom::Element* aElement);

  
  virtual nsresult ValueFromString(const nsAString& aStr,
                                   const mozilla::dom::SVGAnimationElement* aSrcElement,
                                   nsSMILValue& aValue,
                                   bool& aPreventCachingOfSandwich) const override;
  virtual nsSMILValue GetBaseValue() const override;
  virtual nsresult    SetAnimValue(const nsSMILValue& aValue) override;
  virtual void        ClearAnimValue() override;

  







  static bool IsPropertyAnimatable(nsCSSProperty aPropID);

protected:
  nsCSSProperty mPropID;
  
  
  
  
  mozilla::dom::Element*   mElement;
};

#endif 
