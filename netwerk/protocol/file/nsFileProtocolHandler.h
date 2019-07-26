




#ifndef nsFileProtocolHandler_h__
#define nsFileProtocolHandler_h__

#include "nsIFileProtocolHandler.h"
#include "nsWeakReference.h"

class nsFileProtocolHandler : public nsIFileProtocolHandler
                            , public nsSupportsWeakReference
{
    virtual ~nsFileProtocolHandler() {}

public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIPROTOCOLHANDLER
    NS_DECL_NSIFILEPROTOCOLHANDLER

    nsFileProtocolHandler();

    nsresult Init();
};

#endif 
