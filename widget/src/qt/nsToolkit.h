




































#ifndef nsToolkit_h__      
#define nsToolkit_h__

#include "nsIToolkit.h"





 
class nsToolkit : public nsIToolkit
{
public:
    nsToolkit();
    virtual ~nsToolkit();
  
    NS_DECL_ISUPPORTS
    NS_IMETHOD Init(PRThread *aThread);
};

#endif  
