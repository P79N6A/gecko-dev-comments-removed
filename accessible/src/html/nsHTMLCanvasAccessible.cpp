




































#include "nsHTMLCanvasAccessible.h"

using namespace mozilla::a11y;

nsHTMLCanvasAccessible::
  nsHTMLCanvasAccessible(nsIContent* aContent, nsIWeakReference* aShell) :
  nsHyperTextAccessible(aContent, aShell)
{
}

PRUint32
nsHTMLCanvasAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_CANVAS;
}
