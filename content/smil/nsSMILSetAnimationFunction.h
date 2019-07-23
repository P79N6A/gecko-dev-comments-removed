




































#ifndef NS_SMILSETANIMATIONFUNCTION_H_
#define NS_SMILSETANIMATIONFUNCTION_H_

#include "nsSMILAnimationFunction.h"







class nsSMILSetAnimationFunction : public nsSMILAnimationFunction
{
public:
  












  virtual PRBool SetAttr(nsIAtom* aAttribute, const nsAString& aValue,
                         nsAttrValue& aResult, nsresult* aParseResult = nsnull);

  





  virtual PRBool UnsetAttr(nsIAtom* aAttribute);

protected:
  NS_OVERRIDE virtual nsresult
    InterpolateResult(const nsSMILValueArray& aValues,
                      nsSMILValue& aResult, nsSMILValue& aBaseValue);

  virtual PRBool             HasAttr(nsIAtom* aAttName) const;
  virtual const nsAttrValue* GetAttr(nsIAtom* aAttName) const;
  virtual PRBool             GetAttr(nsIAtom* aAttName,
                                     nsAString& aResult) const;

  PRBool IsDisallowedAttribute(const nsIAtom* aAttribute) const;
};

#endif 
