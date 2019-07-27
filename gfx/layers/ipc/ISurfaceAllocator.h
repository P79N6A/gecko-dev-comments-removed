




#ifndef GFX_LAYERS_ISURFACEDEALLOCATOR
#define GFX_LAYERS_ISURFACEDEALLOCATOR

#include <stddef.h>                     
#include <stdint.h>                     
#include "gfxTypes.h"
#include "mozilla/gfx/Point.h"          
#include "mozilla/ipc/SharedMemory.h"   
#include "mozilla/RefPtr.h"
#include "nsIMemoryReporter.h"          
#include "mozilla/Atomics.h"            
#include "mozilla/layers/LayersMessages.h" 
#include "LayersTypes.h"
#include <vector>
#include "mozilla/layers/AtomicRefCountedWithFinalize.h"








#ifdef MOZ_WIDGET_GONK
#define MOZ_HAVE_SURFACEDESCRIPTORGRALLOC
#endif

namespace mozilla {
namespace ipc {
class Shmem;
}
namespace gfx {
class DataSourceSurface;
}

namespace layers {

class MaybeMagicGrallocBufferHandle;

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

TemporaryRef<gfx::DrawTarget> GetDrawTargetForDescriptor(const SurfaceDescriptor& aDescriptor, gfx::BackendType aBackend);
TemporaryRef<gfx::DataSourceSurface> GetSurfaceForDescriptor(const SurfaceDescriptor& aDescriptor);









class ISurfaceAllocator : public AtomicRefCountedWithFinalize<ISurfaceAllocator>
{
public:
  MOZ_DECLARE_REFCOUNTED_TYPENAME(ISurfaceAllocator)
  ISurfaceAllocator()
    : mDefaultMessageLoop(MessageLoop::current())
  {}

  void Finalize();

  







  virtual LayersBackend GetCompositorBackendType() const = 0;

  




  virtual bool AllocShmem(size_t aSize,
                          mozilla::ipc::SharedMemory::SharedMemoryType aType,
                          mozilla::ipc::Shmem* aShmem) = 0;

  



  virtual bool AllocUnsafeShmem(size_t aSize,
                                mozilla::ipc::SharedMemory::SharedMemoryType aType,
                                mozilla::ipc::Shmem* aShmem) = 0;

  




  bool AllocShmemSection(size_t aSize,
                         mozilla::layers::ShmemSection* aShmemSection);

  


  void FreeShmemSection(mozilla::layers::ShmemSection& aShmemSection);

  


  virtual void DeallocShmem(mozilla::ipc::Shmem& aShmem) = 0;

  
  virtual bool AllocSurfaceDescriptor(const gfx::IntSize& aSize,
                                      gfxContentType aContent,
                                      SurfaceDescriptor* aBuffer);

  
  virtual bool AllocSurfaceDescriptorWithCaps(const gfx::IntSize& aSize,
                                              gfxContentType aContent,
                                              uint32_t aCaps,
                                              SurfaceDescriptor* aBuffer);

  


  virtual int32_t GetMaxTextureSize() const { return INT32_MAX; }

  virtual void DestroySharedSurface(SurfaceDescriptor* aSurface);

  
  bool AllocGrallocBuffer(const gfx::IntSize& aSize,
                          uint32_t aFormat,
                          uint32_t aUsage,
                          MaybeMagicGrallocBufferHandle* aHandle);

  void DeallocGrallocBuffer(MaybeMagicGrallocBufferHandle* aHandle);

  void DropGrallocBuffer(MaybeMagicGrallocBufferHandle* aHandle);

  virtual bool IPCOpen() const { return true; }
  virtual bool IsSameProcess() const = 0;

  virtual bool IsImageBridgeChild() const { return false; }

  virtual MessageLoop * GetMessageLoop() const
  {
    return mDefaultMessageLoop;
  }

  
  static bool IsShmem(SurfaceDescriptor* aSurface);

protected:

  virtual bool IsOnCompositorSide() const = 0;

  virtual ~ISurfaceAllocator();

  void ShrinkShmemSectionHeap();

  
  std::vector<mozilla::ipc::Shmem> mUsedShmems;

  MessageLoop* mDefaultMessageLoop;

  friend class AtomicRefCountedWithFinalize<ISurfaceAllocator>;
};

class GfxMemoryImageReporter final : public nsIMemoryReporter
{
  ~GfxMemoryImageReporter() {}

public:
  NS_DECL_ISUPPORTS

  GfxMemoryImageReporter()
  {
#ifdef DEBUG
    
    
    static bool hasRun = false;
    MOZ_ASSERT(!hasRun);
    hasRun = true;
#endif
  }

  MOZ_DEFINE_MALLOC_SIZE_OF_ON_ALLOC(MallocSizeOfOnAlloc)
  MOZ_DEFINE_MALLOC_SIZE_OF_ON_FREE(MallocSizeOfOnFree)

  static void DidAlloc(void* aPointer)
  {
    sAmount += MallocSizeOfOnAlloc(aPointer);
  }

  static void WillFree(void* aPointer)
  {
    sAmount -= MallocSizeOfOnFree(aPointer);
  }

  NS_IMETHOD CollectReports(nsIHandleReportCallback* aHandleReport,
                            nsISupports* aData, bool aAnonymize) override
  {
    return MOZ_COLLECT_REPORT(
      "explicit/gfx/heap-textures", KIND_HEAP, UNITS_BYTES, sAmount,
      "Heap memory shared between threads by texture clients and hosts.");
  }

private:
  static mozilla::Atomic<size_t> sAmount;
};

} 
} 

#endif
