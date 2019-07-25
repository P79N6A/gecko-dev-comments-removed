





































#include "nsCOMPtr.h"
#include "nsRootAccessibleWrap.h"
#include "nsIServiceManager.h"
#include "nsIAccessibilityService.h"





nsRootAccessibleWrap::
  nsRootAccessibleWrap(nsIDocument* aDocument, nsIContent* aRootContent,
                       nsIPresShell* aPresShell) :
  nsRootAccessible(aDocument, aRootContent, aPresShell)
{
}

nsRootAccessibleWrap::~nsRootAccessibleWrap()
{
}

