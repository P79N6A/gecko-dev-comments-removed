





































#ifndef nsIconProtocolHandler_h___
#define nsIconProtocolHandler_h___

#include "nsWeakReference.h"
#include "nsIProtocolHandler.h"

class nsIconProtocolHandler : public nsIProtocolHandler, public nsSupportsWeakReference
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROTOCOLHANDLER

    
    nsIconProtocolHandler();
    virtual ~nsIconProtocolHandler();

protected:
};

#endif
