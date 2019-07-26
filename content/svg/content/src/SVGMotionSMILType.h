






#ifndef MOZILLA_SVGMOTIONSMILTYPE_H_
#define MOZILLA_SVGMOTIONSMILTYPE_H_

#include "mozilla/Attributes.h"
#include "gfxMatrix.h"
#include "nsISMILType.h"

class gfxFlattenedPath;
class nsSMILValue;

namespace mozilla {




enum RotateType {
  eRotateType_Explicit,     
  eRotateType_Auto,         
  eRotateType_AutoReverse   
};








class SVGMotionSMILType : public nsISMILType
{
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
  
  static gfxMatrix CreateMatrix(const nsSMILValue& aSMILVal);

  
  
  static nsSMILValue ConstructSMILValue(gfxFlattenedPath* aPath,
                                        float aDist,
                                        RotateType aRotateType,
                                        float aRotateAngle);

private:
  
  MOZ_CONSTEXPR SVGMotionSMILType() {}
};

} 

#endif 
