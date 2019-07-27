




#include "TextureHostBasic.h"
#ifdef XP_MACOSX
#include "MacIOSurfaceTextureHostBasic.h"
#endif

using namespace mozilla::gl;
using namespace mozilla::gfx;

namespace mozilla {
namespace layers {

already_AddRefed<TextureHost>
CreateTextureHostBasic(const SurfaceDescriptor& aDesc,
                       ISurfaceAllocator* aDeallocator,
                       TextureFlags aFlags)
{
#ifdef XP_MACOSX
  if (aDesc.type() == SurfaceDescriptor::TSurfaceDescriptorMacIOSurface) {
    const SurfaceDescriptorMacIOSurface& desc =
      aDesc.get_SurfaceDescriptorMacIOSurface();
    return MakeAndAddRef<MacIOSurfaceTextureHostBasic>(aFlags, desc);
  }
#endif
  return CreateBackendIndependentTextureHost(aDesc, aDeallocator, aFlags);
}

} 
} 
