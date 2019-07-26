




#ifndef GFX_LAYERS_ISURFACEDEALLOCATOR
#define GFX_LAYERS_ISURFACEDEALLOCATOR

#include "mozilla/ipc/SharedMemory.h"
#include "mozilla/RefPtr.h"
#include "gfxPoint.h"
#include "gfxASurface.h"








#ifdef MOZ_WIDGET_GONK
#define MOZ_HAVE_SURFACEDESCRIPTORGRALLOC
#endif

class gfxSharedImageSurface;
class gfxASurface;

namespace base {
class Thread;
} 

namespace mozilla {
namespace ipc {
class Shmem;
} 
namespace layers {

class PGrallocBufferChild;
class MaybeMagicGrallocBufferHandle;

enum BufferCapabilities {
  DEFAULT_BUFFER_CAPS = 0,
  



  MAP_AS_IMAGE_SURFACE = 1 << 0
};

class SurfaceDescriptor;


ipc::SharedMemory::SharedMemoryType OptimalShmemType();
bool IsSurfaceDescriptorValid(const SurfaceDescriptor& aSurface);
bool IsSurfaceDescriptorOwned(const SurfaceDescriptor& aDescriptor);
bool ReleaseOwnedSurfaceDescriptor(const SurfaceDescriptor& aDescriptor);









class ISurfaceAllocator
{
public:
ISurfaceAllocator() {}

  




  virtual bool AllocShmem(size_t aSize,
                          ipc::SharedMemory::SharedMemoryType aType,
                          ipc::Shmem* aShmem) = 0;

  



  virtual bool AllocUnsafeShmem(size_t aSize,
                                ipc::SharedMemory::SharedMemoryType aType,
                                ipc::Shmem* aShmem) = 0;
  


  virtual void DeallocShmem(ipc::Shmem& aShmem) = 0;

  
  virtual bool AllocSharedImageSurface(const gfxIntSize& aSize,
                                       gfxASurface::gfxContentType aContent,
                                       gfxSharedImageSurface** aBuffer);
  virtual bool AllocSurfaceDescriptor(const gfxIntSize& aSize,
                                      gfxASurface::gfxContentType aContent,
                                      SurfaceDescriptor* aBuffer);

  
  virtual bool AllocSurfaceDescriptorWithCaps(const gfxIntSize& aSize,
                                              gfxASurface::gfxContentType aContent,
                                              uint32_t aCaps,
                                              SurfaceDescriptor* aBuffer);

  virtual void DestroySharedSurface(SurfaceDescriptor* aSurface);

protected:
  
  
  virtual bool IsOnCompositorSide() const = 0;
  static bool PlatformDestroySharedSurface(SurfaceDescriptor* aSurface);
  virtual bool PlatformAllocSurfaceDescriptor(const gfxIntSize& aSize,
                                              gfxASurface::gfxContentType aContent,
                                              uint32_t aCaps,
                                              SurfaceDescriptor* aBuffer);
  virtual PGrallocBufferChild* AllocGrallocBuffer(const gfxIntSize& aSize,
                                                  gfxASurface::gfxContentType aContent,
                                                  MaybeMagicGrallocBufferHandle* aHandle)
  {
    return nullptr;
  }

  ~ISurfaceAllocator() {}
};

} 
} 

#endif
