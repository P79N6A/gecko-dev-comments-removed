





































#ifndef NS_SMILFLOATTYPE_H_
#define NS_SMILFLOATTYPE_H_

#include "nsISMILType.h"

class nsSMILFloatType : public nsISMILType
{
public:
  virtual nsresult Init(nsSMILValue& aValue) const;
  virtual void     Destroy(nsSMILValue&) const;
  virtual nsresult Assign(nsSMILValue& aDest, const nsSMILValue& aSrc) const;
  virtual nsresult Add(nsSMILValue& aDest, const nsSMILValue& aValueToAdd,
                       PRUint32 aCount) const;
  virtual nsresult ComputeDistance(const nsSMILValue& aFrom,
                                   const nsSMILValue& aTo,
                                   double& aDistance) const;
  virtual nsresult Interpolate(const nsSMILValue& aStartVal,
                               const nsSMILValue& aEndVal,
                               double aUnitDistance,
                               nsSMILValue& aResult) const;

  static nsSMILFloatType sSingleton;

private:
  nsSMILFloatType() {}
};

#endif 
