





































#include "nsCOMPtr.h"
#include "nsRootAccessibleWrap.h"
#include "nsIServiceManager.h"
#include "nsIAccessibilityService.h"



nsRootAccessibleWrap::nsRootAccessibleWrap(nsIDOMNode *aDOMNode, nsIWeakReference *aShell): 
  nsRootAccessible(aDOMNode, aShell)
{
}

nsRootAccessibleWrap::~nsRootAccessibleWrap()
{
}

