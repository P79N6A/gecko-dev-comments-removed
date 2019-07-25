





































#include "mozilla/plugins/PluginSurfaceParent.h"
#include "mozilla/gfx/SharedDIBSurface.h"

using mozilla::gfx::SharedDIBSurface;

namespace mozilla {
namespace plugins {

PluginSurfaceParent::PluginSurfaceParent(const WindowsSharedMemoryHandle& handle,
                                         const gfxIntSize& size,
                                         bool transparent)
{
  SharedDIBSurface* dibsurf = new SharedDIBSurface();
  if (dibsurf->Attach(handle, size.width, size.height, transparent))
    mSurface = dibsurf;
}

}
}
