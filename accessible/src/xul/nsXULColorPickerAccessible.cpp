






































#include "nsXULColorPickerAccessible.h"
#include "nsIDOMElement.h"









nsXULColorPickerTileAccessible::nsXULColorPickerTileAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsFormControlAccessible(aNode, aShell)
{ 
}




NS_IMETHODIMP nsXULColorPickerTileAccessible::GetRole(PRUint32 *_retval)
{
  *_retval = nsIAccessibleRole::ROLE_PUSHBUTTON;
  return NS_OK;
}




NS_IMETHODIMP nsXULColorPickerTileAccessible::GetState(PRUint32 *_retval)
{
  
  nsFormControlAccessible::GetState(_retval);
  *_retval |= nsIAccessibleStates::STATE_FOCUSABLE;

  
  nsCOMPtr<nsIDOMElement> element(do_QueryInterface(mDOMNode));
  NS_ASSERTION(element, "No XUL Element for colorpicker");
  PRBool isFocused = PR_FALSE;
  element->HasAttribute(NS_LITERAL_STRING("hover"), &isFocused);
  if (isFocused)
    *_retval |= nsIAccessibleStates::STATE_FOCUSED;

  PRBool isSelected = PR_FALSE;
  element->HasAttribute(NS_LITERAL_STRING("selected"), &isSelected);
  if (isFocused)
    *_retval |= nsIAccessibleStates::STATE_SELECTED;

  return NS_OK;
}

NS_IMETHODIMP nsXULColorPickerTileAccessible::GetName(nsAString& _retval)
{
  return GetXULName(_retval);
}

NS_IMETHODIMP nsXULColorPickerTileAccessible::GetValue(nsAString& _retval)
{
  nsCOMPtr<nsIDOMElement> element(do_QueryInterface(mDOMNode));
  NS_ASSERTION(element, "No XUL Element for colorpicker");
  return element->GetAttribute(NS_LITERAL_STRING("color"), _retval);
}








nsXULColorPickerAccessible::nsXULColorPickerAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsXULColorPickerTileAccessible(aNode, aShell)
{ 
}




NS_IMETHODIMP nsXULColorPickerAccessible::GetState(PRUint32 *_retval)
{
  
  nsFormControlAccessible::GetState(_retval);
  *_retval |= nsIAccessibleStates::STATE_FOCUSABLE |
              nsIAccessibleStates::STATE_HASPOPUP;

  return NS_OK;
}

NS_IMETHODIMP nsXULColorPickerAccessible::GetRole(PRUint32 *_retval)
{
  *_retval = nsIAccessibleRole::ROLE_BUTTONDROPDOWNGRID;
  return NS_OK;
}

