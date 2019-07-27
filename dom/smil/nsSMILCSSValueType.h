






#ifndef NS_SMILCSSVALUETYPE_H_
#define NS_SMILCSSVALUETYPE_H_

#include "nsISMILType.h"
#include "nsCSSProperty.h"
#include "mozilla/Attributes.h"

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
  
  
  virtual void     Init(nsSMILValue& aValue) const override;
  virtual void     Destroy(nsSMILValue&) const override;
  virtual nsresult Assign(nsSMILValue& aDest,
                          const nsSMILValue& aSrc) const override;
  virtual bool     IsEqual(const nsSMILValue& aLeft,
                           const nsSMILValue& aRight) const override;
  virtual nsresult Add(nsSMILValue& aDest,
                       const nsSMILValue& aValueToAdd,
                       uint32_t aCount) const override;
  virtual nsresult ComputeDistance(const nsSMILValue& aFrom,
                                   const nsSMILValue& aTo,
                                   double& aDistance) const override;
  virtual nsresult Interpolate(const nsSMILValue& aStartVal,
                               const nsSMILValue& aEndVal,
                               double aUnitDistance,
                               nsSMILValue& aResult) const override;

public:
  
  
  






















  static void ValueFromString(nsCSSProperty aPropID,
                              Element* aTargetElement,
                              const nsAString& aString,
                              nsSMILValue& aValue,
                              bool* aIsContextSensitive);

  











  static bool ValueToString(const nsSMILValue& aValue, nsAString& aString);

private:
  
  MOZ_CONSTEXPR nsSMILCSSValueType() {}
};

#endif 
