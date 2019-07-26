




#ifndef nsInputStreamChannel_h__
#define nsInputStreamChannel_h__

#include "nsBaseChannel.h"
#include "nsIInputStreamChannel.h"



class nsInputStreamChannel : public nsBaseChannel
                           , public nsIInputStreamChannel
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIINPUTSTREAMCHANNEL

    nsInputStreamChannel() :
      mIsSrcdocChannel(false) {}

protected:
    virtual ~nsInputStreamChannel() {}

    virtual nsresult OpenContentStream(bool async, nsIInputStream **result,
                                       nsIChannel** channel);

private:
    nsCOMPtr<nsIInputStream> mContentStream;
    nsString mSrcdocData;
    bool mIsSrcdocChannel;
};

#endif 
