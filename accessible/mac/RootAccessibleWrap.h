








#ifndef mozilla_a11y_RootAccessibleWrap_h__
#define mozilla_a11y_RootAccessibleWrap_h__

#include "RootAccessible.h"

namespace mozilla {
namespace a11y {

class RootAccessibleWrap : public RootAccessible
{
public:
  RootAccessibleWrap(nsIDocument* aDocument, nsIContent* aRootContent,
                     nsIPresShell* aPresShell);
  virtual ~RootAccessibleWrap();

    Class GetNativeType ();
    
    
    
    void GetNativeWidget (void **aOutView);
};

} 
} 

#endif
