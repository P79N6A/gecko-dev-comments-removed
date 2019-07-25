




































#include "FileIOObject.h"
#include "nsDOMFile.h"
#include "nsDOMError.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIDOMEvent.h"
#include "nsIDOMProgressEvent.h"
#include "nsComponentManagerUtils.h"
#include "nsEventDispatcher.h"

#define ERROR_STR "error"
#define ABORT_STR "abort"
#define PROGRESS_STR "progress"

namespace mozilla {
namespace dom {

const PRUint64 kUnknownSize = PRUint64(-1);

NS_IMPL_ADDREF_INHERITED(FileIOObject, nsDOMEventTargetWrapperCache)
NS_IMPL_RELEASE_INHERITED(FileIOObject, nsDOMEventTargetWrapperCache)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(FileIOObject)
  NS_INTERFACE_MAP_ENTRY(nsITimerCallback)
  NS_INTERFACE_MAP_ENTRY(nsIStreamListener)
  NS_INTERFACE_MAP_ENTRY(nsIRequestObserver)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetWrapperCache)

NS_IMPL_CYCLE_COLLECTION_CLASS(FileIOObject)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(FileIOObject,
                                                  nsDOMEventTargetWrapperCache)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mProgressNotifier)
  NS_CYCLE_COLLECTION_TRAVERSE_EVENT_HANDLER(abort)
  NS_CYCLE_COLLECTION_TRAVERSE_EVENT_HANDLER(error)
  NS_CYCLE_COLLECTION_TRAVERSE_EVENT_HANDLER(progress)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(FileIOObject,
                                                nsDOMEventTargetWrapperCache)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mProgressNotifier)
  NS_CYCLE_COLLECTION_UNLINK_EVENT_HANDLER(abort)
  NS_CYCLE_COLLECTION_UNLINK_EVENT_HANDLER(error)
  NS_CYCLE_COLLECTION_UNLINK_EVENT_HANDLER(progress)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

FileIOObject::FileIOObject()
  : mProgressEventWasDelayed(PR_FALSE),
    mTimerIsActive(PR_FALSE),
    mReadyState(0),
    mTotal(0), mTransferred(0)
{}

FileIOObject::~FileIOObject()
{
  if (mListenerManager)
    mListenerManager->Disconnect();
}

void
FileIOObject::StartProgressEventTimer()
{
  if (!mProgressNotifier) {
    mProgressNotifier = do_CreateInstance(NS_TIMER_CONTRACTID);
  }
  if (mProgressNotifier) {
    mProgressEventWasDelayed = PR_FALSE;
    mTimerIsActive = PR_TRUE;
    mProgressNotifier->Cancel();
    mProgressNotifier->InitWithCallback(this, NS_PROGRESS_EVENT_INTERVAL,
                                        nsITimer::TYPE_ONE_SHOT);
  }
}

void
FileIOObject::ClearProgressEventTimer()
{
  mProgressEventWasDelayed = PR_FALSE;
  mTimerIsActive = PR_FALSE;
  if (mProgressNotifier) {
    mProgressNotifier->Cancel();
  }
}

void
FileIOObject::DispatchError(nsresult rv, nsAString& finalEvent)
{
  
  switch (rv) {
  case NS_ERROR_FILE_NOT_FOUND:
    mError = new nsDOMFileError(nsIDOMFileError::NOT_FOUND_ERR);
    break;
  case NS_ERROR_FILE_ACCESS_DENIED:
    mError = new nsDOMFileError(nsIDOMFileError::SECURITY_ERR);
    break;
  default:
    mError = new nsDOMFileError(nsIDOMFileError::NOT_READABLE_ERR);
    break;
  }

  
  DispatchProgressEvent(NS_LITERAL_STRING(ERROR_STR));
  DispatchProgressEvent(finalEvent);
}

