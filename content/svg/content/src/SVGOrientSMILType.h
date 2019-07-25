




#ifndef MOZILLA_SVGORIENTSMILTYPE_H_
#define MOZILLA_SVGORIENTSMILTYPE_H_

#include "nsISMILType.h"

class nsSMILValue;















namespace mozilla {

class SVGOrientSMILType : public nsISMILType
{
public:
  
  static SVGOrientSMILType sSingleton;

protected:
  
  
  virtual void     Init(nsSMILValue& aValue) const;
  virtual void     Destroy(nsSMILValue&) const;
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
  
  
  SVGOrientSMILType()  {}
  ~SVGOrientSMILType() {}
};

} 

#endif 
