




































#ifndef nsBaseContentStream_h__
#define nsBaseContentStream_h__

#include "nsIAsyncInputStream.h"
#include "nsIEventTarget.h"
#include "nsCOMPtr.h"
























class nsBaseContentStream : public nsIAsyncInputStream
{
public: 
  NS_DECL_ISUPPORTS
  NS_DECL_NSIINPUTSTREAM
  NS_DECL_NSIASYNCINPUTSTREAM

  nsBaseContentStream(bool nonBlocking)
    : mStatus(NS_OK)
    , mNonBlocking(nonBlocking) {
  }

  nsresult Status() { return mStatus; }
  bool IsNonBlocking() { return mNonBlocking; }
  bool IsClosed() { return NS_FAILED(mStatus); }

  
  bool HasPendingCallback() { return mCallback != nsnull; }

  
  nsIEventTarget *CallbackTarget() { return mCallbackTarget; }

  
  
  
  
  void DispatchCallback(bool async = true);

  
  void DispatchCallbackSync() { DispatchCallback(false); }

protected:
  virtual ~nsBaseContentStream() {}

private:
  
  
  virtual void OnCallbackPending() {}

private:
  nsCOMPtr<nsIInputStreamCallback> mCallback;
  nsCOMPtr<nsIEventTarget>         mCallbackTarget;
  nsresult                         mStatus;
  bool                             mNonBlocking;
};

#endif 
