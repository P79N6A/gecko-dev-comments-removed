







































#include "nsViewSourceChannel.h"
#include "nsIIOService.h"
#include "nsIServiceManager.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsMimeTypes.h"
#include "nsNetUtil.h"
#include "nsIHttpHeaderVisitor.h"

NS_IMPL_ADDREF(nsViewSourceChannel)
NS_IMPL_RELEASE(nsViewSourceChannel)




NS_INTERFACE_MAP_BEGIN(nsViewSourceChannel)
    NS_INTERFACE_MAP_ENTRY(nsIViewSourceChannel)
    NS_INTERFACE_MAP_ENTRY(nsIStreamListener)
    NS_INTERFACE_MAP_ENTRY(nsIRequestObserver)
    NS_INTERFACE_MAP_ENTRY_CONDITIONAL(nsIHttpChannel, mHttpChannel)
    NS_INTERFACE_MAP_ENTRY_CONDITIONAL(nsICachingChannel, mCachingChannel)
    NS_INTERFACE_MAP_ENTRY_CONDITIONAL(nsIUploadChannel, mUploadChannel)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIRequest, nsIViewSourceChannel)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIChannel, nsIViewSourceChannel)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIViewSourceChannel)
NS_INTERFACE_MAP_END_THREADSAFE

nsresult
nsViewSourceChannel::Init(nsIURI* uri)
{
    mOriginalURI = uri;

    nsCAutoString path;
    nsresult rv = uri->GetPath(path);
    if (NS_FAILED(rv))
      return rv;

    nsCOMPtr<nsIIOService> pService(do_GetIOService(&rv));
    if (NS_FAILED(rv)) return rv;

    nsCAutoString scheme;
    rv = pService->ExtractScheme(path, scheme);
    if (NS_FAILED(rv))
      return rv;

    
    if (scheme.LowerCaseEqualsLiteral("javascript")) {
      NS_WARNING("blocking view-source:javascript:");
      return NS_ERROR_INVALID_ARG;
    }

    rv = pService->NewChannel(path, nsnull, nsnull, getter_AddRefs(mChannel));
    if (NS_FAILED(rv))
      return rv;
 
    mChannel->SetOriginalURI(mOriginalURI);
    mHttpChannel = do_QueryInterface(mChannel);
    mCachingChannel = do_QueryInterface(mChannel);
    mUploadChannel = do_QueryInterface(mChannel);
    
    return NS_OK;
}




NS_IMETHODIMP
nsViewSourceChannel::GetName(nsACString &result)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsViewSourceChannel::IsPending(PRBool *result)
{
    NS_ENSURE_TRUE(mChannel, NS_ERROR_FAILURE);

    return mChannel->IsPending(result);
}

NS_IMETHODIMP
nsViewSourceChannel::GetStatus(nsresult *status)
{
    NS_ENSURE_TRUE(mChannel, NS_ERROR_FAILURE);

    return mChannel->GetStatus(status);
}

NS_IMETHODIMP
nsViewSourceChannel::Cancel(nsresult status)
{
    NS_ENSURE_TRUE(mChannel, NS_ERROR_FAILURE);

    return mChannel->Cancel(status);
}

NS_IMETHODIMP
nsViewSourceChannel::Suspend(void)
{
    NS_ENSURE_TRUE(mChannel, NS_ERROR_FAILURE);

    return mChannel->Suspend();
}

NS_IMETHODIMP
nsViewSourceChannel::Resume(void)
{
    NS_ENSURE_TRUE(mChannel, NS_ERROR_FAILURE);

    return mChannel->Resume();
}




NS_IMETHODIMP
nsViewSourceChannel::GetOriginalURI(nsIURI* *aURI)
{
    NS_ASSERTION(aURI, "Null out param!");
    *aURI = mOriginalURI;
    NS_IF_ADDREF(*aURI);
    return NS_OK;
}

