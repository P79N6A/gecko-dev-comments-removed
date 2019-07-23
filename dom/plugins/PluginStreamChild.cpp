




































#include "PluginStreamChild.h"
#include "mozilla/plugins/PluginInstanceChild.h"

namespace mozilla {
namespace plugins {

PluginStreamChild::PluginStreamChild(PluginInstanceChild* instance)
  : mInstance(instance)
{
  memset(&mStream, 0, sizeof(mStream));
  mStream.ndata = static_cast<AStream*>(this);
}

int32_t
PluginStreamChild::NPN_Write(int32_t length, void* buffer)
{
  AssertPluginThread();

  int32_t written = 0;
  CallNPN_Write(nsCString(static_cast<char*>(buffer), length),
                &written);
  if (written < 0)
    mInstance->CallPPluginStreamDestructor(this, NPERR_GENERIC_ERROR, true);

  return written;
}

void
PluginStreamChild::NPP_DestroyStream(NPError reason)
{
  AssertPluginThread();

  if (mClosed)
    return;

  mClosed = true;
  mInstance->mPluginIface->destroystream(&mInstance->mData, &mStream, reason);
}

} 
} 
