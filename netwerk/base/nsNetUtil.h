





#ifndef nsNetUtil_h__
#define nsNetUtil_h__

#include "nsCOMPtr.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsILoadGroup.h"
#include "nsINetUtil.h"
#include "nsIRequest.h"
#include "nsILoadInfo.h"
#include "nsIIOService.h"
#include "mozilla/Services.h"
#include "nsNetCID.h"

class nsIURI;
class nsIPrincipal;
class nsIAsyncStreamCopier;
class nsIAuthPrompt;
class nsIAuthPrompt2;
class nsIChannel;
class nsIChannelPolicy;
class nsIDownloadObserver;
class nsIEventTarget;
class nsIFileProtocolHandler;
class nsIFileStream;
class nsIInputStream;
class nsIInputStreamPump;
class nsIInterfaceRequestor;
class nsINestedURI;
class nsINetworkInterface;
class nsIOutputStream;
class nsIParentChannel;
class nsIPersistentProperties;
class nsIProxyInfo;
class nsIRequestObserver;
class nsIStreamListener;
class nsIStreamLoader;
class nsIStreamLoaderObserver;
class nsIUnicharStreamLoader;
class nsIUnicharStreamLoaderObserver;

template <class> class nsCOMPtr;
template <typename> struct already_AddRefed;

#ifdef MOZILLA_INTERNAL_API
#include "nsReadableUtils.h"
#include "nsString.h"
#else
#include "nsStringAPI.h"
#endif

#ifdef MOZILLA_INTERNAL_API
already_AddRefed<nsIIOService> do_GetIOService(nsresult *error = 0);

already_AddRefed<nsINetUtil> do_GetNetUtil(nsresult *error = 0);

#else

const nsGetServiceByContractIDWithError do_GetIOService(nsresult *error = 0);


const nsGetServiceByContractIDWithError do_GetNetUtil(nsresult *error = 0);

#endif


nsresult net_EnsureIOService(nsIIOService **ios, nsCOMPtr<nsIIOService> &grip);

nsresult NS_NewURI(nsIURI **result,
                   const nsACString &spec,
                   const char *charset = nullptr,
                   nsIURI *baseURI = nullptr,
                   nsIIOService *ioService = nullptr);     

nsresult NS_NewURI(nsIURI **result,
                   const nsAString &spec,
                   const char *charset = nullptr,
                   nsIURI *baseURI = nullptr,
                   nsIIOService *ioService = nullptr);     

nsresult NS_NewURI(nsIURI **result,
                  const char *spec,
                  nsIURI *baseURI = nullptr,
                  nsIIOService *ioService = nullptr);     

nsresult NS_NewFileURI(nsIURI **result,
                       nsIFile *spec,
                       nsIIOService *ioService = nullptr);     












































































nsresult NS_NewChannelInternal(nsIChannel           **outChannel,
                               nsIURI                *aUri,
                               nsINode               *aLoadingNode,
                               nsIPrincipal          *aLoadingPrincipal,
                               nsIPrincipal          *aTriggeringPrincipal,
                               nsSecurityFlags        aSecurityFlags,
                               nsContentPolicyType    aContentPolicyType,
                               nsILoadGroup          *aLoadGroup = nullptr,
                               nsIInterfaceRequestor *aCallbacks = nullptr,
                               nsLoadFlags            aLoadFlags = nsIRequest::LOAD_NORMAL,
                               nsIIOService          *aIoService = nullptr);


nsresult NS_NewChannelInternal(nsIChannel           **outChannel,
                               nsIURI                *aUri,
                               nsILoadInfo           *aLoadInfo,
                               nsILoadGroup          *aLoadGroup = nullptr,
                               nsIInterfaceRequestor *aCallbacks = nullptr,
                               nsLoadFlags            aLoadFlags = nsIRequest::LOAD_NORMAL,
                               nsIIOService          *aIoService = nullptr);


