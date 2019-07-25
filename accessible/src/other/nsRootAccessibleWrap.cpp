





































#include "nsCOMPtr.h"
#include "nsRootAccessibleWrap.h"
#include "nsIServiceManager.h"
#include "nsIAccessibilityService.h"





nsRootAccessibleWrap::
  nsRootAccessibleWrap(nsIDocument *aDocument, nsIContent *aRootContent,
                       nsIWeakReference *aShell) :
  nsRootAccessible(aDocument, aRootContent, aShell)
{
}

nsRootAccessibleWrap::~nsRootAccessibleWrap()
{
}

