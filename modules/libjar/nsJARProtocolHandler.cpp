





































#include "nsAutoPtr.h"
#include "nsJARProtocolHandler.h"
#include "nsIIOService.h"
#include "nsCRT.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsJARURI.h"
#include "nsIURL.h"
#include "nsJARChannel.h"
#include "nsXPIDLString.h"
#include "nsString.h"
#include "nsNetCID.h"
#include "nsIMIMEService.h"
#include "nsMimeTypes.h"

static NS_DEFINE_CID(kZipReaderCacheCID, NS_ZIPREADERCACHE_CID);

#define NS_JAR_CACHE_SIZE 32



nsJARProtocolHandler *gJarHandler = nsnull;

nsJARProtocolHandler::nsJARProtocolHandler()
{
}

nsJARProtocolHandler::~nsJARProtocolHandler()
{
}

nsresult
nsJARProtocolHandler::Init()
{
    nsresult rv;

    mJARCache = do_CreateInstance(kZipReaderCacheCID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = mJARCache->Init(NS_JAR_CACHE_SIZE);
    return rv;
}

nsIMIMEService * 
nsJARProtocolHandler::MimeService()
{
    if (!mMimeService)
        mMimeService = do_GetService("@mozilla.org/mime;1");

    return mMimeService.get();
}

NS_IMPL_THREADSAFE_ISUPPORTS3(nsJARProtocolHandler,
                              nsIJARProtocolHandler,
                              nsIProtocolHandler,
                              nsISupportsWeakReference)

nsJARProtocolHandler*
nsJARProtocolHandler::GetSingleton()
{
    if (!gJarHandler) {
        gJarHandler = new nsJARProtocolHandler();
        if (!gJarHandler)
            return nsnull;

        NS_ADDREF(gJarHandler);
        nsresult rv = gJarHandler->Init();
        if (NS_FAILED(rv)) {
            NS_RELEASE(gJarHandler);
            return nsnull;
        }
    }
    NS_ADDREF(gJarHandler);
    return gJarHandler;
}

NS_IMETHODIMP
nsJARProtocolHandler::GetJARCache(nsIZipReaderCache* *result)
{
    *result = mJARCache;
    NS_ADDREF(*result);
    return NS_OK;
}




NS_IMETHODIMP
nsJARProtocolHandler::GetScheme(nsACString &result)
{
    result.AssignLiteral("jar");
    return NS_OK;
}

NS_IMETHODIMP
nsJARProtocolHandler::GetDefaultPort(PRInt32 *result)
{
    *result = -1;        
    return NS_OK;
}

NS_IMETHODIMP
nsJARProtocolHandler::GetProtocolFlags(PRUint32 *result)
{
    
    
    *result = URI_NORELATIVE | URI_NOAUTH | URI_LOADABLE_BY_ANYONE;
    


    return NS_OK;
}

NS_IMETHODIMP
nsJARProtocolHandler::NewURI(const nsACString &aSpec,
                             const char *aCharset,
                             nsIURI *aBaseURI,
                             nsIURI **result)
{
    nsresult rv = NS_OK;

    nsRefPtr<nsJARURI> jarURI = new nsJARURI();
    if (!jarURI)
        return NS_ERROR_OUT_OF_MEMORY;

    rv = jarURI->Init(aCharset);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = jarURI->SetSpecWithBase(aSpec, aBaseURI);
    if (NS_FAILED(rv))
        return rv;

    NS_ADDREF(*result = jarURI);
    return rv;
}

NS_IMETHODIMP
nsJARProtocolHandler::NewChannel(nsIURI *uri, nsIChannel **result)
{
    nsJARChannel *chan = new nsJARChannel();
    if (!chan)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(chan);

    nsresult rv = chan->Init(uri);
    if (NS_FAILED(rv)) {
        NS_RELEASE(chan);
        return rv;
    }

    *result = chan;
    return NS_OK;
}


NS_IMETHODIMP
nsJARProtocolHandler::AllowPort(PRInt32 port, const char *scheme, PRBool *_retval)
{
    
    *_retval = PR_FALSE;
    return NS_OK;
}