nsresult 
NS_NewChannelWithTriggeringPrincipal(nsIChannel           **outChannel,
                                     nsIURI                *aUri,
                                     nsINode               *aLoadingNode,
                                     nsIPrincipal          *aTriggeringPrincipal,
                                     nsSecurityFlags        aSecurityFlags,
                                     nsContentPolicyType    aContentPolicyType,
                                     nsILoadGroup          *aLoadGroup = nullptr,
                                     nsIInterfaceRequestor *aCallbacks = nullptr,
                                     nsLoadFlags            aLoadFlags = nsIRequest::LOAD_NORMAL,
                                     nsIIOService          *aIoService = nullptr);



nsresult 
NS_NewChannelWithTriggeringPrincipal(nsIChannel           **outChannel,
                                     nsIURI                *aUri,
                                     nsIPrincipal          *aLoadingPrincipal,
                                     nsIPrincipal          *aTriggeringPrincipal,
                                     nsSecurityFlags        aSecurityFlags,
                                     nsContentPolicyType    aContentPolicyType,
                                     nsILoadGroup          *aLoadGroup = nullptr,
                                     nsIInterfaceRequestor *aCallbacks = nullptr,
                                     nsLoadFlags            aLoadFlags = nsIRequest::LOAD_NORMAL,
                                     nsIIOService          *aIoService = nullptr);


nsresult 
NS_NewChannel(nsIChannel           **outChannel,
              nsIURI                *aUri,
              nsINode               *aLoadingNode,
              nsSecurityFlags        aSecurityFlags,
              nsContentPolicyType    aContentPolicyType,
              nsILoadGroup          *aLoadGroup = nullptr,
              nsIInterfaceRequestor *aCallbacks = nullptr,
              nsLoadFlags            aLoadFlags = nsIRequest::LOAD_NORMAL,
              nsIIOService          *aIoService = nullptr);


nsresult 
NS_NewChannel(nsIChannel           **outChannel,
              nsIURI                *aUri,
              nsIPrincipal          *aLoadingPrincipal,
              nsSecurityFlags        aSecurityFlags,
              nsContentPolicyType    aContentPolicyType,
              nsILoadGroup          *aLoadGroup = nullptr,
              nsIInterfaceRequestor *aCallbacks = nullptr,
              nsLoadFlags            aLoadFlags = nsIRequest::LOAD_NORMAL,
              nsIIOService          *aIoService = nullptr);

nsresult NS_MakeAbsoluteURI(nsACString       &result,
                            const nsACString &spec,
                            nsIURI           *baseURI);

nsresult NS_MakeAbsoluteURI(char        **result,
                            const char   *spec,
                            nsIURI       *baseURI);

nsresult NS_MakeAbsoluteURI(nsAString       &result,
                            const nsAString &spec,
                            nsIURI          *baseURI);




int32_t NS_GetDefaultPort(const char *scheme,
                          nsIIOService *ioService = nullptr);





bool NS_StringToACE(const nsACString &idn, nsACString &result);






int32_t NS_GetRealPort(nsIURI *aURI);

nsresult 
NS_NewInputStreamChannelInternal(nsIChannel        **outChannel,
                                 nsIURI             *aUri,
                                 nsIInputStream     *aStream,
                                 const nsACString   &aContentType,
                                 const nsACString   &aContentCharset,
                                 nsILoadInfo        *aLoadInfo);

nsresult NS_NewInputStreamChannelInternal(nsIChannel        **outChannel,
                                          nsIURI             *aUri,
                                          nsIInputStream     *aStream,
                                          const nsACString   &aContentType,
                                          const nsACString   &aContentCharset,
                                          nsINode            *aLoadingNode,
                                          nsIPrincipal       *aLoadingPrincipal,
                                          nsIPrincipal       *aTriggeringPrincipal,
                                          nsSecurityFlags     aSecurityFlags,
                                          nsContentPolicyType aContentPolicyType,
                                          nsIURI             *aBaseURI = nullptr);


