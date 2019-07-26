






#ifndef NS_SMILMAPPEDATTRIBUTE_H_
#define NS_SMILMAPPEDATTRIBUTE_H_

#include "mozilla/Attributes.h"
#include "nsSMILCSSProperty.h"








#define SMIL_MAPPED_ATTR_STYLERULE_ATOM nsGkAtoms::_empty







class nsSMILMappedAttribute : public nsSMILCSSProperty {
public:
  






  nsSMILMappedAttribute(nsCSSProperty aPropID, mozilla::dom::Element* aElement) :
    nsSMILCSSProperty(aPropID, aElement) {}

  
  virtual nsresult ValueFromString(const nsAString& aStr,
                                   const mozilla::dom::SVGAnimationElement* aSrcElement,
                                   nsSMILValue& aValue,
                                   bool& aPreventCachingOfSandwich) const MOZ_OVERRIDE;
  virtual nsSMILValue GetBaseValue() const MOZ_OVERRIDE;
  virtual nsresult    SetAnimValue(const nsSMILValue& aValue) MOZ_OVERRIDE;
  virtual void        ClearAnimValue() MOZ_OVERRIDE;

protected:
  
  void FlushChangesToTargetAttr() const;
  already_AddRefed<nsIAtom> GetAttrNameAtom() const;
};
#endif 
