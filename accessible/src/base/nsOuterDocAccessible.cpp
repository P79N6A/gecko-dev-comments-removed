





































#include "nsOuterDocAccessible.h"

#include "States.h"
#include "nsAccUtils.h"
#include "nsDocAccessible.h"





nsOuterDocAccessible::
  nsOuterDocAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsAccessibleWrap(aContent, aShell)
{
}




NS_IMPL_ISUPPORTS_INHERITED0(nsOuterDocAccessible,
                             nsAccessible)




PRUint32
nsOuterDocAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_INTERNAL_FRAME;
}

PRUint64
nsOuterDocAccessible::NativeState()
{
  return nsAccessible::NativeState() & ~states::FOCUSABLE;
}

nsAccessible*
nsOuterDocAccessible::GetChildAtPoint(PRInt32 aX, PRInt32 aY,
                                      EWhichChildAtPoint aWhichChild)
{
  PRInt32 docX = 0, docY = 0, docWidth = 0, docHeight = 0;
  nsresult rv = GetBounds(&docX, &docY, &docWidth, &docHeight);
  NS_ENSURE_SUCCESS(rv, nsnull);

  if (aX < docX || aX >= docX + docWidth || aY < docY || aY >= docY + docHeight)
    return nsnull;

  
  
  nsAccessible* child = GetChildAt(0);
  NS_ENSURE_TRUE(child, nsnull);

  if (aWhichChild = eDeepestChild)
    return child->GetChildAtPoint(aX, aY, eDeepestChild);
  return child;
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
  
  
  
  
  NS_LOG_ACCDOCDESTROY_MSG("A11y outerdoc shutdown")
  NS_LOG_ACCDOCDESTROY_ACCADDRESS("outerdoc", this)

  nsAccessible *childAcc = mChildren.SafeElementAt(0, nsnull);
  if (childAcc) {
    NS_LOG_ACCDOCDESTROY("outerdoc's child document shutdown",
                         childAcc->GetDocumentNode())
    childAcc->Shutdown();
  }

  nsAccessibleWrap::Shutdown();
}




void
nsOuterDocAccessible::InvalidateChildren()
{
  
  
  
  
  
  
  
  

  SetChildrenFlag(eChildrenUninitialized);
}

PRBool
nsOuterDocAccessible::AppendChild(nsAccessible *aAccessible)
{
  
  
  
  
  
  if (mChildren.Length())
    mChildren[0]->Shutdown();

  if (!nsAccessible::AppendChild(aAccessible))
    return PR_FALSE;

  NS_LOG_ACCDOCCREATE("append document to outerdoc",
                      aAccessible->GetDocumentNode())
  NS_LOG_ACCDOCCREATE_ACCADDRESS("outerdoc", this)

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

  NS_LOG_ACCDOCDESTROY("remove document from outerdoc",
                       child->GetDocumentNode())
  NS_LOG_ACCDOCDESTROY_ACCADDRESS("outerdoc", this)

  PRBool wasRemoved = nsAccessible::RemoveChild(child);

  NS_ASSERTION(!mChildren.Length(),
               "This child document of outerdoc accessible wasn't removed!");

  return wasRemoved;
}





void
nsOuterDocAccessible::CacheChildren()
{
  
  
  nsIDocument* outerDoc = mContent->GetCurrentDoc();
  if (outerDoc) {
    nsIDocument* innerDoc = outerDoc->GetSubDocumentFor(mContent);
    if (innerDoc)
      GetAccService()->GetDocAccessible(innerDoc);
  }
}
