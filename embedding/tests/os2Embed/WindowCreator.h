





























#ifndef __WindowCreator_h_
#define __WindowCreator_h_

#include "nsIWindowCreator.h"

class WindowCreator :
      public nsIWindowCreator
{
public:
    WindowCreator();
    virtual ~WindowCreator();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWINDOWCREATOR
};

#endif

