




































#include "nsXULMenuAccessibleWrap.h"
#include "nsINameSpaceManager.h"

using namespace mozilla::a11y;





nsXULMenuitemAccessibleWrap::
  nsXULMenuitemAccessibleWrap(nsIContent* aContent, nsDocAccessible* aDoc) :
  nsXULMenuitemAccessible(aContent, aDoc)
{
}

ENameValueFlag
nsXULMenuitemAccessibleWrap::Name(nsString& aName)
{
  
  
  nsXULMenuitemAccessible::Name(aName);
  if (aName.IsEmpty())
    return eNameOK;
  
  nsAutoString accel;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::acceltext, accel);
  if (!accel.IsEmpty())
    aName += NS_LITERAL_STRING("\t") + accel;

  return eNameOK;
}
