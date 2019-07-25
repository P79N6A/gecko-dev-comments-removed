




































#ifndef SVGTRANSFORMLISTSMILTYPE_H_
#define SVGTRANSFORMLISTSMILTYPE_H_

#include "nsISMILType.h"
#include "nsTArray.h"

class nsSMILValue;

namespace mozilla {

class SVGTransform;
class SVGTransformList;
class SVGTransformSMILData;




























































class SVGTransformListSMILType : public nsISMILType
{
public:
  
  static SVGTransformListSMILType sSingleton;

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
  
  
  static nsresult AppendTransform(const SVGTransformSMILData& aTransform,
                                  nsSMILValue& aValue);
  static bool AppendTransforms(const SVGTransformList& aList,
                                 nsSMILValue& aValue);
  static bool GetTransforms(const nsSMILValue& aValue,
                              nsTArray<SVGTransform>& aTransforms);


private:
  
  
  SVGTransformListSMILType() {}
  ~SVGTransformListSMILType() {}
};

} 

#endif 
