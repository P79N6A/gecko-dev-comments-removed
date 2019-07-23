









































#ifndef _nsRootAccessibleWrap_H_
#define _nsRootAccessibleWrap_H_

#include "nsRootAccessible.h"

class nsRootAccessibleWrap: public nsRootAccessible
{
public:
    nsRootAccessibleWrap(nsIDOMNode *aNode, nsIWeakReference *aShell);
    virtual ~nsRootAccessibleWrap();
};


#endif
