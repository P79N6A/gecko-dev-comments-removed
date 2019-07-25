




































#ifndef NS_SMILSETANIMATIONFUNCTION_H_
#define NS_SMILSETANIMATIONFUNCTION_H_

#include "nsSMILAnimationFunction.h"







class nsSMILSetAnimationFunction : public nsSMILAnimationFunction
{
public:
  












  virtual PRBool SetAttr(nsIAtom* aAttribute, const nsAString& aValue,
                         nsAttrValue& aResult, nsresult* aParseResult = nsnull);

  





  NS_OVERRIDE virtual PRBool UnsetAttr(nsIAtom* aAttribute);

protected:
  
  
  
  
  NS_OVERRIDE virtual PRBool IsToAnimation() const {
    return PR_FALSE;
  }
  NS_OVERRIDE virtual PRBool             HasAttr(nsIAtom* aAttName) const;
  NS_OVERRIDE virtual const nsAttrValue* GetAttr(nsIAtom* aAttName) const;
  NS_OVERRIDE virtual PRBool             GetAttr(nsIAtom* aAttName,
                                                 nsAString& aResult) const;
  NS_OVERRIDE virtual PRBool WillReplace() const;

  PRBool IsDisallowedAttribute(const nsIAtom* aAttribute) const;
};

#endif 
