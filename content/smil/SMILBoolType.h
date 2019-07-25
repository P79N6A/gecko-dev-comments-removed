



































#ifndef MOZILLA_SMILBOOLTYPE_H_
#define MOZILLA_SMILBOOLTYPE_H_

#include "nsISMILType.h"

namespace mozilla {

class SMILBoolType : public nsISMILType
{
public:
  
  static SMILBoolType sSingleton;

protected:
  
  
  virtual void     Init(nsSMILValue& aValue) const;
  virtual void     Destroy(nsSMILValue&) const;
  virtual nsresult Assign(nsSMILValue& aDest, const nsSMILValue& aSrc) const;
  virtual nsresult Add(nsSMILValue& aDest, const nsSMILValue& aValueToAdd,
                       PRUint32 aCount) const;
  virtual bool IsEqual(const nsSMILValue& aLeft,
                         const nsSMILValue& aRight) const;
  virtual nsresult ComputeDistance(const nsSMILValue& aFrom,
                                   const nsSMILValue& aTo,
                                   double& aDistance) const;
  virtual nsresult Interpolate(const nsSMILValue& aStartVal,
                               const nsSMILValue& aEndVal,
                               double aUnitDistance,
                               nsSMILValue& aResult) const;

private:
  
  
  SMILBoolType()  {}
  ~SMILBoolType() {}
};

} 

#endif 
