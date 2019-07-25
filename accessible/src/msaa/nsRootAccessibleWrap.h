






































#ifndef _nsRootAccessibleWrap_H_
#define _nsRootAccessibleWrap_H_

#include "nsRootAccessible.h"

class nsRootAccessibleWrap : public nsRootAccessible
{
public:
  nsRootAccessibleWrap(nsIDocument* aDocument, nsIContent* aRootContent,
                       nsIPresShell* aPresShell);
  virtual ~nsRootAccessibleWrap();

  
  virtual void DocumentActivated(nsDocAccessible* aDocument);
};

#endif
