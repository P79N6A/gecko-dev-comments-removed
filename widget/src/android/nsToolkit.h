





































#ifndef nsToolkit_h__
#define nsToolkit_h__

#include <nsIToolkit.h>

class nsToolkit :
    public nsIToolkit
{
public:
    NS_DECL_ISUPPORTS

    
    NS_IMETHOD Init(PRThread *aThread);

    nsToolkit();
    virtual ~nsToolkit();
};

#endif 
