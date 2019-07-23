




































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
  virtual nsSMILAnimationFunction::nsSMILCalcMode GetCalcMode() const;

  virtual PRBool             HasAttr(nsIAtom* aAttName) const;
  virtual const nsAttrValue* GetAttr(nsIAtom* aAttName) const;
  virtual PRBool             GetAttr(nsIAtom* aAttName,
                                     nsAString& aResult) const;

  PRBool IsDisallowedAttribute(const nsIAtom* aAttribute) const;
};

#endif 
