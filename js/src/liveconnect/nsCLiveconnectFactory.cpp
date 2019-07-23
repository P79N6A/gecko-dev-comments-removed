











































#include "prtypes.h"
#include "nspr.h"
#include "prmem.h"
#include "prmon.h"
#include "prlog.h"

#include "nsXPCOM.h"
#include "nsCLiveconnect.h"
#include "nsCLiveconnectFactory.h"
#include "nsIComponentManager.h"
#include "nsIComponentRegistrar.h"

static NS_DEFINE_CID(kCLiveconnectCID, NS_CLIVECONNECT_CID);

extern "C" NS_EXPORT nsresult
JSJ_RegisterLiveConnectFactory()
{
    nsCOMPtr<nsIComponentRegistrar> registrar;
    nsresult rv = NS_GetComponentRegistrar(getter_AddRefs(registrar));
    if (NS_FAILED(rv))
        return rv;
      
    nsCOMPtr<nsIFactory> factory = new nsCLiveconnectFactory;
    if (factory) {
        return registrar->RegisterFactory(kCLiveconnectCID, "LiveConnect",
                                          "@mozilla.org/liveconnect/liveconnect;1",
                                          factory);
    }
    return NS_ERROR_OUT_OF_MEMORY;
}

NS_IMPL_ISUPPORTS1(nsCLiveconnectFactory, nsIFactory)




NS_METHOD
nsCLiveconnectFactory::CreateInstance(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
	if (!aResult)
		return NS_ERROR_INVALID_POINTER;

	*aResult  = NULL;

    NS_ENSURE_PROPER_AGGREGATION(aOuter, aIID);

	nsCLiveconnect* liveconnect = new nsCLiveconnect(aOuter);
	if (liveconnect == NULL)
		return NS_ERROR_OUT_OF_MEMORY;
		
    nsISupports* inner = liveconnect->InnerObject();
    nsresult result = inner->QueryInterface(aIID, aResult);
	if (NS_FAILED(result))
		delete liveconnect;

	return result;
}

NS_METHOD
nsCLiveconnectFactory::LockFactory(PRBool aLock)
{
   return NS_OK;
}




nsCLiveconnectFactory::nsCLiveconnectFactory(void)
{
}

nsCLiveconnectFactory::~nsCLiveconnectFactory()
{
}
