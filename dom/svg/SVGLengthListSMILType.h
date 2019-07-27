




#ifndef MOZILLA_SVGLENGTHLISTSMILTYPE_H_
#define MOZILLA_SVGLENGTHLISTSMILTYPE_H_

#include "mozilla/Attributes.h"
#include "nsISMILType.h"

class nsSMILValue;

namespace mozilla {






class SVGLengthListSMILType : public nsISMILType
{
public:
  
  static SVGLengthListSMILType sSingleton;

protected:
  
  

  













































  virtual void     Init(nsSMILValue& aValue) const override;

  virtual void     Destroy(nsSMILValue& aValue) const override;
  virtual nsresult Assign(nsSMILValue& aDest, const nsSMILValue& aSrc) const override;
  virtual bool     IsEqual(const nsSMILValue& aLeft,
                           const nsSMILValue& aRight) const override;
  virtual nsresult Add(nsSMILValue& aDest, const nsSMILValue& aValueToAdd,
                       uint32_t aCount) const override;
  virtual nsresult ComputeDistance(const nsSMILValue& aFrom,
                                   const nsSMILValue& aTo,
                                   double& aDistance) const override;
  virtual nsresult Interpolate(const nsSMILValue& aStartVal,
                               const nsSMILValue& aEndVal,
                               double aUnitDistance,
                               nsSMILValue& aResult) const override;

private:
  
  MOZ_CONSTEXPR SVGLengthListSMILType() {}
};

} 

#endif 
