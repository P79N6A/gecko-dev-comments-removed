




#include "TextureHostBasic.h"
#include "MacIOSurfaceTextureHostBasic.h"

using namespace mozilla::gl;
using namespace mozilla::gfx;

namespace mozilla {
namespace layers {

TemporaryRef<TextureHost>
CreateTextureHostBasic(uint64_t aID,
                       const SurfaceDescriptor& aDesc,
                       ISurfaceAllocator* aDeallocator,
                       TextureFlags aFlags)
{
  RefPtr<TextureHost> result;
  switch (aDesc.type()) {
#ifdef XP_MACOSX
    case SurfaceDescriptor::TSurfaceDescriptorMacIOSurface: {
      const SurfaceDescriptorMacIOSurface& desc =
        aDesc.get_SurfaceDescriptorMacIOSurface();
      result = new MacIOSurfaceTextureHostBasic(aID, aFlags, desc);
      break;
    }
#endif
    default: {
      result = CreateBackendIndependentTextureHost(aID, aDesc, aDeallocator, aFlags);
      break;
    }
  }

  return result;
}

} 
} 
