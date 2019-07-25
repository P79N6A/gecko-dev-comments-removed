




































#ifndef NS_SMILSETANIMATIONFUNCTION_H_
#define NS_SMILSETANIMATIONFUNCTION_H_

#include "nsSMILAnimationFunction.h"







class nsSMILSetAnimationFunction : public nsSMILAnimationFunction
{
public:
  












  virtual bool SetAttr(nsIAtom* aAttribute, const nsAString& aValue,
                         nsAttrValue& aResult, nsresult* aParseResult = nsnull);

  





  NS_OVERRIDE virtual bool UnsetAttr(nsIAtom* aAttribute);

protected:
  
  
  
  
  NS_OVERRIDE virtual bool IsToAnimation() const {
    return PR_FALSE;
  }

  
  NS_OVERRIDE virtual bool IsValueFixedForSimpleDuration() const {
    return PR_TRUE;
  }
  NS_OVERRIDE virtual bool               HasAttr(nsIAtom* aAttName) const;
  NS_OVERRIDE virtual const nsAttrValue* GetAttr(nsIAtom* aAttName) const;
  NS_OVERRIDE virtual bool               GetAttr(nsIAtom* aAttName,
                                                 nsAString& aResult) const;
  NS_OVERRIDE virtual bool WillReplace() const;

  bool IsDisallowedAttribute(const nsIAtom* aAttribute) const;
};

#endif 
