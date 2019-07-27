




#ifndef mozilla_image_decoders_icon_nsIconProtocolHandler_h
#define mozilla_image_decoders_icon_nsIconProtocolHandler_h

#include "nsWeakReference.h"
#include "nsIProtocolHandler.h"

class nsIconProtocolHandler : public nsIProtocolHandler,
                              public nsSupportsWeakReference
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROTOCOLHANDLER

    
    nsIconProtocolHandler();

protected:
    virtual ~nsIconProtocolHandler();
};

#endif 
