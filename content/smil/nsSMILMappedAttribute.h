






































#ifndef NS_SMILMAPPEDATTRIBUTE_H_
#define NS_SMILMAPPEDATTRIBUTE_H_

#include "nsSMILCSSProperty.h"








#define SMIL_MAPPED_ATTR_STYLERULE_ATOM nsGkAtoms::_empty







class nsSMILMappedAttribute : public nsSMILCSSProperty {
public:
  






  nsSMILMappedAttribute(nsCSSProperty aPropID, mozilla::dom::Element* aElement) :
    nsSMILCSSProperty(aPropID, aElement) {}

  
  virtual nsresult ValueFromString(const nsAString& aStr,
                                   const nsISMILAnimationElement* aSrcElement,
                                   nsSMILValue& aValue,
                                   bool& aPreventCachingOfSandwich) const;
  virtual nsSMILValue GetBaseValue() const;
  virtual nsresult    SetAnimValue(const nsSMILValue& aValue);
  virtual void        ClearAnimValue();

protected:
  
  void FlushChangesToTargetAttr() const;
  already_AddRefed<nsIAtom> GetAttrNameAtom() const;
};
#endif 
