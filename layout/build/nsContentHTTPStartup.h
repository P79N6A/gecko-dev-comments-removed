





































#include "nsIObserver.h"

#define NS_CONTENTHTTPSTARTUP_CONTRACTID \
  "@mozilla.org/content/http-startup;1"


#define NS_CONTENTHTTPSTARTUP_CID \
{ 0xc2f6ef7e, 0xafd5, 0x4be4, \
    {0xa1, 0xf5, 0xc8, 0x24, 0xef, 0xa4, 0x23, 0x1b} }

struct nsModuleComponentInfo;
class nsIFile;

class nsContentHTTPStartup : public nsIObserver
{
public:
    nsContentHTTPStartup() { }
    virtual ~nsContentHTTPStartup() {}
  
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

public:
    static NS_IMETHODIMP
    RegisterHTTPStartup(nsIComponentManager*         aCompMgr,
                        nsIFile*                     aPath,
                        const char*                  aRegistryLocation,
                        const char*                  aComponentType,
                        const nsModuleComponentInfo* aInfo);

    static NS_IMETHODIMP
    UnregisterHTTPStartup(nsIComponentManager*         aCompMgr,
                          nsIFile*                     aPath,
                          const char*                  aRegistryLocation,
                          const nsModuleComponentInfo* aInfo);
  
private:
    nsresult setUserAgent();
  
};
