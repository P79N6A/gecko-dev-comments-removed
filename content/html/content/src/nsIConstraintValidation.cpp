




































#include "nsIConstraintValidation.h"

#include "nsAString.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLFormElement.h"
#include "nsDOMValidityState.h"
#include "nsIFormControl.h"
#include "nsHTMLFormElement.h"
#include "nsContentUtils.h"

const PRUint16 nsIConstraintValidation::sContentSpecifiedMaxLengthMessage = 256;

nsIConstraintValidation::nsIConstraintValidation()
  : mValidityBitField(0)
  , mValidity(nsnull)
  
  , mBarredFromConstraintValidation(false)
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

NS_IMETHODIMP
nsIConstraintValidation::GetValidationMessage(nsAString& aValidationMessage)
{
  aValidationMessage.Truncate();

  if (IsCandidateForConstraintValidation() && !IsValid()) {
    nsCOMPtr<nsIContent> content = do_QueryInterface(this);
    NS_ASSERTION(content, "This class should be inherited by HTML elements only!");

    nsAutoString authorMessage;
    content->GetAttr(kNameSpaceID_None, nsGkAtoms::x_moz_errormessage,
                     authorMessage);

    if (!authorMessage.IsEmpty()) {
      aValidationMessage.Assign(authorMessage);
      if (aValidationMessage.Length() > sContentSpecifiedMaxLengthMessage) {
        aValidationMessage.Truncate(sContentSpecifiedMaxLengthMessage);
      }
    } else if (GetValidityState(VALIDITY_STATE_CUSTOM_ERROR)) {
      aValidationMessage.Assign(mCustomValidity);
      if (aValidationMessage.Length() > sContentSpecifiedMaxLengthMessage) {
        aValidationMessage.Truncate(sContentSpecifiedMaxLengthMessage);
      }
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
nsIConstraintValidation::CheckValidity(bool* aValidity)
{
  if (!IsCandidateForConstraintValidation() || IsValid()) {
    *aValidity = true;
    return NS_OK;
  }

  *aValidity = false;

  nsCOMPtr<nsIContent> content = do_QueryInterface(this);
  NS_ASSERTION(content, "This class should be inherited by HTML elements only!");

  return nsContentUtils::DispatchTrustedEvent(content->GetOwnerDoc(), content,
                                              NS_LITERAL_STRING("invalid"),
                                              false, true);
}

void
nsIConstraintValidation::SetValidityState(ValidityStateType aState,
                                          bool aValue)
{
  bool previousValidity = IsValid();

  if (aValue) {
    mValidityBitField |= aState;
  } else {
    mValidityBitField &= ~aState;
  }

  
  if (previousValidity != IsValid() && IsCandidateForConstraintValidation()) {
    nsCOMPtr<nsIFormControl> formCtrl = do_QueryInterface(this);
    NS_ASSERTION(formCtrl, "This interface should be used by form elements!");

    nsHTMLFormElement* form =
      static_cast<nsHTMLFormElement*>(formCtrl->GetFormElement());
    if (form) {
      form->UpdateValidity(IsValid());
    }
  }
}

void
nsIConstraintValidation::SetCustomValidity(const nsAString& aError)
{
  mCustomValidity.Assign(aError);
  SetValidityState(VALIDITY_STATE_CUSTOM_ERROR, !mCustomValidity.IsEmpty());
}

void
nsIConstraintValidation::SetBarredFromConstraintValidation(bool aBarred)
{
  bool previousBarred = mBarredFromConstraintValidation;

  mBarredFromConstraintValidation = aBarred;

  
  
  if (!IsValid() && previousBarred != mBarredFromConstraintValidation) {
    nsCOMPtr<nsIFormControl> formCtrl = do_QueryInterface(this);
    NS_ASSERTION(formCtrl, "This interface should be used by form elements!");

    nsHTMLFormElement* form =
      static_cast<nsHTMLFormElement*>(formCtrl->GetFormElement());
    if (form) {
      
      
      
      form->UpdateValidity(aBarred);
    }
  }
}

