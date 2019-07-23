








































#ifndef nsSimpleNotifier_H__
#define nsSimpleNotifier_H__

#include "nsIXPINotifier.h"

class nsSimpleNotifier : public nsIXPIListener
{
    public:

        nsSimpleNotifier();
        virtual ~nsSimpleNotifier();

        NS_DECL_NSIXPILISTENER
        NS_IMETHOD            QueryInterface(REFNSIID aIID, void** aInstancePtr);
        NS_IMETHOD_(nsrefcnt) AddRef(void);
        NS_IMETHOD_(nsrefcnt) Release(void);
};

#endif
