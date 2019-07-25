




































#ifndef nsFileProtocolHandler_h__
#define nsFileProtocolHandler_h__

#include "nsIFileProtocolHandler.h"
#include "nsWeakReference.h"

class nsFileProtocolHandler : public nsIFileProtocolHandler
                            , public nsSupportsWeakReference
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROTOCOLHANDLER
    NS_DECL_NSIFILEPROTOCOLHANDLER

    nsFileProtocolHandler();
    virtual ~nsFileProtocolHandler() {}

    nsresult Init();
};

#endif 
