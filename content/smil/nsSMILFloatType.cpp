





































#include "nsSMILFloatType.h"
#include "nsSMILValue.h"
#include "nsDebug.h"
#include <math.h>

 nsSMILFloatType nsSMILFloatType::sSingleton;

nsresult
nsSMILFloatType::Init(nsSMILValue& aValue) const
{
  NS_PRECONDITION(aValue.mType == this || aValue.IsNull(),
    "Unexpected value type");
  aValue.mU.mDouble = 0.0;
  aValue.mType = this;
  return NS_OK;
}

void
nsSMILFloatType::Destroy(nsSMILValue& aValue) const
{
  NS_PRECONDITION(aValue.mType == this, "Unexpected SMIL value.");
  aValue.mU.mDouble = 0.0;
  aValue.mType      = &nsSMILNullType::sSingleton;
}

nsresult
nsSMILFloatType::Assign(nsSMILValue& aDest, const nsSMILValue& aSrc) const
{
  NS_PRECONDITION(aDest.mType == aSrc.mType, "Incompatible SMIL types.");
  NS_PRECONDITION(aDest.mType == this, "Unexpected SMIL value.");
  aDest.mU.mDouble = aSrc.mU.mDouble;
  return NS_OK;
}

nsresult
nsSMILFloatType::Add(nsSMILValue& aDest, const nsSMILValue& aValueToAdd,
                     PRUint32 aCount) const
{
  NS_PRECONDITION(aValueToAdd.mType == aDest.mType,
                  "Trying to add invalid types");
  NS_PRECONDITION(aValueToAdd.mType == this, "Unexpected source type");
  aDest.mU.mDouble += aValueToAdd.mU.mDouble * aCount;
  return NS_OK;
}

nsresult
nsSMILFloatType::ComputeDistance(const nsSMILValue& aFrom,
                                 const nsSMILValue& aTo,
                                 double& aDistance) const
{
  NS_PRECONDITION(aFrom.mType == aTo.mType,"Trying to compare different types");
  NS_PRECONDITION(aFrom.mType == this, "Unexpected source type");
  
  const double &from = aFrom.mU.mDouble;
  const double &to   = aTo.mU.mDouble;

  aDistance = fabs(to - from);

  return NS_OK;
}

nsresult
nsSMILFloatType::Interpolate(const nsSMILValue& aStartVal,
                             const nsSMILValue& aEndVal,
                             double aUnitDistance,
                             nsSMILValue& aResult) const
{
  NS_PRECONDITION(aStartVal.mType == aEndVal.mType,
      "Trying to interpolate different types");
  NS_PRECONDITION(aStartVal.mType == this,
      "Unexpected types for interpolation.");
  NS_PRECONDITION(aResult.mType   == this, "Unexpected result type.");

  const double &startVal = aStartVal.mU.mDouble;
  const double &endVal   = aEndVal.mU.mDouble;

  aResult.mU.mDouble = (startVal + (endVal - startVal) * aUnitDistance);

  return NS_OK;
}
