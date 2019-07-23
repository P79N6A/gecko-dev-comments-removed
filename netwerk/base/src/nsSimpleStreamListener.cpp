





































#include "nsSimpleStreamListener.h"






NS_IMPL_ISUPPORTS3(nsSimpleStreamListener,
                   nsISimpleStreamListener,
                   nsIStreamListener,
                   nsIRequestObserver)






NS_IMETHODIMP
nsSimpleStreamListener::OnStartRequest(nsIRequest *aRequest,
                                       nsISupports *aContext)
{
    return mObserver ?
        mObserver->OnStartRequest(aRequest, aContext) : NS_OK;
}

NS_IMETHODIMP
nsSimpleStreamListener::OnStopRequest(nsIRequest* request,
                                      nsISupports *aContext,
                                      nsresult aStatus)
{
    return mObserver ?
        mObserver->OnStopRequest(request, aContext, aStatus) : NS_OK;
}






NS_IMETHODIMP
nsSimpleStreamListener::OnDataAvailable(nsIRequest* request,
                                        nsISupports *aContext,
                                        nsIInputStream *aSource,
                                        PRUint32 aOffset,
                                        PRUint32 aCount)
{
    PRUint32 writeCount;
    nsresult rv = mSink->WriteFrom(aSource, aCount, &writeCount);
    
    
    
    if (NS_SUCCEEDED(rv) && (writeCount == 0))
        return NS_BASE_STREAM_CLOSED;
    return rv;
}






NS_IMETHODIMP
nsSimpleStreamListener::Init(nsIOutputStream *aSink,
                             nsIRequestObserver *aObserver)
{
    NS_PRECONDITION(aSink, "null output stream");

    mSink = aSink;
    mObserver = aObserver;

    return NS_OK;
}
