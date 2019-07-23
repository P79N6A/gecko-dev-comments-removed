





































#ifndef TOOLKIT_H      
#define TOOLKIT_H

#include "nsIToolkit.h"





 

class nsToolkit : public nsIToolkit
{

  public:

            NS_DECL_ISUPPORTS

                            nsToolkit();
            NS_IMETHOD      Init(PRThread *aThread);

private:
                            ~nsToolkit();
};

#endif  
