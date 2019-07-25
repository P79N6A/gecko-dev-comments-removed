





































#ifndef NS_SMILVALUE_H_
#define NS_SMILVALUE_H_

#include "nsISMILType.h"
#include "nsSMILNullType.h"










class nsSMILValue
{
public:
  nsSMILValue() : mU(), mType(&nsSMILNullType::sSingleton) { }
  explicit nsSMILValue(const nsISMILType* aType);
  nsSMILValue(const nsSMILValue& aVal);

  ~nsSMILValue()
  {
    mType->Destroy(*this);
  }

  const nsSMILValue& operator=(const nsSMILValue& aVal);

  
  
  bool operator==(const nsSMILValue& aVal) const;
  bool operator!=(const nsSMILValue& aVal) const {
    return !(*this == aVal);
  }

  bool IsNull() const
  {
    return (mType == &nsSMILNullType::sSingleton);
  }

  
  void     Swap(nsSMILValue& aOther);

  nsresult Add(const nsSMILValue& aValueToAdd, PRUint32 aCount = 1);
  nsresult SandwichAdd(const nsSMILValue& aValueToAdd);
  nsresult ComputeDistance(const nsSMILValue& aTo, double& aDistance) const;
  nsresult Interpolate(const nsSMILValue& aEndVal,
                       double aUnitDistance,
                       nsSMILValue& aResult) const;

  union {
    bool mBool;
    PRUint64 mUint;
    PRInt64 mInt;
    double mDouble;
    struct {
      float mAngle;
      PRUint16 mUnit;
      PRUint16 mOrientType;
    } mOrient;
    PRInt32 mIntPair[2];
    float mNumberPair[2];
    void* mPtr;
  } mU;
  const nsISMILType* mType;

protected:
  void InitAndCheckPostcondition(const nsISMILType* aNewType);
  void DestroyAndCheckPostcondition();
  void DestroyAndReinit(const nsISMILType* aNewType);
};

#endif  
