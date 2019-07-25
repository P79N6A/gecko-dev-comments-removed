









































#ifndef _nsRootAccessibleWrap_H_
#define _nsRootAccessibleWrap_H_

#include "nsRootAccessible.h"

struct objc_class;

class nsRootAccessibleWrap : public nsRootAccessible
{
public:
  nsRootAccessibleWrap(nsIDocument *aDocument, nsIContent *aRootContent,
                       nsIWeakReference *aShell);
  virtual ~nsRootAccessibleWrap();

    objc_class* GetNativeType ();
    
    
    
    void GetNativeWidget (void **aOutView);
};


#endif
