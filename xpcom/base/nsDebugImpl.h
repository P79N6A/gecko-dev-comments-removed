



































#include "nsIDebug.h"
#include "nsIDebug2.h"

class nsDebugImpl : public nsIDebug2
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIDEBUG
    NS_DECL_NSIDEBUG2
    
    static nsresult Create(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr);
};


#define NS_DEBUG_CONTRACTID "@mozilla.org/xpcom/debug;1"
#define NS_DEBUG_CLASSNAME  "nsDebug Interface"
#define NS_DEBUG_CID                                 \
{ /* a80b1fb3-aaf6-4852-b678-c27eb7a518af */         \
  0xa80b1fb3,                                        \
    0xaaf6,                                          \
    0x4852,                                          \
    {0xb6, 0x78, 0xc2, 0x7e, 0xb7, 0xa5, 0x18, 0xaf} \
}            
