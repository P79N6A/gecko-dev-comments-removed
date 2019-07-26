




#ifndef GFX_LAYERS_ISURFACEDEALLOCATOR
#define GFX_LAYERS_ISURFACEDEALLOCATOR

#include <stddef.h>                     
#include <stdint.h>                     
#include "gfxTypes.h"
#include "gfxPoint.h"                   
#include "mozilla/ipc/SharedMemory.h"   








#ifdef MOZ_WIDGET_GONK
#define MOZ_HAVE_SURFACEDESCRIPTORGRALLOC
#endif

class gfxSharedImageSurface;

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
  



  MAP_AS_IMAGE_SURFACE = 1 << 0,
  


  USING_GL_RENDERING_ONLY = 1 << 1
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
                                       gfxContentType aContent,
                                       gfxSharedImageSurface** aBuffer);
  virtual bool AllocSurfaceDescriptor(const gfxIntSize& aSize,
                                      gfxContentType aContent,
                                      SurfaceDescriptor* aBuffer);

  
  virtual bool AllocSurfaceDescriptorWithCaps(const gfxIntSize& aSize,
                                              gfxContentType aContent,
                                              uint32_t aCaps,
                                              SurfaceDescriptor* aBuffer);

  virtual void DestroySharedSurface(SurfaceDescriptor* aSurface);

  
  virtual PGrallocBufferChild* AllocGrallocBuffer(const gfxIntSize& aSize,
                                                  uint32_t aFormat,
                                                  uint32_t aUsage,
                                                  MaybeMagicGrallocBufferHandle* aHandle)
  {
    return nullptr;
  }
protected:
  
  
  virtual bool IsOnCompositorSide() const = 0;
  static bool PlatformDestroySharedSurface(SurfaceDescriptor* aSurface);
  virtual bool PlatformAllocSurfaceDescriptor(const gfxIntSize& aSize,
                                              gfxContentType aContent,
                                              uint32_t aCaps,
                                              SurfaceDescriptor* aBuffer);


  ~ISurfaceAllocator() {}
};

} 
} 

#endif
