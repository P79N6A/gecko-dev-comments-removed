



#include "RedirectChannelRegistrar.h"

namespace mozilla {
namespace net {

NS_IMPL_ISUPPORTS(RedirectChannelRegistrar, nsIRedirectChannelRegistrar)

RedirectChannelRegistrar::RedirectChannelRegistrar()
  : mRealChannels(32)
  , mParentChannels(32)
  , mId(1)
{
}

NS_IMETHODIMP
RedirectChannelRegistrar::RegisterChannel(nsIChannel *channel,
                                          uint32_t *_retval)
{
  mRealChannels.Put(mId, channel);
  *_retval = mId;

  ++mId;

  
  if (!mId)
    mId = 1;

  return NS_OK;
}

NS_IMETHODIMP
RedirectChannelRegistrar::GetRegisteredChannel(uint32_t id,
                                               nsIChannel **_retval)
{
  if (!mRealChannels.Get(id, _retval))
    return NS_ERROR_NOT_AVAILABLE;

  return NS_OK;
}

NS_IMETHODIMP
RedirectChannelRegistrar::LinkChannels(uint32_t id,
                                       nsIParentChannel *channel,
                                       nsIChannel** _retval)
{
  if (!mRealChannels.Get(id, _retval))
    return NS_ERROR_NOT_AVAILABLE;

  mParentChannels.Put(id, channel);
  return NS_OK;
}

NS_IMETHODIMP
RedirectChannelRegistrar::GetParentChannel(uint32_t id,
                                           nsIParentChannel **_retval)
{
  if (!mParentChannels.Get(id, _retval))
    return NS_ERROR_NOT_AVAILABLE;

  return NS_OK;
}

NS_IMETHODIMP
RedirectChannelRegistrar::DeregisterChannels(uint32_t id)
{
  mRealChannels.Remove(id);
  mParentChannels.Remove(id);
  return NS_OK;
}

}
}
