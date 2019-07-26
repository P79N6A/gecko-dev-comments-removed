






#include "mozilla/layers/PLayerTransaction.h"
#include "mozilla/layers/ShadowLayers.h"
#include "mozilla/layers/LayerManagerComposite.h"
#include "mozilla/layers/CompositorTypes.h"

#include "gfxPlatform.h"

#include "gfxSharedQuartzSurface.h"

using namespace mozilla::gl;

namespace mozilla {
namespace layers {

bool
ISurfaceAllocator::PlatformAllocSurfaceDescriptor(const gfxIntSize& aSize,
                                                  gfxContentType aContent,
                                                  uint32_t aCaps,
                                                  SurfaceDescriptor* aBuffer)
{
  return false;
}

 already_AddRefed<gfxASurface>
ShadowLayerForwarder::PlatformOpenDescriptor(OpenMode aMode,
                                             const SurfaceDescriptor& aSurface)
{
  if (aSurface.type() == SurfaceDescriptor::TShmem) {
    return gfxSharedQuartzSurface::Open(aSurface.get_Shmem());
  } else if (aSurface.type() == SurfaceDescriptor::TMemoryImage) {
    const MemoryImage& image = aSurface.get_MemoryImage();
    gfxImageFormat format
      = static_cast<gfxImageFormat>(image.format());

    nsRefPtr<gfxASurface> surf =
      new gfxQuartzSurface((unsigned char*)image.data(),
                           image.size(),
                           image.stride(),
                           format);
    return surf.forget();

  }
  return nullptr;
}

 bool
ShadowLayerForwarder::PlatformCloseDescriptor(const SurfaceDescriptor& aDescriptor)
{
  return false;
}

 bool
ShadowLayerForwarder::PlatformGetDescriptorSurfaceContentType(
  const SurfaceDescriptor& aDescriptor, OpenMode aMode,
  gfxContentType* aContent,
  gfxASurface** aSurface)
{
  return false;
}

 bool
ShadowLayerForwarder::PlatformGetDescriptorSurfaceSize(
  const SurfaceDescriptor& aDescriptor, OpenMode aMode,
  gfxIntSize* aSize,
  gfxASurface** aSurface)
{
  return false;
}

 bool
ShadowLayerForwarder::PlatformGetDescriptorSurfaceImageFormat(
  const SurfaceDescriptor&,
  OpenMode,
  gfxImageFormat*,
  gfxASurface**)
{
  return false;
}

bool
ShadowLayerForwarder::PlatformDestroySharedSurface(SurfaceDescriptor* aSurface)
{
  return false;
}

 void
ShadowLayerForwarder::PlatformSyncBeforeUpdate()
{
}

 void
LayerManagerComposite::PlatformSyncBeforeReplyUpdate()
{
}

bool
ISurfaceAllocator::PlatformDestroySharedSurface(SurfaceDescriptor*)
{
  return false;
}

 already_AddRefed<TextureImage>
LayerManagerComposite::OpenDescriptorForDirectTexturing(GLContext*,
                                                        const SurfaceDescriptor&,
                                                        GLenum)
{
  return nullptr;
}

 bool
LayerManagerComposite::SupportsDirectTexturing()
{
  return false;
}


} 
} 
