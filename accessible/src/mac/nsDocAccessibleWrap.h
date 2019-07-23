





































#ifndef _nsDocAccessibleWrap_H_
#define _nsDocAccessibleWrap_H_

#include "nsDocAccessible.h"

struct objc_class;

class nsDocAccessibleWrap: public nsDocAccessible
{
  public:
    nsDocAccessibleWrap(nsIDOMNode *aNode, nsIWeakReference *aShell);
    virtual ~nsDocAccessibleWrap();
    
    NS_IMETHOD FireToolkitEvent(PRUint32 aEvent, nsIAccessible* aAccessible, 
                                void* aData);
};

#endif

