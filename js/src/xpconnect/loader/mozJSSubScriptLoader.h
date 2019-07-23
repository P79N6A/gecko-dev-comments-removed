







































#include "nsCOMPtr.h"
#include "mozIJSSubScriptLoader.h"
#include "nsIScriptSecurityManager.h"

#define MOZ_JSSUBSCRIPTLOADER_CID                    \
{ /* 829814d6-1dd2-11b2-8e08-82fa0a339b00 */         \
    0x929814d6,                                      \
    0x1dd2,                                          \
    0x11b2,                                          \
    {0x8e, 0x08, 0x82, 0xfa, 0x0a, 0x33, 0x9b, 0x00} \
}

class mozJSSubScriptLoader : public mozIJSSubScriptLoader
{
public:
    mozJSSubScriptLoader();
    virtual ~mozJSSubScriptLoader();

    
    NS_DECL_ISUPPORTS
    NS_DECL_MOZIJSSUBSCRIPTLOADER

private:
    nsCOMPtr<nsIPrincipal> mSystemPrincipal;
    
};
