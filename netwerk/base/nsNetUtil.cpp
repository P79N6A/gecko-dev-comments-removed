





#include "mozilla/LoadContext.h"
#include "mozilla/LoadInfo.h"
#include "nsNetUtil.h"
#include "nsNetUtil.inl"
#include "mozIApplicationClearPrivateDataParams.h"
#include "nsCategoryCache.h"
#include "nsHashKeys.h"
#include "nsHttp.h"
#include "nsIAsyncStreamCopier.h"
#include "nsIAuthPrompt.h"
#include "nsIAuthPrompt2.h"
#include "nsIAuthPromptAdapterFactory.h"
#include "nsIBufferedStreams.h"
#include "nsIChannelEventSink.h"
#include "nsIContentSniffer.h"
#include "nsIDownloader.h"
#include "nsIFileProtocolHandler.h"
#include "nsIFileStreams.h"
#include "nsIFileURL.h"
#include "nsIIDNService.h"
#include "nsIInputStreamChannel.h"
#include "nsIInputStreamPump.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsILoadContext.h"
#include "nsIMIMEHeaderParam.h"
#include "nsIMutable.h"
#include "nsIOfflineCacheUpdate.h"
#include "nsIPersistentProperties2.h"
#include "nsIPrivateBrowsingChannel.h"
#include "nsIPropertyBag2.h"
#include "nsIProtocolProxyService.h"
#include "nsIRedirectChannelRegistrar.h"
#include "nsIRequestObserverProxy.h"
#include "nsIScriptSecurityManager.h"
#include "nsISimpleStreamListener.h"
#include "nsISocketProvider.h"
#include "nsISocketProviderService.h"
#include "nsIStandardURL.h"
#include "nsIStreamLoader.h"
#include "nsIStreamTransportService.h"
#include "nsStringStream.h"
#include "nsISyncStreamListener.h"
#include "nsITransport.h"
#include "nsIUnicharStreamLoader.h"
#include "nsIURIWithPrincipal.h"
#include "nsIURLParser.h"
#include "nsIUUIDGenerator.h"
#include "nsIViewSourceChannel.h"
#include "nsInterfaceRequestorAgg.h"
#include "plstr.h"
#include "nsINestedURI.h"

#ifdef MOZ_WIDGET_GONK
#include "nsINetworkManager.h"
#include "nsThreadUtils.h" 
#endif

#include <limits>

nsresult 
NS_NewChannelWithTriggeringPrincipal(nsIChannel           **outChannel,
                                     nsIURI                *aUri,
                                     nsINode               *aLoadingNode,
                                     nsIPrincipal          *aTriggeringPrincipal,
                                     nsSecurityFlags        aSecurityFlags,
                                     nsContentPolicyType    aContentPolicyType,
                                     nsILoadGroup          *aLoadGroup ,
                                     nsIInterfaceRequestor *aCallbacks ,
                                     nsLoadFlags            aLoadFlags ,
                                     nsIIOService          *aIoService )
{
  MOZ_ASSERT(aLoadingNode);
  NS_ASSERTION(aTriggeringPrincipal, "Can not create channel without a triggering Principal!");
  return NS_NewChannelInternal(outChannel,
                               aUri,
                               aLoadingNode,
                               aLoadingNode->NodePrincipal(),
                               aTriggeringPrincipal,
                               aSecurityFlags,
                               aContentPolicyType,
                               aLoadGroup,
                               aCallbacks,
                               aLoadFlags,
                               aIoService);
}


nsresult 
NS_NewChannelWithTriggeringPrincipal(nsIChannel           **outChannel,
                                     nsIURI                *aUri,
                                     nsIPrincipal          *aLoadingPrincipal,
                                     nsIPrincipal          *aTriggeringPrincipal,
                                     nsSecurityFlags        aSecurityFlags,
                                     nsContentPolicyType    aContentPolicyType,
                                     nsILoadGroup          *aLoadGroup ,
                                     nsIInterfaceRequestor *aCallbacks ,
                                     nsLoadFlags            aLoadFlags ,
                                     nsIIOService          *aIoService )
{
  NS_ASSERTION(aLoadingPrincipal, "Can not create channel without a loading Principal!");
  return NS_NewChannelInternal(outChannel,
                               aUri,
                               nullptr, 
                               aLoadingPrincipal,
                               aTriggeringPrincipal,
                               aSecurityFlags,
                               aContentPolicyType,
                               aLoadGroup,
                               aCallbacks,
                               aLoadFlags,
                               aIoService);
}

nsresult
NS_NewFileURI(nsIURI **result,
              nsIFile *spec,
              nsIIOService *ioService )     
{
    nsresult rv;
    nsCOMPtr<nsIIOService> grip;
    rv = net_EnsureIOService(&ioService, grip);
    if (ioService)
        rv = ioService->NewFileURI(spec, result);
    return rv;
}

nsresult 
NS_NewChannel(nsIChannel           **outChannel,
              nsIURI                *aUri,
              nsINode               *aLoadingNode,
              nsSecurityFlags        aSecurityFlags,
              nsContentPolicyType    aContentPolicyType,
              nsILoadGroup          *aLoadGroup ,
              nsIInterfaceRequestor *aCallbacks ,
              nsLoadFlags            aLoadFlags ,
              nsIIOService          *aIoService )
{
  NS_ASSERTION(aLoadingNode, "Can not create channel without a loading Node!");
  return NS_NewChannelInternal(outChannel,
                               aUri,
                               aLoadingNode,
                               aLoadingNode->NodePrincipal(),
                               nullptr, 
                               aSecurityFlags,
                               aContentPolicyType,
                               aLoadGroup,
                               aCallbacks,
                               aLoadFlags,
                               aIoService);
}

nsresult
NS_MakeAbsoluteURI(nsACString       &result,
                   const nsACString &spec,
                   nsIURI           *baseURI)
{
    nsresult rv;
    if (!baseURI) {
        NS_WARNING("It doesn't make sense to not supply a base URI");
        result = spec;
        rv = NS_OK;
    }
    else if (spec.IsEmpty())
        rv = baseURI->GetSpec(result);
    else
        rv = baseURI->Resolve(spec, result);
    return rv;
}

nsresult
NS_MakeAbsoluteURI(char        **result,
                   const char   *spec,
                   nsIURI       *baseURI)
{
    nsresult rv;
    nsAutoCString resultBuf;
    rv = NS_MakeAbsoluteURI(resultBuf, nsDependentCString(spec), baseURI);
    if (NS_SUCCEEDED(rv)) {
        *result = ToNewCString(resultBuf);
        if (!*result)
            rv = NS_ERROR_OUT_OF_MEMORY;
    }
    return rv;
}

nsresult
NS_MakeAbsoluteURI(nsAString       &result,
                   const nsAString &spec,
                   nsIURI          *baseURI)
{
    nsresult rv;
    if (!baseURI) {
        NS_WARNING("It doesn't make sense to not supply a base URI");
        result = spec;
        rv = NS_OK;
    }
    else {
        nsAutoCString resultBuf;
        if (spec.IsEmpty())
            rv = baseURI->GetSpec(resultBuf);
        else
            rv = baseURI->Resolve(NS_ConvertUTF16toUTF8(spec), resultBuf);
        if (NS_SUCCEEDED(rv))
            CopyUTF8toUTF16(resultBuf, result);
    }
    return rv;
}

int32_t
NS_GetDefaultPort(const char *scheme,
                  nsIIOService *ioService )
{
  nsresult rv;

  nsCOMPtr<nsIIOService> grip;
  net_EnsureIOService(&ioService, grip);
  if (!ioService)
      return -1;

  nsCOMPtr<nsIProtocolHandler> handler;
  rv = ioService->GetProtocolHandler(scheme, getter_AddRefs(handler));
  if (NS_FAILED(rv))
    return -1;
  int32_t port;
  rv = handler->GetDefaultPort(&port);
  return NS_SUCCEEDED(rv) ? port : -1;
}





bool
NS_StringToACE(const nsACString &idn, nsACString &result)
{
  nsCOMPtr<nsIIDNService> idnSrv = do_GetService(NS_IDNSERVICE_CONTRACTID);
  if (!idnSrv)
    return false;
  nsresult rv = idnSrv->ConvertUTF8toACE(idn, result);
  if (NS_FAILED(rv))
    return false;

  return true;
}

int32_t
NS_GetRealPort(nsIURI *aURI)
{
    int32_t port;
    nsresult rv = aURI->GetPort(&port);
    if (NS_FAILED(rv))
        return -1;

    if (port != -1)
        return port; 

    

    
    nsAutoCString scheme;
    rv = aURI->GetScheme(scheme);
    if (NS_FAILED(rv))
        return -1;

    return NS_GetDefaultPort(scheme.get());
}

