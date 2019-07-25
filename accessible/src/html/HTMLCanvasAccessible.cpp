




#include "HTMLCanvasAccessible.h"

#include "Role.h"

using namespace mozilla::a11y;

HTMLCanvasAccessible::
  HTMLCanvasAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  HyperTextAccessible(aContent, aDoc)
{
}

role
HTMLCanvasAccessible::NativeRole()
{
  return roles::CANVAS;
}
