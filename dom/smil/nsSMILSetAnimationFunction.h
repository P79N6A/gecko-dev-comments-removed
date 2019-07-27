




#ifndef NS_SMILSETANIMATIONFUNCTION_H_
#define NS_SMILSETANIMATIONFUNCTION_H_

#include "mozilla/Attributes.h"
#include "nsSMILAnimationFunction.h"







class nsSMILSetAnimationFunction : public nsSMILAnimationFunction
{
public:
  












  virtual bool SetAttr(nsIAtom* aAttribute, const nsAString& aValue,
                         nsAttrValue& aResult, nsresult* aParseResult = nullptr) override;

  





  virtual bool UnsetAttr(nsIAtom* aAttribute) override;

protected:
  
  
  
  
  virtual bool IsToAnimation() const override {
    return false;
  }

  
  virtual bool IsValueFixedForSimpleDuration() const override {
    return true;
  }
  virtual bool               HasAttr(nsIAtom* aAttName) const override;
  virtual const nsAttrValue* GetAttr(nsIAtom* aAttName) const override;
  virtual bool               GetAttr(nsIAtom* aAttName,
                                     nsAString& aResult) const override;
  virtual bool WillReplace() const override;

  bool IsDisallowedAttribute(const nsIAtom* aAttribute) const;
};

#endif 