nsresult 
NS_NewInputStreamChannelInternal(nsIChannel        **outChannel,
                                 nsIURI             *aUri,
                                 nsIInputStream     *aStream,
                                 const nsACString   &aContentType,
                                 const nsACString   &aContentCharset,
                                 nsILoadInfo        *aLoadInfo)
{
  nsresult rv;
  nsCOMPtr<nsIInputStreamChannel> isc =
    do_CreateInstance(NS_INPUTSTREAMCHANNEL_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = isc->SetURI(aUri);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = isc->SetContentStream(aStream);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIChannel> channel = do_QueryInterface(isc, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aContentType.IsEmpty()) {
    rv = channel->SetContentType(aContentType);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (!aContentCharset.IsEmpty()) {
    rv = channel->SetContentCharset(aContentCharset);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  channel->SetLoadInfo(aLoadInfo);

  
  
  if (aLoadInfo && aLoadInfo->GetLoadingSandboxed()) {
    channel->SetOwner(nullptr);
  }

  channel.forget(outChannel);
  return NS_OK;
}

nsresult
NS_NewInputStreamChannelInternal(nsIChannel        **outChannel,
                                 nsIURI             *aUri,
                                 nsIInputStream     *aStream,
                                 const nsACString   &aContentType,
                                 const nsACString   &aContentCharset,
                                 nsINode            *aLoadingNode,
                                 nsIPrincipal       *aLoadingPrincipal,
                                 nsIPrincipal       *aTriggeringPrincipal,
                                 nsSecurityFlags     aSecurityFlags,
                                 nsContentPolicyType aContentPolicyType,
                                 nsIURI             *aBaseURI )
{
  nsCOMPtr<nsILoadInfo> loadInfo =
    new mozilla::LoadInfo(aLoadingPrincipal,
                          aTriggeringPrincipal,
                          aLoadingNode,
                          aSecurityFlags,
                          aContentPolicyType,
                          aBaseURI);
  if (!loadInfo) {
    return NS_ERROR_UNEXPECTED;
  }
  return NS_NewInputStreamChannelInternal(outChannel,
                                          aUri,
                                          aStream,
                                          aContentType,
                                          aContentCharset,
                                          loadInfo);
}

nsresult 
NS_NewInputStreamChannel(nsIChannel        **outChannel,
                         nsIURI             *aUri,
                         nsIInputStream     *aStream,
                         nsIPrincipal       *aLoadingPrincipal,
                         nsSecurityFlags     aSecurityFlags,
                         nsContentPolicyType aContentPolicyType,
                         const nsACString   &aContentType    ,
                         const nsACString   &aContentCharset )
{
  return NS_NewInputStreamChannelInternal(outChannel,
                                          aUri,
                                          aStream,
                                          aContentType,
                                          aContentCharset,
                                          nullptr, 
                                          aLoadingPrincipal,
                                          nullptr, 
                                          aSecurityFlags,
                                          aContentPolicyType);
}

nsresult
NS_NewInputStreamChannelInternal(nsIChannel        **outChannel,
                                 nsIURI             *aUri,
                                 const nsAString    &aData,
                                 const nsACString   &aContentType,
                                 nsINode            *aLoadingNode,
                                 nsIPrincipal       *aLoadingPrincipal,
                                 nsIPrincipal       *aTriggeringPrincipal,
                                 nsSecurityFlags     aSecurityFlags,
                                 nsContentPolicyType aContentPolicyType,
                                 bool                aIsSrcdocChannel ,
                                 nsIURI             *aBaseURI )
{
  nsresult rv;
  nsCOMPtr<nsIStringInputStream> stream;
  stream = do_CreateInstance(NS_STRINGINPUTSTREAM_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef MOZILLA_INTERNAL_API
    uint32_t len;
    char* utf8Bytes = ToNewUTF8String(aData, &len);
    rv = stream->AdoptData(utf8Bytes, len);
#else
    char* utf8Bytes = ToNewUTF8String(aData);
    rv = stream->AdoptData(utf8Bytes, strlen(utf8Bytes));
#endif

  nsCOMPtr<nsIChannel> channel;
  rv = NS_NewInputStreamChannelInternal(getter_AddRefs(channel),
                                        aUri,
                                        stream,
                                        aContentType,
                                        NS_LITERAL_CSTRING("UTF-8"),
                                        aLoadingNode,
                                        aLoadingPrincipal,
                                        aTriggeringPrincipal,
                                        aSecurityFlags,
                                        aContentPolicyType,
                                        aBaseURI);

  NS_ENSURE_SUCCESS(rv, rv);

  if (aIsSrcdocChannel) {
    nsCOMPtr<nsIInputStreamChannel> inStrmChan = do_QueryInterface(channel);
    NS_ENSURE_TRUE(inStrmChan, NS_ERROR_FAILURE);
    inStrmChan->SetSrcdocData(aData);
  }
  channel.forget(outChannel);
  return NS_OK;
}

nsresult
NS_NewInputStreamChannel(nsIChannel        **outChannel,
                         nsIURI             *aUri,
                         const nsAString    &aData,
                         const nsACString   &aContentType,
                         nsIPrincipal       *aLoadingPrincipal,
                         nsSecurityFlags     aSecurityFlags,
                         nsContentPolicyType aContentPolicyType,
                         bool                aIsSrcdocChannel ,
                         nsIURI             *aBaseURI )
{
  return NS_NewInputStreamChannelInternal(outChannel,
                                          aUri,
                                          aData,
                                          aContentType,
                                          nullptr, 
                                          aLoadingPrincipal,
                                          nullptr, 
                                          aSecurityFlags,
                                          aContentPolicyType,
                                          aIsSrcdocChannel,
                                          aBaseURI);
}

nsresult
NS_NewInputStreamPump(nsIInputStreamPump **result,
                      nsIInputStream      *stream,
                      int64_t              streamPos ,
                      int64_t              streamLen ,
                      uint32_t             segsize ,
                      uint32_t             segcount ,
                      bool                 closeWhenDone )
{
    nsresult rv;
    nsCOMPtr<nsIInputStreamPump> pump =
        do_CreateInstance(NS_INPUTSTREAMPUMP_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv)) {
        rv = pump->Init(stream, streamPos, streamLen,
                        segsize, segcount, closeWhenDone);
        if (NS_SUCCEEDED(rv)) {
            *result = nullptr;
            pump.swap(*result);
        }
    }
    return rv;
}

nsresult
NS_NewAsyncStreamCopier(nsIAsyncStreamCopier **result,
                        nsIInputStream        *source,
                        nsIOutputStream       *sink,
                        nsIEventTarget        *target,
                        bool                   sourceBuffered ,
                        bool                   sinkBuffered ,
                        uint32_t               chunkSize ,
                        bool                   closeSource ,
                        bool                   closeSink )
{
    nsresult rv;
    nsCOMPtr<nsIAsyncStreamCopier> copier =
        do_CreateInstance(NS_ASYNCSTREAMCOPIER_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv)) {
        rv = copier->Init(source, sink, target, sourceBuffered, sinkBuffered,
                          chunkSize, closeSource, closeSink);
        if (NS_SUCCEEDED(rv)) {
            *result = nullptr;
            copier.swap(*result);
        }
    }
    return rv;
}

nsresult
NS_NewLoadGroup(nsILoadGroup      **result,
                nsIRequestObserver *obs)
{
    nsresult rv;
    nsCOMPtr<nsILoadGroup> group =
        do_CreateInstance(NS_LOADGROUP_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv)) {
        rv = group->SetGroupObserver(obs);
        if (NS_SUCCEEDED(rv)) {
            *result = nullptr;
            group.swap(*result);
        }
    }
    return rv;
}

bool NS_IsReasonableHTTPHeaderValue(const nsACString &aValue)
{
  return mozilla::net::nsHttp::IsReasonableHeaderValue(aValue);
}

bool NS_IsValidHTTPToken(const nsACString &aToken)
{
  return mozilla::net::nsHttp::IsValidToken(aToken);
}

nsresult
NS_NewLoadGroup(nsILoadGroup **aResult, nsIPrincipal *aPrincipal)
{
    using mozilla::LoadContext;
    nsresult rv;

    nsCOMPtr<nsILoadGroup> group =
        do_CreateInstance(NS_LOADGROUP_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsRefPtr<LoadContext> loadContext = new LoadContext(aPrincipal);
    rv = group->SetNotificationCallbacks(loadContext);
    NS_ENSURE_SUCCESS(rv, rv);

    group.forget(aResult);
    return rv;
}

bool
NS_LoadGroupMatchesPrincipal(nsILoadGroup *aLoadGroup,
                             nsIPrincipal *aPrincipal)
{
    if (!aPrincipal) {
      return false;
    }

    
    
    
    bool isNullPrincipal;
    nsresult rv = aPrincipal->GetIsNullPrincipal(&isNullPrincipal);
    NS_ENSURE_SUCCESS(rv, false);
    if (isNullPrincipal) {
      return true;
    }

    if (!aLoadGroup) {
        return false;
    }

    nsCOMPtr<nsILoadContext> loadContext;
    NS_QueryNotificationCallbacks(nullptr, aLoadGroup, NS_GET_IID(nsILoadContext),
                                  getter_AddRefs(loadContext));
    NS_ENSURE_TRUE(loadContext, false);

    
    uint32_t contextAppId;
    bool contextInBrowserElement;
    rv = loadContext->GetAppId(&contextAppId);
    NS_ENSURE_SUCCESS(rv, false);
    rv = loadContext->GetIsInBrowserElement(&contextInBrowserElement);
    NS_ENSURE_SUCCESS(rv, false);

    uint32_t principalAppId;
    bool principalInBrowserElement;
    rv = aPrincipal->GetAppId(&principalAppId);
    NS_ENSURE_SUCCESS(rv, false);
    rv = aPrincipal->GetIsInBrowserElement(&principalInBrowserElement);
    NS_ENSURE_SUCCESS(rv, false);

    return contextAppId == principalAppId &&
           contextInBrowserElement == principalInBrowserElement;
}

nsresult
NS_NewDownloader(nsIStreamListener   **result,
                 nsIDownloadObserver  *observer,
                 nsIFile              *downloadLocation )
{
    nsresult rv;
    nsCOMPtr<nsIDownloader> downloader =
        do_CreateInstance(NS_DOWNLOADER_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv)) {
        rv = downloader->Init(observer, downloadLocation);
        if (NS_SUCCEEDED(rv)) {
            downloader.forget(result);
        }
    }
    return rv;
}

nsresult
NS_NewStreamLoaderInternal(nsIStreamLoader        **outStream,
                           nsIURI                  *aUri,
                           nsIStreamLoaderObserver *aObserver,
                           nsINode                 *aLoadingNode,
                           nsIPrincipal            *aLoadingPrincipal,
                           nsSecurityFlags          aSecurityFlags,
                           nsContentPolicyType      aContentPolicyType,
                           nsISupports             *aContext ,
                           nsILoadGroup            *aLoadGroup ,
                           nsIInterfaceRequestor   *aCallbacks ,
                           nsLoadFlags              aLoadFlags ,
                           nsIURI                  *aReferrer )
{
   nsCOMPtr<nsIChannel> channel;
   nsresult rv = NS_NewChannelInternal(getter_AddRefs(channel),
                                       aUri,
                                       aLoadingNode,
                                       aLoadingPrincipal,
                                       nullptr, 
                                       aSecurityFlags,
                                       aContentPolicyType,
                                       aLoadGroup,
                                       aCallbacks,
                                       aLoadFlags);

  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel));
  if (httpChannel) {
    httpChannel->SetReferrer(aReferrer);
  }
  rv = NS_NewStreamLoader(outStream, aObserver);
  NS_ENSURE_SUCCESS(rv, rv);
  return channel->AsyncOpen(*outStream, aContext);
}


