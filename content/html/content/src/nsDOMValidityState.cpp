




































#include "nsDOMValidityState.h"

#include "nsDOMClassInfo.h"


DOMCI_DATA(ValidityState, nsDOMValidityState)

NS_IMPL_ADDREF(nsDOMValidityState)
NS_IMPL_RELEASE(nsDOMValidityState)

NS_INTERFACE_MAP_BEGIN(nsDOMValidityState)
  NS_INTERFACE_MAP_ENTRY(nsIDOMValidityState)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMValidityState)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(ValidityState)
NS_INTERFACE_MAP_END

nsDOMValidityState::nsDOMValidityState(nsConstraintValidation* aConstraintValidation)
  : mConstraintValidation(aConstraintValidation)
{
}

NS_IMETHODIMP
nsDOMValidityState::GetValueMissing(PRBool* aValueMissing)
{
  *aValueMissing = GetValidityState(nsConstraintValidation::VALIDITY_STATE_VALUE_MISSING);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMValidityState::GetTypeMismatch(PRBool* aTypeMismatch)
{
  *aTypeMismatch = GetValidityState(nsConstraintValidation::VALIDITY_STATE_TYPE_MISMATCH);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMValidityState::GetPatternMismatch(PRBool* aPatternMismatch)
{
  *aPatternMismatch = GetValidityState(nsConstraintValidation::VALIDITY_STATE_PATTERN_MISMATCH);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMValidityState::GetTooLong(PRBool* aTooLong)
{
  *aTooLong = GetValidityState(nsConstraintValidation::VALIDITY_STATE_TOO_LONG);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMValidityState::GetRangeUnderflow(PRBool* aRangeUnderflow)
{
  *aRangeUnderflow = GetValidityState(nsConstraintValidation::VALIDITY_STATE_RANGE_UNDERFLOW);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMValidityState::GetRangeOverflow(PRBool* aRangeOverflow)
{
  *aRangeOverflow = GetValidityState(nsConstraintValidation::VALIDITY_STATE_RANGE_OVERFLOW);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMValidityState::GetStepMismatch(PRBool* aStepMismatch)
{
  *aStepMismatch = GetValidityState(nsConstraintValidation::VALIDITY_STATE_STEP_MISMATCH);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMValidityState::GetCustomError(PRBool* aCustomError)
{
  *aCustomError = GetValidityState(nsConstraintValidation::VALIDITY_STATE_CUSTOM_ERROR);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMValidityState::GetValid(PRBool* aValid)
{
  *aValid = !mConstraintValidation || mConstraintValidation->IsValid();
  return NS_OK;
}

