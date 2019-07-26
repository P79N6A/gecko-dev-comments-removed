




#ifndef GFX_LAYERS_ISURFACEDEALLOCATOR
#define GFX_LAYERS_ISURFACEDEALLOCATOR

#include <stddef.h>                     
#include <stdint.h>                     
#include "gfxTypes.h"
#include "gfxPoint.h"                   
#include "mozilla/ipc/SharedMemory.h"   
#include "mozilla/WeakPtr.h"
#include "nsIMemoryReporter.h"          
#include "mozilla/Atomics.h"            








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
class MemoryTextureClient;
class MemoryTextureHost;

enum BufferCapabilities {
  DEFAULT_BUFFER_CAPS = 0,
  



  MAP_AS_IMAGE_SURFACE = 1 << 0,
  


  USING_GL_RENDERING_ONLY = 1 << 1
};

class SurfaceDescriptor;


mozilla::ipc::SharedMemory::SharedMemoryType OptimalShmemType();
bool IsSurfaceDescriptorValid(const SurfaceDescriptor& aSurface);
bool IsSurfaceDescriptorOwned(const SurfaceDescriptor& aDescriptor);
bool ReleaseOwnedSurfaceDescriptor(const SurfaceDescriptor& aDescriptor);









class ISurfaceAllocator : public SupportsWeakPtr<ISurfaceAllocator>
{
public:
ISurfaceAllocator() {}

  




  virtual bool AllocShmem(size_t aSize,
                          mozilla::ipc::SharedMemory::SharedMemoryType aType,
                          mozilla::ipc::Shmem* aShmem) = 0;

  



  virtual bool AllocUnsafeShmem(size_t aSize,
                                mozilla::ipc::SharedMemory::SharedMemoryType aType,
                                mozilla::ipc::Shmem* aShmem) = 0;
  


  virtual void DeallocShmem(mozilla::ipc::Shmem& aShmem) = 0;

  
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

  virtual bool IPCOpen() const { return true; }

  
  static bool IsShmem(SurfaceDescriptor* aSurface);

protected:
  
  
  virtual bool IsOnCompositorSide() const = 0;
  static bool PlatformDestroySharedSurface(SurfaceDescriptor* aSurface);
  virtual bool PlatformAllocSurfaceDescriptor(const gfxIntSize& aSize,
                                              gfxContentType aContent,
                                              uint32_t aCaps,
                                              SurfaceDescriptor* aBuffer);


  ~ISurfaceAllocator() {}
};

class GfxMemoryImageReporter MOZ_FINAL : public mozilla::MemoryUniReporter
{
public:
  GfxMemoryImageReporter()
    : MemoryUniReporter("explicit/gfx/heap-textures", KIND_HEAP, UNITS_BYTES,
                        "Heap memory shared between threads by texture clients and hosts.")
  {
#ifdef DEBUG
    
    
    static bool hasRun = false;
    MOZ_ASSERT(!hasRun);
    hasRun = true;
#endif
  }

  static void DidAlloc(void* aPointer)
  {
    sAmount += MallocSizeOfOnAlloc(aPointer);
  }

  static void WillFree(void* aPointer)
  {
    sAmount -= MallocSizeOfOnFree(aPointer);
  }

private:
  int64_t Amount() MOZ_OVERRIDE { return sAmount; }

  static mozilla::Atomic<int32_t> sAmount;
};

} 
} 

#endif
