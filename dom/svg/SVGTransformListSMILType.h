




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
  
  
  virtual void     Init(nsSMILValue& aValue) const override;
  virtual void     Destroy(nsSMILValue& aValue) const override;
  virtual nsresult Assign(nsSMILValue& aDest, const nsSMILValue& aSrc) const override;
  virtual bool     IsEqual(const nsSMILValue& aLeft,
                           const nsSMILValue& aRight) const override;
  virtual nsresult Add(nsSMILValue& aDest,
                       const nsSMILValue& aValueToAdd,
                       uint32_t aCount) const override;
  virtual nsresult SandwichAdd(nsSMILValue& aDest,
                               const nsSMILValue& aValueToAdd) const override;
  virtual nsresult ComputeDistance(const nsSMILValue& aFrom,
                                   const nsSMILValue& aTo,
                                   double& aDistance) const override;
  virtual nsresult Interpolate(const nsSMILValue& aStartVal,
                               const nsSMILValue& aEndVal,
                               double aUnitDistance,
                               nsSMILValue& aResult) const override;

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
