




#ifndef nsViewSourceHandler_h___
#define nsViewSourceHandler_h___

#include "nsIProtocolHandler.h"
#include "mozilla/Attributes.h"

class nsViewSourceHandler MOZ_FINAL : public nsIProtocolHandler
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROTOCOLHANDLER

    nsViewSourceHandler();

    
    
    nsresult NewSrcdocChannel(nsIURI* uri, const nsAString &srcdoc,
                              nsIChannel** result);

    static nsViewSourceHandler* GetInstance();

private:
    ~nsViewSourceHandler();

    static nsViewSourceHandler* gInstance;
};

#endif 
