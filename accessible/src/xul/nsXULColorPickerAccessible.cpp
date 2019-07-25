





































#include "nsXULColorPickerAccessible.h"

#include "nsAccUtils.h"
#include "nsAccTreeWalker.h"
#include "nsCoreUtils.h"

#include "nsIDOMElement.h"






nsXULColorPickerTileAccessible::
  nsXULColorPickerTileAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsAccessibleWrap(aContent, aShell)
{
}




NS_IMETHODIMP
nsXULColorPickerTileAccessible::GetValue(nsAString& aValue)
{
  aValue.Truncate();

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  mContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::color, aValue);
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

  
  PRBool isFocused = mContent->HasAttr(kNameSpaceID_None,
                                       nsAccessibilityAtoms::hover);
  if (isFocused)
    *aState |= nsIAccessibleStates::STATE_FOCUSED;

  PRBool isSelected = mContent->HasAttr(kNameSpaceID_None,
                                        nsAccessibilityAtoms::selected);
  if (isSelected)
    *aState |= nsIAccessibleStates::STATE_SELECTED;

  return NS_OK;
}






nsXULColorPickerAccessible::
  nsXULColorPickerAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXULColorPickerTileAccessible(aContent, aShell)
{
}




PRBool
nsXULColorPickerAccessible::Init()
{
  if (!nsXULColorPickerTileAccessible::Init())
    return PR_FALSE;

  nsCoreUtils::GeneratePopupTree(mContent, PR_TRUE);
  return PR_TRUE;
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
  nsAccTreeWalker walker(mWeakShell, mContent, PR_TRUE);

  nsRefPtr<nsAccessible> child;
  while ((child = walker.GetNextChild())) {
    PRUint32 role = nsAccUtils::Role(child);

    
    if (role == nsIAccessibleRole::ROLE_ALERT) {
      AppendChild(child);
      return;
    }
  }
}
