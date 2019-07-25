





































#ifndef nsIconChannel_h_
#define nsIconChannel_h_

#include "nsIChannel.h"
#include "nsIURI.h"
#include "nsIIconURI.h"
#include "nsCOMPtr.h"






class nsIconChannel : public nsIChannel {
  public:
    NS_DECL_ISUPPORTS
    NS_FORWARD_NSIREQUEST(mRealChannel->)
    NS_FORWARD_NSICHANNEL(mRealChannel->)

    nsIconChannel() {}
    ~nsIconChannel() {}

    




    NS_HIDDEN_(nsresult) Init(nsIURI* aURI);
  private:
    



    nsCOMPtr<nsIChannel> mRealChannel;
};

#endif
