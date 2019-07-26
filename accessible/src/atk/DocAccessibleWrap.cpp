





#include "nsMai.h"
#include "DocAccessibleWrap.h"

using namespace mozilla::a11y;





DocAccessibleWrap::
  DocAccessibleWrap(nsIDocument* aDocument, nsIContent* aRootContent,
                    nsIPresShell* aPresShell) :
  DocAccessible(aDocument, aRootContent, aPresShell), mActivated(false)
{
}

DocAccessibleWrap::~DocAccessibleWrap()
{
}

