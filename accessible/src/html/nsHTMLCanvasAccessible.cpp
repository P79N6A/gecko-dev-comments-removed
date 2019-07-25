




































#include "nsHTMLCanvasAccessible.h"

#include "Role.h"

using namespace mozilla::a11y;

nsHTMLCanvasAccessible::
  nsHTMLCanvasAccessible(nsIContent* aContent, nsIWeakReference* aShell) :
  nsHyperTextAccessible(aContent, aShell)
{
}

role
nsHTMLCanvasAccessible::NativeRole()
{
  return roles::CANVAS;
}
