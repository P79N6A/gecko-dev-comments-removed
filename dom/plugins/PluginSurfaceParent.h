





































#ifndef dom_plugins_PluginSurfaceParent_h
#define dom_plugins_PluginSurfaceParent_h

#include "mozilla/plugins/PPluginSurfaceParent.h"
#include "gfxASurface.h"
#include "nsAutoPtr.h"
#include "mozilla/plugins/PluginMessageUtils.h"

#ifndef XP_WIN
#error "This header is for Windows only."
#endif

namespace mozilla {
namespace plugins {

class PluginSurfaceParent : public PPluginSurfaceParent
{
public:
  PluginSurfaceParent(const WindowsSharedMemoryHandle& handle,
                      const gfxIntSize& size,
                      const bool transparent);
  ~PluginSurfaceParent() { }

  gfxASurface* Surface() { return mSurface; }

private:
  nsRefPtr<gfxASurface> mSurface;
};

} 
} 

#endif 
