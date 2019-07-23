








































#ifndef nsStubNotifier_H__
#define nsStubNotifier_H__

#include "xpistub.h"
#include "nsIXPINotifier.h"

class nsStubListener : public nsIXPIListener
{
    public:

        nsStubListener( pfnXPIProgress );
        virtual ~nsStubListener();

        NS_DECL_ISUPPORTS
        NS_DECL_NSIXPILISTENER

    private:
        pfnXPIProgress  m_progress;
};

#endif
