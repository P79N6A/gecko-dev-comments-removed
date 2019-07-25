




































#include "nsDOMValidityState.h"

#include "nsDOMClassInfo.h"
#include "nsConstraintValidation.h"


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
  *aValueMissing = mConstraintValidation && mConstraintValidation->IsValueMissing();
  return NS_OK;
}

NS_IMETHODIMP
nsDOMValidityState::GetTypeMismatch(PRBool* aTypeMismatch)
{
  *aTypeMismatch = mConstraintValidation && mConstraintValidation->HasTypeMismatch();
  return NS_OK;
}

NS_IMETHODIMP
nsDOMValidityState::GetPatternMismatch(PRBool* aPatternMismatch)
{
  *aPatternMismatch = mConstraintValidation && mConstraintValidation->HasPatternMismatch();
  return NS_OK;
}

NS_IMETHODIMP
nsDOMValidityState::GetTooLong(PRBool* aTooLong)
{
  *aTooLong = mConstraintValidation && mConstraintValidation->IsTooLong();
  return NS_OK;
}

NS_IMETHODIMP
nsDOMValidityState::GetRangeUnderflow(PRBool* aRangeUnderflow)
{
  *aRangeUnderflow = mConstraintValidation && mConstraintValidation->HasRangeUnderflow();
  return NS_OK;
}

NS_IMETHODIMP
nsDOMValidityState::GetRangeOverflow(PRBool* aRangeOverflow)
{
  *aRangeOverflow = mConstraintValidation && mConstraintValidation->HasRangeOverflow();
  return NS_OK;
}

NS_IMETHODIMP
nsDOMValidityState::GetStepMismatch(PRBool* aStepMismatch)
{
  *aStepMismatch = mConstraintValidation && mConstraintValidation->HasStepMismatch();
  return NS_OK;
}

NS_IMETHODIMP
nsDOMValidityState::GetCustomError(PRBool* aCustomError)
{
  *aCustomError = mConstraintValidation && mConstraintValidation->HasCustomError();
  return NS_OK;
}

NS_IMETHODIMP
nsDOMValidityState::GetValid(PRBool* aValid)
{
  *aValid = !mConstraintValidation || mConstraintValidation->IsValid();
  return NS_OK;
}

