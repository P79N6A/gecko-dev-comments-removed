





#ifndef NETWERK_SCTP_DATACHANNEL_DATACHANNELLISTENER_H_
#define NETWERK_SCTP_DATACHANNEL_DATACHANNELLISTENER_H_

#include "nsISupports.h"
#include "nsString.h"

namespace mozilla {




class DataChannelListener {
public:
  virtual ~DataChannelListener() {}

  
  virtual nsresult OnMessageAvailable(nsISupports *aContext,
                                      const nsACString& message) = 0;

  
  virtual nsresult OnBinaryMessageAvailable(nsISupports *aContext,
                                            const nsACString& message) = 0;

  
  virtual nsresult OnChannelConnected(nsISupports *aContext) = 0;

  
  virtual nsresult OnChannelClosed(nsISupports *aContext) = 0;
};

}

#endif
