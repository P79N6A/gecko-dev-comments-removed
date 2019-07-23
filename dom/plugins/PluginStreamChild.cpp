




































#include "PluginStreamChild.h"
#include "mozilla/plugins/PluginInstanceChild.h"

namespace mozilla {
namespace plugins {

PluginStreamChild::PluginStreamChild()
{
  memset(&mStream, 0, sizeof(mStream));
  mStream.ndata = static_cast<AStream*>(this);
}

bool
PluginStreamChild::Answer__delete__(const NPReason& reason,
                                    const bool& artificial)
{
  AssertPluginThread();
  if (!artificial)
    NPP_DestroyStream(reason);
  return true;
}

int32_t
PluginStreamChild::NPN_Write(int32_t length, void* buffer)
{
  AssertPluginThread();

  int32_t written = 0;
  CallNPN_Write(nsCString(static_cast<char*>(buffer), length),
                &written);
  if (written < 0)
    PPluginStreamChild::Call__delete__(this, NPERR_GENERIC_ERROR, true);
  

  return written;
}

void
PluginStreamChild::NPP_DestroyStream(NPError reason)
{
  AssertPluginThread();

  if (mClosed)
    return;

  mClosed = true;
  Instance()->mPluginIface->destroystream(
    &Instance()->mData, &mStream, reason);
}

PluginInstanceChild*
PluginStreamChild::Instance()
{
  return static_cast<PluginInstanceChild*>(Manager());
}

} 
} 
