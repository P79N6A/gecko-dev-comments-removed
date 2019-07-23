











































#include "nsChromeProtocolHandler.h"
#include "nsChromeRegistry.h"
#include "nsCOMPtr.h"
#include "nsContentCID.h"
#include "nsCRT.h"
#include "nsThreadUtils.h"
#include "nsIChannel.h"
#include "nsIChromeRegistry.h"
#include "nsIComponentManager.h"
#include "nsIFastLoadService.h"
#include "nsIFile.h"
#include "nsIFileURL.h"
#include "nsIFileChannel.h"
#include "nsIIOService.h"
#include "nsIJARChannel.h"
#include "nsIJARURI.h"
#include "nsILoadGroup.h"
#include "nsIObjectOutputStream.h"
#include "nsIScriptSecurityManager.h"
#include "nsIServiceManager.h"
#include "nsIStandardURL.h"
#include "nsIStreamListener.h"
#include "nsNetUtil.h"
#include "nsXPIDLString.h"
#include "nsString.h"
#include "prlog.h"

#ifdef MOZ_XUL
#include "nsIXULPrototypeCache.h"
#endif



static NS_DEFINE_CID(kXULPrototypeCacheCID,      NS_XULPROTOTYPECACHE_CID);






















#define LOG(args) PR_LOG(gLog, PR_LOG_DEBUG, args)

class nsCachedChromeChannel : public nsIChannel
{
protected:
    ~nsCachedChromeChannel();

    nsCOMPtr<nsIURI>            mURI;
    nsCOMPtr<nsIURI>            mOriginalURI;
    nsCOMPtr<nsILoadGroup>      mLoadGroup;
    nsCOMPtr<nsIStreamListener> mListener;
    nsCOMPtr<nsISupports>       mContext;
    nsLoadFlags                 mLoadFlags;
    nsCOMPtr<nsISupports>       mOwner;
    nsresult                    mStatus;

#ifdef PR_LOGGING
    static PRLogModuleInfo* gLog;
#endif

    void HandleLoadEvent();

public:
    nsCachedChromeChannel(nsIURI* aURI);

    NS_DECL_ISUPPORTS

    
    NS_IMETHOD GetName(nsACString &result) { return mURI->GetSpec(result); }
    NS_IMETHOD IsPending(PRBool *_retval) { *_retval = (mListener != nsnull); return NS_OK; }
    NS_IMETHOD GetStatus(nsresult *status) { *status = mStatus; return NS_OK; }
    NS_IMETHOD Cancel(nsresult status)  { mStatus = status; return NS_OK; }
    NS_IMETHOD Suspend(void) { return NS_OK; } 
    NS_IMETHOD Resume(void)  { return NS_OK; } 
    NS_IMETHOD GetLoadGroup(nsILoadGroup **);
    NS_IMETHOD SetLoadGroup(nsILoadGroup *);
    NS_IMETHOD GetLoadFlags(nsLoadFlags *);
    NS_IMETHOD SetLoadFlags(nsLoadFlags);

    
    NS_DECL_NSICHANNEL
};

#ifdef PR_LOGGING
PRLogModuleInfo* nsCachedChromeChannel::gLog;
#endif

NS_IMPL_ISUPPORTS2(nsCachedChromeChannel, nsIChannel, nsIRequest)

nsCachedChromeChannel::nsCachedChromeChannel(nsIURI* aURI)
    : mURI(aURI)
    , mLoadFlags(nsIRequest::LOAD_NORMAL)
    , mStatus(NS_OK)
{
#ifdef PR_LOGGING
    if (! gLog)
        gLog = PR_NewLogModule("nsCachedChromeChannel");
#endif

    LOG(("nsCachedChromeChannel[%p]: created", this));
}


nsCachedChromeChannel::~nsCachedChromeChannel()
{
    LOG(("nsCachedChromeChannel[%p]: destroyed", this));
}


NS_IMETHODIMP
nsCachedChromeChannel::GetOriginalURI(nsIURI* *aOriginalURI)
{
    *aOriginalURI = mOriginalURI ? mOriginalURI : mURI;
    NS_ADDREF(*aOriginalURI);
    return NS_OK;
}

