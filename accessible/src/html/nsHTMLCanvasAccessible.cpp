




#include "nsHTMLCanvasAccessible.h"

#include "Role.h"

using namespace mozilla::a11y;

nsHTMLCanvasAccessible::
  nsHTMLCanvasAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  nsHyperTextAccessible(aContent, aDoc)
{
}

role
nsHTMLCanvasAccessible::NativeRole()
{
  return roles::CANVAS;
}