NS_IMETHODIMP
nsViewSourceChannel::SetOriginalURI(nsIURI* aURI)
{
    mOriginalURI = aURI;
    return NS_OK;
}

NS_IMETHODIMP
nsViewSourceChannel::GetURI(nsIURI* *aURI)
{
    NS_ENSURE_TRUE(mChannel, NS_ERROR_FAILURE);

    nsCOMPtr<nsIURI> uri;
    nsresult rv = mChannel->GetURI(getter_AddRefs(uri));
    if (NS_FAILED(rv))
      return rv;

    
    if (!uri) {
      NS_ERROR("inner channel returned NS_OK and a null URI");
      return NS_ERROR_UNEXPECTED;
    }

    nsCAutoString spec;
    uri->GetSpec(spec);

    

    return NS_NewURI(aURI, nsCAutoString(NS_LITERAL_CSTRING("view-source:")+spec), nsnull);
}

NS_IMETHODIMP
nsViewSourceChannel::Open(nsIInputStream **_retval)
{
    NS_ENSURE_TRUE(mChannel, NS_ERROR_FAILURE);

    nsresult rv = mChannel->Open(_retval);
    if (NS_SUCCEEDED(rv)) {
        mOpened = PR_TRUE;
    }
    
    return rv;
}

NS_IMETHODIMP
nsViewSourceChannel::AsyncOpen(nsIStreamListener *aListener, nsISupports *ctxt)
{
    NS_ENSURE_TRUE(mChannel, NS_ERROR_FAILURE);

    mListener = aListener;

    




    
    nsCOMPtr<nsILoadGroup> loadGroup;
    mChannel->GetLoadGroup(getter_AddRefs(loadGroup));
    if (loadGroup)
        loadGroup->AddRequest(static_cast<nsIViewSourceChannel*>
                                         (this), nsnull);
    
    nsresult rv = mChannel->AsyncOpen(this, ctxt);

    if (NS_FAILED(rv) && loadGroup)
        loadGroup->RemoveRequest(static_cast<nsIViewSourceChannel*>
                                            (this),
                                 nsnull, rv);

    if (NS_SUCCEEDED(rv)) {
        mOpened = PR_TRUE;
    }
    
    return rv;
}














NS_IMETHODIMP
nsViewSourceChannel::GetLoadFlags(PRUint32 *aLoadFlags)
{
    NS_ENSURE_TRUE(mChannel, NS_ERROR_FAILURE);

    nsresult rv = mChannel->GetLoadFlags(aLoadFlags);
    if (NS_FAILED(rv))
      return rv;

    
    
    
    if (mIsDocument)
      *aLoadFlags |= ::nsIChannel::LOAD_DOCUMENT_URI;

    return rv;
}

NS_IMETHODIMP
nsViewSourceChannel::SetLoadFlags(PRUint32 aLoadFlags)
{
    NS_ENSURE_TRUE(mChannel, NS_ERROR_FAILURE);

    
    
    
 
    
    
    
    
    mIsDocument = (aLoadFlags & ::nsIChannel::LOAD_DOCUMENT_URI) ? PR_TRUE : PR_FALSE;

    return mChannel->SetLoadFlags((aLoadFlags |
                                   ::nsIRequest::LOAD_FROM_CACHE) &
                                  ~::nsIChannel::LOAD_DOCUMENT_URI);
}

NS_IMETHODIMP
nsViewSourceChannel::GetContentType(nsACString &aContentType) 
{
    NS_ENSURE_TRUE(mChannel, NS_ERROR_FAILURE);

    aContentType.Truncate();

    if (mContentType.IsEmpty())
    {
        
        nsresult rv;
        nsCAutoString contentType;
        rv = mChannel->GetContentType(contentType);
        if (NS_FAILED(rv)) return rv;

        
        
        
        
        if (!contentType.Equals(UNKNOWN_CONTENT_TYPE)) {
          contentType = VIEWSOURCE_CONTENT_TYPE;
        }

        mContentType = contentType;
    }

    aContentType = mContentType;
    return NS_OK;
}