nsresult
FileIOObject::DispatchProgressEvent(const nsAString& aType)
{
  nsCOMPtr<nsIDOMEvent> event;
  nsresult rv = nsEventDispatcher::CreateEvent(nsnull, nsnull,
                                               NS_LITERAL_STRING("ProgressEvent"),
                                               getter_AddRefs(event));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrivateDOMEvent> privevent(do_QueryInterface(event));
  NS_ENSURE_TRUE(privevent, NS_ERROR_UNEXPECTED);

  privevent->SetTrusted(PR_TRUE);
  nsCOMPtr<nsIDOMProgressEvent> progress = do_QueryInterface(event);
  NS_ENSURE_TRUE(progress, NS_ERROR_UNEXPECTED);

  bool known;
  PRUint64 size;
  if (mTotal != kUnknownSize) {
    known = PR_TRUE;
    size = mTotal;
  } else {
    known = PR_FALSE;
    size = 0;
  }
  rv = progress->InitProgressEvent(aType, PR_FALSE, PR_FALSE, known,
                                   mTransferred, size);
  NS_ENSURE_SUCCESS(rv, rv);

  return DispatchDOMEvent(nsnull, event, nsnull, nsnull);
}


NS_IMETHODIMP
FileIOObject::Notify(nsITimer* aTimer)
{
  nsresult rv;
  mTimerIsActive = PR_FALSE;

  if (mProgressEventWasDelayed) {
    rv = DispatchProgressEvent(NS_LITERAL_STRING("progress"));
    NS_ENSURE_SUCCESS(rv, rv);

    StartProgressEventTimer();
  }

  return NS_OK;
}


NS_IMETHODIMP
FileIOObject::OnStartRequest(nsIRequest *aRequest, nsISupports *aContext)
{
  return DoOnStartRequest(aRequest, aContext);
}

NS_IMETHODIMP
FileIOObject::DoOnStartRequest(nsIRequest *request, nsISupports *ctxt)
{
  return NS_OK;
}

NS_IMETHODIMP
FileIOObject::OnDataAvailable(nsIRequest *aRequest,
                              nsISupports *aContext,
                              nsIInputStream *aInputStream,
                              PRUint32 aOffset,
                              PRUint32 aCount)
{
  nsresult rv;
  rv = DoOnDataAvailable(aRequest, aContext, aInputStream, aOffset, aCount);
  NS_ENSURE_SUCCESS(rv, rv);

  mTransferred += aCount;

  
  if (mTimerIsActive) {
    mProgressEventWasDelayed = PR_TRUE;
  } else {
    rv = DispatchProgressEvent(NS_LITERAL_STRING(PROGRESS_STR));
    NS_ENSURE_SUCCESS(rv, rv);

    StartProgressEventTimer();
  }

  return NS_OK;
}

NS_IMETHODIMP
FileIOObject::OnStopRequest(nsIRequest* aRequest, nsISupports* aContext,
                            nsresult aStatus)
{
  
  
  if (aRequest != mChannel)
    return NS_OK;

  
  ClearProgressEventTimer();

  
  mReadyState = 2;

  nsString successEvent, termEvent;
  nsresult rv = DoOnStopRequest(aRequest, aContext, aStatus,
                                successEvent, termEvent);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (NS_FAILED(aStatus)) {
    DispatchError(aStatus, termEvent);
    return NS_OK;
  }

  
  DispatchProgressEvent(successEvent);
  DispatchProgressEvent(termEvent);

  return NS_OK;
}

NS_IMETHODIMP
FileIOObject::Abort()
{
  if (mReadyState != 1)
    return NS_ERROR_DOM_FILE_ABORT_ERR;

  ClearProgressEventTimer();

  mReadyState = 2; 
                   
  mError = new nsDOMFileError(nsIDOMFileError::ABORT_ERR);

  nsString finalEvent;
  nsresult rv = DoAbort(finalEvent);

  
  DispatchProgressEvent(NS_LITERAL_STRING(ABORT_STR));
  DispatchProgressEvent(finalEvent);

  return rv;
}

NS_IMETHODIMP
FileIOObject::GetReadyState(PRUint16 *aReadyState)
{
  *aReadyState = mReadyState;
  return NS_OK;
}

NS_IMETHODIMP
FileIOObject::GetError(nsIDOMFileError** aError)
{
  NS_IF_ADDREF(*aError = mError);
  return NS_OK;
}

} 
} 
