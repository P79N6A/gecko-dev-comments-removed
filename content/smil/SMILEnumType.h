



































#ifndef MOZILLA_SMILENUMTYPE_H_
#define MOZILLA_SMILENUMTYPE_H_

#include "nsISMILType.h"

namespace mozilla {

class SMILEnumType : public nsISMILType
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

  static SMILEnumType sSingleton;

private:
  SMILEnumType() {}
};

} 

#endif 
