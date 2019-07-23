



































#if !defined(nsMediaStream_h_)
#define nsMediaStream_h_

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsIChannel.h"
#include "nsIPrincipal.h"
#include "nsIURI.h"
#include "prlock.h"




#define SEEK_VS_READ_THRESHOLD (32*1024)

class nsMediaDecoder;




class nsStreamStrategy 
{
public:
 nsStreamStrategy(nsMediaDecoder* aDecoder, nsIChannel* aChannel, nsIURI* aURI) :
    mDecoder(aDecoder),
    mChannel(aChannel),
    mURI(aURI),
    mLock(nsnull)  
  {
    MOZ_COUNT_CTOR(nsStreamStrategy);
    mLock = PR_NewLock();
  }

  virtual ~nsStreamStrategy()
  {
    PR_DestroyLock(mLock);
    MOZ_COUNT_DTOR(nsStreamStrategy);
  }

  
  

  





  virtual nsresult Open(nsIStreamListener** aStreamListener) = 0;
  virtual nsresult Close() = 0;
  virtual nsresult Read(char* aBuffer, PRUint32 aCount, PRUint32* aBytes) = 0;
  virtual nsresult Seek(PRInt32 aWhence, PRInt64 aOffset) = 0;
  virtual PRInt64  Tell() = 0;
  virtual PRUint32 Available() = 0;
  virtual float    DownloadRate() = 0;
  virtual void     Cancel() { }
  virtual nsIPrincipal* GetCurrentPrincipal() = 0;
  virtual void     Suspend() = 0;
  virtual void     Resume() = 0;

protected:
  
  
  
  nsMediaDecoder* mDecoder;

  
  
  nsCOMPtr<nsIChannel> mChannel;

  
  
  nsCOMPtr<nsIURI> mURI;

  
  
  
  
  PRLock* mLock;
};















class nsMediaStream
{
 public:
  nsMediaStream();
  ~nsMediaStream();

  






  nsresult Open(nsMediaDecoder* aDecoder, nsIURI* aURI,
                nsIChannel* aChannel, nsIStreamListener** aListener);

  
  
  nsresult Close();

  
  
  
  
  
  
  nsresult Read(char* aBuffer, PRUint32 aCount, PRUint32* aBytes);

  
  
  
  
  
  
  
  nsresult Seek(PRInt32 aWhence, PRInt64 aOffset);

  
  
  PRInt64 Tell();

  
  
  PRUint32 Available();

  
  
  
  float DownloadRate();

  
  
  float PlaybackRate();

  
  
  void Cancel();

  
  nsIPrincipal* GetCurrentPrincipal();

  
  
  void Suspend();

  
  
  void Resume();

 private:
  
  
  
  
  nsAutoPtr<nsStreamStrategy> mStreamStrategy;

  
  
  
  PRIntervalTime mPlaybackRateStart;

  
  
  
  
  PRUint32 mPlaybackRateCount;
};

#endif
