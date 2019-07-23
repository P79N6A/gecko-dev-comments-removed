






































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

NS_IMETHODIMP
nsHTMLTextAccessible::GetName(nsAString& aName)
{
  
  aName.Truncate();
  return AppendTextTo(aName, 0, PR_UINT32_MAX);
}

NS_IMETHODIMP nsHTMLTextAccessible::GetRole(PRUint32 *aRole)
{
  nsIFrame *frame = GetFrame();
  
  
  if (frame && frame->IsGeneratedContentFrame()) {
    *aRole = nsIAccessibleRole::ROLE_STATICTEXT;
    return NS_OK;
  }

  return nsTextAccessible::GetRole(aRole);
}

NS_IMETHODIMP
nsHTMLTextAccessible::GetState(PRUint32 *aState, PRUint32 *aExtraState)
{
  nsresult rv = nsTextAccessible::GetState(aState, aExtraState);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!mDOMNode)
    return NS_OK;

  nsCOMPtr<nsIAccessible> docAccessible = 
    do_QueryInterface(nsCOMPtr<nsIAccessibleDocument>(GetDocAccessible()));
  if (docAccessible) {
     PRUint32 state, extState;
     docAccessible->GetFinalState(&state, &extState);
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
  GetRole(&role);
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

NS_IMETHODIMP nsHTMLHRAccessible::GetRole(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_SEPARATOR;
  return NS_OK;
}

nsHTMLBRAccessible::nsHTMLBRAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell):
nsLeafAccessible(aDomNode, aShell)
{ 
}

NS_IMETHODIMP nsHTMLBRAccessible::GetRole(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_WHITESPACE;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLBRAccessible::GetState(PRUint32 *aState, PRUint32 *aExtraState)
{
  *aState = nsIAccessibleStates::STATE_READONLY;
  if (aExtraState) {
    *aExtraState = mDOMNode ? 0 : nsIAccessibleStates::EXT_STATE_DEFUNCT;
  }
  return NS_OK;
}

nsresult
nsHTMLBRAccessible::GetNameInternal(nsAString& aName)
{
  aName = static_cast<PRUnichar>('\n');    
  return NS_OK;
}



NS_IMPL_ISUPPORTS_INHERITED0(nsHTMLLabelAccessible, nsLinkableAccessible)

nsHTMLLabelAccessible::nsHTMLLabelAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell):
nsTextAccessible(aDomNode, aShell)
{ 
}

nsresult
nsHTMLLabelAccessible::GetNameInternal(nsAString& aReturn)
{ 
  nsresult rv = NS_ERROR_FAILURE;
  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));

  nsAutoString name;
  if (content)
    rv = AppendFlatStringFromSubtree(content, &name);

  if (NS_SUCCEEDED(rv)) {
    
    name.CompressWhitespace();
    aReturn = name;
  }

  return rv;
}

NS_IMETHODIMP nsHTMLLabelAccessible::GetRole(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_LABEL;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLLabelAccessible::GetState(PRUint32 *aState, PRUint32 *aExtraState)
{
  nsresult rv = nsTextAccessible::GetState(aState, aExtraState);
  NS_ENSURE_SUCCESS(rv, rv);
  if (mDOMNode) {
    *aState &= (nsIAccessibleStates::STATE_LINKED |
                nsIAccessibleStates::STATE_TRAVERSED); 
  }
  return NS_OK;
}

NS_IMETHODIMP nsHTMLLabelAccessible::GetFirstChild(nsIAccessible **aFirstChild) 
{  
  
  return nsAccessible::GetFirstChild(aFirstChild);
}

  
NS_IMETHODIMP nsHTMLLabelAccessible::GetLastChild(nsIAccessible **aLastChild)
{  
  
  return nsAccessible::GetLastChild(aLastChild);
}


NS_IMETHODIMP nsHTMLLabelAccessible::GetChildCount(PRInt32 *aAccChildCount) 
{
  
  return nsAccessible::GetChildCount(aAccChildCount);
}

nsHTMLLIAccessible::nsHTMLLIAccessible(nsIDOMNode *aDOMNode, nsIWeakReference* aShell, 
                                       const nsAString& aBulletText):
  nsLinkableAccessible(aDOMNode, aShell)
{
  if (!aBulletText.IsEmpty()) {
    mBulletAccessible = new nsHTMLListBulletAccessible(mDOMNode, mWeakShell, 
                                                       aBulletText);
    if (mBulletAccessible)
      mBulletAccessible->Init();
  }
}

nsresult
nsHTMLLIAccessible::Shutdown()
{
  if (mBulletAccessible) {
    
    mBulletAccessible->Shutdown();
  }
  nsresult rv = nsLinkableAccessible::Shutdown();
  mBulletAccessible = nsnull;
  return rv;
}

NS_IMETHODIMP
nsHTMLLIAccessible::GetState(PRUint32 *aState, PRUint32 *aExtraState)
{
  nsresult rv = nsAccessibleWrap::GetState(aState, aExtraState);
  NS_ENSURE_SUCCESS(rv, rv);

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

void nsHTMLLIAccessible::CacheChildren()
{
  if (!mWeakShell || mAccChildCount != eChildCountUninitialized) {
    return;
  }

  nsAccessibleWrap::CacheChildren();

  if (mBulletAccessible) {
    mBulletAccessible->SetNextSibling(mFirstChild);
    mBulletAccessible->SetParent(this); 
    SetFirstChild(mBulletAccessible);
    ++ mAccChildCount;
  }
}



nsHTMLListBulletAccessible::
  nsHTMLListBulletAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell,
                             const nsAString& aBulletText) :
    nsLeafAccessible(aDomNode, aShell), mWeakParent(nsnull),
    mBulletText(aBulletText)
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
  mWeakParent = nsnull;

  return nsLeafAccessible::Shutdown();
}

NS_IMETHODIMP
nsHTMLListBulletAccessible::GetName(nsAString &aName)
{
  
  aName = mBulletText;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLListBulletAccessible::GetRole(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_STATICTEXT;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLListBulletAccessible::GetState(PRUint32 *aState, PRUint32 *aExtraState)
{
  nsresult rv = nsLeafAccessible::GetState(aState, aExtraState);
  NS_ENSURE_SUCCESS(rv, rv);

  *aState &= ~nsIAccessibleStates::STATE_FOCUSABLE;
  *aState |= nsIAccessibleStates::STATE_READONLY;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLListBulletAccessible::SetParent(nsIAccessible *aParentAccessible)
{
  mParent = nsnull;
  mWeakParent = aParentAccessible;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLListBulletAccessible::GetParent(nsIAccessible **aParentAccessible)
{
  NS_IF_ADDREF(*aParentAccessible = mWeakParent);
  return NS_OK;
}

NS_IMETHODIMP
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



NS_IMETHODIMP
nsHTMLListAccessible::GetState(PRUint32 *aState, PRUint32 *aExtraState)
{
  nsresult rv = nsHyperTextAccessibleWrap::GetState(aState, aExtraState);
  NS_ENSURE_SUCCESS(rv, rv);

  *aState |= nsIAccessibleStates::STATE_READONLY;
  return NS_OK;
}

