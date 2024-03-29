





#include "nsCOMPtr.h"
#include "mozIJSSubScriptLoader.h"

class nsIPrincipal;
class nsIURI;
class LoadSubScriptOptions;

#define MOZ_JSSUBSCRIPTLOADER_CID                                             \
{ /* 829814d6-1dd2-11b2-8e08-82fa0a339b00 */                                  \
    0x929814d6,                                                               \
    0x1dd2,                                                                   \
    0x11b2,                                                                   \
    {0x8e, 0x08, 0x82, 0xfa, 0x0a, 0x33, 0x9b, 0x00}                          \
}

class nsIIOService;

class mozJSSubScriptLoader : public mozIJSSubScriptLoader
{
public:
    mozJSSubScriptLoader();

    
    NS_DECL_ISUPPORTS
    NS_DECL_MOZIJSSUBSCRIPTLOADER

private:
    virtual ~mozJSSubScriptLoader();

    nsresult ReadScript(nsIURI* uri, JSContext* cx, JSObject* target_obj,
                        const nsAString& charset, const char* uriStr,
                        nsIIOService* serv, nsIPrincipal* principal,
                        bool reuseGlobal, JS::MutableHandleScript script,
                        JS::MutableHandleFunction function);

    nsresult ReadScriptAsync(nsIURI* uri, JSObject* target_obj,
                             const nsAString& charset,
                             nsIIOService* serv, bool reuseGlobal,
                             bool cache, JS::MutableHandleValue retval);

    nsresult DoLoadSubScriptWithOptions(const nsAString& url,
                                        LoadSubScriptOptions& options,
                                        JSContext* cx,
                                        JS::MutableHandle<JS::Value> retval);

    nsCOMPtr<nsIPrincipal> mSystemPrincipal;
};
