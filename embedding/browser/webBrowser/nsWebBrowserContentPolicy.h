





































#include "nsIContentPolicy.h"


#define NS_WEBBROWSERCONTENTPOLICY_CID \
{ 0xf66bc334, 0x1dd1, 0x11b2, { 0xba, 0xb2, 0x90, 0xe0, 0x4f, 0xe1, 0x5c, 0x19 } }

#define NS_WEBBROWSERCONTENTPOLICY_CONTRACTID "@mozilla.org/embedding/browser/content-policy;1"

class nsWebBrowserContentPolicy : public nsIContentPolicy
{
public:
    nsWebBrowserContentPolicy();
    virtual ~nsWebBrowserContentPolicy();
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSICONTENTPOLICY
};