NS_IMETHODIMP
nsViewSourceChannel::SetContentType(const nsACString &aContentType)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    if (!mOpened) {
        
        return NS_ERROR_NOT_AVAILABLE;
    }
    
    mContentType = aContentType;
    return NS_OK;
}

NS_IMETHODIMP
nsViewSourceChannel::GetContentCharset(nsACString &aContentCharset)
{
    NS_ENSURE_TRUE(mChannel, NS_ERROR_FAILURE);

    return mChannel->GetContentCharset(aContentCharset);
}

NS_IMETHODIMP
nsViewSourceChannel::SetContentCharset(const nsACString &aContentCharset)
{
    NS_ENSURE_TRUE(mChannel, NS_ERROR_FAILURE);

    return mChannel->SetContentCharset(aContentCharset);
}

NS_IMETHODIMP
nsViewSourceChannel::GetContentLength(PRInt32 *aContentLength)
{
    NS_ENSURE_TRUE(mChannel, NS_ERROR_FAILURE);

    return mChannel->GetContentLength(aContentLength);
}

NS_IMETHODIMP
nsViewSourceChannel::SetContentLength(PRInt32 aContentLength)
{
    NS_ENSURE_TRUE(mChannel, NS_ERROR_FAILURE);

    return mChannel->SetContentLength(aContentLength);
}

NS_IMETHODIMP
nsViewSourceChannel::GetLoadGroup(nsILoadGroup* *aLoadGroup)
{
    NS_ENSURE_TRUE(mChannel, NS_ERROR_FAILURE);

    return mChannel->GetLoadGroup(aLoadGroup);
}

NS_IMETHODIMP
nsViewSourceChannel::SetLoadGroup(nsILoadGroup* aLoadGroup)
{
    NS_ENSURE_TRUE(mChannel, NS_ERROR_FAILURE);

    return mChannel->SetLoadGroup(aLoadGroup);
}

NS_IMETHODIMP
nsViewSourceChannel::GetOwner(nsISupports* *aOwner)
{
    NS_ENSURE_TRUE(mChannel, NS_ERROR_FAILURE);

    return mChannel->GetOwner(aOwner);
}

NS_IMETHODIMP
nsViewSourceChannel::SetOwner(nsISupports* aOwner)
{
    NS_ENSURE_TRUE(mChannel, NS_ERROR_FAILURE);

    return mChannel->SetOwner(aOwner);
}

NS_IMETHODIMP
nsViewSourceChannel::GetNotificationCallbacks(nsIInterfaceRequestor* *aNotificationCallbacks)
{
    NS_ENSURE_TRUE(mChannel, NS_ERROR_FAILURE);

    return mChannel->GetNotificationCallbacks(aNotificationCallbacks);
}

NS_IMETHODIMP
nsViewSourceChannel::SetNotificationCallbacks(nsIInterfaceRequestor* aNotificationCallbacks)
{
    NS_ENSURE_TRUE(mChannel, NS_ERROR_FAILURE);

    return mChannel->SetNotificationCallbacks(aNotificationCallbacks);
}

NS_IMETHODIMP 
nsViewSourceChannel::GetSecurityInfo(nsISupports * *aSecurityInfo)
{
    NS_ENSURE_TRUE(mChannel, NS_ERROR_FAILURE);

    return mChannel->GetSecurityInfo(aSecurityInfo);
}


NS_IMETHODIMP
nsViewSourceChannel::GetOriginalContentType(nsACString &aContentType) 
{
    NS_ENSURE_TRUE(mChannel, NS_ERROR_FAILURE);

    return mChannel->GetContentType(aContentType);
}

NS_IMETHODIMP
nsViewSourceChannel::SetOriginalContentType(const nsACString &aContentType)
{
    NS_ENSURE_TRUE(mChannel, NS_ERROR_FAILURE);

    
    mContentType.Truncate();

    return mChannel->SetContentType(aContentType);
}


