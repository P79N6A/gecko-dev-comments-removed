





































#include "nsRootAccessibleWrap.h"
#include "nsIAccessible.h"
#include "nsIAccessibleDocument.h"
#include "nsIServiceManager.h"









nsRootAccessibleWrap::
  nsRootAccessibleWrap(nsIDocument *aDocument, nsIContent *aRootContent,
                       nsIWeakReference *aShell) :
  nsRootAccessible(aDocument, aRootContent, aShell)
{
}

nsRootAccessibleWrap::~nsRootAccessibleWrap()
{
}

