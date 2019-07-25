



#include "nsStreamListenerTee.h"
#include "nsProxyRelease.h"

NS_IMPL_ISUPPORTS3(nsStreamListenerTee,
                   nsIStreamListener,
                   nsIRequestObserver,
                   nsIStreamListenerTee)

NS_IMETHODIMP
nsStreamListenerTee::OnStartRequest(nsIRequest *request,
                                    nsISupports *context)
{
    NS_ENSURE_TRUE(mListener, NS_ERROR_NOT_INITIALIZED);
    nsresult rv1 = mListener->OnStartRequest(request, context);
    nsresult rv2 = NS_OK;
    if (mObserver)
        rv2 = mObserver->OnStartRequest(request, context);
  
    
    return (NS_FAILED(rv2) && NS_SUCCEEDED(rv1)) ? rv2 : rv1;
}

NS_IMETHODIMP
nsStreamListenerTee::OnStopRequest(nsIRequest *request,
                                   nsISupports *context,
                                   nsresult status)
{
    NS_ENSURE_TRUE(mListener, NS_ERROR_NOT_INITIALIZED);
    
    if (mInputTee) {
        mInputTee->SetSink(nullptr);
        mInputTee = 0;
    }

    
    if (mEventTarget) {
        nsIOutputStream *sink = nullptr;
        mSink.swap(sink);
        NS_ProxyRelease(mEventTarget, sink);
    }
    else {
        mSink = 0;
    }

    nsresult rv = mListener->OnStopRequest(request, context, status);
    if (mObserver)
        mObserver->OnStopRequest(request, context, status);
    mObserver = 0;
    return rv;
}

NS_IMETHODIMP
nsStreamListenerTee::OnDataAvailable(nsIRequest *request,
                                     nsISupports *context,
                                     nsIInputStream *input,
                                     uint64_t offset,
                                     uint32_t count)
{
    NS_ENSURE_TRUE(mListener, NS_ERROR_NOT_INITIALIZED);
    NS_ENSURE_TRUE(mSink, NS_ERROR_NOT_INITIALIZED);

    nsCOMPtr<nsIInputStream> tee;
    nsresult rv;

    if (!mInputTee) {
        if (mEventTarget)
            rv = NS_NewInputStreamTeeAsync(getter_AddRefs(tee), input,
                                           mSink, mEventTarget);
        else
            rv = NS_NewInputStreamTee(getter_AddRefs(tee), input, mSink);
        if (NS_FAILED(rv)) return rv;

        mInputTee = do_QueryInterface(tee, &rv);
        if (NS_FAILED(rv)) return rv;
    }
    else {
        
        rv = mInputTee->SetSource(input);
        if (NS_FAILED(rv)) return rv;

        tee = do_QueryInterface(mInputTee, &rv);
        if (NS_FAILED(rv)) return rv;
    }

    return mListener->OnDataAvailable(request, context, tee, offset, count);
}

NS_IMETHODIMP
nsStreamListenerTee::Init(nsIStreamListener *listener,
                          nsIOutputStream *sink,
                          nsIRequestObserver *requestObserver)
{
    NS_ENSURE_ARG_POINTER(listener);
    NS_ENSURE_ARG_POINTER(sink);
    mListener = listener;
    mSink = sink;
    mObserver = requestObserver;
    return NS_OK;
}

NS_IMETHODIMP
nsStreamListenerTee::InitAsync(nsIStreamListener *listener,
                               nsIEventTarget *eventTarget,
                               nsIOutputStream *sink,
                               nsIRequestObserver *requestObserver)
{
    NS_ENSURE_ARG_POINTER(eventTarget);
    mEventTarget = eventTarget;
    return Init(listener, sink, requestObserver);
}
