




































#include "nsXULAlertAccessible.h"




NS_IMPL_ISUPPORTS_INHERITED0(nsXULAlertAccessible, nsAccessible)

nsXULAlertAccessible::nsXULAlertAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell) :
  nsAccessibleWrap(aNode, aShell)
{
}

NS_IMETHODIMP nsXULAlertAccessible::GetRole(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_ALERT;
  return NS_OK;
}

NS_IMETHODIMP nsXULAlertAccessible::GetState(PRUint32 *aState)
{
  nsAccessible::GetState(aState);
  *aState &= ~nsIAccessibleStates::STATE_FOCUSABLE;
  *aState |= nsIAccessibleStates::STATE_ALERT_MEDIUM; 
  return NS_OK;
}

#if 0


NS_IMETHODIMP nsXULAlertAccessible::GetName(nsAString &aName)
{
  nsCOMPtr<nsIPresShell> presShell(do_QueryReferent(mWeakShell));
  if (!presShell) {
    return NS_ERROR_FAILURE; 
  }
  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  NS_ASSERTION(content, "Should not be null if we still have a presShell");

  nsCOMPtr<nsIDOMNodeList> siblingList;
  
  presShell->GetDocument()->GetXBLChildNodesFor(content,
                                                getter_AddRefs(siblingList));
  if (siblingList) {
    PRUint32 length, count;
    siblingList->GetLength(&length);
    for (count = 0; count < length; count ++) {
      nsCOMPtr<nsIDOMNode> domNode;
      siblingList->Item(count, getter_AddRefs(domNode));
      nsCOMPtr<nsIDOMXULLabeledControlElement> labeledEl(do_QueryInterface(domNode));
      if (labeledEl) {
        nsAutoString label;
        labeledEl->GetLabel(label);
        aName += NS_LITERAL_STRING(" ") + label + NS_LITERAL_STRING(" ");
      }
      else {
        nsCOMPtr<nsIContent> content(do_QueryInterface(domNode));
        if (content) {
          AppendFlatStringFromSubtree(content, &aName);
        }
      }
    }
  }
  else {
    AppendFlatStringFromSubtree(content, &aName);
  }

  return NS_OK;
}
#endif
