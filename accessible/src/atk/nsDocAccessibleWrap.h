












































#ifndef _nsDocAccessibleWrap_H_
#define _nsDocAccessibleWrap_H_

#include "nsDocAccessible.h"

class nsDocAccessibleWrap: public nsDocAccessible
{
public:
  nsDocAccessibleWrap(nsIDOMNode *aNode, nsIWeakReference *aShell);
  virtual ~nsDocAccessibleWrap();

  PRBool mActivated;
};

#endif
