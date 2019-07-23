





































#include "nsURILoader.h"
#include "nsAutoPtr.h"
#include "nsIURIContentListener.h"
#include "nsIContentHandler.h"
#include "nsILoadGroup.h"
#include "nsIDocumentLoader.h"
#include "nsIWebProgress.h"
#include "nsIWebProgressListener.h"
#include "nsIIOService.h"
#include "nsIServiceManager.h"
#include "nsIStreamListener.h"
#include "nsIURI.h"
#include "nsIChannel.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIProgressEventSink.h"
#include "nsIInputStream.h"
#include "nsIStreamConverterService.h"
#include "nsWeakReference.h"
#include "nsIHttpChannel.h"
#include "nsIMultiPartChannel.h"
#include "netCore.h"
#include "nsCRT.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeOwner.h"

#include "nsXPIDLString.h"
#include "nsString.h"

#include "nsIDOMWindowInternal.h"
#include "nsReadableUtils.h"
#include "nsDOMError.h"

#include "nsICategoryManager.h"
#include "nsCExternalHandlerService.h" 

#include "nsIMIMEHeaderParam.h"
#include "nsNetCID.h"

#include "nsMimeTypes.h"

#include "nsDocLoader.h"

#ifdef PR_LOGGING
PRLogModuleInfo* nsURILoader::mLog = nsnull;
#endif

#define LOG(args) PR_LOG(nsURILoader::mLog, PR_LOG_DEBUG, args)
#define LOG_ERROR(args) PR_LOG(nsURILoader::mLog, PR_LOG_ERROR, args)
#define LOG_ENABLED() PR_LOG_TEST(nsURILoader::mLog, PR_LOG_DEBUG)







class nsDocumentOpenInfo : public nsIStreamListener
{
public:
  
  nsDocumentOpenInfo();

  
  
  nsDocumentOpenInfo(nsIInterfaceRequestor* aWindowContext,
                     PRUint32 aFlags,
                     nsURILoader* aURILoader);

  NS_DECL_ISUPPORTS

  




  nsresult Prepare();

  
  
  nsresult DispatchContent(nsIRequest *request, nsISupports * aCtxt);

  
  
  
  nsresult ConvertData(nsIRequest *request,
                       nsIURIContentListener *aListener,
                       const nsACString & aSrcContentType,
                       const nsACString & aOutContentType);

  





  PRBool TryContentListener(nsIURIContentListener* aListener,
                            nsIChannel* aChannel);

  
  NS_DECL_NSIREQUESTOBSERVER

  
  NS_DECL_NSISTREAMLISTENER

protected:
  ~nsDocumentOpenInfo();

protected:
  



  nsCOMPtr<nsIURIContentListener> m_contentListener;

  



  nsCOMPtr<nsIStreamListener> m_targetStreamListener;

  



  nsCOMPtr<nsIInterfaceRequestor> m_originalContext;

  





  PRUint32 mFlags;

  


  nsCString mContentType;

  



  nsRefPtr<nsURILoader> mURILoader;
};

NS_IMPL_THREADSAFE_ADDREF(nsDocumentOpenInfo)
NS_IMPL_THREADSAFE_RELEASE(nsDocumentOpenInfo)

NS_INTERFACE_MAP_BEGIN(nsDocumentOpenInfo)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIRequestObserver)
  NS_INTERFACE_MAP_ENTRY(nsIRequestObserver)
  NS_INTERFACE_MAP_ENTRY(nsIStreamListener)
NS_INTERFACE_MAP_END_THREADSAFE

nsDocumentOpenInfo::nsDocumentOpenInfo()
{
  NS_NOTREACHED("This should never be called\n");
}

nsDocumentOpenInfo::nsDocumentOpenInfo(nsIInterfaceRequestor* aWindowContext,
                                       PRUint32 aFlags,
                                       nsURILoader* aURILoader)
  : m_originalContext(aWindowContext),
    mFlags(aFlags),
    mURILoader(aURILoader)
{
}

nsDocumentOpenInfo::~nsDocumentOpenInfo()
{
}

nsresult nsDocumentOpenInfo::Prepare()
{
  LOG(("[0x%p] nsDocumentOpenInfo::Prepare", this));

  nsresult rv;

  
  m_contentListener = do_GetInterface(m_originalContext, &rv);
  return rv;
}

