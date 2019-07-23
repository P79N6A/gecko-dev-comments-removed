





































#ifndef NS_SMILVALUE_H_
#define NS_SMILVALUE_H_

#include "nsISMILType.h"
#include "nsSMILNullType.h"

class nsSMILValue
{
public:
  nsSMILValue() : mU(), mType(&nsSMILNullType::sSingleton) { }
  nsSMILValue(const nsISMILType* aType);
  nsSMILValue(const nsSMILValue& aVal);

  ~nsSMILValue()
  {
    mType->Destroy(*this);
  }

  const nsSMILValue& operator=(const nsSMILValue& aVal);

  PRBool IsNull() const
  {
    return (mType == &nsSMILNullType::sSingleton);
  }

  nsresult Add(const nsSMILValue& aValueToAdd, PRUint32 aCount = 1);
  nsresult SandwichAdd(const nsSMILValue& aValueToAdd);
  nsresult ComputeDistance(const nsSMILValue& aTo, double& aDistance) const;
  nsresult Interpolate(const nsSMILValue& aEndVal,
                       double aUnitDistance,
                       nsSMILValue& aResult) const;

  union {
    PRBool mBool;
    PRUint64 mUint;
    PRInt64 mInt;
    double mDouble;
    void* mPtr;
  } mU;
  const nsISMILType* mType;

protected:
  nsresult InitAndCheckPostcondition(const nsISMILType* aNewType);
  void     DestroyAndCheckPostcondition();
  nsresult DestroyAndReinit(const nsISMILType* aNewType);
};

#endif  