NS_IMETHODIMP
nsViewSourceChannel::OnStartRequest(nsIRequest *aRequest, nsISupports *aContext)
{
    NS_ENSURE_TRUE(mListener, NS_ERROR_FAILURE);
    
    mChannel = do_QueryInterface(aRequest);
    mHttpChannel = do_QueryInterface(aRequest);
    mCachingChannel = do_QueryInterface(aRequest);
    mUploadChannel = do_QueryInterface(aRequest);
    
    return mListener->OnStartRequest(static_cast<nsIViewSourceChannel*>
                                                (this),
                                     aContext);
}


NS_IMETHODIMP
nsViewSourceChannel::OnStopRequest(nsIRequest *aRequest, nsISupports* aContext,
                               nsresult aStatus)
{
    NS_ENSURE_TRUE(mListener, NS_ERROR_FAILURE);
    if (mChannel)
    {
        nsCOMPtr<nsILoadGroup> loadGroup;
        mChannel->GetLoadGroup(getter_AddRefs(loadGroup));
        if (loadGroup)
        {
            loadGroup->RemoveRequest(static_cast<nsIViewSourceChannel*>
                                                (this),
                                     nsnull, aStatus);
        }
    }
    return mListener->OnStopRequest(static_cast<nsIViewSourceChannel*>
                                               (this),
                                    aContext, aStatus);
}



NS_IMETHODIMP
nsViewSourceChannel::OnDataAvailable(nsIRequest *aRequest, nsISupports* aContext,
                               nsIInputStream *aInputStream, PRUint32 aSourceOffset,
                               PRUint32 aLength) 
{
    NS_ENSURE_TRUE(mListener, NS_ERROR_FAILURE);
    return mListener->OnDataAvailable(static_cast<nsIViewSourceChannel*>
                                                 (this),
                                      aContext, aInputStream,
                                      aSourceOffset, aLength);
}







NS_IMETHODIMP
nsViewSourceChannel::GetRequestMethod(nsACString & aRequestMethod)
{
    return !mHttpChannel ? NS_ERROR_NULL_POINTER :
        mHttpChannel->GetRequestMethod(aRequestMethod);
}

NS_IMETHODIMP
nsViewSourceChannel::SetRequestMethod(const nsACString & aRequestMethod)
{
    return !mHttpChannel ? NS_ERROR_NULL_POINTER :
        mHttpChannel->SetRequestMethod(aRequestMethod);
}

NS_IMETHODIMP
nsViewSourceChannel::GetReferrer(nsIURI * *aReferrer)
{
    return !mHttpChannel ? NS_ERROR_NULL_POINTER :
        mHttpChannel->GetReferrer(aReferrer);
}

NS_IMETHODIMP
nsViewSourceChannel::SetReferrer(nsIURI * aReferrer)
{
    return !mHttpChannel ? NS_ERROR_NULL_POINTER :
        mHttpChannel->SetReferrer(aReferrer);
}

NS_IMETHODIMP
nsViewSourceChannel::GetRequestHeader(const nsACString & aHeader,
                                      nsACString & aValue)
{
    return !mHttpChannel ? NS_ERROR_NULL_POINTER :
        mHttpChannel->GetRequestHeader(aHeader, aValue);
}

NS_IMETHODIMP
nsViewSourceChannel::SetRequestHeader(const nsACString & aHeader,
                                      const nsACString & aValue,
                                      PRBool aMerge)
{
    return !mHttpChannel ? NS_ERROR_NULL_POINTER :
        mHttpChannel->SetRequestHeader(aHeader, aValue, aMerge);
}

NS_IMETHODIMP
nsViewSourceChannel::VisitRequestHeaders(nsIHttpHeaderVisitor *aVisitor)
{
    return !mHttpChannel ? NS_ERROR_NULL_POINTER :
        mHttpChannel->VisitRequestHeaders(aVisitor);
}

