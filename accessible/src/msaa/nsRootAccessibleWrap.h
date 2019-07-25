









































#ifndef _nsRootAccessibleWrap_H_
#define _nsRootAccessibleWrap_H_

#include "nsCOMPtr.h"
#include "nsRootAccessible.h"

class nsRootAccessibleWrap: public nsRootAccessible
{
public:
  nsRootAccessibleWrap(nsIDocument *aDocument, nsIContent *aRootContent,
                       nsIWeakReference *aShell);
  virtual ~nsRootAccessibleWrap();
};

#endif
