






































#include "nsAccessNodeWrap.h"
#include "nsApplicationAccessibleWrap.h"
#include "nsMaiInterfaceText.h"

PRBool nsAccessNodeWrap::gHaveNewTextSignals = PR_FALSE;














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
  gHaveNewTextSignals = g_signal_lookup("text-insert", ATK_TYPE_TEXT);
}

void nsAccessNodeWrap::ShutdownAccessibility()
{
  nsAccessNode::ShutdownXPAccessibility();
}