NS_IMETHODIMP nsDocumentOpenInfo::OnStartRequest(nsIRequest *request, nsISupports * aCtxt)
{
  LOG(("[0x%p] nsDocumentOpenInfo::OnStartRequest", this));
  
  nsresult rv = NS_OK;

  
  
  
  
  
  
  
  
  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(request, &rv));

  if (NS_SUCCEEDED(rv)) {
    PRUint32 responseCode = 0;

    rv = httpChannel->GetResponseStatus(&responseCode);

    if (NS_FAILED(rv)) {
      LOG_ERROR(("  Failed to get HTTP response status"));
      
      
      return NS_OK;
    }

    LOG(("  HTTP response status: %d", responseCode));

    if (204 == responseCode || 205 == responseCode) {
      return NS_BINDING_ABORTED;
    }
  }

  
  
  
  nsresult status;

  rv = request->GetStatus(&status);
  
  NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to get request status!");
  if (NS_FAILED(rv)) return rv;

  if (NS_FAILED(status)) {
    LOG_ERROR(("  Request failed, status: 0x%08X", rv));
  
    
    
    
    
    return NS_OK;
  }

  rv = DispatchContent(request, aCtxt);

  LOG(("  After dispatch, m_targetStreamListener: 0x%p, rv: 0x%08X", m_targetStreamListener.get(), rv));

  NS_ASSERTION(NS_SUCCEEDED(rv) || !m_targetStreamListener,
               "Must not have an m_targetStreamListener with a failure return!");

  NS_ENSURE_SUCCESS(rv, rv);
  
  if (m_targetStreamListener)
    rv = m_targetStreamListener->OnStartRequest(request, aCtxt);

  LOG(("  OnStartRequest returning: 0x%08X", rv));
  
  return rv;
}

NS_IMETHODIMP nsDocumentOpenInfo::OnDataAvailable(nsIRequest *request, nsISupports * aCtxt,
                                                  nsIInputStream * inStr, PRUint32 sourceOffset, PRUint32 count)
{
  
  

  nsresult rv = NS_OK;
  
  if (m_targetStreamListener)
    rv = m_targetStreamListener->OnDataAvailable(request, aCtxt, inStr, sourceOffset, count);
  return rv;
}

NS_IMETHODIMP nsDocumentOpenInfo::OnStopRequest(nsIRequest *request, nsISupports *aCtxt, 
                                                nsresult aStatus)
{
  LOG(("[0x%p] nsDocumentOpenInfo::OnStopRequest", this));
  
  if ( m_targetStreamListener)
  {
    nsCOMPtr<nsIStreamListener> listener(m_targetStreamListener);

    
    
    m_targetStreamListener = 0;
    mContentType.Truncate();
    listener->OnStopRequest(request, aCtxt, aStatus);
  }

  
  
  
  
  return NS_OK;
}

