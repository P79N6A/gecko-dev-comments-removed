



































#ifndef MOZILLA_SVGPOINTLISTSMILTYPE_H_
#define MOZILLA_SVGPOINTLISTSMILTYPE_H_

#include "nsISMILType.h"

class nsSMILValue;

namespace mozilla {






class SVGPointListSMILType : public nsISMILType
{
public:
  
  static SVGPointListSMILType sSingleton;

protected:
  
  

  virtual void     Init(nsSMILValue& aValue) const;

  virtual void     Destroy(nsSMILValue& aValue) const;
  virtual nsresult Assign(nsSMILValue& aDest, const nsSMILValue& aSrc) const;
  virtual bool     IsEqual(const nsSMILValue& aLeft,
                           const nsSMILValue& aRight) const;
  virtual nsresult Add(nsSMILValue& aDest, const nsSMILValue& aValueToAdd,
                       PRUint32 aCount) const;
  virtual nsresult ComputeDistance(const nsSMILValue& aFrom,
                                   const nsSMILValue& aTo,
                                   double& aDistance) const;
  virtual nsresult Interpolate(const nsSMILValue& aStartVal,
                               const nsSMILValue& aEndVal,
                               double aUnitDistance,
                               nsSMILValue& aResult) const;

private:
  
  
  SVGPointListSMILType() {}
  ~SVGPointListSMILType() {}
};

} 

#endif 
