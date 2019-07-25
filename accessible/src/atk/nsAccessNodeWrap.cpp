






































#include "nsAccessNodeWrap.h"
#include "nsApplicationAccessibleWrap.h"
#include "nsMaiInterfaceText.h"














nsAccessNodeWrap::
    nsAccessNodeWrap(nsIContent *aContent, nsIWeakReference *aShell) :
    nsAccessNode(aContent, aShell)
{
}




nsAccessNodeWrap::~nsAccessNodeWrap()
{
}

void nsAccessNodeWrap::InitAccessibility()
{
  nsAccessNode::InitXPAccessibility();
}

void nsAccessNodeWrap::ShutdownAccessibility()
{
  nsAccessNode::ShutdownXPAccessibility();
}

