




































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

  nsBaseContentStream(PRBool nonBlocking)
    : mStatus(NS_OK)
    , mNonBlocking(nonBlocking) {
  }

  nsresult Status() { return mStatus; }
  PRBool IsNonBlocking() { return mNonBlocking; }
  PRBool IsClosed() { return NS_FAILED(mStatus); }

  
  PRBool HasPendingCallback() { return mCallback != nsnull; }

  
  nsIEventTarget *CallbackTarget() { return mCallbackTarget; }

  
  
  
  
  void DispatchCallback(PRBool async = PR_TRUE);

  
  void DispatchCallbackSync() { DispatchCallback(PR_FALSE); }

protected:
  virtual ~nsBaseContentStream() {}

private:
  
  
  virtual void OnCallbackPending() {}

private:
  nsCOMPtr<nsIInputStreamCallback> mCallback;
  nsCOMPtr<nsIEventTarget>         mCallbackTarget;
  nsresult                         mStatus;
  PRPackedBool                     mNonBlocking;
};

#endif 
