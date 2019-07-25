






































#ifndef NS_SMILCSSVALUETYPE_H_
#define NS_SMILCSSVALUETYPE_H_

#include "nsISMILType.h"
#include "nsCSSProperty.h"
#include "nscore.h" 

class nsAString;

namespace mozilla {
namespace dom {
class Element;
} 
} 




class nsSMILCSSValueType : public nsISMILType
{
public:
  typedef mozilla::dom::Element Element;

  
  static nsSMILCSSValueType sSingleton;

protected:
  
  
  NS_OVERRIDE virtual void     Init(nsSMILValue& aValue) const;
  NS_OVERRIDE virtual void     Destroy(nsSMILValue&) const;
  NS_OVERRIDE virtual nsresult Assign(nsSMILValue& aDest,
                                      const nsSMILValue& aSrc) const;
  NS_OVERRIDE virtual bool     IsEqual(const nsSMILValue& aLeft,
                                       const nsSMILValue& aRight) const;
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

public:
  
  
  






















  static void ValueFromString(nsCSSProperty aPropID,
                              Element* aTargetElement,
                              const nsAString& aString,
                              nsSMILValue& aValue,
                              bool* aIsContextSensitive);

  











  static bool ValueToString(const nsSMILValue& aValue, nsAString& aString);

private:
  
  
  nsSMILCSSValueType()  {}
  ~nsSMILCSSValueType() {}
};

#endif 
