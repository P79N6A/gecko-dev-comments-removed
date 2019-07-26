




#include "TextureHostBasic.h"
#include "TextureHostX11.h"
#include "MacIOSurfaceTextureHostBasic.h"

using namespace mozilla::gl;
using namespace mozilla::gfx;

namespace mozilla {
namespace layers {

TemporaryRef<TextureHost>
CreateTextureHostBasic(const SurfaceDescriptor& aDesc,
                       ISurfaceAllocator* aDeallocator,
                       TextureFlags aFlags)
{
#ifdef XP_MACOSX
  if (aDesc.type() == SurfaceDescriptor::TSurfaceDescriptorMacIOSurface) {
    const SurfaceDescriptorMacIOSurface& desc =
      aDesc.get_SurfaceDescriptorMacIOSurface();
    RefPtr<TextureHost> result = new MacIOSurfaceTextureHostBasic(aFlags, desc);
    return result;
  }
#endif
#ifdef MOZ_X11
  if (aDesc.type() == SurfaceDescriptor::TSurfaceDescriptorX11) {
    const SurfaceDescriptorX11& desc = aDesc.get_SurfaceDescriptorX11();
    RefPtr<TextureHost> result = new TextureHostX11(aFlags, desc);
    return result;
  }
#endif

  return CreateBackendIndependentTextureHost(aDesc, aDeallocator, aFlags);
}

} 
} 
