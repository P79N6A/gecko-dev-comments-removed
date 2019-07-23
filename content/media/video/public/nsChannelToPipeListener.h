



































#if !defined(nsChannelToPipeListener_h_)
#define nsChannelToPipeListener_h_

#include "nsCOMPtr.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsIRequestObserver.h"
#include "nsIStreamListener.h"
#include "nsIPrincipal.h"

class nsMediaDecoder;






class nsChannelToPipeListener : public nsIStreamListener
{
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIREQUESTOBSERVER

  
  NS_DECL_NSISTREAMLISTENER

  public:
  
  
  
  nsChannelToPipeListener(nsMediaDecoder* aDecoder,
                          PRBool aSeeking = PR_FALSE);
  nsresult Init();
  nsresult GetInputStream(nsIInputStream** aStream);
  void Stop();
  void Cancel();

  nsIPrincipal* GetCurrentPrincipal();

private:
  nsCOMPtr<nsIInputStream> mInput;
  nsCOMPtr<nsIOutputStream> mOutput;
  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsRefPtr<nsMediaDecoder> mDecoder;

  
  PRPackedBool mSeeking;
};

#endif