nsresult 
NS_NewInputStreamChannel(nsIChannel        **outChannel,
                         nsIURI             *aUri,
                         nsIInputStream     *aStream,
                         nsIPrincipal       *aLoadingPrincipal,
                         nsSecurityFlags     aSecurityFlags,
                         nsContentPolicyType aContentPolicyType,
                         const nsACString   &aContentType    = EmptyCString(),
                         const nsACString   &aContentCharset = EmptyCString());

nsresult NS_NewInputStreamChannelInternal(nsIChannel        **outChannel,
                                          nsIURI             *aUri,
                                          const nsAString    &aData,
                                          const nsACString   &aContentType,
                                          nsINode            *aLoadingNode,
                                          nsIPrincipal       *aLoadingPrincipal,
                                          nsIPrincipal       *aTriggeringPrincipal,
                                          nsSecurityFlags     aSecurityFlags,
                                          nsContentPolicyType aContentPolicyType,
                                          bool                aIsSrcdocChannel = false,
                                          nsIURI             *aBaseURI = nullptr);

nsresult NS_NewInputStreamChannel(nsIChannel        **outChannel,
                                  nsIURI             *aUri,
                                  const nsAString    &aData,
                                  const nsACString   &aContentType,
                                  nsIPrincipal       *aLoadingPrincipal,
                                  nsSecurityFlags     aSecurityFlags,
                                  nsContentPolicyType aContentPolicyType,
                                  bool                aIsSrcdocChannel = false,
                                  nsIURI             *aBaseURI = nullptr);

nsresult NS_NewInputStreamPump(nsIInputStreamPump **result,
                               nsIInputStream      *stream,
                               int64_t              streamPos = int64_t(-1),
                               int64_t              streamLen = int64_t(-1),
                               uint32_t             segsize = 0,
                               uint32_t             segcount = 0,
                               bool                 closeWhenDone = false);




nsresult NS_NewAsyncStreamCopier(nsIAsyncStreamCopier **result,
                                 nsIInputStream        *source,
                                 nsIOutputStream       *sink,
                                 nsIEventTarget        *target,
                                 bool                   sourceBuffered = true,
                                 bool                   sinkBuffered = true,
                                 uint32_t               chunkSize = 0,
                                 bool                   closeSource = true,
                                 bool                   closeSink = true);

nsresult NS_NewLoadGroup(nsILoadGroup      **result,
                         nsIRequestObserver *obs);


nsresult
NS_NewLoadGroup(nsILoadGroup **aResult, nsIPrincipal* aPrincipal);







bool
NS_LoadGroupMatchesPrincipal(nsILoadGroup *aLoadGroup,
                             nsIPrincipal *aPrincipal);

nsresult NS_NewDownloader(nsIStreamListener   **result,
                          nsIDownloadObserver  *observer,
                          nsIFile              *downloadLocation = nullptr);

nsresult NS_NewStreamLoader(nsIStreamLoader        **result,
                            nsIStreamLoaderObserver *observer,
                            nsIRequestObserver      *requestObserver = nullptr);

nsresult NS_NewStreamLoaderInternal(nsIStreamLoader        **outStream,
                                    nsIURI                  *aUri,
                                    nsIStreamLoaderObserver *aObserver,
                                    nsINode                 *aLoadingNode,
                                    nsIPrincipal            *aLoadingPrincipal,
                                    nsSecurityFlags          aSecurityFlags,
                                    nsContentPolicyType      aContentPolicyType,
                                    nsISupports             *aContext = nullptr,
                                    nsILoadGroup            *aLoadGroup = nullptr,
                                    nsIInterfaceRequestor   *aCallbacks = nullptr,
                                    nsLoadFlags              aLoadFlags = nsIRequest::LOAD_NORMAL,
                                    nsIURI                  *aReferrer = nullptr);