NS_IMETHODIMP
nsViewSourceChannel::GetAllowPipelining(PRBool *aAllowPipelining)
{
    return !mHttpChannel ? NS_ERROR_NULL_POINTER :
        mHttpChannel->GetAllowPipelining(aAllowPipelining);
}

NS_IMETHODIMP
nsViewSourceChannel::SetAllowPipelining(PRBool aAllowPipelining)
{
    return !mHttpChannel ? NS_ERROR_NULL_POINTER :
        mHttpChannel->SetAllowPipelining(aAllowPipelining);
}

NS_IMETHODIMP
nsViewSourceChannel::GetRedirectionLimit(PRUint32 *aRedirectionLimit)
{
    return !mHttpChannel ? NS_ERROR_NULL_POINTER :
        mHttpChannel->GetRedirectionLimit(aRedirectionLimit);
}

NS_IMETHODIMP
nsViewSourceChannel::SetRedirectionLimit(PRUint32 aRedirectionLimit)
{
    return !mHttpChannel ? NS_ERROR_NULL_POINTER :
        mHttpChannel->SetRedirectionLimit(aRedirectionLimit);
}

NS_IMETHODIMP
nsViewSourceChannel::GetResponseStatus(PRUint32 *aResponseStatus)
{
    return !mHttpChannel ? NS_ERROR_NULL_POINTER :
        mHttpChannel->GetResponseStatus(aResponseStatus);
}

NS_IMETHODIMP
nsViewSourceChannel::GetResponseStatusText(nsACString & aResponseStatusText)
{
    return !mHttpChannel ? NS_ERROR_NULL_POINTER :
        mHttpChannel->GetResponseStatusText(aResponseStatusText);
}

NS_IMETHODIMP
nsViewSourceChannel::GetRequestSucceeded(PRBool *aRequestSucceeded)
{
    return !mHttpChannel ? NS_ERROR_NULL_POINTER :
        mHttpChannel->GetRequestSucceeded(aRequestSucceeded);
}

NS_IMETHODIMP
nsViewSourceChannel::GetResponseHeader(const nsACString & aHeader,
                                       nsACString & aValue)
{
    if (!mHttpChannel)
        return NS_ERROR_NULL_POINTER;

    if (!aHeader.Equals(NS_LITERAL_CSTRING("Content-Type"),
                        nsCaseInsensitiveCStringComparator())) {
        aValue.Truncate();
        return NS_OK;
    }
        
    return mHttpChannel->GetResponseHeader(aHeader, aValue);
}

NS_IMETHODIMP
nsViewSourceChannel::SetResponseHeader(const nsACString & header,
                                       const nsACString & value, PRBool merge)
{
    return !mHttpChannel ? NS_ERROR_NULL_POINTER :
        mHttpChannel->SetResponseHeader(header, value, merge);
}

NS_IMETHODIMP
nsViewSourceChannel::VisitResponseHeaders(nsIHttpHeaderVisitor *aVisitor)
{
    if (!mHttpChannel)
        return NS_ERROR_NULL_POINTER;

    NS_NAMED_LITERAL_CSTRING(contentTypeStr, "Content-Type");
    nsCAutoString contentType;
    nsresult rv =
        mHttpChannel->GetResponseHeader(contentTypeStr, contentType);
    if (NS_SUCCEEDED(rv))
        aVisitor->VisitHeader(contentTypeStr, contentType);
    return NS_OK;
}

NS_IMETHODIMP
nsViewSourceChannel::IsNoStoreResponse(PRBool *_retval)
{
    return !mHttpChannel ? NS_ERROR_NULL_POINTER :
        mHttpChannel->IsNoStoreResponse(_retval);
}

NS_IMETHODIMP
nsViewSourceChannel::IsNoCacheResponse(PRBool *_retval)
{
    return !mHttpChannel ? NS_ERROR_NULL_POINTER :
        mHttpChannel->IsNoCacheResponse(_retval);
} 
