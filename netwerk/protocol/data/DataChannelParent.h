





#ifndef NS_DATACHANNELPARENT_H
#define NS_DATACHANNELPARENT_H

#include "nsIParentChannel.h"
#include "nsISupportsImpl.h"

#include "mozilla/net/PDataChannelParent.h"

namespace mozilla {
namespace net {




class DataChannelParent : public nsIParentChannel
                        , public PDataChannelParent
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPARENTCHANNEL
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER

    bool Init(const uint32_t& aArgs);

private:
    ~DataChannelParent();

    virtual void ActorDestroy(ActorDestroyReason why) override;
};

} 
} 

#endif 
