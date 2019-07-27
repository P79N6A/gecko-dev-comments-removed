



#ifndef nsIconChannel_h_
#define nsIconChannel_h_

#include "mozilla/Attributes.h"

#include "nsIChannel.h"
#include "nsIStreamListener.h"
#include "nsIURI.h"
#include "nsIIconURI.h"
#include "nsCOMPtr.h"




class nsIconChannel final : public nsIChannel
{
  public:
    NS_DECL_ISUPPORTS
    NS_FORWARD_NSIREQUEST(mRealChannel->)
    NS_FORWARD_NSICHANNEL(mRealChannel->)

    nsIconChannel() { }

    static void Shutdown();

    
    
    
    nsresult Init(nsIURI* aURI);
  private:
    ~nsIconChannel() { }
    
    
    nsCOMPtr<nsIChannel> mRealChannel;

    nsresult InitWithGIO(nsIMozIconURI* aIconURI);
};

#endif
