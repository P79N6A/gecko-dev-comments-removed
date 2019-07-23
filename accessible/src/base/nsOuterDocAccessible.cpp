





































#include "nsOuterDocAccessible.h"
#include "nsIAccessibilityService.h"
#include "nsIAccessibleDocument.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsIServiceManager.h"
#include "nsIContent.h"

NS_IMPL_ISUPPORTS_INHERITED0(nsOuterDocAccessible, nsAccessible)

nsOuterDocAccessible::nsOuterDocAccessible(nsIDOMNode* aNode, 
                                           nsIWeakReference* aShell):
  nsAccessibleWrap(aNode, aShell)
{
}


nsresult
nsOuterDocAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_INTERNAL_FRAME;
  return NS_OK;
}

nsresult
nsOuterDocAccessible::GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState)
{
  nsresult rv = nsAccessible::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  *aState &= ~nsIAccessibleStates::STATE_FOCUSABLE;
  return NS_OK;
}


NS_IMETHODIMP
nsOuterDocAccessible::GetChildAtPoint(PRInt32 aX, PRInt32 aY,
                                      nsIAccessible **aAccessible)
{
  NS_ENSURE_ARG_POINTER(aAccessible);
  *aAccessible = nsnull;
  if (!mDOMNode) {
    return NS_ERROR_FAILURE;
  }
  PRInt32 docX, docY, docWidth, docHeight;
  GetBounds(&docX, &docY, &docWidth, &docHeight);
  if (aX < docX || aX >= docX + docWidth || aY < docY || aY >= docY + docHeight) {
    return NS_ERROR_FAILURE;
  }

  return GetFirstChild(aAccessible);  
}


NS_IMETHODIMP
nsOuterDocAccessible::GetDeepestChildAtPoint(PRInt32 aX, PRInt32 aY,
                                      nsIAccessible **aAccessible)
{
  
  
  nsCOMPtr<nsIAccessible> childAcc;
  nsresult rv = GetChildAtPoint(aX, aY, getter_AddRefs(childAcc));
  NS_ENSURE_SUCCESS(rv, rv);

  if (!childAcc)
    return NS_OK;

  return childAcc->GetDeepestChildAtPoint(aX, aY, aAccessible);
}

void nsOuterDocAccessible::CacheChildren()
{  
  
  
  if (!mWeakShell) {
    mAccChildCount = eChildCountUninitialized;
    return;   
  }
  if (mAccChildCount != eChildCountUninitialized) {
    return;
  }

  InvalidateChildren();
  mAccChildCount = 0;

  
  
  
  

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  NS_ASSERTION(content, "No nsIContent for <browser>/<iframe>/<editor> dom node");

  nsCOMPtr<nsIDocument> outerDoc = content->GetDocument();
  if (!outerDoc) {
    return;
  }

  nsIDocument *innerDoc = outerDoc->GetSubDocumentFor(content);
  nsCOMPtr<nsIDOMNode> innerNode(do_QueryInterface(innerDoc));
  if (!innerNode) {
    return;
  }

  nsCOMPtr<nsIAccessible> innerAccessible;
  nsCOMPtr<nsIAccessibilityService> accService = 
    do_GetService("@mozilla.org/accessibilityService;1");
  accService->GetAccessibleFor(innerNode, getter_AddRefs(innerAccessible));
  nsCOMPtr<nsPIAccessible> privateInnerAccessible = 
    do_QueryInterface(innerAccessible);
  if (!privateInnerAccessible) {
    return;
  }

  
  mAccChildCount = 1;
  SetFirstChild(innerAccessible); 
  privateInnerAccessible->SetParent(this);
  privateInnerAccessible->SetNextSibling(nsnull);
}

nsresult
nsOuterDocAccessible::GetAttributesInternal(nsIPersistentProperties *aAttributes)
{
  nsAutoString tag;
  aAttributes->GetStringProperty(NS_LITERAL_CSTRING("tag"), tag);
  if (!tag.IsEmpty()) {
    
    
    return NS_OK;
  }
  return nsAccessible::GetAttributesInternal(aAttributes);
}


NS_IMETHODIMP
nsOuterDocAccessible::GetNumActions(PRUint8 *aNumActions)
{
  NS_ENSURE_ARG_POINTER(aNumActions);
  *aNumActions = 0;

  return NS_OK;
}

NS_IMETHODIMP
nsOuterDocAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  aName.Truncate();

  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
nsOuterDocAccessible::GetActionDescription(PRUint8 aIndex, nsAString& aDescription)
{
  aDescription.Truncate();

  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
nsOuterDocAccessible::DoAction(PRUint8 aIndex)
{
  return NS_ERROR_INVALID_ARG;
}
