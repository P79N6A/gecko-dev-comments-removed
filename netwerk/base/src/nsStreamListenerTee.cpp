




































#include "nsStreamListenerTee.h"

NS_IMPL_ISUPPORTS3(nsStreamListenerTee,
                   nsIStreamListener,
                   nsIRequestObserver,
                   nsIStreamListenerTee)

NS_IMETHODIMP
nsStreamListenerTee::OnStartRequest(nsIRequest *request,
                                    nsISupports *context)
{
    NS_ENSURE_TRUE(mListener, NS_ERROR_NOT_INITIALIZED);
    return mListener->OnStartRequest(request, context);
}

NS_IMETHODIMP
nsStreamListenerTee::OnStopRequest(nsIRequest *request,
                                   nsISupports *context,
                                   nsresult status)
{
    NS_ENSURE_TRUE(mListener, NS_ERROR_NOT_INITIALIZED);
    
    if (mInputTee) {
        mInputTee->SetSink(nsnull);
        mInputTee = 0;
    }
    mSink = 0;
    return mListener->OnStopRequest(request, context, status);
}

NS_IMETHODIMP
nsStreamListenerTee::OnDataAvailable(nsIRequest *request,
                                     nsISupports *context,
                                     nsIInputStream *input,
                                     PRUint32 offset,
                                     PRUint32 count)
{
    NS_ENSURE_TRUE(mListener, NS_ERROR_NOT_INITIALIZED);
    NS_ENSURE_TRUE(mSink, NS_ERROR_NOT_INITIALIZED);

    nsCOMPtr<nsIInputStream> tee;
    nsresult rv;

    if (!mInputTee) {
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
                          nsIOutputStream *sink)
{
    mListener = listener;
    mSink = sink;
    return NS_OK;
}
