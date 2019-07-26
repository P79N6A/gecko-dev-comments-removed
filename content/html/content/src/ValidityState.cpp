




#include "mozilla/dom/ValidityState.h"
#include "mozilla/dom/ValidityStateBinding.h"

#include "nsDOMClassInfoID.h"
#include "nsCycleCollectionParticipant.h"
#include "nsContentUtils.h"

DOMCI_DATA(ValidityState, mozilla::dom::ValidityState)

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(ValidityState)
NS_IMPL_CYCLE_COLLECTING_ADDREF(ValidityState)
NS_IMPL_CYCLE_COLLECTING_RELEASE(ValidityState)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(ValidityState)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsIDOMValidityState)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMValidityState)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(ValidityState)
NS_INTERFACE_MAP_END

ValidityState::ValidityState(nsIConstraintValidation* aConstraintValidation)
  : mConstraintValidation(aConstraintValidation)
{
  SetIsDOMBinding();
}

NS_IMETHODIMP
ValidityState::GetValueMissing(bool* aValueMissing)
{
  *aValueMissing = ValueMissing();
  return NS_OK;
}

NS_IMETHODIMP
ValidityState::GetTypeMismatch(bool* aTypeMismatch)
{
  *aTypeMismatch = TypeMismatch();
  return NS_OK;
}

NS_IMETHODIMP
ValidityState::GetPatternMismatch(bool* aPatternMismatch)
{
  *aPatternMismatch = PatternMismatch();
  return NS_OK;
}

NS_IMETHODIMP
ValidityState::GetTooLong(bool* aTooLong)
{
  *aTooLong = TooLong();
  return NS_OK;
}

NS_IMETHODIMP
ValidityState::GetRangeUnderflow(bool* aRangeUnderflow)
{
  *aRangeUnderflow = RangeUnderflow();
  return NS_OK;
}

NS_IMETHODIMP
ValidityState::GetRangeOverflow(bool* aRangeOverflow)
{
  *aRangeOverflow = RangeOverflow();
  return NS_OK;
}

NS_IMETHODIMP
ValidityState::GetStepMismatch(bool* aStepMismatch)
{
  *aStepMismatch = StepMismatch();
  return NS_OK;
}

NS_IMETHODIMP
ValidityState::GetCustomError(bool* aCustomError)
{
  *aCustomError = CustomError();
  return NS_OK;
}

NS_IMETHODIMP
ValidityState::GetValid(bool* aValid)
{
  *aValid = Valid();
  return NS_OK;
}

JSObject*
ValidityState::WrapObject(JSContext* aCx, JSObject* aScope)
{
  return ValidityStateBinding::Wrap(aCx, aScope, this);
}

} 
} 

