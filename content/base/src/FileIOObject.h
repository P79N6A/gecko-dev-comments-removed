




































#ifndef FileIOObject_h__
#define FileIOObject_h__

#include "nsIDOMEventTarget.h"
#include "nsDOMEventTargetWrapperCache.h"
#include "nsIChannel.h"
#include "nsIFile.h"
#include "nsIDOMFile.h"
#include "nsIStreamListener.h"
#include "nsITimer.h"

#include "nsCOMPtr.h"

#define NS_PROGRESS_EVENT_INTERVAL 50

namespace mozilla {
namespace dom {

extern const PRUint64 kUnknownSize;



class FileIOObject : public nsDOMEventTargetWrapperCache,
                     public nsIStreamListener,
                     public nsITimerCallback
{
public:
  FileIOObject();
  ~FileIOObject();

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_METHOD Abort();
  NS_METHOD GetReadyState(PRUint16* aReadyState);
  NS_METHOD GetError(nsIDOMFileError** aError);

  NS_DECL_EVENT_HANDLER(abort);
  NS_DECL_EVENT_HANDLER(error);
  NS_DECL_EVENT_HANDLER(progress);

  NS_DECL_NSITIMERCALLBACK

  NS_DECL_NSISTREAMLISTENER

  NS_DECL_NSIREQUESTOBSERVER

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(FileIOObject,
                                           nsDOMEventTargetWrapperCache)

protected:
  
  NS_IMETHOD DoAbort(nsAString& aEvent) = 0;
  
  
  NS_IMETHOD DoOnStartRequest(nsIRequest *aRequest, nsISupports *aContext);
  
  NS_IMETHOD DoOnStopRequest(nsIRequest *aRequest, nsISupports *aContext,
                             nsresult aStatus, nsAString& aSuccessEvent,
                             nsAString& aTerminationEvent) = 0;
  
  NS_IMETHOD DoOnDataAvailable(nsIRequest *aRequest, nsISupports *aContext,
                               nsIInputStream *aInputStream, PRUint32 aOffset,
                               PRUint32 aCount) = 0;

  void StartProgressEventTimer();
  void ClearProgressEventTimer();
  void DispatchError(nsresult rv, nsAString& finalEvent);
  nsresult DispatchProgressEvent(const nsAString& aType);

  nsCOMPtr<nsITimer> mProgressNotifier;
  bool mProgressEventWasDelayed;
  bool mTimerIsActive;

  nsCOMPtr<nsIDOMFileError> mError;
  nsCOMPtr<nsIChannel> mChannel;

  PRUint16 mReadyState;

  PRUint64 mTotal;
  PRUint64 mTransferred;
};

} 
} 

#endif
