




#include "TextureHostBasic.h"
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
  RefPtr<TextureHost> result;
  switch (aDesc.type()) {
#ifdef XP_MACOSX
    case SurfaceDescriptor::TSurfaceDescriptorMacIOSurface: {
      const SurfaceDescriptorMacIOSurface& desc =
        aDesc.get_SurfaceDescriptorMacIOSurface();
      result = new MacIOSurfaceTextureHostBasic(aFlags, desc);
      break;
    }
#endif
    default: {
      result = CreateBackendIndependentTextureHost(aDesc, aDeallocator, aFlags);
      break;
    }
  }

  return result;
}

} 
} 
