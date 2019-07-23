




































#if !defined(nsChannelReader_h___)
#define nsChannelReader_h___

#include "nsCOMPtr.h"
#include "nsIChannel.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsIRequestObserver.h"
#include "nsIStreamListener.h"
#include "nsIPrincipal.h"

#include "oggplay/oggplay.h"

class nsOggDecoder;






class nsChannelToPipeListener : public nsIStreamListener
{
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIREQUESTOBSERVER

  
  NS_DECL_NSISTREAMLISTENER

  public:
  nsChannelToPipeListener(nsOggDecoder* aDecoder);
  nsresult Init();
  void GetInputStream(nsIInputStream** aStream);
  void Stop();
  double BytesPerSecond() const;

  nsIPrincipal* GetCurrentPrincipal();

private:
  nsCOMPtr<nsIInputStream> mInput;
  nsCOMPtr<nsIOutputStream> mOutput;
  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsOggDecoder* mDecoder;

  
  
  PRIntervalTime mIntervalStart;

  
  
  PRIntervalTime mIntervalEnd;

  
  PRInt64 mTotalBytes;
};

class nsChannelReader : public OggPlayReader
{
public:
  nsChannelReader();
  nsresult Init(nsOggDecoder* aDecoder, nsIURI* aURI);
  PRUint32 Available();
  OggPlayErrorCode initialise(int aBlock);
  OggPlayErrorCode destroy();
  size_t io_read(char* aBuffer, size_t aCount);
  int io_seek(long aOffset, int aWhence);
  long io_tell();  

  
  
  double BytesPerSecond() const;

  
  nsIPrincipal* GetCurrentPrincipal();

public:
  nsCOMPtr<nsIChannel>  mChannel;
  nsCOMPtr<nsIInputStream>  mInput;
  nsCOMPtr<nsChannelToPipeListener> mListener;
  unsigned long mCurrentPosition;
};

#endif
