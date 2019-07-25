





































#include "nsChannelClassifier.h"

#include "nsNetUtil.h"
#include "nsIChannel.h"
#include "nsIProtocolHandler.h"
#include "nsICachingChannel.h"
#include "nsICacheEntryDescriptor.h"
#include "prlog.h"

#if defined(PR_LOGGING)



static PRLogModuleInfo *gChannelClassifierLog;
#endif
#define LOG(args)     PR_LOG(gChannelClassifierLog, PR_LOG_DEBUG, args)

NS_IMPL_ISUPPORTS1(nsChannelClassifier,
                   nsIURIClassifierCallback)

nsChannelClassifier::nsChannelClassifier()
{
#if defined(PR_LOGGING)
    if (!gChannelClassifierLog)
        gChannelClassifierLog = PR_NewLogModule("nsChannelClassifier");
#endif
}

nsresult
nsChannelClassifier::Start(nsIChannel *aChannel)
{
    
    
    PRUint32 status;
    aChannel->GetStatus(&status);
    if (NS_FAILED(status))
        return NS_OK;

    
    
    if (HasBeenClassified(aChannel)) {
        return NS_OK;
    }

    nsCOMPtr<nsIURI> uri;
    nsresult rv = aChannel->GetURI(getter_AddRefs(uri));
    NS_ENSURE_SUCCESS(rv, rv);

    
    bool hasFlags;
    rv = NS_URIChainHasFlags(uri,
                             nsIProtocolHandler::URI_DANGEROUS_TO_LOAD,
                             &hasFlags);
    NS_ENSURE_SUCCESS(rv, rv);
    if (hasFlags) return NS_OK;

    rv = NS_URIChainHasFlags(uri,
                             nsIProtocolHandler::URI_IS_LOCAL_FILE,
                             &hasFlags);
    NS_ENSURE_SUCCESS(rv, rv);
    if (hasFlags) return NS_OK;

    rv = NS_URIChainHasFlags(uri,
                             nsIProtocolHandler::URI_IS_UI_RESOURCE,
                             &hasFlags);
    NS_ENSURE_SUCCESS(rv, rv);
    if (hasFlags) return NS_OK;

    rv = NS_URIChainHasFlags(uri,
                             nsIProtocolHandler::URI_IS_LOCAL_RESOURCE,
                             &hasFlags);
    NS_ENSURE_SUCCESS(rv, rv);
    if (hasFlags) return NS_OK;

    nsCOMPtr<nsIURIClassifier> uriClassifier =
        do_GetService(NS_URICLASSIFIERSERVICE_CONTRACTID, &rv);
    if (rv == NS_ERROR_FACTORY_NOT_REGISTERED ||
        rv == NS_ERROR_NOT_AVAILABLE) {
        
        return NS_OK;
    }
    NS_ENSURE_SUCCESS(rv, rv);

    bool expectCallback;
    rv = uriClassifier->Classify(uri, this, &expectCallback);
    if (NS_FAILED(rv)) return rv;

    if (expectCallback) {
        
        
        rv = aChannel->Suspend();
        if (NS_FAILED(rv)) {
            
            
            
            return NS_OK;
        }

        mSuspendedChannel = aChannel;
#ifdef DEBUG
        LOG(("nsChannelClassifier[%p]: suspended channel %p",
             this, mSuspendedChannel.get()));
#endif
    }

    return NS_OK;
}



void
nsChannelClassifier::MarkEntryClassified(nsresult status)
{
    nsCOMPtr<nsICachingChannel> cachingChannel =
        do_QueryInterface(mSuspendedChannel);
    if (!cachingChannel) {
        return;
    }

    nsCOMPtr<nsISupports> cacheToken;
    cachingChannel->GetCacheToken(getter_AddRefs(cacheToken));
    if (!cacheToken) {
        return;
    }

    nsCOMPtr<nsICacheEntryDescriptor> cacheEntry =
        do_QueryInterface(cacheToken);
    if (!cacheEntry) {
        return;
    }

    cacheEntry->SetMetaDataElement("necko:classified",
                                   NS_SUCCEEDED(status) ? "1" : nsnull);
}

bool
nsChannelClassifier::HasBeenClassified(nsIChannel *aChannel)
{
    nsCOMPtr<nsICachingChannel> cachingChannel =
        do_QueryInterface(aChannel);
    if (!cachingChannel) {
        return false;
    }

    
    
    bool fromCache;
    if (NS_FAILED(cachingChannel->IsFromCache(&fromCache)) || !fromCache) {
        return false;
    }

    nsCOMPtr<nsISupports> cacheToken;
    cachingChannel->GetCacheToken(getter_AddRefs(cacheToken));
    if (!cacheToken) {
        return false;
    }

    nsCOMPtr<nsICacheEntryDescriptor> cacheEntry =
        do_QueryInterface(cacheToken);
    if (!cacheEntry) {
        return false;
    }

    nsXPIDLCString tag;
    cacheEntry->GetMetaDataElement("necko:classified", getter_Copies(tag));
    return tag.EqualsLiteral("1");
}

NS_IMETHODIMP
nsChannelClassifier::OnClassifyComplete(nsresult aErrorCode)
{
    if (mSuspendedChannel) {
        MarkEntryClassified(aErrorCode);

        if (NS_FAILED(aErrorCode)) {
#ifdef DEBUG
            LOG(("nsChannelClassifier[%p]: cancelling channel %p with error "
                 "code: %x", this, mSuspendedChannel.get(), aErrorCode));
#endif
            mSuspendedChannel->Cancel(aErrorCode);
        }
#ifdef DEBUG
        LOG(("nsChannelClassifier[%p]: resuming channel %p from "
             "OnClassifyComplete", this, mSuspendedChannel.get()));
#endif
        mSuspendedChannel->Resume();
        mSuspendedChannel = nsnull;
    }

    return NS_OK;
}
