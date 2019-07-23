





































#ifndef TOOLKIT_H      
#define TOOLKIT_H

#include "nsdefs.h"
#include "prmon.h"
#include "nsIToolkit.h"
#ifdef DEBUG
#include <stdio.h>
#endif





 

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
