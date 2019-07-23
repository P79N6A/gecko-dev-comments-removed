








































#include "nsIURI.h"
#include "nsIURL.h"
#include "nsExternalProtocolHandler.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsServiceManagerUtils.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIStringBundle.h"
#include "nsIPrefService.h"
#include "nsIPrompt.h"
#include "nsNetUtil.h"
#include "nsIChannelEventSink.h"
#include "nsThreadUtils.h"
#include "nsEscape.h"
#include "nsExternalHelperAppService.h"


#include "nsCExternalHandlerService.h"
#include "nsIExternalProtocolService.h"






class nsExtProtocolChannel : public nsIChannel
{
    friend class nsProtocolRedirect;

public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSICHANNEL
    NS_DECL_NSIREQUEST

    nsExtProtocolChannel();
    virtual ~nsExtProtocolChannel();

    nsresult SetURI(nsIURI*);

private:
    nsresult OpenURL();
    void Finish(nsresult aResult);
    
    nsCOMPtr<nsIURI> mUrl;
    nsCOMPtr<nsIURI> mOriginalURI;
    nsresult mStatus;
    nsLoadFlags mLoadFlags;
    PRBool mIsPending;
    PRBool mWasOpened;
    
    nsCOMPtr<nsIInterfaceRequestor> mCallbacks;
    nsCOMPtr<nsILoadGroup> mLoadGroup;
    nsCOMPtr<nsIStreamListener> mListener;
    nsCOMPtr<nsISupports> mContext;
};

NS_IMPL_THREADSAFE_ADDREF(nsExtProtocolChannel)
NS_IMPL_THREADSAFE_RELEASE(nsExtProtocolChannel)

NS_INTERFACE_MAP_BEGIN(nsExtProtocolChannel)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIChannel)
   NS_INTERFACE_MAP_ENTRY(nsIChannel)
   NS_INTERFACE_MAP_ENTRY(nsIRequest)
NS_INTERFACE_MAP_END_THREADSAFE

nsExtProtocolChannel::nsExtProtocolChannel() : mStatus(NS_OK), 
                                               mIsPending(PR_FALSE),
                                               mWasOpened(PR_FALSE)
{
}

nsExtProtocolChannel::~nsExtProtocolChannel()
{}

NS_IMETHODIMP nsExtProtocolChannel::GetLoadGroup(nsILoadGroup * *aLoadGroup)
{
  NS_IF_ADDREF(*aLoadGroup = mLoadGroup);
  return NS_OK;
}

NS_IMETHODIMP nsExtProtocolChannel::SetLoadGroup(nsILoadGroup * aLoadGroup)
{
  mLoadGroup = aLoadGroup;
  return NS_OK;
}

NS_IMETHODIMP nsExtProtocolChannel::GetNotificationCallbacks(nsIInterfaceRequestor* *aCallbacks)
{
  NS_IF_ADDREF(*aCallbacks = mCallbacks);
  return NS_OK;
}

NS_IMETHODIMP nsExtProtocolChannel::SetNotificationCallbacks(nsIInterfaceRequestor* aCallbacks)
{
  mCallbacks = aCallbacks;
  return NS_OK;
}

NS_IMETHODIMP 
nsExtProtocolChannel::GetSecurityInfo(nsISupports * *aSecurityInfo)
{
  *aSecurityInfo = nsnull;
  return NS_OK;
}

NS_IMETHODIMP nsExtProtocolChannel::GetOriginalURI(nsIURI* *aURI)
{
  NS_IF_ADDREF(*aURI = mOriginalURI);
  return NS_OK; 
}
 
NS_IMETHODIMP nsExtProtocolChannel::SetOriginalURI(nsIURI* aURI)
{
  NS_ENSURE_ARG_POINTER(aURI);
  mOriginalURI = aURI;
  return NS_OK;
}
 
NS_IMETHODIMP nsExtProtocolChannel::GetURI(nsIURI* *aURI)
{
  *aURI = mUrl;
  NS_IF_ADDREF(*aURI);
  return NS_OK; 
}
 
nsresult nsExtProtocolChannel::SetURI(nsIURI* aURI)
{
  mUrl = aURI;
  return NS_OK; 
}

