




#ifndef SVGTRANSFORMLISTSMILTYPE_H_
#define SVGTRANSFORMLISTSMILTYPE_H_

#include "mozilla/Attributes.h"
#include "nsISMILType.h"
#include "nsTArray.h"

class nsSMILValue;

namespace mozilla {

class nsSVGTransform;
class SVGTransformList;
class SVGTransformSMILData;




























































class SVGTransformListSMILType : public nsISMILType
{
public:
  
  static SVGTransformListSMILType*
  Singleton()
  {
    static SVGTransformListSMILType sSingleton;
    return &sSingleton;
  }

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
  
  
  static nsresult AppendTransform(const SVGTransformSMILData& aTransform,
                                  nsSMILValue& aValue);
  static bool AppendTransforms(const SVGTransformList& aList,
                                 nsSMILValue& aValue);
  static bool GetTransforms(const nsSMILValue& aValue,
                              FallibleTArray<nsSVGTransform>& aTransforms);


private:
  
  MOZ_CONSTEXPR SVGTransformListSMILType() {}
};

} 

#endif 