nsresult nsDocumentOpenInfo::DispatchContent(nsIRequest *request, nsISupports * aCtxt)
{
  LOG(("[0x%p] nsDocumentOpenInfo::DispatchContent for type '%s'", this, mContentType.get()));

  NS_PRECONDITION(!m_targetStreamListener,
                  "Why do we already have a target stream listener?");
  
  nsresult rv;
  nsCOMPtr<nsIChannel> aChannel = do_QueryInterface(request);
  if (!aChannel) {
    LOG_ERROR(("  Request is not a channel.  Bailing."));
    return NS_ERROR_FAILURE;
  }

  NS_NAMED_LITERAL_CSTRING(anyType, "*/*");
  if (mContentType.IsEmpty() || mContentType == anyType) {
    rv = aChannel->GetContentType(mContentType);
    if (NS_FAILED(rv)) return rv;
    LOG(("  Got type from channel: '%s'", mContentType.get()));
  }

  PRBool isGuessFromExt =
    mContentType.LowerCaseEqualsASCII(APPLICATION_GUESS_FROM_EXT);
  if (isGuessFromExt) {
    
    
    mContentType = APPLICATION_OCTET_STREAM;
    aChannel->SetContentType(NS_LITERAL_CSTRING(APPLICATION_OCTET_STREAM));
  }

  
  
  
  
  PRBool forceExternalHandling = PR_FALSE;
  nsCAutoString disposition;
  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(request));
  nsCOMPtr<nsIURI> uri;
  if (httpChannel)
  {
    rv = httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("content-disposition"),
                                        disposition);
    httpChannel->GetURI(getter_AddRefs(uri));
  }
  else
  {
    nsCOMPtr<nsIMultiPartChannel> multipartChannel(do_QueryInterface(request));
    if (multipartChannel)
    {
      rv = multipartChannel->GetContentDisposition(disposition);
    }
  }

  LOG(("  Disposition header: '%s'", disposition.get()));

  if (NS_SUCCEEDED(rv) && !disposition.IsEmpty())
  {
    nsCOMPtr<nsIMIMEHeaderParam> mimehdrpar = do_GetService(NS_MIMEHEADERPARAM_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv))
    {
      nsCAutoString fallbackCharset;
      if (uri)
        uri->GetOriginCharset(fallbackCharset);
      nsAutoString dispToken;
      
      rv = mimehdrpar->GetParameter(disposition, "", fallbackCharset,
                                    PR_TRUE, nsnull, dispToken);
      
      
      
      
      if (NS_FAILED(rv) || 
          (
           
           
           !dispToken.IsEmpty() &&
           !dispToken.LowerCaseEqualsLiteral("inline") &&
          
          
          
           !dispToken.EqualsIgnoreCase("filename", 8)) &&
          
           !dispToken.EqualsIgnoreCase("name", 4))
        
        forceExternalHandling = PR_TRUE;
    }
  }

  LOG(("  forceExternalHandling: %s", forceExternalHandling ? "yes" : "no"));
    
  
  nsCOMPtr<nsIURIContentListener> contentListener;
  
  nsXPIDLCString desiredContentType;

  if (!forceExternalHandling)
  {
    
    
    
    
    if (m_contentListener && TryContentListener(m_contentListener, aChannel)) {
      LOG(("  Success!  Our default listener likes this type"));
      
      return NS_OK;
    }

    
    if (!(mFlags & nsIURILoader::DONT_RETARGET)) {

      
      
      
      
      PRInt32 count = mURILoader->m_listeners.Count();
      nsCOMPtr<nsIURIContentListener> listener;
      for (PRInt32 i = 0; i < count; i++) {
        listener = do_QueryReferent(mURILoader->m_listeners[i]);
        if (listener) {
          if (TryContentListener(listener, aChannel)) {
            LOG(("  Found listener registered on the URILoader"));
            return NS_OK;
          }
        } else {
          
          mURILoader->m_listeners.RemoveObjectAt(i--);
          --count;
        }
      }

      
      
      
      
      
      nsCOMPtr<nsICategoryManager> catman =
        do_GetService(NS_CATEGORYMANAGER_CONTRACTID);
      if (catman) {
        nsXPIDLCString contractidString;
        rv = catman->GetCategoryEntry(NS_CONTENT_LISTENER_CATEGORYMANAGER_ENTRY,
                                      mContentType.get(),
                                      getter_Copies(contractidString));
        if (NS_SUCCEEDED(rv) && !contractidString.IsEmpty()) {
          LOG(("  Listener contractid for '%s' is '%s'",
               mContentType.get(), contractidString.get()));

          listener = do_CreateInstance(contractidString);
          LOG(("  Listener from category manager: 0x%p", listener.get()));
          
          if (listener && TryContentListener(listener, aChannel)) {
            LOG(("  Listener from category manager likes this type"));
            return NS_OK;
          }
        }
      }

      
      
      
      nsCAutoString handlerContractID (NS_CONTENT_HANDLER_CONTRACTID_PREFIX);
      handlerContractID += mContentType;

      nsCOMPtr<nsIContentHandler> contentHandler =
        do_CreateInstance(handlerContractID.get());
      if (contentHandler) {
        LOG(("  Content handler found"));
        rv = contentHandler->HandleContent(mContentType.get(),
                                           m_originalContext, request);
        
        
        if (rv != NS_ERROR_WONT_HANDLE_CONTENT) {
          if (NS_FAILED(rv)) {
            
            
            LOG(("  Content handler failed.  Aborting load"));
            request->Cancel(rv);
          }
#ifdef PR_LOGGING
          else {
            LOG(("  Content handler taking over load"));
          }
#endif

          return rv;
        }
      }
    }
  } else if (mFlags & nsIURILoader::DONT_RETARGET) {
    
    
    LOG(("  External handling forced, but not allowed to retarget -> aborting"));
    return NS_ERROR_WONT_HANDLE_CONTENT;
  }

  NS_ASSERTION(!m_targetStreamListener,
               "If we found a listener, why are we not using it?");
  
  
  
  
  
  

  
  
  
  
  
  
  
  if (mContentType != anyType) {
    rv = ConvertData(request, m_contentListener, mContentType, anyType);
    if (NS_FAILED(rv)) {
      m_targetStreamListener = nsnull;
    } else if (m_targetStreamListener) {
      
      
      LOG(("  Converter taking over now"));
      return NS_OK;
    }
  }

  if (mFlags & nsIURILoader::DONT_RETARGET) {
    LOG(("  Listener not interested and no stream converter exists, and retargeting disallowed -> aborting"));
    return NS_ERROR_WONT_HANDLE_CONTENT;
  }

  
  
  
  if (httpChannel) {
    PRBool requestSucceeded;
    httpChannel->GetRequestSucceeded(&requestSucceeded);
    if (!requestSucceeded) {
      
      return NS_ERROR_FILE_NOT_FOUND;
    }
  }
  
  
  
  
  
  
  nsCOMPtr<nsIExternalHelperAppService> helperAppService =
    do_GetService(NS_EXTERNALHELPERAPPSERVICE_CONTRACTID, &rv);
  if (helperAppService) {
    LOG(("  Passing load off to helper app service"));

    
    
    nsLoadFlags loadFlags = 0;
    request->GetLoadFlags(&loadFlags);
    request->SetLoadFlags(loadFlags | nsIChannel::LOAD_RETARGETED_DOCUMENT_URI
                                    | nsIChannel::LOAD_TARGETED);

    if (isGuessFromExt) {
      mContentType = APPLICATION_GUESS_FROM_EXT;
      aChannel->SetContentType(NS_LITERAL_CSTRING(APPLICATION_GUESS_FROM_EXT));
    }

    rv = helperAppService->DoContent(mContentType,
                                     request,
                                     m_originalContext,
                                     getter_AddRefs(m_targetStreamListener));
    if (NS_FAILED(rv)) {
      request->SetLoadFlags(loadFlags);
      m_targetStreamListener = nsnull;
    }
  }
      
  NS_ASSERTION(m_targetStreamListener || NS_FAILED(rv),
               "There is no way we should be successful at this point without a m_targetStreamListener");
  return rv;
}

