











































#ifndef _nsDocAccessibleWrap_H_
#define _nsDocAccessibleWrap_H_

#include "nsDocAccessible.h"

class nsDocAccessibleWrap: public nsDocAccessible
{
public:
  nsDocAccessibleWrap(nsIDocument *aDocument, nsIContent *aRootContent,
                      nsIWeakReference *aShell);
  virtual ~nsDocAccessibleWrap();

  PRBool mActivated;
};

#endif
