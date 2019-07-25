




































#include "nsConstraintValidation.h"

#include "nsAString.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLFormElement.h"
#include "nsDOMValidityState.h"


nsConstraintValidation::nsConstraintValidation()
  : mValidityBitField(0)
  , mValidity(nsnull)
{
}

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
nsConstraintValidation::GetValidationMessage(nsAString& aValidationMessage,
                                             nsGenericHTMLFormElement* aElement)
{
  aValidationMessage.Truncate();

  if (IsCandidateForConstraintValidation(aElement) && !IsValid()) {
    if (GetValidityState(VALIDITY_STATE_CUSTOM_ERROR)) {
      aValidationMessage.Assign(mCustomValidity);
    } else if (GetValidityState(VALIDITY_STATE_TOO_LONG)) {
      GetValidationMessage(aValidationMessage, VALIDITY_STATE_TOO_LONG);
    } else if (GetValidityState(VALIDITY_STATE_VALUE_MISSING)) {
      GetValidationMessage(aValidationMessage, VALIDITY_STATE_VALUE_MISSING);
    } else if (GetValidityState(VALIDITY_STATE_TYPE_MISMATCH)) {
      GetValidationMessage(aValidationMessage, VALIDITY_STATE_TYPE_MISMATCH);
    } else if (GetValidityState(VALIDITY_STATE_PATTERN_MISMATCH)) {
      GetValidationMessage(aValidationMessage, VALIDITY_STATE_PATTERN_MISMATCH);
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
nsConstraintValidation::SetCustomValidity(const nsAString& aError)
{
  mCustomValidity.Assign(aError);
  SetValidityState(VALIDITY_STATE_CUSTOM_ERROR, !mCustomValidity.IsEmpty());
  return NS_OK;
}

PRBool
nsConstraintValidation::IsCandidateForConstraintValidation(const nsGenericHTMLFormElement* const aElement) const
{
  







  
  
  if (aElement->CanBeDisabled() &&
      aElement->HasAttr(kNameSpaceID_None, nsGkAtoms::disabled)) {
    return PR_FALSE;
  }

  return !IsBarredFromConstraintValidation();
}

