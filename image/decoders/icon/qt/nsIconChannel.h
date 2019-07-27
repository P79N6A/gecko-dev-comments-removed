




#ifndef mozilla_image_encoders_icon_qt_nsIconChannel_h
#define mozilla_image_encoders_icon_qt_nsIconChannel_h

#include "mozilla/Attributes.h"

#include "nsIChannel.h"
#include "nsIStreamListener.h"
#include "nsIURI.h"
#include "nsIIconURI.h"
#include "nsCOMPtr.h"




class nsIconChannel final : public nsIChannel {
  public:
    NS_DECL_ISUPPORTS
    NS_FORWARD_NSIREQUEST(mRealChannel->)
    NS_FORWARD_NSICHANNEL(mRealChannel->)

    nsIconChannel() { };

    
    
    
    nsresult Init(nsIURI* aURI);
  private:
    ~nsIconChannel() { };

    
    
    nsCOMPtr<nsIChannel> mRealChannel;
};

#endif 