nsresult nsExtProtocolChannel::OpenURL()
{
  nsresult rv = NS_ERROR_FAILURE;
  nsCOMPtr<nsIExternalProtocolService> extProtService (do_GetService(NS_EXTERNALPROTOCOLSERVICE_CONTRACTID));

  if (extProtService)
  {
#ifdef DEBUG
    nsCAutoString urlScheme;
    mUrl->GetScheme(urlScheme);
    PRBool haveHandler = PR_FALSE;
    extProtService->ExternalProtocolHandlerExists(urlScheme.get(), &haveHandler);
    NS_ASSERTION(haveHandler, "Why do we have a channel for this url if we don't support the protocol?");
#endif

    rv = extProtService->LoadURI(mUrl, mCallbacks);
  }

  
  mCallbacks = 0;

  return rv;
}

NS_IMETHODIMP nsExtProtocolChannel::Open(nsIInputStream **_retval)
{
  OpenURL();
  return NS_ERROR_NO_CONTENT; 
}

class nsProtocolRedirect : public nsRunnable {
  public:
    nsProtocolRedirect(nsIURI *aURI, nsIHandlerInfo *aHandlerInfo,
                       nsIStreamListener *aListener, nsISupports *aContext,
                       nsExtProtocolChannel *aOriginalChannel)
      : mURI(aURI), mHandlerInfo(aHandlerInfo), mListener(aListener), 
        mContext(aContext), mOriginalChannel(aOriginalChannel) {}

    NS_IMETHOD Run() 
    {
      
      nsCOMPtr<nsIHandlerApp> handlerApp;
      nsresult rv = 
        mHandlerInfo->GetPreferredApplicationHandler(getter_AddRefs(handlerApp));
      if (NS_FAILED(rv)) {
        mOriginalChannel->Finish(rv);
        return NS_OK;
      }

      nsCOMPtr<nsIWebHandlerApp> webHandlerApp = do_QueryInterface(handlerApp,
                                                                   &rv);
      if (NS_FAILED(rv)) {
        mOriginalChannel->Finish(rv);
        return NS_OK; 
      }

      nsCAutoString uriTemplate;
      rv = webHandlerApp->GetUriTemplate(uriTemplate);
      if (NS_FAILED(rv)) {
        mOriginalChannel->Finish(rv);
        return NS_OK; 
      }
            
      
      nsCAutoString uriSpecToHandle;
      rv = mURI->GetSpec(uriSpecToHandle);
      if (NS_FAILED(rv)) {
        mOriginalChannel->Finish(rv);
        return NS_OK; 
      }

      
      
      
      

      
      
      
      
      
      nsCAutoString escapedUriSpecToHandle;
      NS_EscapeURL(uriSpecToHandle, esc_Minimal | esc_Forced | esc_Colon,
                   escapedUriSpecToHandle);

      
      
      
      
      uriTemplate.ReplaceSubstring(NS_LITERAL_CSTRING("%s"),
                                   escapedUriSpecToHandle);

      
      
      nsCOMPtr<nsIURI> uriToSend;
      rv = NS_NewURI(getter_AddRefs(uriToSend), uriTemplate);
      if (NS_FAILED(rv)) {
        mOriginalChannel->Finish(rv);
        return NS_OK; 
      }

      
      nsCOMPtr<nsIChannel> newChannel;
      rv = NS_NewChannel(getter_AddRefs(newChannel), uriToSend, nsnull,
                         mOriginalChannel->mLoadGroup,
                         mOriginalChannel->mCallbacks,
                         mOriginalChannel->mLoadFlags 
                         | nsIChannel::LOAD_REPLACE);
      if (NS_FAILED(rv)) {
        mOriginalChannel->Finish(rv);
        return NS_OK; 
      }

      nsCOMPtr<nsIChannelEventSink> eventSink;
      NS_QueryNotificationCallbacks(mOriginalChannel->mCallbacks,
                                    mOriginalChannel->mLoadGroup, eventSink);

      if (eventSink) {
        
        rv = eventSink->OnChannelRedirect(mOriginalChannel, newChannel, 
                                          nsIChannelEventSink::REDIRECT_TEMPORARY |
                                          nsIChannelEventSink::REDIRECT_INTERNAL);
        if (NS_FAILED(rv)) {
          mOriginalChannel->Finish(rv);
          return NS_OK;
        }
      }

      rv = newChannel->AsyncOpen(mListener, mContext);
      if (NS_FAILED(rv)) {
        mOriginalChannel->Finish(rv);
        return NS_OK; 
      }
      
      mOriginalChannel->Finish(NS_BINDING_REDIRECTED);
      return NS_OK;
    }