nsresult 
NS_NewStreamLoader(nsIStreamLoader        **outStream,
                   nsIURI                  *aUri,
                   nsIStreamLoaderObserver *aObserver,
                   nsINode                 *aLoadingNode,
                   nsSecurityFlags          aSecurityFlags,
                   nsContentPolicyType      aContentPolicyType,
                   nsISupports             *aContext ,
                   nsILoadGroup            *aLoadGroup ,
                   nsIInterfaceRequestor   *aCallbacks ,
                   nsLoadFlags              aLoadFlags ,
                   nsIURI                  *aReferrer )
{
  NS_ASSERTION(aLoadingNode, "Can not create stream loader without a loading Node!");
  return NS_NewStreamLoaderInternal(outStream,
                                    aUri,
                                    aObserver,
                                    aLoadingNode,
                                    aLoadingNode->NodePrincipal(),
                                    aSecurityFlags,
                                    aContentPolicyType,
                                    aContext,
                                    aLoadGroup,
                                    aCallbacks,
                                    aLoadFlags,
                                    aReferrer);
}

nsresult 
NS_NewStreamLoader(nsIStreamLoader        **outStream,
                   nsIURI                  *aUri,
                   nsIStreamLoaderObserver *aObserver,
                   nsIPrincipal            *aLoadingPrincipal,
                   nsSecurityFlags          aSecurityFlags,
                   nsContentPolicyType      aContentPolicyType,
                   nsISupports             *aContext ,
                   nsILoadGroup            *aLoadGroup ,
                   nsIInterfaceRequestor   *aCallbacks ,
                   nsLoadFlags              aLoadFlags ,
                   nsIURI                  *aReferrer )
{
  return NS_NewStreamLoaderInternal(outStream,
                                    aUri,
                                    aObserver,
                                    nullptr, 
                                    aLoadingPrincipal,
                                    aSecurityFlags,
                                    aContentPolicyType,
                                    aContext,
                                    aLoadGroup,
                                    aCallbacks,
                                    aLoadFlags,
                                    aReferrer);
}

nsresult
NS_NewUnicharStreamLoader(nsIUnicharStreamLoader        **result,
                          nsIUnicharStreamLoaderObserver *observer)
{
    nsresult rv;
    nsCOMPtr<nsIUnicharStreamLoader> loader =
        do_CreateInstance(NS_UNICHARSTREAMLOADER_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv)) {
        rv = loader->Init(observer);
        if (NS_SUCCEEDED(rv)) {
            *result = nullptr;
            loader.swap(*result);
        }
    }
    return rv;
}

nsresult
NS_NewSyncStreamListener(nsIStreamListener **result,
                         nsIInputStream    **stream)
{
    nsresult rv;
    nsCOMPtr<nsISyncStreamListener> listener =
        do_CreateInstance(NS_SYNCSTREAMLISTENER_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv)) {
        rv = listener->GetInputStream(stream);
        if (NS_SUCCEEDED(rv)) {
            listener.forget(result);
        }
    }
    return rv;
}

nsresult
NS_ImplementChannelOpen(nsIChannel      *channel,
                        nsIInputStream **result)
{
    nsCOMPtr<nsIStreamListener> listener;
    nsCOMPtr<nsIInputStream> stream;
    nsresult rv = NS_NewSyncStreamListener(getter_AddRefs(listener),
                                           getter_AddRefs(stream));
    if (NS_SUCCEEDED(rv)) {
        rv = channel->AsyncOpen(listener, nullptr);
        if (NS_SUCCEEDED(rv)) {
            uint64_t n;
            
            rv = stream->Available(&n);
            if (NS_SUCCEEDED(rv)) {
                *result = nullptr;
                stream.swap(*result);
            }
        }
    }
    return rv;
}

nsresult
NS_NewRequestObserverProxy(nsIRequestObserver **result,
                           nsIRequestObserver  *observer,
                           nsISupports         *context)
{
    nsresult rv;
    nsCOMPtr<nsIRequestObserverProxy> proxy =
        do_CreateInstance(NS_REQUESTOBSERVERPROXY_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv)) {
        rv = proxy->Init(observer, context);
        if (NS_SUCCEEDED(rv)) {
            proxy.forget(result);
        }
    }
    return rv;
}

nsresult
NS_NewSimpleStreamListener(nsIStreamListener **result,
                           nsIOutputStream    *sink,
                           nsIRequestObserver *observer )
{
    nsresult rv;
    nsCOMPtr<nsISimpleStreamListener> listener =
        do_CreateInstance(NS_SIMPLESTREAMLISTENER_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv)) {
        rv = listener->Init(sink, observer);
        if (NS_SUCCEEDED(rv)) {
            listener.forget(result);
        }
    }
    return rv;
}

nsresult
NS_CheckPortSafety(int32_t       port,
                   const char   *scheme,
                   nsIIOService *ioService )
{
    nsresult rv;
    nsCOMPtr<nsIIOService> grip;
    rv = net_EnsureIOService(&ioService, grip);
    if (ioService) {
        bool allow;
        rv = ioService->AllowPort(port, scheme, &allow);
        if (NS_SUCCEEDED(rv) && !allow) {
            NS_WARNING("port blocked");
            rv = NS_ERROR_PORT_ACCESS_NOT_ALLOWED;
        }
    }
    return rv;
}

