




#include "HTMLCanvasAccessible.h"

#include "Role.h"

using namespace mozilla::a11y;

HTMLCanvasAccessible::
  HTMLCanvasAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  HyperTextAccessibleWrap(aContent, aDoc)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(HTMLCanvasAccessible, HyperTextAccessible)

role
HTMLCanvasAccessible::NativeRole()
{
  return roles::CANVAS;
}
