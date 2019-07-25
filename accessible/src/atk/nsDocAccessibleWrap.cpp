








































#include "nsMai.h"
#include "nsDocAccessibleWrap.h"





nsDocAccessibleWrap::
    nsDocAccessibleWrap(nsIDocument *aDocument, nsIContent *aRootContent,
                        nsIWeakReference *aShell) :
    nsDocAccessible(aDocument, aRootContent, aShell), mActivated(false)
{
}

nsDocAccessibleWrap::~nsDocAccessibleWrap()
{
}

