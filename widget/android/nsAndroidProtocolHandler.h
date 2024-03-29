




#ifndef nsAndroidProtocolHandler_h___
#define nsAndroidProtocolHandler_h___

#include "nsIProtocolHandler.h"
#include "nsWeakReference.h"
#include "mozilla/Attributes.h"

#define NS_ANDROIDPROTOCOLHANDLER_CID                 \
{ /* e9cd2b7f-8386-441b-aaf5-0b371846bfd0 */         \
    0xe9cd2b7f,                                      \
    0x8386,                                          \
    0x441b,                                          \
    {0x0b, 0x37, 0x18, 0x46, 0xbf, 0xd0}             \
}

class nsAndroidProtocolHandler final : public nsIProtocolHandler,
                                       public nsSupportsWeakReference
{
public:
    NS_DECL_THREADSAFE_ISUPPORTS

    
    NS_DECL_NSIPROTOCOLHANDLER

    
    nsAndroidProtocolHandler() {}

private:
    ~nsAndroidProtocolHandler() {}
};

#endif 
