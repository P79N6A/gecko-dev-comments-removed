







































#include "nsAccessibilityAtoms.h"
#include "nsAccessibilityUtils.h"
#include "nsBaseWidgetAccessible.h"
#include "nsIDOMXULDescriptionElement.h"
#include "nsINameSpaceManager.h"
#include "nsString.h"
#include "nsXULTextAccessible.h"




nsXULTextAccessible::nsXULTextAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell):
nsHyperTextAccessibleWrap(aDomNode, aShell)
{ 
}


NS_IMETHODIMP nsXULTextAccessible::GetName(nsAString& aName)
{ 
  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  if (!content) {
    return NS_ERROR_FAILURE;  
  }
  if (!content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::value,
                        aName)) {
    
    return AppendFlatStringFromSubtree(content, &aName);
  }
  
  return NS_OK;
}

NS_IMETHODIMP
nsXULTextAccessible::GetState(PRUint32 *aState, PRUint32 *aExtraState)
{
  nsresult rv = nsHyperTextAccessibleWrap::GetState(aState, aExtraState);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  *aState |= nsIAccessibleStates::STATE_READONLY;
  return NS_OK;
}




nsXULTooltipAccessible::nsXULTooltipAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell):
nsLeafAccessible(aDomNode, aShell)
{ 
}

NS_IMETHODIMP nsXULTooltipAccessible::GetName(nsAString& aName)
{
  return GetXULName(aName, PR_TRUE);
}

NS_IMETHODIMP
nsXULTooltipAccessible::GetState(PRUint32 *aState, PRUint32 *aExtraState)
{
  nsresult rv = nsLeafAccessible::GetState(aState, aExtraState);
  NS_ENSURE_SUCCESS(rv, rv);

  *aState &= ~nsIAccessibleStates::STATE_FOCUSABLE;
  *aState |= nsIAccessibleStates::STATE_READONLY;
  return NS_OK;
}

NS_IMETHODIMP nsXULTooltipAccessible::GetRole(PRUint32 *_retval)
{
  *_retval = nsIAccessibleRole::ROLE_TOOLTIP;
  return NS_OK;
}




nsXULLinkAccessible::nsXULLinkAccessible(nsIDOMNode *aDomNode, nsIWeakReference *aShell):
nsLinkableAccessible(aDomNode, aShell)
{
}

NS_IMETHODIMP nsXULLinkAccessible::GetValue(nsAString& aValue)
{
  if (mIsLink) {
    mActionContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::href, aValue);
    return NS_OK;
  }
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsXULLinkAccessible::GetName(nsAString& aName)
{ 
  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  if (!content) {
    return NS_ERROR_FAILURE;  
  }
  if (!content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::value,
                        aName)) {
    
    return AppendFlatStringFromSubtree(content, &aName);
  }
  
  return NS_OK;
}

NS_IMETHODIMP nsXULLinkAccessible::GetRole(PRUint32 *aRole)
{
  if (mIsLink) {
    *aRole = nsIAccessibleRole::ROLE_LINK;
  } else {
    
    *aRole = nsIAccessibleRole::ROLE_PUSHBUTTON;
  }
  
  return NS_OK;
}

void nsXULLinkAccessible::CacheActionContent()
{
  
  nsCOMPtr<nsIContent> mTempContent = do_QueryInterface(mDOMNode);
  if (!mTempContent) {
    return;
  }

  
  if (mTempContent->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::href) ||
      mTempContent->Tag() == nsAccessibilityAtoms::link) {
    mIsLink = PR_TRUE;
    mActionContent = mTempContent;
  }
  else if (nsAccessibilityUtils::HasListener(mTempContent, NS_LITERAL_STRING("click"))) {
    mIsOnclick = PR_TRUE;
    mActionContent = mTempContent;
  }
}
