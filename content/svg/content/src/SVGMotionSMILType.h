






#ifndef MOZILLA_SVGMOTIONSMILTYPE_H_
#define MOZILLA_SVGMOTIONSMILTYPE_H_

#include "mozilla/gfx/2D.h"
#include "mozilla/Attributes.h"
#include "nsISMILType.h"

class nsSMILValue;

namespace mozilla {

namespace gfx {
class Matrix;
}




enum RotateType {
  eRotateType_Explicit,     
  eRotateType_Auto,         
  eRotateType_AutoReverse   
};








class SVGMotionSMILType : public nsISMILType
{
  typedef mozilla::gfx::Path Path;

public:
  
  static SVGMotionSMILType sSingleton;

protected:
  
  
  virtual void     Init(nsSMILValue& aValue) const MOZ_OVERRIDE;
  virtual void     Destroy(nsSMILValue& aValue) const MOZ_OVERRIDE;
  virtual nsresult Assign(nsSMILValue& aDest, const nsSMILValue& aSrc) const MOZ_OVERRIDE;
  virtual bool     IsEqual(const nsSMILValue& aLeft,
                           const nsSMILValue& aRight) const MOZ_OVERRIDE;
  virtual nsresult Add(nsSMILValue& aDest,
                       const nsSMILValue& aValueToAdd,
                       uint32_t aCount) const MOZ_OVERRIDE;
  virtual nsresult SandwichAdd(nsSMILValue& aDest,
                               const nsSMILValue& aValueToAdd) const MOZ_OVERRIDE;
  virtual nsresult ComputeDistance(const nsSMILValue& aFrom,
                                   const nsSMILValue& aTo,
                                   double& aDistance) const MOZ_OVERRIDE;
  virtual nsresult Interpolate(const nsSMILValue& aStartVal,
                               const nsSMILValue& aEndVal,
                               double aUnitDistance,
                               nsSMILValue& aResult) const MOZ_OVERRIDE;
public:
  
  static gfx::Matrix CreateMatrix(const nsSMILValue& aSMILVal);

  
  
  static nsSMILValue ConstructSMILValue(Path* aPath,
                                        float aDist,
                                        RotateType aRotateType,
                                        float aRotateAngle);

private:
  
  MOZ_CONSTEXPR SVGMotionSMILType() {}
};

} 

#endif 
