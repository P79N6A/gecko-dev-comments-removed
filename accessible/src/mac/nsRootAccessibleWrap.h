









































#ifndef _nsRootAccessibleWrap_H_
#define _nsRootAccessibleWrap_H_

#include "nsRootAccessible.h"

struct objc_class;

class nsRootAccessibleWrap : public nsRootAccessible
{
  public:
    nsRootAccessibleWrap(nsIDOMNode *aNode, nsIWeakReference *aShell);
    virtual ~nsRootAccessibleWrap();

    objc_class* GetNativeType ();
    
    
    
    void GetNativeWidget (void **aOutView);
};


#endif
