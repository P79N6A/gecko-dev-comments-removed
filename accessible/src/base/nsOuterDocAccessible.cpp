





































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

  
NS_IMETHODIMP nsOuterDocAccessible::GetName(nsAString& aName) 
{ 
  nsCOMPtr<nsIAccessibleDocument> accDoc(do_QueryInterface(mFirstChild));
  if (!accDoc) {
    return NS_ERROR_FAILURE;
  }
  nsresult rv = accDoc->GetTitle(aName);
  if (NS_FAILED(rv) || aName.IsEmpty()) {
    rv = nsAccessible::GetName(aName);
    if (aName.IsEmpty()) {
      rv = accDoc->GetURL(aName);
    }
  }
  return rv;
}


NS_IMETHODIMP nsOuterDocAccessible::GetRole(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_CLIENT;
  return NS_OK;
}

NS_IMETHODIMP nsOuterDocAccessible::GetState(PRUint32 *aState)
{
  nsAccessible::GetState(aState);
  *aState &= ~nsIAccessibleStates::STATE_FOCUSABLE;
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

  SetFirstChild(nsnull);

  
  
  
  

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  NS_ASSERTION(content, "No nsIContent for <browser>/<iframe>/<editor> dom node");

  nsCOMPtr<nsIDocument> outerDoc = content->GetDocument();
  if (!outerDoc) {
    mAccChildCount = 0;
    return;
  }

  nsIDocument *innerDoc = outerDoc->GetSubDocumentFor(content);
  nsCOMPtr<nsIDOMNode> innerNode(do_QueryInterface(innerDoc));
  if (!innerNode) {
    mAccChildCount = 0;
    return;
  }

  nsCOMPtr<nsIAccessible> innerAccessible;
  nsCOMPtr<nsIAccessibilityService> accService = 
    do_GetService("@mozilla.org/accessibilityService;1");
  accService->GetAccessibleFor(innerNode, getter_AddRefs(innerAccessible));
  nsCOMPtr<nsPIAccessible> privateInnerAccessible = 
    do_QueryInterface(innerAccessible);
  if (!privateInnerAccessible) {
    mAccChildCount = 0;
    return;
  }

  
  mAccChildCount = 1;
  SetFirstChild(innerAccessible); 
  privateInnerAccessible->SetParent(this);
  privateInnerAccessible->SetNextSibling(nsnull);
}

