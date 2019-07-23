




































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

    nsInputStreamChannel() {}

protected:
    virtual ~nsInputStreamChannel() {}

    virtual nsresult OpenContentStream(PRBool async, nsIInputStream **result);

private:
    nsCOMPtr<nsIInputStream> mContentStream;
};

#endif 
