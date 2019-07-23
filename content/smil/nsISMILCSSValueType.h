






































#ifndef NS_ISMILCSSVALUETYPE_H_
#define NS_ISMILCSSVALUETYPE_H_

#include "nsISMILType.h"
#include "nsCSSProperty.h"
#include "nscore.h" 

class nsPresContext;
class nsIContent;
class nsAString;






class nsISMILCSSValueType : public nsISMILType
{
public:
  
  
  NS_OVERRIDE virtual nsresult Init(nsSMILValue& aValue) const = 0;
  NS_OVERRIDE virtual void     Destroy(nsSMILValue& aValue) const = 0;
  NS_OVERRIDE virtual nsresult Assign(nsSMILValue& aDest,
                                      const nsSMILValue& aSrc) const = 0;
  NS_OVERRIDE virtual nsresult Add(nsSMILValue& aDest,
                                   const nsSMILValue& aValueToAdd,
                                   PRUint32 aCount) const = 0;
  NS_OVERRIDE virtual nsresult ComputeDistance(const nsSMILValue& aFrom,
                                               const nsSMILValue& aTo,
                                               double& aDistance) const = 0;
  NS_OVERRIDE virtual nsresult Interpolate(const nsSMILValue& aStartVal,
                                           const nsSMILValue& aEndVal,
                                           double aUnitDistance,
                                           nsSMILValue& aResult) const = 0;

  



  virtual ~nsISMILCSSValueType() {};

  
  
  
  

  














  virtual PRBool ValueFromString(nsCSSProperty aPropID,
                                 nsIContent* aTargetElement,
                                 const nsAString& aString,
                                 nsSMILValue& aValue) const = 0;

  






  virtual PRBool ValueToString(const nsSMILValue& aValue,
                               nsAString& aString) const = 0;
};

#endif 
