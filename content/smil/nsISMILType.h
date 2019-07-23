





































#ifndef NS_ISMILTYPE_H_
#define NS_ISMILTYPE_H_

#include "nscore.h"

class nsSMILValue;































class nsISMILType
{
public:
  







  virtual nsresult Init(nsSMILValue& aValue) const = 0;

  





  virtual void Destroy(nsSMILValue& aValue) const = 0;

  










  virtual nsresult Assign(nsSMILValue& aDest,
                          const nsSMILValue& aSrc) const = 0;

  
































  virtual nsresult Add(nsSMILValue& aDest,
                       const nsSMILValue& aValueToAdd,
                       PRUint32 aCount) const = 0;

  

















  virtual nsresult SandwichAdd(nsSMILValue& aDest,
                               const nsSMILValue& aValueToAdd) const
  {
    return Add(aDest, aValueToAdd, 1);
  }

  














  virtual nsresult ComputeDistance(const nsSMILValue& aFrom,
                                   const nsSMILValue& aTo,
                                   double& aDistance) const = 0;

  

















  virtual nsresult Interpolate(const nsSMILValue& aStartVal,
                               const nsSMILValue& aEndVal,
                               double aUnitDistance,
                               nsSMILValue& aResult) const = 0;

  



  virtual ~nsISMILType() {};
};

#endif 
