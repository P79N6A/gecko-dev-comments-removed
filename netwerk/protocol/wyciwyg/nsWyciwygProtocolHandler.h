





#ifndef nsWyciwygProtocolHandler_h___
#define nsWyciwygProtocolHandler_h___

#include "nsIProtocolHandler.h"
            
class nsWyciwygProtocolHandler : public nsIProtocolHandler
{
    virtual ~nsWyciwygProtocolHandler();

public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROTOCOLHANDLER

    nsWyciwygProtocolHandler();
};

#endif 
