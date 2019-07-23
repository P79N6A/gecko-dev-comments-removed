





































#include "nsAccessNodeWrap.h"
#include "nsAppRootAccessible.h"














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

  
  nsAppRootAccessible::Load();
}

void nsAccessNodeWrap::ShutdownAccessibility()
{
  nsAccessNode::ShutdownXPAccessibility();

  
  nsAppRootAccessible::Unload();
}
