



































#if !defined(nsMediaStream_h_)
#define nsMediaStream_h_

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsIChannel.h"
#include "nsIPrincipal.h"
#include "nsIURI.h"
#include "nsIStreamListener.h"
#include "prlock.h"




#define SEEK_VS_READ_THRESHOLD (32*1024)

class nsMediaDecoder;

















class nsMediaStream 
{
public:
  virtual ~nsMediaStream()
  {
    PR_DestroyLock(mLock);
    MOZ_COUNT_DTOR(nsMediaStream);
  }

  
  
  virtual nsresult Close() = 0;
  
  
  
  
  
  
  virtual nsresult Read(char* aBuffer, PRUint32 aCount, PRUint32* aBytes) = 0;
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual nsresult Seek(PRInt32 aWhence, PRInt64 aOffset) = 0;
  
  
  virtual PRInt64  Tell() = 0;
  
  
  virtual void     Cancel() { }
  
  virtual nsIPrincipal* GetCurrentPrincipal() = 0;
  
  
  virtual void     Suspend() = 0;
  
  
  virtual void     Resume() = 0;

  nsMediaDecoder* Decoder() { return mDecoder; }

  






  static nsresult Open(nsMediaDecoder* aDecoder, nsIURI* aURI,
                       nsIChannel* aChannel, nsMediaStream** aStream,
                       nsIStreamListener** aListener);

protected:
  nsMediaStream(nsMediaDecoder* aDecoder, nsIChannel* aChannel, nsIURI* aURI) :
    mDecoder(aDecoder),
    mChannel(aChannel),
    mURI(aURI),
    mLock(nsnull)  
  {
    MOZ_COUNT_CTOR(nsMediaStream);
    mLock = PR_NewLock();
  }

  





  virtual nsresult Open(nsIStreamListener** aStreamListener) = 0;

  
  
  
  nsMediaDecoder* mDecoder;

  
  
  nsCOMPtr<nsIChannel> mChannel;

  
  
  nsCOMPtr<nsIURI> mURI;

  
  
  
  
  PRLock* mLock;
};

#endif
