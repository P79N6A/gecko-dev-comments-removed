




































#include "nsXULMenuAccessibleWrap.h"
#include "nsINameSpaceManager.h"





nsXULMenuitemAccessibleWrap::
  nsXULMenuitemAccessibleWrap(nsIContent* aContent, nsDocAccessible* aDoc) :
  nsXULMenuitemAccessible(aContent, aDoc)
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
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::acceltext, accel);
  if (!accel.IsEmpty()) {
    aName += NS_LITERAL_STRING("\t") + accel;
  }

  return NS_OK;
}
