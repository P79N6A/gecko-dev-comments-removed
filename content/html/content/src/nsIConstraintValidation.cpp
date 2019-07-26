




#include "nsIConstraintValidation.h"

#include "nsAString.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLFormElement.h"
#include "mozilla/dom/ValidityState.h"
#include "nsIFormControl.h"
#include "nsContentUtils.h"

const uint16_t nsIConstraintValidation::sContentSpecifiedMaxLengthMessage = 256;

nsIConstraintValidation::nsIConstraintValidation()
  : mValidityBitField(0)
  
  , mBarredFromConstraintValidation(false)
{
}

nsIConstraintValidation::~nsIConstraintValidation()
{
  if (mValidity) {
    mValidity->Disconnect();
  }
}

mozilla::dom::ValidityState*
nsIConstraintValidation::Validity()
{
  if (!mValidity) {
    mValidity = new mozilla::dom::ValidityState(this);
  }

  return mValidity;
}

nsresult
nsIConstraintValidation::GetValidity(nsIDOMValidityState** aValidity)
{
  NS_ENSURE_ARG_POINTER(aValidity);

  NS_ADDREF(*aValidity = Validity());

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
    } else if (GetValidityState(VALIDITY_STATE_RANGE_OVERFLOW)) {
      GetValidationMessage(aValidationMessage, VALIDITY_STATE_RANGE_OVERFLOW);
    } else if (GetValidityState(VALIDITY_STATE_RANGE_UNDERFLOW)) {
      GetValidationMessage(aValidationMessage, VALIDITY_STATE_RANGE_UNDERFLOW);
    } else if (GetValidityState(VALIDITY_STATE_STEP_MISMATCH)) {
      GetValidationMessage(aValidationMessage, VALIDITY_STATE_STEP_MISMATCH);
    } else {
      
      return NS_ERROR_UNEXPECTED;
    }
  } else {
    aValidationMessage.Truncate();
  }

  return NS_OK;
}

bool
nsIConstraintValidation::CheckValidity()
{
  if (!IsCandidateForConstraintValidation() || IsValid()) {
    return true;
  }

  nsCOMPtr<nsIContent> content = do_QueryInterface(this);
  NS_ASSERTION(content, "This class should be inherited by HTML elements only!");

  nsContentUtils::DispatchTrustedEvent(content->OwnerDoc(), content,
                                       NS_LITERAL_STRING("invalid"),
                                       false, true);
  return false;
}

nsresult
nsIConstraintValidation::CheckValidity(bool* aValidity)
{
  NS_ENSURE_ARG_POINTER(aValidity);

  *aValidity = CheckValidity();

  return NS_OK;
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

