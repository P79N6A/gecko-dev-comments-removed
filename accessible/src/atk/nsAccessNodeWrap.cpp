





































#include "nsAccessNodeWrap.h"
#include "nsApplicationAccessibleWrap.h"














nsAccessNodeWrap::nsAccessNodeWrap(nsIDOMNode *aNode, nsIWeakReference* aShell): 
  nsAccessNode(aNode, aShell)
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

