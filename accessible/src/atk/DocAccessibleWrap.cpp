





#include "nsMai.h"
#include "DocAccessibleWrap.h"





DocAccessibleWrap::
  DocAccessibleWrap(nsIDocument* aDocument, nsIContent* aRootContent,
                    nsIPresShell* aPresShell) :
  DocAccessible(aDocument, aRootContent, aPresShell), mActivated(false)
{
}

DocAccessibleWrap::~DocAccessibleWrap()
{
}

