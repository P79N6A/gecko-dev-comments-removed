






































#ifndef NS_SMILCSSVALUETYPE_H_
#define NS_SMILCSSVALUETYPE_H_

#include "nsISMILCSSValueType.h"
#include "nscore.h" 

class nsIContent;





class nsSMILCSSValueType : public nsISMILCSSValueType
{
public:
  
  
  NS_OVERRIDE virtual nsresult Init(nsSMILValue& aValue) const;
  NS_OVERRIDE virtual void     Destroy(nsSMILValue&) const;
  NS_OVERRIDE virtual nsresult Assign(nsSMILValue& aDest,
                                      const nsSMILValue& aSrc) const;
  NS_OVERRIDE virtual nsresult Add(nsSMILValue& aDest,
                                   const nsSMILValue& aValueToAdd,
                                   PRUint32 aCount) const;
  NS_OVERRIDE virtual nsresult ComputeDistance(const nsSMILValue& aFrom,
                                               const nsSMILValue& aTo,
                                               double& aDistance) const;
  NS_OVERRIDE virtual nsresult Interpolate(const nsSMILValue& aStartVal,
                                           const nsSMILValue& aEndVal,
                                           double aUnitDistance,
                                           nsSMILValue& aResult) const;

  
  
  NS_OVERRIDE virtual PRBool ValueFromString(nsCSSProperty aPropID,
                                             nsIContent* aTargetElement,
                                             const nsAString& aString,
                                             nsSMILValue& aValue) const;

  NS_OVERRIDE virtual PRBool ValueToString(const nsSMILValue& aValue,
                                           nsAString& aString) const;

  
  static nsSMILCSSValueType sSingleton;

private:
  nsSMILCSSValueType() {}
};

#endif 
