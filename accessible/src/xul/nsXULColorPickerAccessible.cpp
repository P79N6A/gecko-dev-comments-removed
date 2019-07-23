





































#include "nsXULColorPickerAccessible.h"

#include "nsAccessibleTreeWalker.h"

#include "nsIDOMElement.h"






nsXULColorPickerTileAccessible::
  nsXULColorPickerTileAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell) :
  nsAccessibleWrap(aNode, aShell)
{
}




NS_IMETHODIMP
nsXULColorPickerTileAccessible::GetValue(nsAString& aValue)
{
  aValue.Truncate();

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::color, aValue);

  return NS_OK;
}




nsresult
nsXULColorPickerTileAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_PUSHBUTTON;
  return NS_OK;
}

nsresult
nsXULColorPickerTileAccessible::GetStateInternal(PRUint32 *aState,
                                                 PRUint32 *aExtraState)
{
  

  
  nsresult rv = nsAccessibleWrap::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  *aState |= nsIAccessibleStates::STATE_FOCUSABLE;

  
  nsCOMPtr<nsIDOMElement> element(do_QueryInterface(mDOMNode));
  NS_ASSERTION(element, "No XUL Element for colorpicker");
  PRBool isFocused = PR_FALSE;
  element->HasAttribute(NS_LITERAL_STRING("hover"), &isFocused);
  if (isFocused)
    *aState |= nsIAccessibleStates::STATE_FOCUSED;

  PRBool isSelected = PR_FALSE;
  element->HasAttribute(NS_LITERAL_STRING("selected"), &isSelected);
  if (isFocused)
    *aState |= nsIAccessibleStates::STATE_SELECTED;

  return NS_OK;
}









nsXULColorPickerAccessible::nsXULColorPickerAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsXULColorPickerTileAccessible(aNode, aShell)
{ 
}




nsresult
nsXULColorPickerAccessible::Init()
{
  nsresult rv = nsXULColorPickerTileAccessible::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCoreUtils::GeneratePopupTree(mDOMNode, PR_TRUE);
  return NS_OK;
}




nsresult
nsXULColorPickerAccessible::GetStateInternal(PRUint32 *aState,
                                             PRUint32 *aExtraState)
{
  

  
  nsresult rv = nsAccessibleWrap::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  *aState |= nsIAccessibleStates::STATE_FOCUSABLE |
             nsIAccessibleStates::STATE_HASPOPUP;

  return NS_OK;
}

nsresult
nsXULColorPickerAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_BUTTONDROPDOWNGRID;
  return NS_OK;
}




void
nsXULColorPickerAccessible::CacheChildren()
{
  nsCOMPtr<nsIAccessible> menupopupAccessible;
  nsAccessibleTreeWalker walker(mWeakShell, mDOMNode, PR_TRUE);
  walker.GetFirstChild();

  while (walker.mState.accessible) {
    PRUint32 role = nsAccUtils::Role(walker.mState.accessible);

    
    if (role == nsIAccessibleRole::ROLE_ALERT) {
      mChildren.AppendObject(walker.mState.accessible);
      
      nsRefPtr<nsAccessible> menupopupAcc =
        nsAccUtils::QueryObject<nsAccessible>(walker.mState.accessible);
      menupopupAcc->SetParent(this);

      return;
    }

    walker.GetNextSibling();
  }
}