nsresult
nsDocumentOpenInfo::ConvertData(nsIRequest *request,
                                nsIURIContentListener* aListener,
                                const nsACString& aSrcContentType,
                                const nsACString& aOutContentType)
{
  LOG(("[0x%p] nsDocumentOpenInfo::ConvertData from '%s' to '%s'", this,
       PromiseFlatCString(aSrcContentType).get(),
       PromiseFlatCString(aOutContentType).get()));

  NS_PRECONDITION(aSrcContentType != aOutContentType,
                  "ConvertData called when the two types are the same!");
  nsresult rv = NS_OK;

  nsCOMPtr<nsIStreamConverterService> StreamConvService = 
    do_GetService(NS_STREAMCONVERTERSERVICE_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return rv;

  LOG(("  Got converter service"));
  
  
  
  
  
  
  
  
  
  nsCOMPtr<nsDocumentOpenInfo> nextLink =
    new nsDocumentOpenInfo(m_originalContext, mFlags, mURILoader);
  if (!nextLink) return NS_ERROR_OUT_OF_MEMORY;

  LOG(("  Downstream DocumentOpenInfo would be: 0x%p", nextLink.get()));
  
  
  
  nextLink->m_contentListener = aListener;
  
  nextLink->m_targetStreamListener = nsnull;

  
  
  
  
  
  nextLink->mContentType = aOutContentType;

  
  
  
  
  return StreamConvService->AsyncConvertData(PromiseFlatCString(aSrcContentType).get(), 
                                             PromiseFlatCString(aOutContentType).get(), 
                                             nextLink, 
                                             request,
                                             getter_AddRefs(m_targetStreamListener));
}

PRBool
nsDocumentOpenInfo::TryContentListener(nsIURIContentListener* aListener,
                                       nsIChannel* aChannel)
{
  LOG(("[0x%p] nsDocumentOpenInfo::TryContentListener; mFlags = 0x%x",
       this, mFlags));

  NS_PRECONDITION(aListener, "Must have a non-null listener");
  NS_PRECONDITION(aChannel, "Must have a channel");
  
  PRBool listenerWantsContent = PR_FALSE;
  nsXPIDLCString typeToUse;
  
  if (mFlags & nsIURILoader::IS_CONTENT_PREFERRED) {
    aListener->IsPreferred(mContentType.get(),
                           getter_Copies(typeToUse),
                           &listenerWantsContent);
  } else {
    aListener->CanHandleContent(mContentType.get(), PR_FALSE,
                                getter_Copies(typeToUse),
                                &listenerWantsContent);
  }
  if (!listenerWantsContent) {
    LOG(("  Listener is not interested"));
    return PR_FALSE;
  }

  if (!typeToUse.IsEmpty() && typeToUse != mContentType) {
    

    nsresult rv = ConvertData(aChannel, aListener, mContentType, typeToUse);

    if (NS_FAILED(rv)) {
      
      m_targetStreamListener = nsnull;
    }

    LOG(("  Found conversion: %s", m_targetStreamListener ? "yes" : "no"));
    
    
    
    
    return m_targetStreamListener != nsnull;
  }

  
  
  
  nsLoadFlags loadFlags = 0;
  aChannel->GetLoadFlags(&loadFlags);

  
  
  nsLoadFlags newLoadFlags = nsIChannel::LOAD_TARGETED;

  nsCOMPtr<nsIURIContentListener> originalListener =
    do_GetInterface(m_originalContext);
  if (originalListener != aListener) {
    newLoadFlags |= nsIChannel::LOAD_RETARGETED_DOCUMENT_URI;
  }
  aChannel->SetLoadFlags(loadFlags | newLoadFlags);
  
  PRBool abort = PR_FALSE;
  PRBool isPreferred = (mFlags & nsIURILoader::IS_CONTENT_PREFERRED) != 0;
  nsresult rv = aListener->DoContent(mContentType.get(),
                                     isPreferred,
                                     aChannel,
                                     getter_AddRefs(m_targetStreamListener),
                                     &abort);
    
  if (NS_FAILED(rv)) {
    LOG_ERROR(("  DoContent failed"));
    
    
    aChannel->SetLoadFlags(loadFlags);
    m_targetStreamListener = nsnull;
    return PR_FALSE;
  }

  if (abort) {
    
    
    
    LOG(("  Listener has aborted the load"));
    m_targetStreamListener = nsnull;
  }

  NS_ASSERTION(abort || m_targetStreamListener, "DoContent returned no listener?");

  
  return PR_TRUE;
}






nsURILoader::nsURILoader()
{
#ifdef PR_LOGGING
  if (!mLog) {
    mLog = PR_NewLogModule("URILoader");
  }
#endif
}

nsURILoader::~nsURILoader()
{
}

NS_IMPL_ADDREF(nsURILoader)
NS_IMPL_RELEASE(nsURILoader)

NS_INTERFACE_MAP_BEGIN(nsURILoader)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIURILoader)
   NS_INTERFACE_MAP_ENTRY(nsIURILoader)
