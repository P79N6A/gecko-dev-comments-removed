





































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


nsresult
nsOuterDocAccessible::GetChildAtPoint(PRInt32 aX, PRInt32 aY,
                                      PRBool aDeepestChild,
                                      nsIAccessible **aChild)
{
  PRInt32 docX = 0, docY = 0, docWidth = 0, docHeight = 0;
  nsresult rv = GetBounds(&docX, &docY, &docWidth, &docHeight);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aX < docX || aX >= docX + docWidth || aY < docY || aY >= docY + docHeight)
    return NS_OK;

  
  
  nsCOMPtr<nsIAccessible> childAcc;
  rv = GetFirstChild(getter_AddRefs(childAcc));
  NS_ENSURE_SUCCESS(rv, rv);

  if (!childAcc)
    return NS_OK;

  if (aDeepestChild)
    return childAcc->GetDeepestChildAtPoint(aX, aY, aChild);

  NS_ADDREF(*aChild = childAcc);
  return NS_OK;
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
  nsRefPtr<nsAccessible> innerAcc(nsAccUtils::QueryAccessible(innerAccessible));
  if (!innerAcc)
    return;

  
  mAccChildCount = 1;
  SetFirstChild(innerAccessible); 
  innerAcc->SetParent(this);
  innerAcc->SetNextSibling(nsnull);
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