nsresult
NS_CheckPortSafety(nsIURI *uri)
{
    int32_t port;
    nsresult rv = uri->GetPort(&port);
    if (NS_FAILED(rv) || port == -1)  
        return NS_OK;
    nsAutoCString scheme;
    uri->GetScheme(scheme);
    return NS_CheckPortSafety(port, scheme.get());
}

nsresult
NS_NewProxyInfo(const nsACString &type,
                const nsACString &host,
                int32_t           port,
                uint32_t          flags,
                nsIProxyInfo    **result)
{
    nsresult rv;
    nsCOMPtr<nsIProtocolProxyService> pps =
            do_GetService(NS_PROTOCOLPROXYSERVICE_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv))
        rv = pps->NewProxyInfo(type, host, port, flags, UINT32_MAX, nullptr,
                               result);
    return rv;
}

nsresult
NS_GetFileProtocolHandler(nsIFileProtocolHandler **result,
                          nsIIOService            *ioService )
{
    nsresult rv;
    nsCOMPtr<nsIIOService> grip;
    rv = net_EnsureIOService(&ioService, grip);
    if (ioService) {
        nsCOMPtr<nsIProtocolHandler> handler;
        rv = ioService->GetProtocolHandler("file", getter_AddRefs(handler));
        if (NS_SUCCEEDED(rv))
            rv = CallQueryInterface(handler, result);
    }
    return rv;
}

nsresult
NS_GetFileFromURLSpec(const nsACString  &inURL,
                      nsIFile          **result,
                      nsIIOService      *ioService )
{
    nsresult rv;
    nsCOMPtr<nsIFileProtocolHandler> fileHandler;
    rv = NS_GetFileProtocolHandler(getter_AddRefs(fileHandler), ioService);
    if (NS_SUCCEEDED(rv))
        rv = fileHandler->GetFileFromURLSpec(inURL, result);
    return rv;
}

nsresult
NS_GetURLSpecFromFile(nsIFile      *file,
                      nsACString   &url,
                      nsIIOService *ioService )
{
    nsresult rv;
    nsCOMPtr<nsIFileProtocolHandler> fileHandler;
    rv = NS_GetFileProtocolHandler(getter_AddRefs(fileHandler), ioService);
    if (NS_SUCCEEDED(rv))
        rv = fileHandler->GetURLSpecFromFile(file, url);
    return rv;
}

nsresult
NS_GetURLSpecFromActualFile(nsIFile      *file,
                            nsACString   &url,
                            nsIIOService *ioService )
{
    nsresult rv;
    nsCOMPtr<nsIFileProtocolHandler> fileHandler;
    rv = NS_GetFileProtocolHandler(getter_AddRefs(fileHandler), ioService);
    if (NS_SUCCEEDED(rv))
        rv = fileHandler->GetURLSpecFromActualFile(file, url);
    return rv;
}

nsresult
NS_GetURLSpecFromDir(nsIFile      *file,
                     nsACString   &url,
                     nsIIOService *ioService )
{
    nsresult rv;
    nsCOMPtr<nsIFileProtocolHandler> fileHandler;
    rv = NS_GetFileProtocolHandler(getter_AddRefs(fileHandler), ioService);
    if (NS_SUCCEEDED(rv))
        rv = fileHandler->GetURLSpecFromDir(file, url);
    return rv;
}

nsresult
NS_GetReferrerFromChannel(nsIChannel *channel,
                          nsIURI **referrer)
{
    nsresult rv = NS_ERROR_NOT_AVAILABLE;
    *referrer = nullptr;

    nsCOMPtr<nsIPropertyBag2> props(do_QueryInterface(channel));
    if (props) {
      
      
      
      rv = props->GetPropertyAsInterface(NS_LITERAL_STRING("docshell.internalReferrer"),
                                         NS_GET_IID(nsIURI),
                                         reinterpret_cast<void **>(referrer));
      if (NS_FAILED(rv))
        *referrer = nullptr;
    }

    
    
    if (!(*referrer)) {
      nsCOMPtr<nsIHttpChannel> chan(do_QueryInterface(channel));
      if (chan) {
        rv = chan->GetReferrer(referrer);
        if (NS_FAILED(rv))
          *referrer = nullptr;
      }
    }
    return rv;
}

nsresult
NS_ParseContentType(const nsACString &rawContentType,
                    nsCString        &contentType,
                    nsCString        &contentCharset)
{
    
    nsresult rv;
    nsCOMPtr<nsINetUtil> util = do_GetNetUtil(&rv);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCString charset;
    bool hadCharset;
    rv = util->ParseContentType(rawContentType, charset, &hadCharset,
                                contentType);
    if (NS_SUCCEEDED(rv) && hadCharset)
        contentCharset = charset;
    return rv;
}

nsresult
NS_ExtractCharsetFromContentType(const nsACString &rawContentType,
                                 nsCString        &contentCharset,
                                 bool             *hadCharset,
                                 int32_t          *charsetStart,
                                 int32_t          *charsetEnd)
{
    
    nsresult rv;
    nsCOMPtr<nsINetUtil> util = do_GetNetUtil(&rv);
    NS_ENSURE_SUCCESS(rv, rv);

    return util->ExtractCharsetFromContentType(rawContentType,
                                               contentCharset,
                                               charsetStart,
                                               charsetEnd,
                                               hadCharset);
}

nsresult
NS_NewPartialLocalFileInputStream(nsIInputStream **result,
                                  nsIFile         *file,
                                  uint64_t         offset,
                                  uint64_t         length,
                                  int32_t          ioFlags       ,
                                  int32_t          perm          ,
                                  int32_t          behaviorFlags )
{
    nsresult rv;
    nsCOMPtr<nsIPartialFileInputStream> in =
        do_CreateInstance(NS_PARTIALLOCALFILEINPUTSTREAM_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv)) {
        rv = in->Init(file, offset, length, ioFlags, perm, behaviorFlags);
        if (NS_SUCCEEDED(rv))
            rv = CallQueryInterface(in, result);
    }
    return rv;
}

nsresult
NS_NewAtomicFileOutputStream(nsIOutputStream **result,
                                nsIFile       *file,
                                int32_t        ioFlags       ,
                                int32_t        perm          ,
                                int32_t        behaviorFlags )
{
    nsresult rv;
    nsCOMPtr<nsIFileOutputStream> out =
        do_CreateInstance(NS_ATOMICLOCALFILEOUTPUTSTREAM_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv)) {
        rv = out->Init(file, ioFlags, perm, behaviorFlags);
        if (NS_SUCCEEDED(rv))
            out.forget(result);
    }
    return rv;
}

nsresult
NS_NewSafeLocalFileOutputStream(nsIOutputStream **result,
                                nsIFile          *file,
                                int32_t           ioFlags       ,
                                int32_t           perm          ,
                                int32_t           behaviorFlags )
{
    nsresult rv;
    nsCOMPtr<nsIFileOutputStream> out =
        do_CreateInstance(NS_SAFELOCALFILEOUTPUTSTREAM_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv)) {
        rv = out->Init(file, ioFlags, perm, behaviorFlags);
        if (NS_SUCCEEDED(rv))
            out.forget(result);
    }
    return rv;
}

nsresult
NS_NewLocalFileStream(nsIFileStream **result,
                      nsIFile        *file,
                      int32_t         ioFlags       ,
                      int32_t         perm          ,
                      int32_t         behaviorFlags )
{
    nsresult rv;
    nsCOMPtr<nsIFileStream> stream =
        do_CreateInstance(NS_LOCALFILESTREAM_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv)) {
        rv = stream->Init(file, ioFlags, perm, behaviorFlags);
        if (NS_SUCCEEDED(rv))
            stream.forget(result);
    }
    return rv;
}

nsresult
NS_BackgroundInputStream(nsIInputStream **result,
                         nsIInputStream  *stream,
                         uint32_t         segmentSize ,
                         uint32_t         segmentCount )
{
    nsresult rv;
    nsCOMPtr<nsIStreamTransportService> sts =
        do_GetService(NS_STREAMTRANSPORTSERVICE_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv)) {
        nsCOMPtr<nsITransport> inTransport;
        rv = sts->CreateInputTransport(stream, int64_t(-1), int64_t(-1),
                                       true, getter_AddRefs(inTransport));
        if (NS_SUCCEEDED(rv))
            rv = inTransport->OpenInputStream(nsITransport::OPEN_BLOCKING,
                                              segmentSize, segmentCount,
                                              result);
    }
    return rv;
}