NS_INTERFACE_MAP_END

NS_IMETHODIMP nsURILoader::RegisterContentListener(nsIURIContentListener * aContentListener)
{
  nsresult rv = NS_OK;

  nsWeakPtr weakListener = do_GetWeakReference(aContentListener);
  NS_ASSERTION(weakListener, "your URIContentListener must support weak refs!\n");
  
  if (weakListener)
    m_listeners.AppendObject(weakListener);

  return rv;
} 

NS_IMETHODIMP nsURILoader::UnRegisterContentListener(nsIURIContentListener * aContentListener)
{
  nsWeakPtr weakListener = do_GetWeakReference(aContentListener);
  if (weakListener)
    m_listeners.RemoveObject(weakListener);

  return NS_OK;
  
}

NS_IMETHODIMP nsURILoader::OpenURI(nsIChannel *channel, 
                                   PRBool aIsContentPreferred,
                                   nsIInterfaceRequestor *aWindowContext)
{
  NS_ENSURE_ARG_POINTER(channel);

#ifdef PR_LOGGING
  if (LOG_ENABLED()) {
    nsCOMPtr<nsIURI> uri;
    channel->GetURI(getter_AddRefs(uri));
    nsCAutoString spec;
    uri->GetAsciiSpec(spec);
    LOG(("nsURILoader::OpenURI for %s", spec.get()));
  }
#endif

  nsCOMPtr<nsIStreamListener> loader;
  nsresult rv = OpenChannel(channel,
                            aIsContentPreferred ? IS_CONTENT_PREFERRED : 0,
                            aWindowContext,
                            PR_FALSE,
                            getter_AddRefs(loader));

  if (NS_SUCCEEDED(rv)) {
    
    
    
    

    
    rv = channel->AsyncOpen(loader, nsnull);

    
    if (rv == NS_ERROR_NO_CONTENT) {
      LOG(("  rv is NS_ERROR_NO_CONTENT -- doing nothing"));
      rv = NS_OK;
    }
  } else if (rv == NS_ERROR_WONT_HANDLE_CONTENT) {
    
    rv = NS_OK;
  }
  return rv;
}

