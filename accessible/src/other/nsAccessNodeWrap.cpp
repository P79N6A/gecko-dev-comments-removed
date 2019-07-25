





































#include "nsAccessNodeWrap.h"














nsAccessNodeWrap::
  nsAccessNodeWrap(nsIContent* aContent, nsDocAccessible* aDoc) :
  nsAccessNode(aContent, aDoc)
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

