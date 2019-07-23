






































#include "nsHTMLTextAccessible.h"
#include "nsAccessibleTreeWalker.h"
#include "nsIAccessibleDocument.h"
#include "nsIAccessibleEvent.h"
#include "nsIFrame.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsISelection.h"
#include "nsISelectionController.h"
#include "nsComponentManagerUtils.h"





nsHTMLTextAccessible::nsHTMLTextAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell):
nsTextAccessibleWrap(aDomNode, aShell)
{ 
}

NS_IMPL_ISUPPORTS_INHERITED0(nsHTMLTextAccessible, nsTextAccessible)

NS_IMETHODIMP
nsHTMLTextAccessible::GetName(nsAString& aName)
{
  
  aName.Truncate();
  return AppendTextTo(aName, 0, PR_UINT32_MAX);
}

nsresult
nsHTMLTextAccessible::GetRoleInternal(PRUint32 *aRole)
{
  nsIFrame *frame = GetFrame();
  
  
  if (frame && frame->IsGeneratedContentFrame()) {
    *aRole = nsIAccessibleRole::ROLE_STATICTEXT;
    return NS_OK;
  }

  return nsTextAccessible::GetRoleInternal(aRole);
}

nsresult
nsHTMLTextAccessible::GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState)
{
  nsresult rv = nsTextAccessible::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  nsCOMPtr<nsIAccessible> docAccessible = 
    do_QueryInterface(nsCOMPtr<nsIAccessibleDocument>(GetDocAccessible()));
  if (docAccessible) {
     PRUint32 state, extState;
     docAccessible->GetState(&state, &extState);
     if (0 == (extState & nsIAccessibleStates::EXT_STATE_EDITABLE)) {
       *aState |= nsIAccessibleStates::STATE_READONLY; 
     }
  }

  return NS_OK;
}

nsresult
nsHTMLTextAccessible::GetAttributesInternal(nsIPersistentProperties *aAttributes)
{
  if (!mDOMNode) {
    return NS_ERROR_FAILURE;  
  }

  PRUint32 role;
  GetRoleInternal(&role);
  if (role == nsIAccessibleRole::ROLE_STATICTEXT) {
    nsAutoString oldValueUnused;
    aAttributes->SetStringProperty(NS_LITERAL_CSTRING("auto-generated"),
                                  NS_LITERAL_STRING("true"), oldValueUnused);
  }

  return NS_OK;
}





nsHTMLHRAccessible::nsHTMLHRAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell):
nsLeafAccessible(aDomNode, aShell)
{ 
}

nsresult
nsHTMLHRAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_SEPARATOR;
  return NS_OK;
}

nsHTMLBRAccessible::nsHTMLBRAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell):
nsLeafAccessible(aDomNode, aShell)
{ 
}

nsresult
nsHTMLBRAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_WHITESPACE;
  return NS_OK;
}

nsresult
nsHTMLBRAccessible::GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState)
{
  *aState = 0;

  if (IsDefunct()) {
    if (aExtraState)
      *aExtraState = nsIAccessibleStates::EXT_STATE_DEFUNCT;

    return NS_OK_DEFUNCT_OBJECT;
  }

  *aState = nsIAccessibleStates::STATE_READONLY;
  if (aExtraState)
    *aExtraState = 0;

  return NS_OK;
}

nsresult
nsHTMLBRAccessible::GetNameInternal(nsAString& aName)
{
  aName = static_cast<PRUnichar>('\n');    
  return NS_OK;
}





NS_IMPL_ISUPPORTS_INHERITED0(nsHTMLLabelAccessible, nsHyperTextAccessible)

nsHTMLLabelAccessible::
  nsHTMLLabelAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell) :
  nsHyperTextAccessibleWrap(aDomNode, aShell)
{
}

nsresult
nsHTMLLabelAccessible::GetNameInternal(nsAString& aName)
{
  return nsTextEquivUtils::GetNameFromSubtree(this, aName);
}

nsresult
nsHTMLLabelAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_LABEL;
  return NS_OK;
}