NS_IMETHODIMP
nsCachedChromeChannel::SetOriginalURI(nsIURI* aOriginalURI)
{
    mOriginalURI = aOriginalURI;
    return NS_OK;
}

NS_IMETHODIMP
nsCachedChromeChannel::GetURI(nsIURI* *aURI)
{
    *aURI = mURI;
    NS_ADDREF(*aURI);
    return NS_OK;
}

NS_IMETHODIMP
nsCachedChromeChannel::Open(nsIInputStream **_retval)
{

    *_retval = nsnull;
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsCachedChromeChannel::AsyncOpen(nsIStreamListener *listener, nsISupports *ctxt)
{
    NS_ENSURE_ARG_POINTER(listener);

    nsresult rv;

    
    
    LOG(("nsCachedChromeChannel[%p]: posting load event for %p",
        this, listener));

    nsCOMPtr<nsIRunnable> event =
        NS_NEW_RUNNABLE_METHOD(nsCachedChromeChannel, this, HandleLoadEvent);

    
    
    
    rv = NS_DispatchToCurrentThread(event);
    if (NS_FAILED(rv))
        return rv;

    mContext  = ctxt;
    mListener = listener;

    if (mLoadGroup) {
        LOG(("nsCachedChromeChannel[%p]: adding self to load group %p",
            this, mLoadGroup.get()));

        (void) mLoadGroup->AddRequest(this, nsnull);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsCachedChromeChannel::GetSecurityInfo(nsISupports * *aSecurityInfo)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsCachedChromeChannel::GetLoadFlags(nsLoadFlags *aLoadFlags)
{
    *aLoadFlags = mLoadFlags;
    return NS_OK;
}

NS_IMETHODIMP
nsCachedChromeChannel::SetLoadFlags(nsLoadFlags aLoadFlags)
{
    mLoadFlags = aLoadFlags;
    return NS_OK;
}

NS_IMETHODIMP
nsCachedChromeChannel::GetOwner(nsISupports * *aOwner)
{
    *aOwner = mOwner;
    NS_IF_ADDREF(*aOwner);
    return NS_OK;
}

NS_IMETHODIMP
nsCachedChromeChannel::SetOwner(nsISupports * aOwner)
{
    mOwner = aOwner;
    return NS_OK;
}

NS_IMETHODIMP
nsCachedChromeChannel::GetLoadGroup(nsILoadGroup * *aLoadGroup)
{
    *aLoadGroup = mLoadGroup;
    NS_IF_ADDREF(*aLoadGroup);
    return NS_OK;
}

NS_IMETHODIMP
nsCachedChromeChannel::SetLoadGroup(nsILoadGroup * aLoadGroup)
{
    mLoadGroup = aLoadGroup;
    return NS_OK;
}

NS_IMETHODIMP
nsCachedChromeChannel::GetNotificationCallbacks(nsIInterfaceRequestor * *aNotificationCallbacks)
{
    *aNotificationCallbacks = nsnull;
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsCachedChromeChannel::SetNotificationCallbacks(nsIInterfaceRequestor * aNotificationCallbacks)
{
    return NS_OK;	
}

NS_IMETHODIMP
nsCachedChromeChannel::GetContentType(nsACString &aContentType)
{
    aContentType.AssignLiteral("mozilla.application/cached-xul");
    return NS_OK;
}

NS_IMETHODIMP
nsCachedChromeChannel::SetContentType(const nsACString &aContentType)
{
    
    NS_NOTREACHED("don't do that");
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsCachedChromeChannel::GetContentCharset(nsACString &aContentCharset)
{
    aContentCharset.Truncate();
    return NS_OK;
}

NS_IMETHODIMP
nsCachedChromeChannel::SetContentCharset(const nsACString &aContentCharset)
{
    
    NS_NOTREACHED("don't do that");
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsCachedChromeChannel::GetContentLength(PRInt32 *aContentLength)
{
    NS_NOTREACHED("don't do that");
    *aContentLength = 0;
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsCachedChromeChannel::SetContentLength(PRInt32 aContentLength)
{
    NS_NOTREACHED("nsCachedChromeChannel::SetContentLength");
    return NS_ERROR_NOT_IMPLEMENTED;
}

void
nsCachedChromeChannel::HandleLoadEvent()
{
    
    

    
    
    if (NS_FAILED(mStatus))
        return;

    LOG(("nsCachedChromeChannel[%p]: firing OnStartRequest for %p",
        this, mListener.get()));

    mListener->OnStartRequest(this, mContext);

    LOG(("nsCachedChromeChannel[%p]: firing OnStopRequest for %p",
        this, mListener.get()));

    mListener->OnStopRequest(this, mContext, mStatus);

    if (mLoadGroup) {
        LOG(("nsCachedChromeChannel[%p]: removing self from load group %p",
            this, mLoadGroup.get()));
        mLoadGroup->RemoveRequest(this, nsnull, mStatus);
    }

    mListener = nsnull;
    mContext  = nsnull;
}



NS_IMPL_THREADSAFE_ISUPPORTS2(nsChromeProtocolHandler,
                              nsIProtocolHandler,
                              nsISupportsWeakReference)




NS_IMETHODIMP
nsChromeProtocolHandler::GetScheme(nsACString &result)
{
    result.AssignLiteral("chrome");
    return NS_OK;
}

NS_IMETHODIMP
nsChromeProtocolHandler::GetDefaultPort(PRInt32 *result)
{
    *result = -1;        
    return NS_OK;
}

NS_IMETHODIMP
nsChromeProtocolHandler::AllowPort(PRInt32 port, const char *scheme, PRBool *_retval)
{
    
    *_retval = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP
nsChromeProtocolHandler::GetProtocolFlags(PRUint32 *result)
{
    *result = URI_STD | URI_IS_UI_RESOURCE;
    return NS_OK;
}

NS_IMETHODIMP
nsChromeProtocolHandler::NewURI(const nsACString &aSpec,
                                const char *aCharset,
                                nsIURI *aBaseURI,
                                nsIURI **result)
{
    nsresult rv;

    
    

    nsCOMPtr<nsIStandardURL> surl(do_CreateInstance(NS_STANDARDURL_CONTRACTID, &rv));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = surl->Init(nsIStandardURL::URLTYPE_STANDARD, -1, aSpec, aCharset, aBaseURI);
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsIURL> url(do_QueryInterface(surl, &rv));
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    

    rv = nsChromeRegistry::Canonify(url);
    if (NS_FAILED(rv))
        return rv;

    surl->SetMutable(PR_FALSE);

    NS_ADDREF(*result = url);
    return NS_OK;
}

NS_IMETHODIMP
nsChromeProtocolHandler::NewChannel(nsIURI* aURI,
                                    nsIChannel* *aResult)
{
    nsresult rv;

    NS_ENSURE_ARG_POINTER(aURI);
    NS_PRECONDITION(aResult, "Null out param");
    
#ifdef DEBUG
    
    nsresult debug_rv;
    nsCOMPtr<nsIURI> debugClone;
    debug_rv = aURI->Clone(getter_AddRefs(debugClone));
    if (NS_SUCCEEDED(debug_rv)) {
        nsCOMPtr<nsIURL> debugURL (do_QueryInterface(debugClone));
        debug_rv = nsChromeRegistry::Canonify(debugURL);
        if (NS_SUCCEEDED(debug_rv)) {
            PRBool same;
            debug_rv = aURI->Equals(debugURL, &same);
            if (NS_SUCCEEDED(debug_rv)) {
                NS_ASSERTION(same, "Non-canonified chrome uri passed to nsChromeProtocolHandler::NewChannel!");
            }
        }
    }
#endif

    nsCOMPtr<nsIChannel> result;

#ifdef MOZ_XUL
    
    
    nsCOMPtr<nsIXULPrototypeCache> cache
        (do_GetService(kXULPrototypeCacheCID));

    PRBool isCached = PR_FALSE;
    if (cache)
        isCached = cache->IsCached(aURI);
    else
        NS_WARNING("Unable to obtain the XUL prototype cache!");

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    if (isCached) {
        
        
        result = new nsCachedChromeChannel(aURI);
        if (! result)
            return NS_ERROR_OUT_OF_MEMORY;
    }
    else {
#endif






        if (!nsChromeRegistry::gChromeRegistry) {
            
            
            nsCOMPtr<nsIChromeRegistry> reg (do_GetService(NS_CHROMEREGISTRY_CONTRACTID));
        }

        NS_ENSURE_TRUE(nsChromeRegistry::gChromeRegistry, NS_ERROR_FAILURE);

        nsCOMPtr<nsIURI> resolvedURI;
        rv = nsChromeRegistry::gChromeRegistry->ConvertChromeURL(aURI, getter_AddRefs(resolvedURI));
        if (NS_FAILED(rv)) {
#ifdef DEBUG
            nsCAutoString spec;
            aURI->GetSpec(spec);
            printf("Couldn't convert chrome URL: %s\n", spec.get());
#endif
            return rv;
        }

        nsCOMPtr<nsIIOService> ioServ (do_GetIOService(&rv));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = ioServ->NewChannelFromURI(resolvedURI, getter_AddRefs(result));
        if (NS_FAILED(rv)) return rv;

        
        nsCOMPtr<nsIFileChannel> fileChan
            (do_QueryInterface(result));
        if (fileChan) {
#ifdef DEBUG
            nsCOMPtr<nsIFile> file;
            fileChan->GetFile(getter_AddRefs(file));

            PRBool exists = PR_FALSE;
            file->Exists(&exists);
            if (!exists) {
                nsCAutoString path;
                file->GetNativePath(path);
                printf("Chrome file doesn't exist: %s\n", path.get());
            }
#endif
        }
        else {
            nsCOMPtr<nsIJARChannel> jarChan
                (do_QueryInterface(result));
            if (!jarChan) {
                NS_WARNING("Remote chrome not allowed! Only file:, resource:, and jar: are valid.\n");
                result = nsnull;
                return NS_ERROR_FAILURE;
            }
        }

        
        
        rv = result->SetOriginalURI(aURI);
        if (NS_FAILED(rv)) return rv;

        
        
        nsCOMPtr<nsIURL> url = do_QueryInterface(aURI);
        nsCAutoString path;
        rv = url->GetPath(path);
        if (StringBeginsWith(path, NS_LITERAL_CSTRING("/content/")))
        {
            nsCOMPtr<nsIScriptSecurityManager> securityManager =
                     do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
            if (NS_FAILED(rv)) return rv;

            nsCOMPtr<nsIPrincipal> principal;
            rv = securityManager->GetSystemPrincipal(getter_AddRefs(principal));
            if (NS_FAILED(rv)) return rv;

            nsCOMPtr<nsISupports> owner = do_QueryInterface(principal);
            result->SetOwner(owner);
        }

#ifdef MOZ_XUL
        
        
        
        
        
        
        
        
        
        nsCOMPtr<nsIFastLoadService> fastLoadServ(do_GetFastLoadService());
        if (fastLoadServ) {
            nsCOMPtr<nsIObjectOutputStream> objectOutput;
            fastLoadServ->GetOutputStream(getter_AddRefs(objectOutput));
            if (objectOutput) {
                nsCOMPtr<nsIFile> file;

                if (fileChan) {
                    fileChan->GetFile(getter_AddRefs(file));
                } else {
                    nsCOMPtr<nsIURI> uri;
                    result->GetURI(getter_AddRefs(uri));

                    
                    
                    nsCOMPtr<nsIJARURI> jarURI;
                    while ((jarURI = do_QueryInterface(uri)) != nsnull)
                        jarURI->GetJARFile(getter_AddRefs(uri));

                    
                    
                    nsCOMPtr<nsIFileURL> fileURL(do_QueryInterface(uri));
                    if (fileURL)
                        fileURL->GetFile(getter_AddRefs(file));
                }

                if (file) {
                    rv = fastLoadServ->AddDependency(file);
                    if (NS_FAILED(rv))
                        cache->AbortFastLoads();
                }
            }
        }
    }
#endif

    *aResult = result;
    NS_ADDREF(*aResult);
    return NS_OK;
}


