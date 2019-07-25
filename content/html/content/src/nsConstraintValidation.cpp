




































#include "nsConstraintValidation.h"

#include "nsAString.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLFormElement.h"

nsConstraintValidation::~nsConstraintValidation()
{
  if (mValidity) {
    mValidity->Disconnect();
  }
}

nsresult
nsConstraintValidation::GetValidity(nsIDOMValidityState** aValidity)
{
  if (!mValidity) {
    mValidity = new nsDOMValidityState(this);
  }

  NS_ADDREF(*aValidity = mValidity);

  return NS_OK;
}

nsresult
nsConstraintValidation::GetWillValidate(PRBool* aWillValidate,
                                        nsGenericHTMLFormElement* aElement)
{
  *aWillValidate = IsCandidateForConstraintValidation(aElement);
  return NS_OK;
}

nsresult
nsConstraintValidation::GetValidationMessage(nsAString & aValidationMessage,
                                             nsGenericHTMLFormElement* aElement)
{
  aValidationMessage.Truncate();

  if (IsCandidateForConstraintValidation(aElement) && !IsValid()) {
    if (!mCustomValidity.IsEmpty()) {
      aValidationMessage.Assign(mCustomValidity);
    } else if (IsTooLong()) {
      GetValidationMessage(aValidationMessage, VALIDATION_MESSAGE_TOO_LONG);
    } else if (IsValueMissing()) {
      GetValidationMessage(aValidationMessage, VALIDATION_MESSAGE_VALUE_MISSING);
    } else if (HasTypeMismatch()) {
      GetValidationMessage(aValidationMessage, VALIDATION_MESSAGE_TYPE_MISMATCH);
    } else {
      
      
      
      return NS_ERROR_UNEXPECTED;
    }
  } else {
    aValidationMessage.Truncate();
  }

  return NS_OK;
}

nsresult
nsConstraintValidation::CheckValidity(PRBool* aValidity,
                                      nsGenericHTMLFormElement* aElement)
{
  if (!IsCandidateForConstraintValidation(aElement) || IsValid()) {
    *aValidity = PR_TRUE;
    return NS_OK;
  }

  *aValidity = PR_FALSE;

  return nsContentUtils::DispatchTrustedEvent(aElement->GetOwnerDoc(),
                                              static_cast<nsIContent*>(aElement),
                                              NS_LITERAL_STRING("invalid"),
                                              PR_FALSE, PR_TRUE);
}

nsresult
nsConstraintValidation::SetCustomValidity(const nsAString & aError)
{
  mCustomValidity.Assign(aError);
  return NS_OK;
}

PRBool
nsConstraintValidation::HasCustomError() const
{
  return !mCustomValidity.IsEmpty();
}

PRBool
nsConstraintValidation::IsValid()
{
  return !(IsValueMissing() || HasTypeMismatch() || HasPatternMismatch() ||
           IsTooLong() || HasRangeUnderflow() || HasRangeOverflow() ||
           HasStepMismatch() || HasCustomError());
}

PRBool
nsConstraintValidation::IsCandidateForConstraintValidation(nsGenericHTMLFormElement* aElement)
{
  







  
  
  if (aElement->CanBeDisabled() &&
      aElement->HasAttr(kNameSpaceID_None, nsGkAtoms::disabled)) {
    return PR_FALSE;
  }

  return !IsBarredFromConstraintValidation();
}

