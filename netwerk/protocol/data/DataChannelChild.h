





#ifndef NS_DATACHANNELCHILD_H
#define NS_DATACHANNELCHILD_H

#include "nsDataChannel.h"
#include "nsIChildChannel.h"
#include "nsISupportsImpl.h"

#include "mozilla/net/PDataChannelChild.h"

namespace mozilla {
namespace net {

class DataChannelChild : public nsDataChannel
                       , public nsIChildChannel
                       , public PDataChannelChild
{
public:
    explicit DataChannelChild(nsIURI *uri);

    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSICHILDCHANNEL

protected:
    virtual void ActorDestroy(ActorDestroyReason why) override;

private:
    ~DataChannelChild();

    void AddIPDLReference();

    bool mIPCOpen;
};

} 
} 

#endif 
