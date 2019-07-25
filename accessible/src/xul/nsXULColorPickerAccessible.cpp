





































#include "nsXULColorPickerAccessible.h"

#include "States.h"
#include "nsAccUtils.h"
#include "nsAccTreeWalker.h"
#include "nsCoreUtils.h"
#include "nsDocAccessible.h"

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




PRUint32
nsXULColorPickerTileAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_PUSHBUTTON;
}

PRUint64
nsXULColorPickerTileAccessible::NativeState()
{
  

  
  PRUint64 states = nsAccessibleWrap::NativeState();

  states |= states::FOCUSABLE;

  
  PRBool isFocused = mContent->HasAttr(kNameSpaceID_None,
                                       nsAccessibilityAtoms::hover);
  if (isFocused)
    states |= states::FOCUSED;

  PRBool isSelected = mContent->HasAttr(kNameSpaceID_None,
                                        nsAccessibilityAtoms::selected);
  if (isSelected)
    states |= states::SELECTED;

  return states;
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




PRUint64
nsXULColorPickerAccessible::NativeState()
{
  

  
  PRUint64 states = nsAccessibleWrap::NativeState();

  states |= states::FOCUSABLE | states::HASPOPUP;

  return states;
}

PRUint32
nsXULColorPickerAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_BUTTONDROPDOWNGRID;
}




void
nsXULColorPickerAccessible::CacheChildren()
{
  nsAccTreeWalker walker(mWeakShell, mContent, PR_TRUE);

  nsAccessible* child = nsnull;
  while ((child = walker.NextChild())) {
    PRUint32 role = child->Role();

    
    if (role == nsIAccessibleRole::ROLE_ALERT) {
      AppendChild(child);
      return;
    }

    
    GetDocAccessible()->UnbindFromDocument(child);
  }
}
