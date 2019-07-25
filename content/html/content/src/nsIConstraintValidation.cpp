




































#include "nsIConstraintValidation.h"

#include "nsAString.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLFormElement.h"
#include "nsDOMValidityState.h"


nsIConstraintValidation::nsIConstraintValidation()
  : mValidityBitField(0)
  , mValidity(nsnull)
{
}

nsIConstraintValidation::~nsIConstraintValidation()
{
  if (mValidity) {
    mValidity->Disconnect();
  }
}

nsresult
nsIConstraintValidation::GetValidity(nsIDOMValidityState** aValidity)
{
  if (!mValidity) {
    mValidity = new nsDOMValidityState(this);
  }

  NS_ADDREF(*aValidity = mValidity);

  return NS_OK;
}

nsresult
nsIConstraintValidation::GetValidationMessage(nsAString& aValidationMessage)
{
  aValidationMessage.Truncate();

  if (IsCandidateForConstraintValidation() && !IsValid()) {
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
nsIConstraintValidation::CheckValidity(PRBool* aValidity)
{
  if (!IsCandidateForConstraintValidation() || IsValid()) {
    *aValidity = PR_TRUE;
    return NS_OK;
  }

  *aValidity = PR_FALSE;

  nsCOMPtr<nsIContent> content = do_QueryInterface(this);
  NS_ASSERTION(content, "This class should be inherited by HTML elements only!");

  return nsContentUtils::DispatchTrustedEvent(content->GetOwnerDoc(), content,
                                              NS_LITERAL_STRING("invalid"),
                                              PR_FALSE, PR_TRUE);
}

void
nsIConstraintValidation::SetCustomValidity(const nsAString& aError)
{
  mCustomValidity.Assign(aError);
  SetValidityState(VALIDITY_STATE_CUSTOM_ERROR, !mCustomValidity.IsEmpty());
}

PRBool
nsIConstraintValidation::IsCandidateForConstraintValidation() const
{
  







  nsCOMPtr<nsIContent> content =
    do_QueryInterface(const_cast<nsIConstraintValidation*>(this));
  NS_ASSERTION(content, "This class should be inherited by HTML elements only!");

  
  
  
  
  
  
  return !content->HasAttr(kNameSpaceID_None, nsGkAtoms::disabled) &&
         !IsBarredFromConstraintValidation();
}

