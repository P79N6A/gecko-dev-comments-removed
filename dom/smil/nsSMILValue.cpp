





#include "nsSMILValue.h"
#include "nsDebug.h"
#include <string.h>




nsSMILValue::nsSMILValue(const nsISMILType* aType)
  : mType(nsSMILNullType::Singleton())
{
  if (!aType) {
    NS_ERROR("Trying to construct nsSMILValue with null mType pointer");
    return;
  }

  InitAndCheckPostcondition(aType);
}

nsSMILValue::nsSMILValue(const nsSMILValue& aVal)
  : mType(nsSMILNullType::Singleton())
{
  InitAndCheckPostcondition(aVal.mType);
  mType->Assign(*this, aVal);
}

const nsSMILValue&
nsSMILValue::operator=(const nsSMILValue& aVal)
{
  if (&aVal == this)
    return *this;

  if (mType != aVal.mType) {
    DestroyAndReinit(aVal.mType);
  }

  mType->Assign(*this, aVal);

  return *this;
}


nsSMILValue::nsSMILValue(nsSMILValue&& aVal)
  : mU(aVal.mU), 
    mType(aVal.mType)
{
  
  
  aVal.mType = nsSMILNullType::Singleton();
}

nsSMILValue&
nsSMILValue::operator=(nsSMILValue&& aVal)
{
  if (!IsNull()) {
    
    DestroyAndCheckPostcondition();
  }

  
  mU = aVal.mU;
  mType = aVal.mType;

  
  
  aVal.mType = nsSMILNullType::Singleton();

  return *this;
}

bool
nsSMILValue::operator==(const nsSMILValue& aVal) const
{
  if (&aVal == this)
    return true;

  return mType == aVal.mType && mType->IsEqual(*this, aVal);
}

nsresult
nsSMILValue::Add(const nsSMILValue& aValueToAdd, uint32_t aCount)
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
    
    aResult.DestroyAndReinit(mType);
  }

  return mType->Interpolate(*this, aEndVal, aUnitDistance, aResult);
}





void
nsSMILValue::InitAndCheckPostcondition(const nsISMILType* aNewType)
{
  aNewType->Init(*this);
  MOZ_ASSERT(mType == aNewType,
             "Post-condition of Init failed. nsSMILValue is invalid");
}
                
void
nsSMILValue::DestroyAndCheckPostcondition()
{
  mType->Destroy(*this);
  MOZ_ASSERT(IsNull(),
             "Post-condition of Destroy failed. "
             "nsSMILValue not null after destroying");
}

void
nsSMILValue::DestroyAndReinit(const nsISMILType* aNewType)
{
  DestroyAndCheckPostcondition();
  InitAndCheckPostcondition(aNewType);
}