nsresult 
NS_NewStreamLoader(nsIStreamLoader        **outStream,
                   nsIURI                  *aUri,
                   nsIStreamLoaderObserver *aObserver,
                   nsINode                 *aLoadingNode,
                   nsSecurityFlags          aSecurityFlags,
                   nsContentPolicyType      aContentPolicyType,
                   nsISupports             *aContext = nullptr,
                   nsILoadGroup            *aLoadGroup = nullptr,
                   nsIInterfaceRequestor   *aCallbacks = nullptr,
                   nsLoadFlags              aLoadFlags = nsIRequest::LOAD_NORMAL,
                   nsIURI                  *aReferrer = nullptr);

nsresult 
NS_NewStreamLoader(nsIStreamLoader        **outStream,
                   nsIURI                  *aUri,
                   nsIStreamLoaderObserver *aObserver,
                   nsIPrincipal            *aLoadingPrincipal,
                   nsSecurityFlags          aSecurityFlags,
                   nsContentPolicyType      aContentPolicyType,
                   nsISupports             *aContext = nullptr,
                   nsILoadGroup            *aLoadGroup = nullptr,
                   nsIInterfaceRequestor   *aCallbacks = nullptr,
                   nsLoadFlags              aLoadFlags = nsIRequest::LOAD_NORMAL,
                   nsIURI                  *aReferrer = nullptr);

nsresult NS_NewUnicharStreamLoader(nsIUnicharStreamLoader        **result,
                                   nsIUnicharStreamLoaderObserver *observer);

nsresult NS_NewSyncStreamListener(nsIStreamListener **result,
                                  nsIInputStream    **stream);








nsresult NS_ImplementChannelOpen(nsIChannel      *channel,
                                 nsIInputStream **result);

nsresult NS_NewRequestObserverProxy(nsIRequestObserver **result,
                                    nsIRequestObserver  *observer,
                                    nsISupports         *context);

nsresult NS_NewSimpleStreamListener(nsIStreamListener **result,
                                    nsIOutputStream    *sink,
                                    nsIRequestObserver *observer = nullptr);

nsresult NS_CheckPortSafety(int32_t       port,
                            const char   *scheme,
                            nsIIOService *ioService = nullptr);


nsresult NS_CheckPortSafety(nsIURI *uri);

nsresult NS_NewProxyInfo(const nsACString &type,
                         const nsACString &host,
                         int32_t           port,
                         uint32_t          flags,
                         nsIProxyInfo    **result);

nsresult NS_GetFileProtocolHandler(nsIFileProtocolHandler **result,
                                   nsIIOService            *ioService = nullptr);

nsresult NS_GetFileFromURLSpec(const nsACString  &inURL,
                               nsIFile          **result,
                               nsIIOService      *ioService = nullptr);

nsresult NS_GetURLSpecFromFile(nsIFile      *file,
                               nsACString   &url,
                               nsIIOService *ioService = nullptr);








nsresult NS_GetURLSpecFromActualFile(nsIFile      *file,
                                     nsACString   &url,
                                     nsIIOService *ioService = nullptr);








nsresult NS_GetURLSpecFromDir(nsIFile      *file,
                              nsACString   &url,
                              nsIIOService *ioService = nullptr);








nsresult NS_GetReferrerFromChannel(nsIChannel *channel,
                                   nsIURI **referrer);

nsresult NS_ParseContentType(const nsACString &rawContentType,
                             nsCString        &contentType,
                             nsCString        &contentCharset);

nsresult NS_ExtractCharsetFromContentType(const nsACString &rawContentType,
                                          nsCString        &contentCharset,
                                          bool             *hadCharset,
                                          int32_t          *charsetStart,
                                          int32_t          *charsetEnd);

nsresult NS_NewLocalFileInputStream(nsIInputStream **result,
                                    nsIFile         *file,
                                    int32_t          ioFlags       = -1,
                                    int32_t          perm          = -1,
                                    int32_t          behaviorFlags = 0);

