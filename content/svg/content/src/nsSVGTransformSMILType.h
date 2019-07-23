




































#ifndef NS_SVGTRANSFORMSMILTYPE_H_
#define NS_SVGTRANSFORMSMILTYPE_H_

#include "nsISMILType.h"
#include "nsSVGSMILTransform.h"
#include "nsTArray.h"

class nsSMILValue;




























































class nsSVGTransformSMILType : public nsISMILType
{
public:
  
  virtual nsresult Init(nsSMILValue& aValue) const;
  virtual void     Destroy(nsSMILValue& aValue) const;
  virtual nsresult Assign(nsSMILValue& aDest, const nsSMILValue& aSrc) const;
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
  
  PRUint32 GetNumTransforms(const nsSMILValue& aValue) const;
  const nsSVGSMILTransform* GetTransformAt(PRUint32 aIndex,
                                           const nsSMILValue& aValue) const;
  nsresult AppendTransform(const nsSVGSMILTransform& aTransform,
                           nsSMILValue& aValue) const;

  static nsSVGTransformSMILType sSingleton;

protected:
  typedef nsTArray<nsSVGSMILTransform> TransformArray;

private:
  nsSVGTransformSMILType() {}
};

#endif 
