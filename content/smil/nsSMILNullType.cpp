





































#include "nsSMILNullType.h"
#include "nsSMILValue.h"
#include "nsDebug.h"

 nsSMILNullType nsSMILNullType::sSingleton;

nsresult
nsSMILNullType::Assign(nsSMILValue& aDest, const nsSMILValue& aSrc) const
{
  NS_PRECONDITION(aDest.mType == aSrc.mType, "Incompatible SMIL types");
  NS_PRECONDITION(aSrc.mType == this, "Unexpected source type");
  aDest.mU    = aSrc.mU;
  aDest.mType = &sSingleton;
  return NS_OK;
}

bool
nsSMILNullType::IsEqual(const nsSMILValue& aLeft,
                        const nsSMILValue& aRight) const
{
  NS_PRECONDITION(aLeft.mType == aRight.mType, "Incompatible SMIL types");
  NS_PRECONDITION(aLeft.mType == this, "Unexpected type for SMIL value");

  return true;  
}

nsresult
nsSMILNullType::Add(nsSMILValue& aDest, const nsSMILValue& aValueToAdd,
                    PRUint32 aCount) const
{
  NS_NOTREACHED("Adding NULL type");
  return NS_ERROR_FAILURE;
}

nsresult
nsSMILNullType::ComputeDistance(const nsSMILValue& aFrom,
                                const nsSMILValue& aTo,
                                double& aDistance) const
{
  NS_NOTREACHED("Computing distance for NULL type");
  return NS_ERROR_FAILURE;
}

nsresult
nsSMILNullType::Interpolate(const nsSMILValue& aStartVal,
                            const nsSMILValue& aEndVal,
                            double aUnitDistance,
                            nsSMILValue& aResult) const
{
  NS_NOTREACHED("Interpolating NULL type");
  return NS_ERROR_FAILURE;
}
