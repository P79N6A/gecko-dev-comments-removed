




































#include "PluginStreamParent.h"
#include "PluginInstanceParent.h"

namespace mozilla {
namespace plugins {

PluginStreamParent::PluginStreamParent(PluginInstanceParent* npp,
                                       const nsCString& mimeType,
                                       const nsCString& target,
                                       NPError* result)
  : mInstance(npp)
  , mClosed(false)
{
  *result = mInstance->mNPNIface->newstream(mInstance->mNPP,
                                            const_cast<char*>(mimeType.get()),
                                            NullableStringGet(target),
                                            &mStream);
  if (*result == NPERR_NO_ERROR)
    mStream->pdata = static_cast<AStream*>(this);
  else
    mStream = NULL;
}

bool
PluginStreamParent::AnswerNPN_Write(const Buffer& data, int32_t* written)
{
  if (mClosed) {
    *written = -1;
    return true;
  }

  *written = mInstance->mNPNIface->write(mInstance->mNPP, mStream,
                                         data.Length(),
                                         const_cast<char*>(data.get()));
  if (*written < 0)
    mClosed = true;

  return true;
}

void
PluginStreamParent::NPN_DestroyStream(NPReason reason)
{
  if (mClosed)
    return;

  mInstance->mNPNIface->destroystream(mInstance->mNPP, mStream, reason);
  mClosed = true;
}

} 
} 
