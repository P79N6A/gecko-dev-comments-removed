






#include "mozilla/layers/PLayers.h"
#include "mozilla/layers/ShadowLayers.h"
 
#include "gfxPlatform.h"

#include "gfxSharedQuartzSurface.h"

using namespace mozilla::gl;

namespace mozilla {
namespace layers {

bool
ShadowLayerForwarder::PlatformAllocBuffer(const gfxIntSize& aSize,
                                          gfxASurface::gfxContentType aContent,
                                          uint32_t aCaps,
                                          SurfaceDescriptor* aBuffer)
{
  return false;
}

 already_AddRefed<gfxASurface>
ShadowLayerForwarder::PlatformOpenDescriptor(OpenMode aMode,
                                             const SurfaceDescriptor& aSurface)
{
  if (SurfaceDescriptor::TShmem != aSurface.type()) {
    return nullptr;
  }
  return gfxSharedQuartzSurface::Open(aSurface.get_Shmem());
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
ShadowLayerForwarder::PlatformDestroySharedSurface(SurfaceDescriptor* aSurface)
{
  return false;
}

 void
ShadowLayerForwarder::PlatformSyncBeforeUpdate()
{
}

 void
ShadowLayerManager::PlatformSyncBeforeReplyUpdate()
{
}

bool
ShadowLayerManager::PlatformDestroySharedSurface(SurfaceDescriptor*)
{
  return false;
}

 already_AddRefed<TextureImage>
ShadowLayerManager::OpenDescriptorForDirectTexturing(GLContext*,
                                                     const SurfaceDescriptor&,
                                                     GLenum)
{
  return nullptr;
}

} 
} 
