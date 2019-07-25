




#ifndef nsViewSourceHandler_h___
#define nsViewSourceHandler_h___

#include "nsIProtocolHandler.h"
#include "mozilla/Attributes.h"

class nsViewSourceHandler MOZ_FINAL : public nsIProtocolHandler
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROTOCOLHANDLER
};

#endif
