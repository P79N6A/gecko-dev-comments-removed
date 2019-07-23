




































#include "nsBaseContentStream.h"
#include "nsStreamUtils.h"



void
nsBaseContentStream::DispatchCallback(PRBool async)
{
  if (!mCallback)
    return;

  
  

  nsCOMPtr<nsIInputStreamCallback> callback;
  if (async) {
    NS_NewInputStreamReadyEvent(getter_AddRefs(callback), mCallback,
                                mCallbackTarget);
    if (!callback)
      return;  
    mCallback = nsnull;
  } else {
    callback.swap(mCallback);
  }
  mCallbackTarget = nsnull;

  callback->OnInputStreamReady(this);
}




NS_IMPL_THREADSAFE_ADDREF(nsBaseContentStream)
NS_IMPL_THREADSAFE_RELEASE(nsBaseContentStream)


NS_INTERFACE_MAP_BEGIN(nsBaseContentStream)
  NS_INTERFACE_MAP_ENTRY(nsIInputStream)
  NS_INTERFACE_MAP_ENTRY_CONDITIONAL(nsIAsyncInputStream, mNonBlocking)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIInputStream)
NS_INTERFACE_MAP_END_THREADSAFE




NS_IMETHODIMP
nsBaseContentStream::Close()
{
  return IsClosed() ? NS_OK : CloseWithStatus(NS_BASE_STREAM_CLOSED);
}

NS_IMETHODIMP
nsBaseContentStream::Available(PRUint32 *result)
{
  *result = 0;
  return mStatus;
}

NS_IMETHODIMP
nsBaseContentStream::Read(char *buf, PRUint32 count, PRUint32 *result)
{
  return ReadSegments(NS_CopySegmentToBuffer, buf, count, result); 
}

NS_IMETHODIMP
nsBaseContentStream::ReadSegments(nsWriteSegmentFun fun, void *closure,
                                  PRUint32 count, PRUint32 *result)
{
  *result = 0;

  if (mStatus == NS_BASE_STREAM_CLOSED)
    return NS_OK;

  
  if (!IsClosed() && IsNonBlocking())
    return NS_BASE_STREAM_WOULD_BLOCK;

  return mStatus;
}

NS_IMETHODIMP
nsBaseContentStream::IsNonBlocking(PRBool *result)
{
  *result = mNonBlocking;
  return NS_OK;
}




NS_IMETHODIMP
nsBaseContentStream::CloseWithStatus(nsresult status)
{
  if (IsClosed())
    return NS_OK;

  NS_ENSURE_ARG(NS_FAILED(status));
  mStatus = status;

  DispatchCallback();
  return NS_OK;
}

NS_IMETHODIMP
nsBaseContentStream::AsyncWait(nsIInputStreamCallback *callback,
                               PRUint32 flags, PRUint32 requestedCount,
                               nsIEventTarget *target)
{
  
  
  NS_ASSERTION(target, "unexpected parameter");
  NS_ASSERTION(flags == 0, "unexpected parameter");
  NS_ASSERTION(requestedCount == 0, "unexpected parameter");

#ifdef DEBUG
  PRBool correctThread;
  target->IsOnCurrentThread(&correctThread);
  NS_ASSERTION(correctThread, "event target must be on the current thread");
#endif

  mCallback = callback;
  mCallbackTarget = target;

  if (!mCallback)
    return NS_OK;

  
  if (IsClosed()) {
    DispatchCallback();
    return NS_OK;
  }

  OnCallbackPending();
  return NS_OK;
}
