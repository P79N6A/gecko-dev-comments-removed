





#ifndef NS_SMILVALUE_H_
#define NS_SMILVALUE_H_

#include "nsISMILType.h"
#include "nsSMILNullType.h"










class nsSMILValue
{
public:
  nsSMILValue() : mU(), mType(nsSMILNullType::Singleton()) { }
  explicit nsSMILValue(const nsISMILType* aType);
  nsSMILValue(const nsSMILValue& aVal);

  ~nsSMILValue()
  {
    mType->Destroy(*this);
  }

  const nsSMILValue& operator=(const nsSMILValue& aVal);

  
  nsSMILValue(nsSMILValue&& aVal);
  nsSMILValue& operator=(nsSMILValue&& aVal);

  
  
  bool operator==(const nsSMILValue& aVal) const;
  bool operator!=(const nsSMILValue& aVal) const {
    return !(*this == aVal);
  }

  bool IsNull() const
  {
    return (mType == nsSMILNullType::Singleton());
  }

  
  void     Swap(nsSMILValue& aOther);

  nsresult Add(const nsSMILValue& aValueToAdd, uint32_t aCount = 1);
  nsresult SandwichAdd(const nsSMILValue& aValueToAdd);
  nsresult ComputeDistance(const nsSMILValue& aTo, double& aDistance) const;
  nsresult Interpolate(const nsSMILValue& aEndVal,
                       double aUnitDistance,
                       nsSMILValue& aResult) const;

  union {
    bool mBool;
    uint64_t mUint;
    int64_t mInt;
    double mDouble;
    struct {
      float mAngle;
      uint16_t mUnit;
      uint16_t mOrientType;
    } mOrient;
    int32_t mIntPair[2];
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
