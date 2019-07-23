





































#ifndef NS_SMILNULLTYPE_H_
#define NS_SMILNULLTYPE_H_

#include "nsISMILType.h"

class nsSMILNullType : public nsISMILType
{
public:
  
  static nsSMILNullType sSingleton;

protected:
  
  
  virtual nsresult Init(nsSMILValue& aValue) const { return NS_OK; }
  virtual void Destroy(nsSMILValue& aValue) const {}
  virtual nsresult Assign(nsSMILValue& aDest, const nsSMILValue& aSrc) const;

  
  
  virtual PRBool   IsEqual(const nsSMILValue& aLeft,
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
  
  
  nsSMILNullType()  {}
  ~nsSMILNullType() {}
};

#endif 
