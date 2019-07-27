




#ifndef MOZILLA_SMILINTEGERTYPE_H_
#define MOZILLA_SMILINTEGERTYPE_H_

#include "mozilla/Attributes.h"
#include "nsISMILType.h"

namespace mozilla {

class SMILIntegerType : public nsISMILType
{
public:
  virtual void Init(nsSMILValue& aValue) const MOZ_OVERRIDE;
  virtual void Destroy(nsSMILValue& aValue) const MOZ_OVERRIDE;
  virtual nsresult Assign(nsSMILValue& aDest, const nsSMILValue& aSrc) const MOZ_OVERRIDE;
  virtual bool IsEqual(const nsSMILValue& aLeft,
                       const nsSMILValue& aRight) const MOZ_OVERRIDE;
  virtual nsresult Add(nsSMILValue& aDest, const nsSMILValue& aValueToAdd,
                       uint32_t aCount) const MOZ_OVERRIDE;
  virtual nsresult ComputeDistance(const nsSMILValue& aFrom,
                                   const nsSMILValue& aTo,
                                   double& aDistance) const MOZ_OVERRIDE;
  virtual nsresult Interpolate(const nsSMILValue& aStartVal,
                               const nsSMILValue& aEndVal,
                               double aUnitDistance,
                               nsSMILValue& aResult) const MOZ_OVERRIDE;

  static SMILIntegerType*
  Singleton()
  {
    static SMILIntegerType sSingleton;
    return &sSingleton;
  }

private:
  MOZ_CONSTEXPR SMILIntegerType() {}
};

} 

#endif 
