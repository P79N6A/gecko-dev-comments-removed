









































#ifndef _nsAccessNodeWrap_H_
#define _nsAccessNodeWrap_H_

#include "nsAccessNode.h"

class nsAccessNodeWrap :  public nsAccessNode
{
public: 
    nsAccessNodeWrap(nsIDOMNode *aNode, nsIWeakReference* aShell);
    virtual ~nsAccessNodeWrap();

    static void InitAccessibility();
    static void ShutdownAccessibility();
};

#endif
