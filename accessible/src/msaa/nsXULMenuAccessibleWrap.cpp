




































#include "nsXULMenuAccessibleWrap.h"
#include "nsAccessibilityAtoms.h"
#include "nsINameSpaceManager.h"





nsXULMenuitemAccessibleWrap::nsXULMenuitemAccessibleWrap(nsIDOMNode *aDOMNode, 
                                                         nsIWeakReference *aShell):
nsXULMenuitemAccessible(aDOMNode, aShell)
{
}

NS_IMETHODIMP nsXULMenuitemAccessibleWrap::GetName(nsAString& aName)
{
  nsresult rv = nsXULMenuitemAccessible::GetName(aName);
  if (NS_FAILED(rv)) {
    return rv;
  }
  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  NS_ASSERTION(content, "Should not have gotten past nsXULMenuitemAccessible::GetName");
  
  nsAutoString accel;
  content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::acceltext, accel);
  if (!accel.IsEmpty()) {
    aName += NS_LITERAL_STRING("\t") + accel;
  }

  return NS_OK;
}