nsresult
NS_BackgroundOutputStream(nsIOutputStream **result,
                          nsIOutputStream  *stream,
                          uint32_t          segmentSize  ,
                          uint32_t          segmentCount )
{
    nsresult rv;
    nsCOMPtr<nsIStreamTransportService> sts =
        do_GetService(NS_STREAMTRANSPORTSERVICE_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv)) {
        nsCOMPtr<nsITransport> inTransport;
        rv = sts->CreateOutputTransport(stream, int64_t(-1), int64_t(-1),
                                        true, getter_AddRefs(inTransport));
        if (NS_SUCCEEDED(rv))
            rv = inTransport->OpenOutputStream(nsITransport::OPEN_BLOCKING,
                                               segmentSize, segmentCount,
                                               result);
    }
    return rv;
}

nsresult
NS_NewBufferedOutputStream(nsIOutputStream **result,
                           nsIOutputStream  *str,
                           uint32_t          bufferSize)
{
    nsresult rv;
    nsCOMPtr<nsIBufferedOutputStream> out =
        do_CreateInstance(NS_BUFFEREDOUTPUTSTREAM_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv)) {
        rv = out->Init(str, bufferSize);
        if (NS_SUCCEEDED(rv)) {
            out.forget(result);
        }
    }
    return rv;
}

already_AddRefed<nsIOutputStream>
NS_BufferOutputStream(nsIOutputStream *aOutputStream,
                      uint32_t aBufferSize)
{
    NS_ASSERTION(aOutputStream, "No output stream given!");

    nsCOMPtr<nsIOutputStream> bos;
    nsresult rv = NS_NewBufferedOutputStream(getter_AddRefs(bos), aOutputStream,
                                             aBufferSize);
    if (NS_SUCCEEDED(rv))
        return bos.forget();

    bos = aOutputStream;
    return bos.forget();
}

nsresult
NS_ReadInputStreamToBuffer(nsIInputStream *aInputStream,
                           void **aDest,
                           uint32_t aCount)
{
    nsresult rv;

    if (!*aDest) {
        *aDest = malloc(aCount);
        if (!*aDest)
            return NS_ERROR_OUT_OF_MEMORY;
    }

    char * p = reinterpret_cast<char*>(*aDest);
    uint32_t bytesRead;
    uint32_t totalRead = 0;
    while (1) {
        rv = aInputStream->Read(p + totalRead, aCount - totalRead, &bytesRead);
        if (!NS_SUCCEEDED(rv))
            return rv;
        totalRead += bytesRead;
        if (totalRead == aCount)
            break;
        
        if (bytesRead == 0)
            return NS_ERROR_UNEXPECTED;
    }
    return rv;
}

#ifdef MOZILLA_INTERNAL_API

nsresult
NS_ReadInputStreamToString(nsIInputStream *aInputStream,
                           nsACString &aDest,
                           uint32_t aCount)
{
    if (!aDest.SetLength(aCount, mozilla::fallible))
        return NS_ERROR_OUT_OF_MEMORY;
    void* dest = aDest.BeginWriting();
    return NS_ReadInputStreamToBuffer(aInputStream, &dest, aCount);
}

#endif