nsresult NS_NewPartialLocalFileInputStream(nsIInputStream **result,
                                           nsIFile         *file,
                                           uint64_t         offset,
                                           uint64_t         length,
                                           int32_t          ioFlags       = -1,
                                           int32_t          perm          = -1,
                                           int32_t          behaviorFlags = 0);

nsresult NS_NewLocalFileOutputStream(nsIOutputStream **result,
                                     nsIFile          *file,
                                     int32_t           ioFlags       = -1,
                                     int32_t           perm          = -1,
                                     int32_t           behaviorFlags = 0);


nsresult NS_NewAtomicFileOutputStream(nsIOutputStream **result,
                                      nsIFile          *file,
                                      int32_t           ioFlags       = -1,
                                      int32_t           perm          = -1,
                                      int32_t           behaviorFlags = 0);


nsresult NS_NewSafeLocalFileOutputStream(nsIOutputStream **result,
                                         nsIFile          *file,
                                         int32_t           ioFlags       = -1,
                                         int32_t           perm          = -1,
                                         int32_t           behaviorFlags = 0);

nsresult NS_NewLocalFileStream(nsIFileStream **result,
                               nsIFile        *file,
                               int32_t         ioFlags       = -1,
                               int32_t         perm          = -1,
                               int32_t         behaviorFlags = 0);




nsresult NS_BackgroundInputStream(nsIInputStream **result,
                                  nsIInputStream  *stream,
                                  uint32_t         segmentSize  = 0,
                                  uint32_t         segmentCount = 0);




nsresult NS_BackgroundOutputStream(nsIOutputStream **result,
                                   nsIOutputStream  *stream,
                                   uint32_t          segmentSize  = 0,
                                   uint32_t          segmentCount = 0);

MOZ_WARN_UNUSED_RESULT nsresult
NS_NewBufferedInputStream(nsIInputStream **result,
                          nsIInputStream  *str,
                          uint32_t         bufferSize);



nsresult NS_NewBufferedOutputStream(nsIOutputStream **result,
                                    nsIOutputStream  *str,
                                    uint32_t          bufferSize);












already_AddRefed<nsIOutputStream>
NS_BufferOutputStream(nsIOutputStream *aOutputStream,
                      uint32_t aBufferSize);


nsresult NS_NewPostDataStream(nsIInputStream  **result,
                              bool              isFile,
                              const nsACString &data);

nsresult NS_ReadInputStreamToBuffer(nsIInputStream *aInputStream,
                                    void **aDest,
                                    uint32_t aCount);


#ifdef MOZILLA_INTERNAL_API

nsresult NS_ReadInputStreamToString(nsIInputStream *aInputStream,
                                    nsACString &aDest,
                                    uint32_t aCount);

#endif

nsresult
NS_LoadPersistentPropertiesFromURI(nsIPersistentProperties **outResult,
                                   nsIURI                   *aUri,
                                   nsIPrincipal             *aLoadingPrincipal,
                                   nsContentPolicyType       aContentPolicyType,
                                   nsIIOService             *aIoService = nullptr);

nsresult
NS_LoadPersistentPropertiesFromURISpec(nsIPersistentProperties **outResult,
                                       const nsACString         &aSpec,
                                       nsIPrincipal             *aLoadingPrincipal,
                                       nsContentPolicyType       aContentPolicyType,
                                       const char               *aCharset = nullptr,
                                       nsIURI                   *aBaseURI = nullptr,
                                       nsIIOService             *aIoService = nullptr);











