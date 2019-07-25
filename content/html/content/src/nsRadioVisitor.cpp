




































#include "nsRadioVisitor.h"
#include "nsAutoPtr.h"
#include "nsHTMLInputElement.h"
#include "nsEventStates.h"
#include "nsIDocument.h"
#include "nsIConstraintValidation.h"


NS_IMPL_ISUPPORTS1(nsRadioVisitor, nsIRadioVisitor)

PRBool
nsRadioSetCheckedChangedVisitor::Visit(nsIFormControl* aRadio)
{
  nsRefPtr<nsHTMLInputElement> radio =
    static_cast<nsHTMLInputElement*>(aRadio);
  NS_ASSERTION(radio, "Visit() passed a null button!");

  radio->SetCheckedChangedInternal(mCheckedChanged);
  return PR_TRUE;
}

PRBool
nsRadioGetCheckedChangedVisitor::Visit(nsIFormControl* aRadio)
{
  if (aRadio == mExcludeElement) {
    return PR_TRUE;
  }

  nsRefPtr<nsHTMLInputElement> radio =
    static_cast<nsHTMLInputElement*>(aRadio);
  NS_ASSERTION(radio, "Visit() passed a null button!");

  *mCheckedChanged = radio->GetCheckedChanged();
  return PR_FALSE;
}

PRBool
nsRadioSetValueMissingState::Visit(nsIFormControl* aRadio)
{
  if (aRadio == mExcludeElement) {
    return PR_TRUE;
  }

  nsHTMLInputElement* input = static_cast<nsHTMLInputElement*>(aRadio);

  input->SetValidityState(nsIConstraintValidation::VALIDITY_STATE_VALUE_MISSING,
                          mValidity);

  nsIDocument* doc = input->GetCurrentDoc();
  if (mNotify && doc) {
    doc->ContentStateChanged(input, NS_EVENT_STATE_VALID |
                                    NS_EVENT_STATE_INVALID |
                                    NS_EVENT_STATE_MOZ_UI_VALID |
                                    NS_EVENT_STATE_MOZ_UI_INVALID);
  }

  return PR_TRUE;
}

