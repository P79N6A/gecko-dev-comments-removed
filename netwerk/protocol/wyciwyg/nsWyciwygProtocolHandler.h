





#ifndef nsWyciwygProtocolHandler_h___
#define nsWyciwygProtocolHandler_h___

#include "nsIProtocolHandler.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
            
class nsWyciwygProtocolHandler : public nsIProtocolHandler
                               , public nsIObserver
                               , public nsSupportsWeakReference
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROTOCOLHANDLER
    NS_DECL_NSIOBSERVER

    nsWyciwygProtocolHandler();
    virtual ~nsWyciwygProtocolHandler();

    nsresult Init();

    static void GetCacheSessionName(uint32_t aAppId,
                                    bool aInBrowser,
                                    bool aPrivateBrowsing,
                                    nsACString& aSessionName);
};

#endif 