template <class T> inline void
NS_QueryNotificationCallbacks(T            *channel,
                              const nsIID  &iid,
                              void        **result)
{
    NS_PRECONDITION(channel, "null channel");
    *result = nullptr;

    nsCOMPtr<nsIInterfaceRequestor> cbs;
    channel->GetNotificationCallbacks(getter_AddRefs(cbs));
    if (cbs)
        cbs->GetInterface(iid, result);
    if (!*result) {
        
        nsCOMPtr<nsILoadGroup> loadGroup;
        channel->GetLoadGroup(getter_AddRefs(loadGroup));
        if (loadGroup) {
            loadGroup->GetNotificationCallbacks(getter_AddRefs(cbs));
            if (cbs)
                cbs->GetInterface(iid, result);
        }
    }
}





template <class C, class T> inline void
NS_QueryNotificationCallbacks(C           *channel,
                              nsCOMPtr<T> &result)
{
    NS_QueryNotificationCallbacks(channel, NS_GET_TEMPLATE_IID(T),
                                  getter_AddRefs(result));
}





inline void
NS_QueryNotificationCallbacks(nsIInterfaceRequestor  *callbacks,
                              nsILoadGroup           *loadGroup,
                              const nsIID            &iid,
                              void                  **result)
{
    *result = nullptr;

    if (callbacks)
        callbacks->GetInterface(iid, result);
    if (!*result) {
        
        if (loadGroup) {
            nsCOMPtr<nsIInterfaceRequestor> cbs;
            loadGroup->GetNotificationCallbacks(getter_AddRefs(cbs));
            if (cbs)
                cbs->GetInterface(iid, result);
        }
    }
}





bool NS_UsePrivateBrowsing(nsIChannel *channel);



#define NECKO_NO_APP_ID 0
#define NECKO_UNKNOWN_APP_ID UINT32_MAX

#define NECKO_SAFEBROWSING_APP_ID UINT32_MAX - 1





bool NS_GetAppInfo(nsIChannel *aChannel,
                   uint32_t *aAppID,
                   bool *aIsInBrowserElement);






nsresult NS_GetAppInfoFromClearDataNotification(nsISupports *aSubject,
                                                uint32_t *aAppID,
                                                bool *aBrowserOnly);




bool NS_ShouldCheckAppCache(nsIURI *aURI, bool usePrivateBrowsing);

bool NS_ShouldCheckAppCache(nsIPrincipal *aPrincipal, bool usePrivateBrowsing);







void NS_WrapAuthPrompt(nsIAuthPrompt *aAuthPrompt,
                       nsIAuthPrompt2 **aAuthPrompt2);





void NS_QueryAuthPrompt2(nsIInterfaceRequestor  *aCallbacks,
                         nsIAuthPrompt2        **aAuthPrompt);





void NS_QueryAuthPrompt2(nsIChannel      *aChannel,
                         nsIAuthPrompt2 **aAuthPrompt);


template <class T> inline void
NS_QueryNotificationCallbacks(nsIInterfaceRequestor *callbacks,
                              nsILoadGroup          *loadGroup,
                              nsCOMPtr<T>           &result)
{
    NS_QueryNotificationCallbacks(callbacks, loadGroup,
                                  NS_GET_TEMPLATE_IID(T),
                                  getter_AddRefs(result));
}


template <class T> inline void
NS_QueryNotificationCallbacks(const nsCOMPtr<nsIInterfaceRequestor> &aCallbacks,
                              const nsCOMPtr<nsILoadGroup>          &aLoadGroup,
                              nsCOMPtr<T>                           &aResult)
{
    NS_QueryNotificationCallbacks(aCallbacks.get(), aLoadGroup.get(), aResult);
}


template <class T> inline void
NS_QueryNotificationCallbacks(const nsCOMPtr<nsIChannel> &aChannel,
                              nsCOMPtr<T>                &aResult)
{
    NS_QueryNotificationCallbacks(aChannel.get(), aResult);
}






nsresult
NS_NewNotificationCallbacksAggregation(nsIInterfaceRequestor  *callbacks,
                                       nsILoadGroup           *loadGroup,
                                       nsIEventTarget         *target,
                                       nsIInterfaceRequestor **result);

