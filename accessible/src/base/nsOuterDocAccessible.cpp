





































#include "nsOuterDocAccessible.h"

#include "nsAccUtils.h"
#include "nsDocAccessible.h"





nsOuterDocAccessible::
  nsOuterDocAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsAccessibleWrap(aContent, aShell)
{
}




NS_IMPL_ISUPPORTS_INHERITED0(nsOuterDocAccessible,
                             nsAccessible)




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




void
nsOuterDocAccessible::Shutdown()
{
  nsAccessible::InvalidateChildren();

  nsAccessibleWrap::Shutdown();
}




void
nsOuterDocAccessible::InvalidateChildren()
{
  
  
  

  mAreChildrenInitialized = PR_FALSE;
}

PRBool
nsOuterDocAccessible::AppendChild(nsAccessible *aAccessible)
{
  NS_ASSERTION(!mChildren.Length(),
               "Previous child document of outerdoc accessible wasn't removed!");

  if (!mChildren.AppendElement(aAccessible))
    return PR_FALSE;

  aAccessible->SetParent(this);
  return PR_TRUE;
}

PRBool
nsOuterDocAccessible::RemoveChild(nsAccessible *aAccessible)
{
  nsAccessible *child = mChildren.SafeElementAt(0, nsnull);
  if (child != aAccessible) {
    NS_ERROR("Wrong child to remove!");
    return PR_FALSE;
  }

  mChildren.RemoveElement(child);
  NS_ASSERTION(!mChildren.Length(),
               "This child document of outerdoc accessible wasn't removed!");

  return PR_TRUE;
}




void
nsOuterDocAccessible::CacheChildren()
{
  
  
  nsIDocument *outerDoc = mContent->GetCurrentDoc();
  if (!outerDoc)
    return;

  nsIDocument *innerDoc = outerDoc->GetSubDocumentFor(mContent);
  if (!innerDoc)
    return;

  nsDocAccessible *docAcc = GetAccService()->GetDocAccessible(innerDoc);
  NS_ASSERTION(docAcc && docAcc->GetParent() == this,
               "Document accessible isn't a child of outerdoc accessible!");
}
