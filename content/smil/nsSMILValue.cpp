





































#include "nsSMILValue.h"
#include "nsDebug.h"

nsSMILValue::nsSMILValue(const nsISMILType* aType)
: mU(),
  mType(&nsSMILNullType::sSingleton)
{
  if (!aType) return;

  nsresult rv = aType->Init(*this);
  NS_POSTCONDITION(mType == aType || (NS_FAILED(rv) && IsNull()),
    "Post-condition of Init failed. nsSMILValue is invalid.");
}

nsSMILValue::nsSMILValue(const nsSMILValue& aVal)
:
  mU(),
  mType(&nsSMILNullType::sSingleton)
{
  nsresult rv = aVal.mType->Init(*this);
  NS_POSTCONDITION(mType == aVal.mType || (NS_FAILED(rv) && IsNull()),
    "Post-condition of Init failed. nsSMILValue is invalid.");
  if (NS_FAILED(rv)) return;
  mType->Assign(*this, aVal);
}

const nsSMILValue&
nsSMILValue::operator=(const nsSMILValue& aVal)
{
  if (&aVal == this)
    return *this;

  if (mType != aVal.mType) {
    mType->Destroy(*this);
    NS_POSTCONDITION(IsNull(), "nsSMILValue not null after destroying");
    nsresult rv = aVal.mType->Init(*this);
    NS_POSTCONDITION(mType == aVal.mType || (NS_FAILED(rv) && IsNull()),
      "Post-condition of Init failed. nsSMILValue is invalid.");
    if (NS_FAILED(rv)) return *this;
  }

  mType->Assign(*this, aVal);

  return *this;
}

nsresult
nsSMILValue::Add(const nsSMILValue& aValueToAdd, PRUint32 aCount)
{
  if (aValueToAdd.IsNull()) return NS_OK;

  if (aValueToAdd.mType != mType) {
    NS_WARNING("Trying to add incompatible types.");
    return NS_ERROR_FAILURE;
  }

  return mType->Add(*this, aValueToAdd, aCount);
}

nsresult
nsSMILValue::ComputeDistance(const nsSMILValue& aTo, double& aDistance) const
{
  if (aTo.mType != mType) {
    NS_WARNING("Trying to calculate distance between incompatible types.");
    return NS_ERROR_FAILURE;
  }

  return mType->ComputeDistance(*this, aTo, aDistance);
}

nsresult
nsSMILValue::Interpolate(const nsSMILValue& aEndVal,
                         double aUnitDistance,
                         nsSMILValue& aResult) const
{
  if (aEndVal.mType != mType) {
    NS_WARNING("Trying to interpolate between incompatible types.");
    return NS_ERROR_FAILURE;
  }

  if (aResult.mType != mType) {
    aResult.mType->Destroy(aResult);
    NS_POSTCONDITION(aResult.IsNull(), "nsSMILValue not null after destroying");
    nsresult rv = mType->Init(aResult);
    NS_POSTCONDITION(aResult.mType == mType
      || (NS_FAILED(rv) && aResult.IsNull()),
      "Post-condition of Init failed. nsSMILValue is invalid.");
    if (NS_FAILED(rv)) return rv;
  }

  return mType->Interpolate(*this, aEndVal, aUnitDistance, aResult);
}
