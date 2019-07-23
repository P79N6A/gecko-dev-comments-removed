



































#if !defined(nsChannelToPipeListener_h_)
#define nsChannelToPipeListener_h_

#include "nsCOMPtr.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsIRequestObserver.h"
#include "nsIStreamListener.h"
#include "nsIPrincipal.h"



#define NS_MEDIA_UNKNOWN_RATE -1.0

class nsMediaDecoder;






class nsChannelToPipeListener : public nsIStreamListener
{
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIREQUESTOBSERVER

  
  NS_DECL_NSISTREAMLISTENER

  public:
  
  
  
  nsChannelToPipeListener(nsMediaDecoder* aDecoder,
                          PRBool aSeeking = PR_FALSE,
                          PRInt64 aOffset = 0);
  nsresult Init();
  nsresult GetInputStream(nsIInputStream** aStream);
  void Stop();
  void Cancel();

  
  
  double BytesPerSecond() const;

  nsIPrincipal* GetCurrentPrincipal();

private:
  nsCOMPtr<nsIInputStream> mInput;
  nsCOMPtr<nsIOutputStream> mOutput;
  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsRefPtr<nsMediaDecoder> mDecoder;

  
  
  PRIntervalTime mIntervalStart;

  
  
  PRIntervalTime mIntervalEnd;

  
  
  
  PRInt64 mOffset;

  
  PRInt64 mTotalBytes;

  
  PRPackedBool mSeeking;
};

#endif
