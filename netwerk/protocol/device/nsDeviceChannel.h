




































#ifndef nsDeviceChannel_h_
#define nsDeviceChannel_h_

#include "nsBaseChannel.h"

class nsDeviceChannel : public nsBaseChannel
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  nsDeviceChannel();
  ~nsDeviceChannel();

  nsresult Init(nsIURI* uri);
  nsresult OpenContentStream(bool aAsync,
                             nsIInputStream **aStream,
                             nsIChannel **aChannel);
};
#endif
