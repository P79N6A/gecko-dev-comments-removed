









































#ifndef _nsRootAccessibleWrap_H_
#define _nsRootAccessibleWrap_H_

#include "nsRootAccessible.h"

struct objc_class;

class nsRootAccessibleWrap : public nsRootAccessible
{
public:
  nsRootAccessibleWrap(nsIDocument* aDocument, nsIContent* aRootContent,
                       nsIPresShell* aPresShell);
  virtual ~nsRootAccessibleWrap();

    Class GetNativeType ();
    
    
    
    void GetNativeWidget (void **aOutView);
};


#endif
