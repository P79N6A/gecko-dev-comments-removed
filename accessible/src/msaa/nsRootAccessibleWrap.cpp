





































#include "nsRootAccessibleWrap.h"
#include "nsIAccessible.h"
#include "nsIAccessibleDocument.h"
#include "nsIServiceManager.h"







nsRootAccessibleWrap::nsRootAccessibleWrap(nsIDOMNode *aDOMNode, nsIWeakReference *aShell): 
  nsRootAccessible(aDOMNode, aShell)
{
}

nsRootAccessibleWrap::~nsRootAccessibleWrap()
{
}

