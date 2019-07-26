




#ifndef NS_SMILSETANIMATIONFUNCTION_H_
#define NS_SMILSETANIMATIONFUNCTION_H_

#include "nsSMILAnimationFunction.h"







class nsSMILSetAnimationFunction : public nsSMILAnimationFunction
{
public:
  












  virtual bool SetAttr(nsIAtom* aAttribute, const nsAString& aValue,
                         nsAttrValue& aResult, nsresult* aParseResult = nullptr);

  





  virtual bool UnsetAttr(nsIAtom* aAttribute) MOZ_OVERRIDE;

protected:
  
  
  
  
  virtual bool IsToAnimation() const MOZ_OVERRIDE {
    return false;
  }

  
  virtual bool IsValueFixedForSimpleDuration() const MOZ_OVERRIDE {
    return true;
  }
  virtual bool               HasAttr(nsIAtom* aAttName) const MOZ_OVERRIDE;
  virtual const nsAttrValue* GetAttr(nsIAtom* aAttName) const MOZ_OVERRIDE;
  virtual bool               GetAttr(nsIAtom* aAttName,
                                     nsAString& aResult) const MOZ_OVERRIDE;
  virtual bool WillReplace() const MOZ_OVERRIDE;

  bool IsDisallowedAttribute(const nsIAtom* aAttribute) const;
};

#endif 
