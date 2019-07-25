






































#ifndef MOZILLA_SVGMOTIONSMILTYPE_H_
#define MOZILLA_SVGMOTIONSMILTYPE_H_

#include "nsISMILType.h"
#include "gfxMatrix.h"
#include "nsTArray.h"
class nsSVGPathElement;
class nsSMILValue;
class gfxFlattenedPath;

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
  
  
  virtual void     Init(nsSMILValue& aValue) const;
  virtual void     Destroy(nsSMILValue& aValue) const;
  virtual nsresult Assign(nsSMILValue& aDest, const nsSMILValue& aSrc) const;
  virtual bool     IsEqual(const nsSMILValue& aLeft,
                           const nsSMILValue& aRight) const;
  virtual nsresult Add(nsSMILValue& aDest,
                       const nsSMILValue& aValueToAdd,
                       PRUint32 aCount) const;
  virtual nsresult SandwichAdd(nsSMILValue& aDest,
                               const nsSMILValue& aValueToAdd) const;
  virtual nsresult ComputeDistance(const nsSMILValue& aFrom,
                                   const nsSMILValue& aTo,
                                   double& aDistance) const;
  virtual nsresult Interpolate(const nsSMILValue& aStartVal,
                               const nsSMILValue& aEndVal,
                               double aUnitDistance,
                               nsSMILValue& aResult) const;
public:
  
  static gfxMatrix CreateMatrix(const nsSMILValue& aSMILVal);

  
  
  static nsSMILValue ConstructSMILValue(gfxFlattenedPath* aPath,
                                        float aDist,
                                        RotateType aRotateType,
                                        float aRotateAngle);

private:
  
  
  SVGMotionSMILType()  {}
  ~SVGMotionSMILType() {}
};

} 

#endif 
