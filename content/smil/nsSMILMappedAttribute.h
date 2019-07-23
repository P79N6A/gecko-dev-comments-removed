






































#ifndef NS_SMILMAPPEDATTRIBUTE_H_
#define NS_SMILMAPPEDATTRIBUTE_H_

#include "nsSMILCSSProperty.h"








#define SMIL_MAPPED_ATTR_STYLERULE_ATOM nsGkAtoms::_empty







class nsSMILMappedAttribute : public nsSMILCSSProperty {
public:
  






  nsSMILMappedAttribute(nsCSSProperty aPropID, nsIContent* aElement) :
    nsSMILCSSProperty(aPropID, aElement) {}

  
  virtual nsSMILValue GetBaseValue() const;
  virtual nsresult    SetAnimValue(const nsSMILValue& aValue);
  virtual void        ClearAnimValue();

protected:
  
  void FlushChangesToTargetAttr() const;
  already_AddRefed<nsIAtom> GetAttrNameAtom() const;
};
#endif 
