










































#ifndef _nsAccessNodeWrap_H_
#define _nsAccessNodeWrap_H_

#include "nsAccessNode.h"

class nsAccessNodeWrap :  public nsAccessNode
{
public: 
    nsAccessNodeWrap(nsIContent *aContent, nsIWeakReference *aShell);
    virtual ~nsAccessNodeWrap();

    static void InitAccessibility();
    static void ShutdownAccessibility();

    




    static PRBool gHaveNewTextSignals;
};

#endif