  private:
    nsCOMPtr<nsIURI> mURI;
    nsCOMPtr<nsIHandlerInfo> mHandlerInfo;
    nsCOMPtr<nsIStreamListener> mListener;
    nsCOMPtr<nsISupports> mContext;
    nsCOMPtr<nsExtProtocolChannel> mOriginalChannel;
};

NS_IMETHODIMP nsExtProtocolChannel::AsyncOpen(nsIStreamListener *listener, nsISupports *ctxt)
{
  NS_ENSURE_ARG_POINTER(listener);
  NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);
  NS_ENSURE_TRUE(!mWasOpened, NS_ERROR_ALREADY_OPENED);

  mWasOpened = PR_TRUE;
  mListener = listener;
  mContext = ctxt;

  if (!gExtProtSvc) {
    return NS_ERROR_FAILURE;
  }

  nsCAutoString urlScheme;  
  nsresult rv = mUrl->GetScheme(urlScheme);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  nsCOMPtr<nsIHandlerInfo> handlerInfo;
  rv = gExtProtSvc->GetProtocolHandlerInfo(urlScheme, 
                                           getter_AddRefs(handlerInfo));
  if (NS_SUCCEEDED(rv)) {
    PRInt32 preferredAction;                                           
    rv = handlerInfo->GetPreferredAction(&preferredAction);

    
    
    if (preferredAction == nsIHandlerInfo::useHelperApp) {

      
      
      
      nsCOMPtr<nsIRunnable> event = new nsProtocolRedirect(mUrl, handlerInfo,
                                                           listener, ctxt,
                                                           this);

      
      
      rv = NS_DispatchToCurrentThread(event);
      if (NS_SUCCEEDED(rv)) {
        mIsPending = PR_TRUE;

        
        
        if (mLoadGroup)
          (void)mLoadGroup->AddRequest(this, nsnull);

        return rv;
      }
    }
  }
  
  
  OpenURL();
  return NS_ERROR_NO_CONTENT; 
}










void nsExtProtocolChannel::Finish(nsresult aStatus)
{
  mStatus = aStatus;

  if (aStatus != NS_BINDING_REDIRECTED && mListener) {
    (void)mListener->OnStartRequest(this, mContext);
    (void)mListener->OnStopRequest(this, mContext, aStatus);
  }
  
  mIsPending = PR_FALSE;
  
  if (mLoadGroup) {
    (void)mLoadGroup->RemoveRequest(this, nsnull, aStatus);
  }
  return;
}

NS_IMETHODIMP nsExtProtocolChannel::GetLoadFlags(nsLoadFlags *aLoadFlags)
{
  *aLoadFlags = mLoadFlags;
  return NS_OK;
}

NS_IMETHODIMP nsExtProtocolChannel::SetLoadFlags(nsLoadFlags aLoadFlags)
{
  mLoadFlags = aLoadFlags;
  return NS_OK;
}

NS_IMETHODIMP nsExtProtocolChannel::GetContentType(nsACString &aContentType)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsExtProtocolChannel::SetContentType(const nsACString &aContentType)
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsExtProtocolChannel::GetContentCharset(nsACString &aContentCharset)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsExtProtocolChannel::SetContentCharset(const nsACString &aContentCharset)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsExtProtocolChannel::GetContentLength(PRInt32 * aContentLength)
{
  *aContentLength = -1;
  return NS_OK;
}

