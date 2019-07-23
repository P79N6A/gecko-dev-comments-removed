





































#include "nsHTMLLinkAccessible.h"
#include "nsAccessibilityAtoms.h"
#include "nsIAccessibleEvent.h"
#include "nsINameSpaceManager.h"

NS_IMPL_ISUPPORTS_INHERITED0(nsHTMLLinkAccessible, nsLinkableAccessible)

nsHTMLLinkAccessible::nsHTMLLinkAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell, nsIFrame *aFrame):
nsLinkableAccessible(aDomNode, aShell)
{ 
}


NS_IMETHODIMP nsHTMLLinkAccessible::GetName(nsAString& aName)
{ 
  if (!mActionContent)
    return NS_ERROR_FAILURE;

  return AppendFlatStringFromSubtree(mActionContent, &aName);
}


NS_IMETHODIMP nsHTMLLinkAccessible::GetRole(PRUint32 *_retval)
{
  *_retval = nsIAccessibleRole::ROLE_LINK;

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLLinkAccessible::GetState(PRUint32 *aState, PRUint32 *aExtraState)
{
  nsresult rv = nsLinkableAccessible::GetState(aState, aExtraState);
  NS_ENSURE_SUCCESS(rv, rv);

  *aState  &= ~nsIAccessibleStates::STATE_READONLY;

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  if (content && content->HasAttr(kNameSpaceID_None,
                                  nsAccessibilityAtoms::name)) {
    
    
    
    *aState |= nsIAccessibleStates::STATE_SELECTABLE;
  }

  return NS_OK;
}
