





#ifndef nsWyciwygProtocolHandler_h___
#define nsWyciwygProtocolHandler_h___

#include "nsIProtocolHandler.h"
            
class nsWyciwygProtocolHandler : public nsIProtocolHandler
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROTOCOLHANDLER

    nsWyciwygProtocolHandler();
    virtual ~nsWyciwygProtocolHandler();

    nsresult Init();

    static void GetCacheSessionName(uint32_t aAppId,
                                    bool aInBrowser,
                                    bool aPrivateBrowsing,
                                    nsACString& aSessionName);
};

#endif 
