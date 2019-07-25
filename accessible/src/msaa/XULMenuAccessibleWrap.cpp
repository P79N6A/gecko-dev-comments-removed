




#include "XULMenuAccessibleWrap.h"
#include "nsINameSpaceManager.h"

using namespace mozilla::a11y;





XULMenuitemAccessibleWrap::
  XULMenuitemAccessibleWrap(nsIContent* aContent, DocAccessible* aDoc) :
  XULMenuitemAccessible(aContent, aDoc)
{
}

ENameValueFlag
XULMenuitemAccessibleWrap::Name(nsString& aName)
{
  
  
  XULMenuitemAccessible::Name(aName);
  if (aName.IsEmpty())
    return eNameOK;
  
  nsAutoString accel;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::acceltext, accel);
  if (!accel.IsEmpty())
    aName += NS_LITERAL_STRING("\t") + accel;

  return eNameOK;
}