NS_IMETHODIMP
nsExtProtocolChannel::SetContentLength(PRInt32 aContentLength)
{
  NS_NOTREACHED("SetContentLength");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsExtProtocolChannel::GetOwner(nsISupports * *aPrincipal)
{
  NS_NOTREACHED("GetOwner");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsExtProtocolChannel::SetOwner(nsISupports * aPrincipal)
{
  NS_NOTREACHED("SetOwner");
  return NS_ERROR_NOT_IMPLEMENTED;
}





NS_IMETHODIMP nsExtProtocolChannel::GetName(nsACString &result)
{
  return mUrl->GetSpec(result);
}

NS_IMETHODIMP nsExtProtocolChannel::IsPending(PRBool *result)
{
  *result = mIsPending;
  return NS_OK; 
}

NS_IMETHODIMP nsExtProtocolChannel::GetStatus(nsresult *status)
{
  *status = mStatus;
  return NS_OK;
}

NS_IMETHODIMP nsExtProtocolChannel::Cancel(nsresult status)
{
  mStatus = status;
  return NS_OK;
}

NS_IMETHODIMP nsExtProtocolChannel::Suspend()
{
  NS_NOTREACHED("Suspend");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsExtProtocolChannel::Resume()
{
  NS_NOTREACHED("Resume");
  return NS_ERROR_NOT_IMPLEMENTED;
}





nsExternalProtocolHandler::nsExternalProtocolHandler()
{
  m_schemeName = "default";
}


nsExternalProtocolHandler::~nsExternalProtocolHandler()
{}

NS_IMPL_THREADSAFE_ADDREF(nsExternalProtocolHandler)
NS_IMPL_THREADSAFE_RELEASE(nsExternalProtocolHandler)

NS_INTERFACE_MAP_BEGIN(nsExternalProtocolHandler)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIProtocolHandler)
   NS_INTERFACE_MAP_ENTRY(nsIProtocolHandler)
   NS_INTERFACE_MAP_ENTRY(nsIExternalProtocolHandler)
   NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_END_THREADSAFE

NS_IMETHODIMP nsExternalProtocolHandler::GetScheme(nsACString &aScheme)
{
  aScheme = m_schemeName;
  return NS_OK;
}

NS_IMETHODIMP nsExternalProtocolHandler::GetDefaultPort(PRInt32 *aDefaultPort)
{
  *aDefaultPort = 0;
    return NS_OK;
}

NS_IMETHODIMP 
nsExternalProtocolHandler::AllowPort(PRInt32 port, const char *scheme, PRBool *_retval)
{
    
    *_retval = PR_FALSE;
    return NS_OK;
}

PRBool nsExternalProtocolHandler::HaveExternalProtocolHandler(nsIURI * aURI)
{
  PRBool haveHandler = PR_FALSE;
  if (aURI)
  {
    nsCAutoString scheme;
    aURI->GetScheme(scheme);
    if (gExtProtSvc)
      gExtProtSvc->ExternalProtocolHandlerExists(scheme.get(), &haveHandler);
  }

  return haveHandler;
}

NS_IMETHODIMP nsExternalProtocolHandler::GetProtocolFlags(PRUint32 *aUritype)
{
    
    *aUritype = URI_NORELATIVE | URI_NOAUTH | URI_LOADABLE_BY_ANYONE |
        URI_NON_PERSISTABLE | URI_DOES_NOT_RETURN_DATA;
    return NS_OK;
}

NS_IMETHODIMP nsExternalProtocolHandler::NewURI(const nsACString &aSpec,
                                                const char *aCharset, 
                                                nsIURI *aBaseURI,
                                                nsIURI **_retval)
{
  nsresult rv;
  nsCOMPtr<nsIURI> uri = do_CreateInstance(NS_SIMPLEURI_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = uri->SetSpec(aSpec);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = uri);
  return NS_OK;
}

NS_IMETHODIMP nsExternalProtocolHandler::NewChannel(nsIURI *aURI, nsIChannel **_retval)
{
  

  PRBool haveExternalHandler = HaveExternalProtocolHandler(aURI);
  if (haveExternalHandler)
  {
    nsCOMPtr<nsIChannel> channel;
    NS_NEWXPCOM(channel, nsExtProtocolChannel);
    if (!channel) return NS_ERROR_OUT_OF_MEMORY;

    ((nsExtProtocolChannel*) channel.get())->SetURI(aURI);
    channel->SetOriginalURI(aURI);

    if (_retval)
    {
      *_retval = channel;
      NS_IF_ADDREF(*_retval);
      return NS_OK;
    }
  }

  return NS_ERROR_UNKNOWN_PROTOCOL;
}




NS_IMETHODIMP nsExternalProtocolHandler::ExternalAppExistsForScheme(const nsACString& aScheme, PRBool *_retval)
{
  if (gExtProtSvc)
    return gExtProtSvc->ExternalProtocolHandlerExists(
      PromiseFlatCString(aScheme).get(), _retval);

  
  *_retval = PR_FALSE;
  return NS_OK;
}

nsBlockedExternalProtocolHandler::nsBlockedExternalProtocolHandler()
{
    m_schemeName = "default-blocked";
}

NS_IMETHODIMP
nsBlockedExternalProtocolHandler::NewChannel(nsIURI *aURI,
                                             nsIChannel **_retval)
{
    *_retval = nsnull;
    return NS_ERROR_UNKNOWN_PROTOCOL;
}
