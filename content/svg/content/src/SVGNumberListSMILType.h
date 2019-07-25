



































#ifndef MOZILLA_SVGNUMBERLISTSMILTYPE_H_
#define MOZILLA_SVGNUMBERLISTSMILTYPE_H_

#include "nsISMILType.h"

class nsSMILValue;

namespace mozilla {






class SVGNumberListSMILType : public nsISMILType
{
public:
  
  static SVGNumberListSMILType sSingleton;

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
  
  
  SVGNumberListSMILType() {}
  ~SVGNumberListSMILType() {}
};

} 

#endif 
