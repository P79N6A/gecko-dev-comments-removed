



































#include "SMILBoolType.h"
#include "nsSMILValue.h"
#include "nsDebug.h"
#include <math.h>

namespace mozilla {

 SMILBoolType SMILBoolType::sSingleton;

void
SMILBoolType::Init(nsSMILValue& aValue) const
{
  NS_PRECONDITION(aValue.IsNull(), "Unexpected value type");
  aValue.mU.mBool = PR_FALSE;
  aValue.mType = this;
}

void
SMILBoolType::Destroy(nsSMILValue& aValue) const
{
  NS_PRECONDITION(aValue.mType == this, "Unexpected SMIL value");
  aValue.mU.mBool = PR_FALSE;
  aValue.mType = &nsSMILNullType::sSingleton;
}

nsresult
SMILBoolType::Assign(nsSMILValue& aDest, const nsSMILValue& aSrc) const
{
  NS_PRECONDITION(aDest.mType == aSrc.mType, "Incompatible SMIL types");
  NS_PRECONDITION(aDest.mType == this, "Unexpected SMIL value");
  aDest.mU.mBool = aSrc.mU.mBool;
  return NS_OK;
}

bool
SMILBoolType::IsEqual(const nsSMILValue& aLeft,
                      const nsSMILValue& aRight) const
{
  NS_PRECONDITION(aLeft.mType == aRight.mType, "Incompatible SMIL types");
  NS_PRECONDITION(aLeft.mType == this, "Unexpected type for SMIL value");

  return aLeft.mU.mBool == aRight.mU.mBool;
}

nsresult
SMILBoolType::Add(nsSMILValue& aDest, const nsSMILValue& aValueToAdd,
                  PRUint32 aCount) const
{
  NS_PRECONDITION(aValueToAdd.mType == aDest.mType,
                  "Trying to add invalid types");
  NS_PRECONDITION(aValueToAdd.mType == this, "Unexpected source type");
  return NS_ERROR_FAILURE; 
}

nsresult
SMILBoolType::ComputeDistance(const nsSMILValue& aFrom,
                              const nsSMILValue& aTo,
                              double& aDistance) const
{
  NS_PRECONDITION(aFrom.mType == aTo.mType,"Trying to compare different types");
  NS_PRECONDITION(aFrom.mType == this, "Unexpected source type");
  return NS_ERROR_FAILURE; 
}

nsresult
SMILBoolType::Interpolate(const nsSMILValue& aStartVal,
                          const nsSMILValue& aEndVal,
                          double aUnitDistance,
                          nsSMILValue& aResult) const
{
  NS_PRECONDITION(aStartVal.mType == aEndVal.mType,
      "Trying to interpolate different types");
  NS_PRECONDITION(aStartVal.mType == this,
      "Unexpected types for interpolation");
  NS_PRECONDITION(aResult.mType   == this, "Unexpected result type");
  return NS_ERROR_FAILURE; 
}

} 