nsresult nsURILoader::OpenChannel(nsIChannel* channel,
                                  PRUint32 aFlags,
                                  nsIInterfaceRequestor* aWindowContext,
                                  PRBool aChannelIsOpen,
                                  nsIStreamListener** aListener)
{
  NS_ASSERTION(channel, "Trying to open a null channel!");
  NS_ASSERTION(aWindowContext, "Window context must not be null");

#ifdef PR_LOGGING
  if (LOG_ENABLED()) {
    nsCOMPtr<nsIURI> uri;
    channel->GetURI(getter_AddRefs(uri));
    nsCAutoString spec;
    uri->GetAsciiSpec(spec);
    LOG(("nsURILoader::OpenChannel for %s", spec.get()));
  }
#endif

  
  
  nsCOMPtr<nsIURIContentListener> winContextListener(do_GetInterface(aWindowContext));
  if (winContextListener) {
    nsCOMPtr<nsIURI> uri;
    channel->GetURI(getter_AddRefs(uri));
    if (uri) {
      PRBool doAbort = PR_FALSE;
      winContextListener->OnStartURIOpen(uri, &doAbort);

      if (doAbort) {
        LOG(("  OnStartURIOpen aborted load"));
        return NS_ERROR_WONT_HANDLE_CONTENT;
      }
    }
  }

  
  
  nsCOMPtr<nsDocumentOpenInfo> loader =
    new nsDocumentOpenInfo(aWindowContext, aFlags, this);

  if (!loader) return NS_ERROR_OUT_OF_MEMORY;

  
  nsCOMPtr<nsILoadGroup> loadGroup(do_GetInterface(aWindowContext));

  if (!loadGroup) {
    
    
    
    nsCOMPtr<nsIURIContentListener> listener(do_GetInterface(aWindowContext));
    if (listener) {
      nsCOMPtr<nsISupports> cookie;
      listener->GetLoadCookie(getter_AddRefs(cookie));
      if (!cookie) {
        nsRefPtr<nsDocLoader> newDocLoader = new nsDocLoader();
        if (!newDocLoader)
          return NS_ERROR_OUT_OF_MEMORY;
        nsresult rv = newDocLoader->Init();
        if (NS_FAILED(rv))
          return rv;
        rv = nsDocLoader::AddDocLoaderAsChildOfRoot(newDocLoader);
        if (NS_FAILED(rv))
          return rv;
        listener->SetLoadCookie(nsDocLoader::GetAsSupports(newDocLoader));
      }
    }
  }

  
  
  nsCOMPtr<nsILoadGroup> oldGroup;
  channel->GetLoadGroup(getter_AddRefs(oldGroup));
  if (aChannelIsOpen && !SameCOMIdentity(oldGroup, loadGroup)) {
    
    
    
    loadGroup->AddRequest(channel, nsnull);

   if (oldGroup) {
      oldGroup->RemoveRequest(channel, nsnull, NS_BINDING_RETARGETED);
    }
  }

  channel->SetLoadGroup(loadGroup);

  
  nsresult rv = loader->Prepare();
  if (NS_SUCCEEDED(rv))
    NS_ADDREF(*aListener = loader);
  return rv;
}

NS_IMETHODIMP nsURILoader::OpenChannel(nsIChannel* channel,
                                       PRUint32 aFlags,
                                       nsIInterfaceRequestor* aWindowContext,
                                       nsIStreamListener** aListener)
{
  PRBool pending;
  if (NS_FAILED(channel->IsPending(&pending))) {
    pending = PR_FALSE;
  }

  return OpenChannel(channel, aFlags, aWindowContext, pending, aListener);
}

NS_IMETHODIMP nsURILoader::Stop(nsISupports* aLoadCookie)
{
  nsresult rv;
  nsCOMPtr<nsIDocumentLoader> docLoader;

  NS_ENSURE_ARG_POINTER(aLoadCookie);

  docLoader = do_GetInterface(aLoadCookie, &rv);
  if (docLoader) {
    rv = docLoader->Stop();
  }
  return rv;
}

