





#include "nsChannelClassifier.h"

#include "mozIThirdPartyUtil.h"
#include "nsContentUtils.h"
#include "nsNetUtil.h"
#include "nsICacheEntry.h"
#include "nsICachingChannel.h"
#include "nsIChannel.h"
#include "nsIDocShell.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMWindow.h"
#include "nsIHttpChannelInternal.h"
#include "nsIIOService.h"
#include "nsIParentChannel.h"
#include "nsIPermissionManager.h"
#include "nsIProtocolHandler.h"
#include "nsIScriptError.h"
#include "nsIScriptSecurityManager.h"
#include "nsISecureBrowserUI.h"
#include "nsISecurityEventSink.h"
#include "nsIWebProgressListener.h"
#include "nsPIDOMWindow.h"
#include "nsXULAppAPI.h"

#include "mozilla/Preferences.h"

#include "mozilla/Logging.h"

using mozilla::ArrayLength;
using mozilla::Preferences;




static PRLogModuleInfo *gChannelClassifierLog;
#undef LOG
#define LOG(args)     MOZ_LOG(gChannelClassifierLog, mozilla::LogLevel::Debug, args)

NS_IMPL_ISUPPORTS(nsChannelClassifier,
                  nsIURIClassifierCallback)

nsChannelClassifier::nsChannelClassifier()
  : mIsAllowListed(false),
    mSuspendedChannel(false)
{
    if (!gChannelClassifierLog)
        gChannelClassifierLog = PR_NewLogModule("nsChannelClassifier");
}

