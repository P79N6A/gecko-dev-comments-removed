




































#include "nsXULMenuAccessibleWrap.h"
#include "nsAccessibilityAtoms.h"
#include "nsINameSpaceManager.h"





nsXULMenuitemAccessibleWrap::
  nsXULMenuitemAccessibleWrap(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXULMenuitemAccessible(aContent, aShell)
{
}

NS_IMETHODIMP
nsXULMenuitemAccessibleWrap::GetName(nsAString& aName)
{
  
  
  nsresult rv = nsXULMenuitemAccessible::GetName(aName);
  if (NS_FAILED(rv)) {
    return rv;
  }
  
  nsAutoString accel;
  mContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::acceltext, accel);
  if (!accel.IsEmpty()) {
    aName += NS_LITERAL_STRING("\t") + accel;
  }

  return NS_OK;
}
