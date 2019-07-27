



#ifndef RedirectChannelRegistrar_h__
#define RedirectChannelRegistrar_h__

#include "nsIRedirectChannelRegistrar.h"

#include "nsIChannel.h"
#include "nsIParentChannel.h"
#include "nsInterfaceHashtable.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace net {

class RedirectChannelRegistrar final : public nsIRedirectChannelRegistrar
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREDIRECTCHANNELREGISTRAR

  RedirectChannelRegistrar();

private:
  ~RedirectChannelRegistrar() {}

protected:
  typedef nsInterfaceHashtable<nsUint32HashKey, nsIChannel>
          ChannelHashtable;
  typedef nsInterfaceHashtable<nsUint32HashKey, nsIParentChannel>
          ParentChannelHashtable;

  ChannelHashtable mRealChannels;
  ParentChannelHashtable mParentChannels;
  uint32_t mId;
};

} 
} 

#endif
