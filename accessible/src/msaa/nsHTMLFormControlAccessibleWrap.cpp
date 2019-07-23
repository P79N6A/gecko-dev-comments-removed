




































#include "nsHTMLFormControlAccessibleWrap.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIRadioControlElement.h"
#include "nsIRadioGroupContainer.H"
#include "nsTextFormatter.h"





nsHTMLRadioButtonAccessibleWrap::nsHTMLRadioButtonAccessibleWrap(nsIDOMNode *aDOMNode, 
                                                         nsIWeakReference *aShell):
nsHTMLRadioButtonAccessible(aDOMNode, aShell)
{
}

NS_IMETHODIMP nsHTMLRadioButtonAccessibleWrap::GetDescription(nsAString& aDescription)
{
  aDescription.Truncate();

  nsCOMPtr<nsIRadioControlElement> radio(do_QueryInterface(mDOMNode));
  if (!radio) {
    return NS_ERROR_FAILURE;
  }

  NS_ASSERTION(radio, "We should only have HTML radio buttons here");
  nsCOMPtr<nsIRadioGroupContainer> radioGroupContainer =
    radio->GetRadioGroupContainer();

  if (radioGroupContainer) {
    nsCOMPtr<nsIDOMHTMLInputElement> input(do_QueryInterface(mDOMNode));
    PRInt32 radioIndex, radioItemsInGroup;
    if (NS_SUCCEEDED(radioGroupContainer->GetPositionInGroup(input, &radioIndex, 
                                                             &radioItemsInGroup))) {
      
      
      nsTextFormatter::ssprintf(aDescription, NS_LITERAL_STRING("%d of %d").get(),
                                radioIndex + 1, radioItemsInGroup);
      return NS_OK;
    }
  }
  return nsAccessibleWrap::GetDescription(aDescription);
}