nsHTMLLIAccessible::
  nsHTMLLIAccessible(nsIDOMNode *aDOMNode, nsIWeakReference* aShell, 
                     const nsAString& aBulletText):
  nsHyperTextAccessibleWrap(aDOMNode, aShell)
{
  if (!aBulletText.IsEmpty()) {
    mBulletAccessible = new nsHTMLListBulletAccessible(mDOMNode, mWeakShell, 
                                                       aBulletText);
    if (mBulletAccessible)
      mBulletAccessible->Init();
  }
}

NS_IMPL_ISUPPORTS_INHERITED0(nsHTMLLIAccessible, nsHyperTextAccessible)

nsresult
nsHTMLLIAccessible::Shutdown()
{
  if (mBulletAccessible) {
    
    mBulletAccessible->Shutdown();
  }

  nsresult rv = nsHyperTextAccessibleWrap::Shutdown();
  mBulletAccessible = nsnull;
  return rv;
}

nsresult
nsHTMLLIAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_LISTITEM;
  return NS_OK;
}

nsresult
nsHTMLLIAccessible::GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState)
{
  nsresult rv = nsHyperTextAccessibleWrap::GetStateInternal(aState,
                                                            aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  *aState |= nsIAccessibleStates::STATE_READONLY;
  return NS_OK;
}

NS_IMETHODIMP nsHTMLLIAccessible::GetBounds(PRInt32 *x, PRInt32 *y, PRInt32 *width, PRInt32 *height)
{
  nsresult rv = nsAccessibleWrap::GetBounds(x, y, width, height);
  if (NS_FAILED(rv) || !mBulletAccessible) {
    return rv;
  }

  PRInt32 bulletX, bulletY, bulletWidth, bulletHeight;
  rv = mBulletAccessible->GetBounds(&bulletX, &bulletY, &bulletWidth, &bulletHeight);
  NS_ENSURE_SUCCESS(rv, rv);

  *x = bulletX; 
  *width += bulletWidth;
  return NS_OK;
}




void
nsHTMLLIAccessible::CacheChildren()
{
  if (mBulletAccessible) {
    mChildren.AppendObject(mBulletAccessible);
    mBulletAccessible->SetParent(this);
  }

  
  nsAccessibleWrap::CacheChildren();
}





nsHTMLListBulletAccessible::
  nsHTMLListBulletAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell,
                             const nsAString& aBulletText) :
    nsLeafAccessible(aDomNode, aShell), mBulletText(aBulletText)
{
  mBulletText += ' '; 
}

NS_IMETHODIMP
nsHTMLListBulletAccessible::GetUniqueID(void **aUniqueID)
{
  
  *aUniqueID = static_cast<void*>(this);
  return NS_OK;
}

nsresult
nsHTMLListBulletAccessible::Shutdown()
{
  mBulletText.Truncate();
  return nsLeafAccessible::Shutdown();
}

NS_IMETHODIMP
nsHTMLListBulletAccessible::GetName(nsAString &aName)
{
  
  aName = mBulletText;
  return NS_OK;
}

nsresult
nsHTMLListBulletAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_STATICTEXT;
  return NS_OK;
}

nsresult
nsHTMLListBulletAccessible::GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState)
{
  nsresult rv = nsLeafAccessible::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  *aState &= ~nsIAccessibleStates::STATE_FOCUSABLE;
  *aState |= nsIAccessibleStates::STATE_READONLY;
  return NS_OK;
}

nsresult
nsHTMLListBulletAccessible::AppendTextTo(nsAString& aText, PRUint32 aStartOffset,
                                         PRUint32 aLength)
{
  PRUint32 maxLength = mBulletText.Length() - aStartOffset;
  if (aLength > maxLength) {
    aLength = maxLength;
  }
  aText += nsDependentSubstring(mBulletText, aStartOffset, aLength);
  return NS_OK;
}

nsIAccessible*
nsHTMLListBulletAccessible::GetParent()
{
  return mParent;
}





NS_IMPL_ISUPPORTS_INHERITED0(nsHTMLListAccessible, nsHyperTextAccessible)

nsresult
nsHTMLListAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_LIST;
  return NS_OK;
}

nsresult
nsHTMLListAccessible::GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState)
{
  nsresult rv = nsHyperTextAccessibleWrap::GetStateInternal(aState,
                                                            aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  *aState |= nsIAccessibleStates::STATE_READONLY;
  return NS_OK;
}

