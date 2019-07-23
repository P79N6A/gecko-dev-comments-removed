





































#include "nsSMILValue.h"
#include "nsDebug.h"




nsSMILValue::nsSMILValue(const nsISMILType* aType)
  : mType(&nsSMILNullType::sSingleton)
{
  if (!aType) {
    NS_ERROR("Trying to construct nsSMILValue with null mType pointer");
    return;
  }

  InitAndCheckPostcondition(aType);
}

nsSMILValue::nsSMILValue(const nsSMILValue& aVal)
  : mType(&nsSMILNullType::sSingleton)
{
  nsresult rv = InitAndCheckPostcondition(aVal.mType);
  if (NS_FAILED(rv))
    return;

  mType->Assign(*this, aVal);
}

const nsSMILValue&
nsSMILValue::operator=(const nsSMILValue& aVal)
{
  if (&aVal == this)
    return *this;

  if (mType != aVal.mType) {
    nsresult rv = DestroyAndReinit(aVal.mType);
    if (NS_FAILED(rv))
      return *this; 
  }

  mType->Assign(*this, aVal);

  return *this;
}

PRBool
nsSMILValue::operator==(const nsSMILValue& aVal) const
{
  if (&aVal == this)
    return PR_TRUE;

  return mType == aVal.mType && mType->IsEqual(*this, aVal);
}

nsresult
nsSMILValue::Add(const nsSMILValue& aValueToAdd, PRUint32 aCount)
{
  if (aValueToAdd.mType != mType) {
    NS_ERROR("Trying to add incompatible types");
    return NS_ERROR_FAILURE;
  }

  return mType->Add(*this, aValueToAdd, aCount);
}

nsresult
nsSMILValue::SandwichAdd(const nsSMILValue& aValueToAdd)
{
  if (aValueToAdd.mType != mType) {
    NS_ERROR("Trying to add incompatible types");
    return NS_ERROR_FAILURE;
  }

  return mType->SandwichAdd(*this, aValueToAdd);
}

nsresult
nsSMILValue::ComputeDistance(const nsSMILValue& aTo, double& aDistance) const
{
  if (aTo.mType != mType) {
    NS_ERROR("Trying to calculate distance between incompatible types");
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
    NS_ERROR("Trying to interpolate between incompatible types");
    return NS_ERROR_FAILURE;
  }

  if (aResult.mType != mType) {
    
    nsresult rv = aResult.DestroyAndReinit(mType);
    if (NS_FAILED(rv))
      return rv;
  }

  return mType->Interpolate(*this, aEndVal, aUnitDistance, aResult);
}





nsresult
nsSMILValue::InitAndCheckPostcondition(const nsISMILType* aNewType)
{
  nsresult rv = aNewType->Init(*this);
  NS_ABORT_IF_FALSE(mType == aNewType || (NS_FAILED(rv) && IsNull()),
                    "Post-condition of Init failed. nsSMILValue is invalid");
  return rv;
}
                
void
nsSMILValue::DestroyAndCheckPostcondition()
{
  mType->Destroy(*this);
  NS_ABORT_IF_FALSE(IsNull(), "Post-condition of Destroy failed. "
                    "nsSMILValue not null after destroying");
}

nsresult
nsSMILValue::DestroyAndReinit(const nsISMILType* aNewType)
{
  DestroyAndCheckPostcondition();
  return InitAndCheckPostcondition(aNewType);
}