nsresult
nsChannelClassifier::ShouldEnableTrackingProtection(nsIChannel *aChannel,
                                                    bool *result)
{
    
    MOZ_ASSERT(XRE_IsParentProcess());

    NS_ENSURE_ARG(result);
    *result = false;

    if (!Preferences::GetBool("privacy.trackingprotection.enabled", false) &&
        (!Preferences::GetBool("privacy.trackingprotection.pbmode.enabled",
                               false) || !NS_UsePrivateBrowsing(aChannel))) {
      return NS_OK;
    }

    nsresult rv;
    nsCOMPtr<mozIThirdPartyUtil> thirdPartyUtil =
        do_GetService(THIRDPARTYUTIL_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIHttpChannelInternal> chan = do_QueryInterface(aChannel, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIURI> topWinURI;
    rv = chan->GetTopWindowURI(getter_AddRefs(topWinURI));
    NS_ENSURE_SUCCESS(rv, rv);

    if (!topWinURI) {
      LOG(("nsChannelClassifier[%p]: No window URI\n", this));
    }

    nsCOMPtr<nsIURI> chanURI;
    rv = aChannel->GetURI(getter_AddRefs(chanURI));
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    
    
    
    
    
    bool isThirdPartyChannel = true;
    bool isThirdPartyWindow = true;
    thirdPartyUtil->IsThirdPartyURI(chanURI, topWinURI, &isThirdPartyWindow);
    thirdPartyUtil->IsThirdPartyChannel(aChannel, nullptr, &isThirdPartyChannel);
    if (!isThirdPartyWindow || !isThirdPartyChannel) {
        *result = false;
#ifdef DEBUG
        nsCString spec;
        chanURI->GetSpec(spec);
        LOG(("nsChannelClassifier[%p]: Skipping tracking protection checks for "
             "first party or top-level load channel[%p] with uri %s", this, aChannel,
             spec.get()));
#endif
        return NS_OK;
    }

    nsCOMPtr<nsIIOService> ios = do_GetService(NS_IOSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    const char ALLOWLIST_EXAMPLE_PREF[] = "channelclassifier.allowlist_example";
    if (!topWinURI && Preferences::GetBool(ALLOWLIST_EXAMPLE_PREF, false)) {
      LOG(("nsChannelClassifier[%p]: Allowlisting test domain\n", this));
      rv = ios->NewURI(NS_LITERAL_CSTRING("http://allowlisted.example.com"),
                       nullptr, nullptr, getter_AddRefs(topWinURI));
      NS_ENSURE_SUCCESS(rv, rv);
    }

    
    
    
    nsCOMPtr<nsIURL> url = do_QueryInterface(topWinURI, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCString escaped(NS_LITERAL_CSTRING("https://"));
    nsAutoCString temp;
    rv = url->GetHostPort(temp);
    NS_ENSURE_SUCCESS(rv, rv);
    escaped.Append(temp);

    
    rv = ios->NewURI(escaped, nullptr, nullptr, getter_AddRefs(topWinURI));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIPermissionManager> permMgr =
        do_GetService(NS_PERMISSIONMANAGER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    uint32_t permissions = nsIPermissionManager::UNKNOWN_ACTION;
    rv = permMgr->TestPermission(topWinURI, "trackingprotection", &permissions);
    NS_ENSURE_SUCCESS(rv, rv);

#ifdef DEBUG
    if (permissions == nsIPermissionManager::ALLOW_ACTION) {
        LOG(("nsChannelClassifier[%p]: Allowlisting channel[%p] for %s", this,
             aChannel, escaped.get()));
    }
#endif

    if (permissions == nsIPermissionManager::ALLOW_ACTION) {
      mIsAllowListed = true;
      *result = false;
    } else {
      *result = true;
    }

    
    
    
    if (*result) {
#ifdef DEBUG
      nsCString topspec;
      nsCString spec;
      topWinURI->GetSpec(topspec);
      chanURI->GetSpec(spec);
      LOG(("nsChannelClassifier[%p]: Enabling tracking protection checks on channel[%p] "
           "with uri %s for toplevel window %s", this, aChannel, spec.get(),
           topspec.get()));
#endif
      return NS_OK;
    }

    
    
    
    
    return NotifyTrackingProtectionDisabled(aChannel);
}


nsresult
nsChannelClassifier::NotifyTrackingProtectionDisabled(nsIChannel *aChannel)
{
    
    nsCOMPtr<nsIParentChannel> parentChannel;
    NS_QueryNotificationCallbacks(aChannel, parentChannel);
    if (parentChannel) {
      
      
      parentChannel->NotifyTrackingProtectionDisabled();
      return NS_OK;
    }

    nsresult rv;
    nsCOMPtr<mozIThirdPartyUtil> thirdPartyUtil =
        do_GetService(THIRDPARTYUTIL_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMWindow> win;
    rv = thirdPartyUtil->GetTopWindowForChannel(aChannel, getter_AddRefs(win));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsPIDOMWindow> pwin = do_QueryInterface(win, &rv);
    NS_ENSURE_SUCCESS(rv, NS_OK);
    nsCOMPtr<nsIDocShell> docShell = pwin->GetDocShell();
    if (!docShell) {
      return NS_OK;
    }
    nsCOMPtr<nsIDocument> doc = do_GetInterface(docShell, &rv);
    NS_ENSURE_SUCCESS(rv, NS_OK);

    
    
    nsCOMPtr<nsISecurityEventSink> eventSink = do_QueryInterface(docShell, &rv);
    NS_ENSURE_SUCCESS(rv, NS_OK);
    uint32_t state = 0;
    nsCOMPtr<nsISecureBrowserUI> securityUI;
    docShell->GetSecurityUI(getter_AddRefs(securityUI));
    if (!securityUI) {
      return NS_OK;
    }
    doc->SetHasTrackingContentLoaded(true);
    securityUI->GetState(&state);
    state |= nsIWebProgressListener::STATE_LOADED_TRACKING_CONTENT;
    eventSink->OnSecurityChange(nullptr, state);

    return NS_OK;
}

void
nsChannelClassifier::Start(nsIChannel *aChannel)
{
  mChannel = aChannel;

  nsresult rv = StartInternal();
  if (NS_FAILED(rv)) {
    
    
    OnClassifyComplete(NS_OK);
  }
}

nsresult
nsChannelClassifier::StartInternal()
{
    
    MOZ_ASSERT(XRE_IsParentProcess());

    
    
    nsresult status;
    mChannel->GetStatus(&status);
    if (NS_FAILED(status))
        return status;

    
    
    if (HasBeenClassified(mChannel)) {
        return NS_ERROR_UNEXPECTED;
    }

    nsCOMPtr<nsIURI> uri;
    nsresult rv = mChannel->GetURI(getter_AddRefs(uri));
    NS_ENSURE_SUCCESS(rv, rv);

    
    bool hasFlags;
    rv = NS_URIChainHasFlags(uri,
                             nsIProtocolHandler::URI_DANGEROUS_TO_LOAD,
                             &hasFlags);
    NS_ENSURE_SUCCESS(rv, rv);
    if (hasFlags) return NS_ERROR_UNEXPECTED;

    rv = NS_URIChainHasFlags(uri,
                             nsIProtocolHandler::URI_IS_LOCAL_FILE,
                             &hasFlags);
    NS_ENSURE_SUCCESS(rv, rv);
    if (hasFlags) return NS_ERROR_UNEXPECTED;

    rv = NS_URIChainHasFlags(uri,
                             nsIProtocolHandler::URI_IS_UI_RESOURCE,
                             &hasFlags);
    NS_ENSURE_SUCCESS(rv, rv);
    if (hasFlags) return NS_ERROR_UNEXPECTED;

    rv = NS_URIChainHasFlags(uri,
                             nsIProtocolHandler::URI_IS_LOCAL_RESOURCE,
                             &hasFlags);
    NS_ENSURE_SUCCESS(rv, rv);
    if (hasFlags) return NS_ERROR_UNEXPECTED;

    nsCOMPtr<nsIURIClassifier> uriClassifier =
        do_GetService(NS_URICLASSIFIERSERVICE_CONTRACTID, &rv);
    if (rv == NS_ERROR_FACTORY_NOT_REGISTERED ||
        rv == NS_ERROR_NOT_AVAILABLE) {
        
        return NS_ERROR_NOT_AVAILABLE;
    }
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIScriptSecurityManager> securityManager =
        do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIPrincipal> principal;
    rv = securityManager->GetChannelURIPrincipal(mChannel, getter_AddRefs(principal));
    NS_ENSURE_SUCCESS(rv, rv);

    bool expectCallback;
    bool trackingProtectionEnabled = false;
    (void)ShouldEnableTrackingProtection(mChannel, &trackingProtectionEnabled);

#ifdef DEBUG
    {
      nsCString uriSpec;
      uri->GetSpec(uriSpec);
      nsCOMPtr<nsIURI> principalURI;
      principal->GetURI(getter_AddRefs(principalURI));
      nsCString principalSpec;
      principalURI->GetSpec(principalSpec);
      LOG(("nsChannelClassifier: Classifying principal %s on channel with uri %s "
           "[this=%p]", principalSpec.get(), uriSpec.get(), this));
    }
#endif
    rv = uriClassifier->Classify(principal, trackingProtectionEnabled, this,
                                 &expectCallback);
    if (NS_FAILED(rv)) {
        return rv;
    }

    if (expectCallback) {
        
        
        rv = mChannel->Suspend();
        if (NS_FAILED(rv)) {
            
            
            
            LOG(("nsChannelClassifier[%p]: Couldn't suspend channel", this));
            return rv;
        }

        mSuspendedChannel = true;
        LOG(("nsChannelClassifier[%p]: suspended channel %p",
             this, mChannel.get()));
    } else {
        LOG(("nsChannelClassifier[%p]: not expecting callback", this));
        return NS_ERROR_FAILURE;
    }

    return NS_OK;
}



void
nsChannelClassifier::MarkEntryClassified(nsresult status)
{
    
    MOZ_ASSERT(XRE_IsParentProcess());

    
    if (status == NS_ERROR_TRACKING_URI || mIsAllowListed) {
        return;
    }

    nsCOMPtr<nsICachingChannel> cachingChannel = do_QueryInterface(mChannel);
    if (!cachingChannel) {
        return;
    }

    nsCOMPtr<nsISupports> cacheToken;
    cachingChannel->GetCacheToken(getter_AddRefs(cacheToken));
    if (!cacheToken) {
        return;
    }

    nsCOMPtr<nsICacheEntry> cacheEntry =
        do_QueryInterface(cacheToken);
    if (!cacheEntry) {
        return;
    }

    cacheEntry->SetMetaDataElement("necko:classified",
                                   NS_SUCCEEDED(status) ? "1" : nullptr);
}

bool
nsChannelClassifier::HasBeenClassified(nsIChannel *aChannel)
{
    
    MOZ_ASSERT(XRE_IsParentProcess());

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

    nsCOMPtr<nsICacheEntry> cacheEntry =
        do_QueryInterface(cacheToken);
    if (!cacheEntry) {
        return false;
    }

    nsXPIDLCString tag;
    cacheEntry->GetMetaDataElement("necko:classified", getter_Copies(tag));
    return tag.EqualsLiteral("1");
}


nsresult
nsChannelClassifier::SetBlockedTrackingContent(nsIChannel *channel)
{
  
  nsCOMPtr<nsIParentChannel> parentChannel;
  NS_QueryNotificationCallbacks(channel, parentChannel);
  if (parentChannel) {
    
    
    
    return NS_OK;
  }

  nsresult rv;
  nsCOMPtr<nsIDOMWindow> win;
  nsCOMPtr<mozIThirdPartyUtil> thirdPartyUtil =
    do_GetService(THIRDPARTYUTIL_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, NS_OK);
  rv = thirdPartyUtil->GetTopWindowForChannel(channel, getter_AddRefs(win));
  NS_ENSURE_SUCCESS(rv, NS_OK);
  nsCOMPtr<nsPIDOMWindow> pwin = do_QueryInterface(win, &rv);
  NS_ENSURE_SUCCESS(rv, NS_OK);
  nsCOMPtr<nsIDocShell> docShell = pwin->GetDocShell();
  if (!docShell) {
    return NS_OK;
  }
  nsCOMPtr<nsIDocument> doc = do_GetInterface(docShell, &rv);
  NS_ENSURE_SUCCESS(rv, NS_OK);

  
  
  nsCOMPtr<nsISecurityEventSink> eventSink = do_QueryInterface(docShell, &rv);
  NS_ENSURE_SUCCESS(rv, NS_OK);
  uint32_t state = 0;
  nsCOMPtr<nsISecureBrowserUI> securityUI;
  docShell->GetSecurityUI(getter_AddRefs(securityUI));
  if (!securityUI) {
    return NS_OK;
  }
  doc->SetHasTrackingContentBlocked(true);
  securityUI->GetState(&state);
  state |= nsIWebProgressListener::STATE_BLOCKED_TRACKING_CONTENT;
  eventSink->OnSecurityChange(nullptr, state);

  
  nsCOMPtr<nsIURI> uri;
  channel->GetURI(getter_AddRefs(uri));
  nsCString utf8spec;
  uri->GetSpec(utf8spec);
  NS_ConvertUTF8toUTF16 spec(utf8spec);
  const char16_t* params[] = { spec.get() };
  nsContentUtils::ReportToConsole(nsIScriptError::warningFlag,
                                  NS_LITERAL_CSTRING("Tracking Protection"),
                                  doc,
                                  nsContentUtils::eNECKO_PROPERTIES,
                                  "TrackingUriBlocked",
                                  params, ArrayLength(params));

  return NS_OK;
}

NS_IMETHODIMP
nsChannelClassifier::OnClassifyComplete(nsresult aErrorCode)
{
    
    MOZ_ASSERT(XRE_IsParentProcess());

    LOG(("nsChannelClassifier[%p]:OnClassifyComplete %d", this, aErrorCode));
    if (mSuspendedChannel) {
        MarkEntryClassified(aErrorCode);

        if (NS_FAILED(aErrorCode)) {
#ifdef DEBUG
            nsCOMPtr<nsIURI> uri;
            mChannel->GetURI(getter_AddRefs(uri));
            nsCString spec;
            uri->GetSpec(spec);
            LOG(("nsChannelClassifier[%p]: cancelling channel %p for %s "
                 "with error code: %x", this, mChannel.get(),
                 spec.get(), aErrorCode));
#endif

            
            
            
            if (aErrorCode == NS_ERROR_TRACKING_URI) {
              SetBlockedTrackingContent(mChannel);
            }

            mChannel->Cancel(aErrorCode);
        }
        LOG(("nsChannelClassifier[%p]: resuming channel %p from "
             "OnClassifyComplete", this, mChannel.get()));
        mChannel->Resume();
    }

    mChannel = nullptr;

    return NS_OK;
}
