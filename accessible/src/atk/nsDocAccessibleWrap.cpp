








































#include "nsMai.h"
#include "nsDocAccessibleWrap.h"





nsDocAccessibleWrap::
  nsDocAccessibleWrap(nsIDocument* aDocument, nsIContent* aRootContent,
                      nsIPresShell* aPresShell) :
  nsDocAccessible(aDocument, aRootContent, aPresShell), mActivated(false)
{
}

nsDocAccessibleWrap::~nsDocAccessibleWrap()
{
}

