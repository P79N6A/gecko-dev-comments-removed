




#include "TextureHostBasic.h"
#ifdef XP_MACOSX
#include "MacIOSurfaceTextureHostBasic.h"
#endif

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
  return CreateBackendIndependentTextureHost(aDesc, aDeallocator, aFlags);
}

} 
} 