nsresult
NS_NewNotificationCallbacksAggregation(nsIInterfaceRequestor  *callbacks,
                                       nsILoadGroup           *loadGroup,
                                       nsIInterfaceRequestor **result);




bool NS_IsOffline();

bool NS_IsAppOffline(uint32_t appId);

bool NS_IsAppOffline(nsIPrincipal *principal);







nsresult NS_DoImplGetInnermostURI(nsINestedURI *nestedURI, nsIURI **result);

nsresult NS_ImplGetInnermostURI(nsINestedURI *nestedURI, nsIURI **result);






nsresult NS_EnsureSafeToReturn(nsIURI *uri, nsIURI **result);




void NS_TryToSetImmutable(nsIURI *uri);






already_AddRefed<nsIURI> NS_TryToMakeImmutable(nsIURI *uri,
                                               nsresult *outRv = nullptr);





nsresult NS_URIChainHasFlags(nsIURI   *uri,
                             uint32_t  flags,
                             bool     *result);





already_AddRefed<nsIURI> NS_GetInnermostURI(nsIURI *aURI);








nsresult NS_GetFinalChannelURI(nsIChannel *channel, nsIURI **uri);





uint32_t NS_SecurityHashURI(nsIURI *aURI);

bool NS_SecurityCompareURIs(nsIURI *aSourceURI,
                            nsIURI *aTargetURI,
                            bool aStrictFileOriginPolicy);

bool NS_URIIsLocalFile(nsIURI *aURI);







bool NS_RelaxStrictFileOriginPolicy(nsIURI *aTargetURI,
                                    nsIURI *aSourceURI,
                                    bool aAllowDirectoryTarget = false);

bool NS_IsInternalSameURIRedirect(nsIChannel *aOldChannel,
                                  nsIChannel *aNewChannel,
                                  uint32_t aFlags);

bool NS_IsHSTSUpgradeRedirect(nsIChannel *aOldChannel,
                              nsIChannel *aNewChannel,
                              uint32_t aFlags);

nsresult NS_LinkRedirectChannels(uint32_t channelId,
                                 nsIParentChannel *parentChannel,
                                 nsIChannel **_result);





nsresult NS_MakeRandomInvalidURLString(nsCString &result);








nsresult NS_CheckIsJavaCompatibleURLString(nsCString& urlString, bool *result);





uint32_t NS_GetContentDispositionFromToken(const nsAString &aDispToken);






uint32_t NS_GetContentDispositionFromHeader(const nsACString &aHeader,
                                            nsIChannel *aChan = nullptr);







nsresult NS_GetFilenameFromDisposition(nsAString &aFilename,
                                       const nsACString &aDisposition,
                                       nsIURI *aURI = nullptr);




void net_EnsurePSMInit();




bool NS_IsAboutBlank(nsIURI *uri);

nsresult NS_GenerateHostPort(const nsCString &host, int32_t port,
                             nsACString &hostLine);









void NS_SniffContent(const char *aSnifferType, nsIRequest *aRequest,
                     const uint8_t *aData, uint32_t aLength,
                     nsACString &aSniffedType);






bool NS_IsSrcdocChannel(nsIChannel *aChannel);







bool NS_IsReasonableHTTPHeaderValue(const nsACString &aValue);





bool NS_IsValidHTTPToken(const nsACString &aToken);

namespace mozilla {
namespace net {

const static uint64_t kJS_MAX_SAFE_UINTEGER = +9007199254740991ULL;
const static  int64_t kJS_MIN_SAFE_INTEGER  = -9007199254740991LL;
const static  int64_t kJS_MAX_SAFE_INTEGER  = +9007199254740991LL;


bool InScriptableRange(int64_t val);


bool InScriptableRange(uint64_t val);

} 
} 


#ifndef MOZILLA_INTERNAL_API
#include "nsNetUtil.inl"
#endif

#endif 