nsresult
NS_LoadPersistentPropertiesFromURI(nsIPersistentProperties **outResult,
                                   nsIURI                   *aUri,
                                   nsIPrincipal             *aLoadingPrincipal,
                                   nsContentPolicyType       aContentPolicyType,
                                   nsIIOService             *aIoService )
{
    nsCOMPtr<nsIChannel> channel;
    nsresult rv = NS_NewChannel(getter_AddRefs(channel),
                                aUri,
                                aLoadingPrincipal,
                                nsILoadInfo::SEC_NORMAL,
                                aContentPolicyType,
                                nullptr,     
                                nullptr,     
                                nsIRequest::LOAD_NORMAL,
                                aIoService);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIInputStream> in;
    rv = channel->Open(getter_AddRefs(in));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIPersistentProperties> properties =
      do_CreateInstance(NS_PERSISTENTPROPERTIES_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = properties->Load(in);
    NS_ENSURE_SUCCESS(rv, rv);

    properties.swap(*outResult);
    return NS_OK;
 }

nsresult
NS_LoadPersistentPropertiesFromURISpec(nsIPersistentProperties **outResult,
                                       const nsACString         &aSpec,
                                       nsIPrincipal             *aLoadingPrincipal,
                                       nsContentPolicyType       aContentPolicyType,
                                       const char               *aCharset ,
                                       nsIURI                   *aBaseURI ,
                                       nsIIOService             *aIoService )
{
    nsCOMPtr<nsIURI> uri;
    nsresult rv = NS_NewURI(getter_AddRefs(uri),
                            aSpec,
                            aCharset,
                            aBaseURI,
                            aIoService);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_LoadPersistentPropertiesFromURI(outResult,
                                              uri,
                                              aLoadingPrincipal,
                                              aContentPolicyType,
                                              aIoService);
}

bool
NS_UsePrivateBrowsing(nsIChannel *channel)
{
    bool isPrivate = false;
    bool isOverriden = false;
    nsCOMPtr<nsIPrivateBrowsingChannel> pbChannel = do_QueryInterface(channel);
    if (pbChannel &&
        NS_SUCCEEDED(pbChannel->IsPrivateModeOverriden(&isPrivate, &isOverriden)) &&
        isOverriden) {
        return isPrivate;
    }
    nsCOMPtr<nsILoadContext> loadContext;
    NS_QueryNotificationCallbacks(channel, loadContext);
    return loadContext && loadContext->UsePrivateBrowsing();
}

bool
NS_GetAppInfo(nsIChannel *aChannel,
              uint32_t *aAppID,
              bool *aIsInBrowserElement)
{
    nsCOMPtr<nsILoadContext> loadContext;
    NS_QueryNotificationCallbacks(aChannel, loadContext);
    if (!loadContext) {
        return false;
    }

    nsresult rv = loadContext->GetAppId(aAppID);
    NS_ENSURE_SUCCESS(rv, false);

    rv = loadContext->GetIsInBrowserElement(aIsInBrowserElement);
    NS_ENSURE_SUCCESS(rv, false);

    return true;
}

nsresult
NS_GetAppInfoFromClearDataNotification(nsISupports *aSubject,
                                       uint32_t *aAppID,
                                       bool *aBrowserOnly)
{
    nsresult rv;

    nsCOMPtr<mozIApplicationClearPrivateDataParams>
        clearParams(do_QueryInterface(aSubject));
    MOZ_ASSERT(clearParams);
    if (!clearParams) {
        return NS_ERROR_UNEXPECTED;
    }

    uint32_t appId;
    rv = clearParams->GetAppId(&appId);
    MOZ_ASSERT(NS_SUCCEEDED(rv));
    MOZ_ASSERT(appId != NECKO_UNKNOWN_APP_ID);
    NS_ENSURE_SUCCESS(rv, rv);
    if (appId == NECKO_UNKNOWN_APP_ID) {
        return NS_ERROR_UNEXPECTED;
    }

    bool browserOnly = false;
    rv = clearParams->GetBrowserOnly(&browserOnly);
    MOZ_ASSERT(NS_SUCCEEDED(rv));
    NS_ENSURE_SUCCESS(rv, rv);

    *aAppID = appId;
    *aBrowserOnly = browserOnly;
    return NS_OK;
}

bool
NS_ShouldCheckAppCache(nsIURI *aURI, bool usePrivateBrowsing)
{
    if (usePrivateBrowsing) {
        return false;
    }

    nsCOMPtr<nsIOfflineCacheUpdateService> offlineService =
        do_GetService("@mozilla.org/offlinecacheupdate-service;1");
    if (!offlineService) {
        return false;
    }

    bool allowed;
    nsresult rv = offlineService->OfflineAppAllowedForURI(aURI,
                                                          nullptr,
                                                          &allowed);
    return NS_SUCCEEDED(rv) && allowed;
}

bool
NS_ShouldCheckAppCache(nsIPrincipal *aPrincipal, bool usePrivateBrowsing)
{
    if (usePrivateBrowsing) {
        return false;
    }

    nsCOMPtr<nsIOfflineCacheUpdateService> offlineService =
        do_GetService("@mozilla.org/offlinecacheupdate-service;1");
    if (!offlineService) {
        return false;
    }

    bool allowed;
    nsresult rv = offlineService->OfflineAppAllowed(aPrincipal,
                                                    nullptr,
                                                    &allowed);
    return NS_SUCCEEDED(rv) && allowed;
}

void
NS_WrapAuthPrompt(nsIAuthPrompt   *aAuthPrompt,
                  nsIAuthPrompt2 **aAuthPrompt2)
{
    nsCOMPtr<nsIAuthPromptAdapterFactory> factory =
        do_GetService(NS_AUTHPROMPT_ADAPTER_FACTORY_CONTRACTID);
    if (!factory)
        return;

    NS_WARNING("Using deprecated nsIAuthPrompt");
    factory->CreateAdapter(aAuthPrompt, aAuthPrompt2);
}

void
NS_QueryAuthPrompt2(nsIInterfaceRequestor  *aCallbacks,
                    nsIAuthPrompt2        **aAuthPrompt)
{
    CallGetInterface(aCallbacks, aAuthPrompt);
    if (*aAuthPrompt)
        return;

    
    nsCOMPtr<nsIAuthPrompt> prompt(do_GetInterface(aCallbacks));
    if (!prompt)
        return;

    NS_WrapAuthPrompt(prompt, aAuthPrompt);
}

void
NS_QueryAuthPrompt2(nsIChannel      *aChannel,
                    nsIAuthPrompt2 **aAuthPrompt)
{
    *aAuthPrompt = nullptr;

    
    
    
    
    
    nsCOMPtr<nsIInterfaceRequestor> callbacks;
    aChannel->GetNotificationCallbacks(getter_AddRefs(callbacks));
    if (callbacks) {
        NS_QueryAuthPrompt2(callbacks, aAuthPrompt);
        if (*aAuthPrompt)
            return;
    }

    nsCOMPtr<nsILoadGroup> group;
    aChannel->GetLoadGroup(getter_AddRefs(group));
    if (!group)
        return;

    group->GetNotificationCallbacks(getter_AddRefs(callbacks));
    if (!callbacks)
        return;
    NS_QueryAuthPrompt2(callbacks, aAuthPrompt);
}

nsresult
NS_NewNotificationCallbacksAggregation(nsIInterfaceRequestor  *callbacks,
                                       nsILoadGroup           *loadGroup,
                                       nsIEventTarget         *target,
                                       nsIInterfaceRequestor **result)
{
    nsCOMPtr<nsIInterfaceRequestor> cbs;
    if (loadGroup)
        loadGroup->GetNotificationCallbacks(getter_AddRefs(cbs));
    return NS_NewInterfaceRequestorAggregation(callbacks, cbs, target, result);
}

nsresult
NS_NewNotificationCallbacksAggregation(nsIInterfaceRequestor  *callbacks,
                                       nsILoadGroup           *loadGroup,
                                       nsIInterfaceRequestor **result)
{
    return NS_NewNotificationCallbacksAggregation(callbacks, loadGroup, nullptr, result);
}

bool
NS_IsOffline()
{
    bool offline = true;
    bool connectivity = true;
    nsCOMPtr<nsIIOService> ios = do_GetIOService();
    if (ios) {
        ios->GetOffline(&offline);
        ios->GetConnectivity(&connectivity);
    }
    return offline || !connectivity;
}

bool
NS_IsAppOffline(uint32_t appId)
{
    bool appOffline = false;
    nsCOMPtr<nsIIOService> io(
        do_GetService("@mozilla.org/network/io-service;1"));
    if (io) {
        io->IsAppOffline(appId, &appOffline);
    }
    return appOffline;
}

bool
NS_IsAppOffline(nsIPrincipal *principal)
{
    if (!principal) {
        return NS_IsOffline();
    }
    uint32_t appId = nsIScriptSecurityManager::UNKNOWN_APP_ID;
    principal->GetAppId(&appId);

    return NS_IsAppOffline(appId);
}

nsresult
NS_DoImplGetInnermostURI(nsINestedURI *nestedURI, nsIURI **result)
{
    NS_PRECONDITION(nestedURI, "Must have a nested URI!");
    NS_PRECONDITION(!*result, "Must have null *result");

    nsCOMPtr<nsIURI> inner;
    nsresult rv = nestedURI->GetInnerURI(getter_AddRefs(inner));
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    nsCOMPtr<nsINestedURI> nestedInner(do_QueryInterface(inner));
    while (nestedInner) {
        rv = nestedInner->GetInnerURI(getter_AddRefs(inner));
        NS_ENSURE_SUCCESS(rv, rv);
        nestedInner = do_QueryInterface(inner);
    }

    
    inner.swap(*result);

    return rv;
}

nsresult
NS_ImplGetInnermostURI(nsINestedURI *nestedURI, nsIURI **result)
{
    
    *result = nullptr;

    return NS_DoImplGetInnermostURI(nestedURI, result);
}

nsresult
NS_EnsureSafeToReturn(nsIURI *uri, nsIURI **result)
{
    NS_PRECONDITION(uri, "Must have a URI");

    
    bool isMutable = true;
    nsCOMPtr<nsIMutable> mutableObj(do_QueryInterface(uri));
    if (mutableObj) {
        nsresult rv = mutableObj->GetMutable(&isMutable);
        isMutable = NS_FAILED(rv) || isMutable;
    }

    if (!isMutable) {
        NS_ADDREF(*result = uri);
        return NS_OK;
    }

    nsresult rv = uri->Clone(result);
    if (NS_SUCCEEDED(rv) && !*result) {
        NS_ERROR("nsIURI.clone contract was violated");
        return NS_ERROR_UNEXPECTED;
    }

    return rv;
}

void
NS_TryToSetImmutable(nsIURI *uri)
{
    nsCOMPtr<nsIMutable> mutableObj(do_QueryInterface(uri));
    if (mutableObj) {
        mutableObj->SetMutable(false);
    }
}

already_AddRefed<nsIURI>
NS_TryToMakeImmutable(nsIURI *uri,
                      nsresult *outRv )
{
    nsresult rv;
    nsCOMPtr<nsINetUtil> util = do_GetNetUtil(&rv);

    nsCOMPtr<nsIURI> result;
    if (NS_SUCCEEDED(rv)) {
        NS_ASSERTION(util, "do_GetNetUtil lied");
        rv = util->ToImmutableURI(uri, getter_AddRefs(result));
    }

    if (NS_FAILED(rv)) {
        result = uri;
    }

    if (outRv) {
        *outRv = rv;
    }

    return result.forget();
}

nsresult
NS_URIChainHasFlags(nsIURI   *uri,
                    uint32_t  flags,
                    bool     *result)
{
    nsresult rv;
    nsCOMPtr<nsINetUtil> util = do_GetNetUtil(&rv);
    NS_ENSURE_SUCCESS(rv, rv);

    return util->URIChainHasFlags(uri, flags, result);
}

already_AddRefed<nsIURI>
NS_GetInnermostURI(nsIURI *aURI)
{
    NS_PRECONDITION(aURI, "Must have URI");

    nsCOMPtr<nsIURI> uri = aURI;

    nsCOMPtr<nsINestedURI> nestedURI(do_QueryInterface(uri));
    if (!nestedURI) {
        return uri.forget();
    }

    nsresult rv = nestedURI->GetInnermostURI(getter_AddRefs(uri));
    if (NS_FAILED(rv)) {
        return nullptr;
    }

    return uri.forget();
}

nsresult
NS_GetFinalChannelURI(nsIChannel *channel, nsIURI **uri)
{
    *uri = nullptr;
    nsLoadFlags loadFlags = 0;
    nsresult rv = channel->GetLoadFlags(&loadFlags);
    NS_ENSURE_SUCCESS(rv, rv);

    if (loadFlags & nsIChannel::LOAD_REPLACE) {
        return channel->GetURI(uri);
    }

    return channel->GetOriginalURI(uri);
}

uint32_t
NS_SecurityHashURI(nsIURI *aURI)
{
    nsCOMPtr<nsIURI> baseURI = NS_GetInnermostURI(aURI);

    nsAutoCString scheme;
    uint32_t schemeHash = 0;
    if (NS_SUCCEEDED(baseURI->GetScheme(scheme)))
        schemeHash = mozilla::HashString(scheme);

    
    if (scheme.EqualsLiteral("file"))
        return schemeHash; 

    if (scheme.EqualsLiteral("imap") ||
        scheme.EqualsLiteral("mailbox") ||
        scheme.EqualsLiteral("news"))
    {
        nsAutoCString spec;
        uint32_t specHash;
        nsresult res = baseURI->GetSpec(spec);
        if (NS_SUCCEEDED(res))
            specHash = mozilla::HashString(spec);
        else
            specHash = static_cast<uint32_t>(res);
        return specHash;
    }

    nsAutoCString host;
    uint32_t hostHash = 0;
    if (NS_SUCCEEDED(baseURI->GetAsciiHost(host)))
        hostHash = mozilla::HashString(host);

    return mozilla::AddToHash(schemeHash, hostHash, NS_GetRealPort(baseURI));
}

bool
NS_SecurityCompareURIs(nsIURI *aSourceURI,
                       nsIURI *aTargetURI,
                       bool    aStrictFileOriginPolicy)
{
    
    
    
    
    
    if (aSourceURI && aSourceURI == aTargetURI)
    {
        return true;
    }

    if (!aTargetURI || !aSourceURI)
    {
        return false;
    }

    
    nsCOMPtr<nsIURI> sourceBaseURI = NS_GetInnermostURI(aSourceURI);
    nsCOMPtr<nsIURI> targetBaseURI = NS_GetInnermostURI(aTargetURI);

    
    nsCOMPtr<nsIURIWithPrincipal> uriPrinc = do_QueryInterface(sourceBaseURI);
    if (uriPrinc) {
        uriPrinc->GetPrincipalUri(getter_AddRefs(sourceBaseURI));
    }

    uriPrinc = do_QueryInterface(targetBaseURI);
    if (uriPrinc) {
        uriPrinc->GetPrincipalUri(getter_AddRefs(targetBaseURI));
    }

    if (!sourceBaseURI || !targetBaseURI)
        return false;

    
    nsAutoCString targetScheme;
    bool sameScheme = false;
    if (NS_FAILED( targetBaseURI->GetScheme(targetScheme) ) ||
        NS_FAILED( sourceBaseURI->SchemeIs(targetScheme.get(), &sameScheme) ) ||
        !sameScheme)
    {
        
        return false;
    }

    
    
    if (targetScheme.EqualsLiteral("file"))
    {
        
        if (!aStrictFileOriginPolicy)
            return true;

        nsCOMPtr<nsIFileURL> sourceFileURL(do_QueryInterface(sourceBaseURI));
        nsCOMPtr<nsIFileURL> targetFileURL(do_QueryInterface(targetBaseURI));

        if (!sourceFileURL || !targetFileURL)
            return false;

        nsCOMPtr<nsIFile> sourceFile, targetFile;

        sourceFileURL->GetFile(getter_AddRefs(sourceFile));
        targetFileURL->GetFile(getter_AddRefs(targetFile));

        if (!sourceFile || !targetFile)
            return false;

        
        bool filesAreEqual = false;
        nsresult rv = sourceFile->Equals(targetFile, &filesAreEqual);
        return NS_SUCCEEDED(rv) && filesAreEqual;
    }

    
    if (targetScheme.EqualsLiteral("imap") ||
        targetScheme.EqualsLiteral("mailbox") ||
        targetScheme.EqualsLiteral("news"))
    {
        
        
        nsAutoCString targetSpec;
        nsAutoCString sourceSpec;
        return ( NS_SUCCEEDED( targetBaseURI->GetSpec(targetSpec) ) &&
                 NS_SUCCEEDED( sourceBaseURI->GetSpec(sourceSpec) ) &&
                 targetSpec.Equals(sourceSpec) );
    }

    
    nsAutoCString targetHost;
    nsAutoCString sourceHost;
    if (NS_FAILED( targetBaseURI->GetAsciiHost(targetHost) ) ||
        NS_FAILED( sourceBaseURI->GetAsciiHost(sourceHost) ))
    {
        return false;
    }

    nsCOMPtr<nsIStandardURL> targetURL(do_QueryInterface(targetBaseURI));
    nsCOMPtr<nsIStandardURL> sourceURL(do_QueryInterface(sourceBaseURI));
    if (!targetURL || !sourceURL)
    {
        return false;
    }

#ifdef MOZILLA_INTERNAL_API
    if (!targetHost.Equals(sourceHost, nsCaseInsensitiveCStringComparator() ))
#else
    if (!targetHost.Equals(sourceHost, CaseInsensitiveCompare))
#endif
    {
        return false;
    }

    return NS_GetRealPort(targetBaseURI) == NS_GetRealPort(sourceBaseURI);
}

bool
NS_URIIsLocalFile(nsIURI *aURI)
{
  nsCOMPtr<nsINetUtil> util = do_GetNetUtil();

  bool isFile;
  return util && NS_SUCCEEDED(util->ProtocolHasFlags(aURI,
                                nsIProtocolHandler::URI_IS_LOCAL_FILE,
                                &isFile)) &&
         isFile;
}

bool
NS_RelaxStrictFileOriginPolicy(nsIURI *aTargetURI,
                               nsIURI *aSourceURI,
                               bool aAllowDirectoryTarget )
{
  if (!NS_URIIsLocalFile(aTargetURI)) {
    
    NS_NOTREACHED("NS_RelaxStrictFileOriginPolicy called with non-file URI");
    return false;
  }

  if (!NS_URIIsLocalFile(aSourceURI)) {
    
    
    
    
    
    return false;
  }

  
  
  
  nsCOMPtr<nsIFileURL> targetFileURL(do_QueryInterface(aTargetURI));
  nsCOMPtr<nsIFileURL> sourceFileURL(do_QueryInterface(aSourceURI));
  nsCOMPtr<nsIFile> targetFile;
  nsCOMPtr<nsIFile> sourceFile;
  bool targetIsDir;

  
  
  if (!sourceFileURL || !targetFileURL ||
      NS_FAILED(targetFileURL->GetFile(getter_AddRefs(targetFile))) ||
      NS_FAILED(sourceFileURL->GetFile(getter_AddRefs(sourceFile))) ||
      !targetFile || !sourceFile ||
      NS_FAILED(targetFile->Normalize()) ||
#ifndef MOZ_WIDGET_ANDROID
      NS_FAILED(sourceFile->Normalize()) ||
#endif
      (!aAllowDirectoryTarget &&
       (NS_FAILED(targetFile->IsDirectory(&targetIsDir)) || targetIsDir))) {
    return false;
  }

  
  
  
  
  
  bool sourceIsDir;
  bool allowed = false;
  nsresult rv = sourceFile->IsDirectory(&sourceIsDir);
  if (NS_SUCCEEDED(rv) && sourceIsDir) {
    rv = sourceFile->Contains(targetFile, &allowed);
  } else {
    nsCOMPtr<nsIFile> sourceParent;
    rv = sourceFile->GetParent(getter_AddRefs(sourceParent));
    if (NS_SUCCEEDED(rv) && sourceParent) {
      rv = sourceParent->Equals(targetFile, &allowed);
      if (NS_FAILED(rv) || !allowed) {
        rv = sourceParent->Contains(targetFile, &allowed);
      } else {
        MOZ_ASSERT(aAllowDirectoryTarget,
                   "sourceFile->Parent == targetFile, but targetFile "
                   "should've been disallowed if it is a directory");
      }
    }
  }

  if (NS_SUCCEEDED(rv) && allowed) {
    return true;
  }

  return false;
}

bool
NS_IsInternalSameURIRedirect(nsIChannel *aOldChannel,
                             nsIChannel *aNewChannel,
                             uint32_t aFlags)
{
  if (!(aFlags & nsIChannelEventSink::REDIRECT_INTERNAL)) {
    return false;
  }

  nsCOMPtr<nsIURI> oldURI, newURI;
  aOldChannel->GetURI(getter_AddRefs(oldURI));
  aNewChannel->GetURI(getter_AddRefs(newURI));

  if (!oldURI || !newURI) {
    return false;
  }

  bool res;
  return NS_SUCCEEDED(oldURI->Equals(newURI, &res)) && res;
}

bool
NS_IsHSTSUpgradeRedirect(nsIChannel *aOldChannel,
                         nsIChannel *aNewChannel,
                         uint32_t aFlags)
{
  if (!(aFlags & nsIChannelEventSink::REDIRECT_STS_UPGRADE)) {
    return false;
  }

  nsCOMPtr<nsIURI> oldURI, newURI;
  aOldChannel->GetURI(getter_AddRefs(oldURI));
  aNewChannel->GetURI(getter_AddRefs(newURI));

  if (!oldURI || !newURI) {
    return false;
  }

  bool isHttp;
  if (NS_FAILED(oldURI->SchemeIs("http", &isHttp)) || !isHttp) {
    return false;
  }

  bool isHttps;
  if (NS_FAILED(newURI->SchemeIs("https", &isHttps)) || !isHttps) {
    return false;
  }

  nsCOMPtr<nsIURI> upgradedURI;
  if (NS_FAILED(oldURI->Clone(getter_AddRefs(upgradedURI)))) {
    return false;
  }

  if (NS_FAILED(upgradedURI->SetScheme(NS_LITERAL_CSTRING("https")))) {
    return false;
  }

  int32_t oldPort = -1;
  if (NS_FAILED(oldURI->GetPort(&oldPort))) {
    return false;
  }
  if (oldPort == 80 || oldPort == -1) {
    upgradedURI->SetPort(-1);
  } else {
    upgradedURI->SetPort(oldPort);
  }

  bool res;
  return NS_SUCCEEDED(upgradedURI->Equals(newURI, &res)) && res;
}

nsresult
NS_LinkRedirectChannels(uint32_t channelId,
                        nsIParentChannel *parentChannel,
                        nsIChannel **_result)
{
  nsresult rv;

  nsCOMPtr<nsIRedirectChannelRegistrar> registrar =
      do_GetService("@mozilla.org/redirectchannelregistrar;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return registrar->LinkChannels(channelId,
                                 parentChannel,
                                 _result);
}

#define NS_FAKE_SCHEME "http://"
#define NS_FAKE_TLD ".invalid"
nsresult NS_MakeRandomInvalidURLString(nsCString &result)
{
  nsresult rv;
  nsCOMPtr<nsIUUIDGenerator> uuidgen =
    do_GetService("@mozilla.org/uuid-generator;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsID idee;
  rv = uuidgen->GenerateUUIDInPlace(&idee);
  NS_ENSURE_SUCCESS(rv, rv);

  char chars[NSID_LENGTH];
  idee.ToProvidedString(chars);

  result.AssignLiteral(NS_FAKE_SCHEME);
  
  result.Append(chars + 1, NSID_LENGTH - 3);
  result.AppendLiteral(NS_FAKE_TLD);

  return NS_OK;
}
#undef NS_FAKE_SCHEME
#undef NS_FAKE_TLD

nsresult
NS_CheckIsJavaCompatibleURLString(nsCString &urlString, bool *result)
{
  *result = false; 

  nsresult rv = NS_OK;
  nsCOMPtr<nsIURLParser> urlParser =
    do_GetService(NS_STDURLPARSER_CONTRACTID, &rv);
  if (NS_FAILED(rv) || !urlParser)
    return NS_ERROR_FAILURE;

  bool compatible = true;
  uint32_t schemePos = 0;
  int32_t schemeLen = 0;
  urlParser->ParseURL(urlString.get(), -1, &schemePos, &schemeLen,
                      nullptr, nullptr, nullptr, nullptr);
  if (schemeLen != -1) {
    nsCString scheme;
    scheme.Assign(urlString.get() + schemePos, schemeLen);
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (PL_strcasecmp(scheme.get(), "http") &&
        PL_strcasecmp(scheme.get(), "https") &&
        PL_strcasecmp(scheme.get(), "file") &&
        PL_strcasecmp(scheme.get(), "ftp") &&
        PL_strcasecmp(scheme.get(), "gopher") &&
        PL_strcasecmp(scheme.get(), "chrome"))
      compatible = false;
  } else {
    compatible = false;
  }

  *result = compatible;

  return NS_OK;
}





uint32_t
NS_GetContentDispositionFromToken(const nsAString &aDispToken)
{
  
  
  
  
  if (aDispToken.IsEmpty() ||
      aDispToken.LowerCaseEqualsLiteral("inline") ||
      
      
      
      StringHead(aDispToken, 8).LowerCaseEqualsLiteral("filename"))
    return nsIChannel::DISPOSITION_INLINE;

  return nsIChannel::DISPOSITION_ATTACHMENT;
}

uint32_t
NS_GetContentDispositionFromHeader(const nsACString &aHeader,
                                   nsIChannel *aChan )
{
  nsresult rv;
  nsCOMPtr<nsIMIMEHeaderParam> mimehdrpar = do_GetService(NS_MIMEHEADERPARAM_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    return nsIChannel::DISPOSITION_ATTACHMENT;

  nsAutoCString fallbackCharset;
  if (aChan) {
    nsCOMPtr<nsIURI> uri;
    aChan->GetURI(getter_AddRefs(uri));
    if (uri)
      uri->GetOriginCharset(fallbackCharset);
  }

  nsAutoString dispToken;
  rv = mimehdrpar->GetParameterHTTP(aHeader, "", fallbackCharset, true, nullptr,
                                    dispToken);

  if (NS_FAILED(rv)) {
    
    if (rv == NS_ERROR_FIRST_HEADER_FIELD_COMPONENT_EMPTY)
        return nsIChannel::DISPOSITION_INLINE;
    return nsIChannel::DISPOSITION_ATTACHMENT;
  }

  return NS_GetContentDispositionFromToken(dispToken);
}

nsresult
NS_GetFilenameFromDisposition(nsAString &aFilename,
                              const nsACString &aDisposition,
                              nsIURI *aURI )
{
  aFilename.Truncate();

  nsresult rv;
  nsCOMPtr<nsIMIMEHeaderParam> mimehdrpar =
      do_GetService(NS_MIMEHEADERPARAM_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIURL> url = do_QueryInterface(aURI);

  nsAutoCString fallbackCharset;
  if (url)
    url->GetOriginCharset(fallbackCharset);
  
  rv = mimehdrpar->GetParameterHTTP(aDisposition, "filename",
                                    fallbackCharset, true, nullptr,
                                    aFilename);

  if (NS_FAILED(rv)) {
    aFilename.Truncate();
    return rv;
  }

  if (aFilename.IsEmpty())
    return NS_ERROR_NOT_AVAILABLE;

  return NS_OK;
}

void net_EnsurePSMInit()
{
    nsCOMPtr<nsISocketProviderService> spserv =
            do_GetService(NS_SOCKETPROVIDERSERVICE_CONTRACTID);
    if (spserv) {
        nsCOMPtr<nsISocketProvider> provider;
        spserv->GetSocketProvider("ssl", getter_AddRefs(provider));
    }
}

bool NS_IsAboutBlank(nsIURI *uri)
{
    
    bool isAbout = false;
    if (NS_FAILED(uri->SchemeIs("about", &isAbout)) || !isAbout) {
        return false;
    }

    nsAutoCString str;
    uri->GetSpec(str);
    return str.EqualsLiteral("about:blank");
}

nsresult
NS_GenerateHostPort(const nsCString& host, int32_t port,
                    nsACString &hostLine)
{
    if (strchr(host.get(), ':')) {
        
        hostLine.Assign('[');
        
        int scopeIdPos = host.FindChar('%');
        if (scopeIdPos == -1)
            hostLine.Append(host);
        else if (scopeIdPos > 0)
            hostLine.Append(Substring(host, 0, scopeIdPos));
        else
          return NS_ERROR_MALFORMED_URI;
        hostLine.Append(']');
    }
    else
        hostLine.Assign(host);
    if (port != -1) {
        hostLine.Append(':');
        hostLine.AppendInt(port);
    }
    return NS_OK;
}

void
NS_SniffContent(const char *aSnifferType, nsIRequest *aRequest,
                const uint8_t *aData, uint32_t aLength,
                nsACString &aSniffedType)
{
  typedef nsCategoryCache<nsIContentSniffer> ContentSnifferCache;
  extern ContentSnifferCache* gNetSniffers;
  extern ContentSnifferCache* gDataSniffers;
  ContentSnifferCache* cache = nullptr;
  if (!strcmp(aSnifferType, NS_CONTENT_SNIFFER_CATEGORY)) {
    if (!gNetSniffers) {
      gNetSniffers = new ContentSnifferCache(NS_CONTENT_SNIFFER_CATEGORY);
    }
    cache = gNetSniffers;
  } else if (!strcmp(aSnifferType, NS_DATA_SNIFFER_CATEGORY)) {
    if (!gDataSniffers) {
      gDataSniffers = new ContentSnifferCache(NS_DATA_SNIFFER_CATEGORY);
    }
    cache = gDataSniffers;
  } else {
    
    MOZ_ASSERT(false);
    return;
  }

  nsCOMArray<nsIContentSniffer> sniffers;
  cache->GetEntries(sniffers);
  for (int32_t i = 0; i < sniffers.Count(); ++i) {
    nsresult rv = sniffers[i]->GetMIMETypeFromContent(aRequest, aData, aLength, aSniffedType);
    if (NS_SUCCEEDED(rv) && !aSniffedType.IsEmpty()) {
      return;
    }
  }

  aSniffedType.Truncate();
}

bool
NS_IsSrcdocChannel(nsIChannel *aChannel)
{
  bool isSrcdoc;
  nsCOMPtr<nsIInputStreamChannel> isr = do_QueryInterface(aChannel);
  if (isr) {
    isr->GetIsSrcdocChannel(&isSrcdoc);
    return isSrcdoc;
  }
  nsCOMPtr<nsIViewSourceChannel> vsc = do_QueryInterface(aChannel);
  if (vsc) {
    vsc->GetIsSrcdocChannel(&isSrcdoc);
    return isSrcdoc;
  }
  return false;
}

namespace mozilla {
namespace net {

bool
InScriptableRange(int64_t val)
{
    return (val <= kJS_MAX_SAFE_INTEGER) && (val >= kJS_MIN_SAFE_INTEGER);
}

bool
InScriptableRange(uint64_t val)
{
    return val <= kJS_MAX_SAFE_UINTEGER;
}

} 
} 
