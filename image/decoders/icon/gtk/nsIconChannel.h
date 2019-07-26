



#ifndef nsIconChannel_h_
#define nsIconChannel_h_

#include "mozilla/Attributes.h"

#include "nsIChannel.h"
#include "nsIStreamListener.h"
#include "nsIURI.h"
#include "nsIIconURI.h"
#include "nsCOMPtr.h"






class nsIconChannel MOZ_FINAL : public nsIChannel {
  public:
    NS_DECL_ISUPPORTS
    NS_FORWARD_NSIREQUEST(mRealChannel->)
    NS_FORWARD_NSICHANNEL(mRealChannel->)

    nsIconChannel() {}
    ~nsIconChannel() {}

    static void Shutdown();

    




    nsresult Init(nsIURI* aURI);
  private:
    



    nsCOMPtr<nsIChannel> mRealChannel;

    


    nsresult InitWithGnome(nsIMozIconURI *aURI);
    nsresult InitWithGIO(nsIMozIconURI *aIconURI);
};

#endif
