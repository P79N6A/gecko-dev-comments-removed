






#include "ISurfaceAllocator.h"
#include <sys/types.h>                  
#include "gfx2DGlue.h"                  
#include "gfxPlatform.h"                
#include "gfxSharedImageSurface.h"      
#include "mozilla/Assertions.h"         
#include "mozilla/Atomics.h"            
#include "mozilla/ipc/SharedMemory.h"   
#include "mozilla/layers/LayersSurfaces.h"  
#include "mozilla/layers/SharedBufferManagerChild.h"
#include "ShadowLayerUtils.h"
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsDebug.h"                    
#include "nsXULAppAPI.h"                
#include "mozilla/ipc/Shmem.h"
#include "mozilla/layers/ImageDataSerializer.h"
#ifdef DEBUG
#include "prenv.h"
#endif

using namespace mozilla::ipc;

namespace mozilla {
namespace layers {

NS_IMPL_ISUPPORTS(GfxMemoryImageReporter, nsIMemoryReporter)

mozilla::Atomic<size_t> GfxMemoryImageReporter::sAmount(0);

mozilla::ipc::SharedMemory::SharedMemoryType OptimalShmemType()
{
  return mozilla::ipc::SharedMemory::TYPE_BASIC;
}

bool
IsSurfaceDescriptorValid(const SurfaceDescriptor& aSurface)
{
  return aSurface.type() != SurfaceDescriptor::T__None &&
         aSurface.type() != SurfaceDescriptor::Tnull_t;
}

ISurfaceAllocator::~ISurfaceAllocator()
{
  
  MOZ_ASSERT(mUsedShmems.empty());
}

void
ISurfaceAllocator::Finalize()
{
  ShrinkShmemSectionHeap();
}

static inline uint8_t*
GetAddressFromDescriptor(const SurfaceDescriptor& aDescriptor, size_t& aSize)
{
  MOZ_ASSERT(IsSurfaceDescriptorValid(aDescriptor));
  MOZ_ASSERT(aDescriptor.type() == SurfaceDescriptor::TSurfaceDescriptorShmem ||
             aDescriptor.type() == SurfaceDescriptor::TSurfaceDescriptorMemory);
  if (aDescriptor.type() == SurfaceDescriptor::TSurfaceDescriptorShmem) {
    Shmem shmem(aDescriptor.get_SurfaceDescriptorShmem().data());
    aSize = shmem.Size<uint8_t>();
    return shmem.get<uint8_t>();
  } else {
    const SurfaceDescriptorMemory& image = aDescriptor.get_SurfaceDescriptorMemory();
    aSize = std::numeric_limits<size_t>::max();
    return reinterpret_cast<uint8_t*>(image.data());
  }
}

already_AddRefed<gfx::DrawTarget>
GetDrawTargetForDescriptor(const SurfaceDescriptor& aDescriptor, gfx::BackendType aBackend)
{
  size_t size;
  uint8_t* data = GetAddressFromDescriptor(aDescriptor, size);
  ImageDataDeserializer image(data, size);
  return image.GetAsDrawTarget(aBackend);
}

already_AddRefed<gfx::DataSourceSurface>
GetSurfaceForDescriptor(const SurfaceDescriptor& aDescriptor)
{
  size_t size;
  uint8_t* data = GetAddressFromDescriptor(aDescriptor, size);
  ImageDataDeserializer image(data, size);
  return image.GetAsSurface();
}

bool
ISurfaceAllocator::AllocSurfaceDescriptor(const gfx::IntSize& aSize,
                                          gfxContentType aContent,
                                          SurfaceDescriptor* aBuffer)
{
  return AllocSurfaceDescriptorWithCaps(aSize, aContent, DEFAULT_BUFFER_CAPS, aBuffer);
}

bool
ISurfaceAllocator::AllocSurfaceDescriptorWithCaps(const gfx::IntSize& aSize,
                                                  gfxContentType aContent,
                                                  uint32_t aCaps,
                                                  SurfaceDescriptor* aBuffer)
{
  gfx::SurfaceFormat format =
    gfxPlatform::GetPlatform()->Optimal2DFormatForContent(aContent);
  size_t size = ImageDataSerializer::ComputeMinBufferSize(aSize, format);
  if (!size) {
    return false;
  }
  if (IsSameProcess()) {
    uint8_t *data = new (std::nothrow) uint8_t[size];
    if (!data) {
      return false;
    }
    GfxMemoryImageReporter::DidAlloc(data);
#ifdef XP_MACOSX
    
    
    if (format == gfx::SurfaceFormat::A8) {
      memset(data, 0, size);
    }
#endif
    *aBuffer = SurfaceDescriptorMemory((uintptr_t)data, format);
  } else {

    mozilla::ipc::SharedMemory::SharedMemoryType shmemType = OptimalShmemType();
    mozilla::ipc::Shmem shmem;
    if (!AllocUnsafeShmem(size, shmemType, &shmem)) {
      return false;
    }

    *aBuffer = SurfaceDescriptorShmem(shmem, format);
  }
  
  uint8_t* data = GetAddressFromDescriptor(*aBuffer, size);
  ImageDataSerializer serializer(data, size);
  serializer.InitializeBufferInfo(aSize, format);
  return true;
}

 bool
ISurfaceAllocator::IsShmem(SurfaceDescriptor* aSurface)
{
  return aSurface && (aSurface->type() == SurfaceDescriptor::TSurfaceDescriptorShmem);
}

void
ISurfaceAllocator::DestroySharedSurface(SurfaceDescriptor* aSurface)
{
  MOZ_ASSERT(aSurface);
  if (!aSurface) {
    return;
  }
  if (!IPCOpen()) {
    return;
  }
  switch (aSurface->type()) {
    case SurfaceDescriptor::TSurfaceDescriptorShmem:
      DeallocShmem(aSurface->get_SurfaceDescriptorShmem().data());
      break;
    case SurfaceDescriptor::TSurfaceDescriptorMemory:
      GfxMemoryImageReporter::WillFree((uint8_t*)aSurface->get_SurfaceDescriptorMemory().data());
      delete [] (uint8_t*)aSurface->get_SurfaceDescriptorMemory().data();
      break;
    case SurfaceDescriptor::Tnull_t:
    case SurfaceDescriptor::T__None:
      break;
    default:
      NS_RUNTIMEABORT("surface type not implemented!");
  }
  *aSurface = SurfaceDescriptor();
}



const uint32_t sShmemPageSize = 4096;

#ifdef DEBUG
const uint32_t sSupportedBlockSize = 4;
#endif

enum AllocationStatus
{
  STATUS_ALLOCATED,
  STATUS_FREED
};

struct ShmemSectionHeapHeader
{
  Atomic<uint32_t> mTotalBlocks;
  Atomic<uint32_t> mAllocatedBlocks;
};

struct ShmemSectionHeapAllocation
{
  Atomic<uint32_t> mStatus;
  uint32_t mSize;
};

bool
ISurfaceAllocator::AllocShmemSection(size_t aSize, mozilla::layers::ShmemSection* aShmemSection)
{
  
  
  MOZ_ASSERT(aSize == sSupportedBlockSize);
  MOZ_ASSERT(aShmemSection);

  uint32_t allocationSize = (aSize + sizeof(ShmemSectionHeapAllocation));

  for (size_t i = 0; i < mUsedShmems.size(); i++) {
    ShmemSectionHeapHeader* header = mUsedShmems[i].get<ShmemSectionHeapHeader>();
    if ((header->mAllocatedBlocks + 1) * allocationSize + sizeof(ShmemSectionHeapHeader) < sShmemPageSize) {
      aShmemSection->shmem() = mUsedShmems[i];
      MOZ_ASSERT(mUsedShmems[i].IsWritable());
      break;
    }
  }

  if (!aShmemSection->shmem().IsWritable()) {
    ipc::Shmem tmp;
    if (!AllocUnsafeShmem(sShmemPageSize, ipc::SharedMemory::TYPE_BASIC, &tmp)) {
      return false;
    }

    ShmemSectionHeapHeader* header = tmp.get<ShmemSectionHeapHeader>();
    header->mTotalBlocks = 0;
    header->mAllocatedBlocks = 0;

    mUsedShmems.push_back(tmp);
    aShmemSection->shmem() = tmp;
  }

  MOZ_ASSERT(aShmemSection->shmem().IsWritable());

  ShmemSectionHeapHeader* header = aShmemSection->shmem().get<ShmemSectionHeapHeader>();
  uint8_t* heap = aShmemSection->shmem().get<uint8_t>() + sizeof(ShmemSectionHeapHeader);

  ShmemSectionHeapAllocation* allocHeader = nullptr;

  if (header->mTotalBlocks > header->mAllocatedBlocks) {
    
    for (size_t i = 0; i < header->mTotalBlocks; i++) {
      allocHeader = reinterpret_cast<ShmemSectionHeapAllocation*>(heap);

      if (allocHeader->mStatus == STATUS_FREED) {
        break;
      }
      heap += allocationSize;
    }
    MOZ_ASSERT(allocHeader && allocHeader->mStatus == STATUS_FREED);
    MOZ_ASSERT(allocHeader->mSize == sSupportedBlockSize);
  } else {
    heap += header->mTotalBlocks * allocationSize;

    header->mTotalBlocks++;
    allocHeader = reinterpret_cast<ShmemSectionHeapAllocation*>(heap);
    allocHeader->mSize = aSize;
  }

  MOZ_ASSERT(allocHeader);
  header->mAllocatedBlocks++;
  allocHeader->mStatus = STATUS_ALLOCATED;

  aShmemSection->size() = aSize;
  aShmemSection->offset() = (heap + sizeof(ShmemSectionHeapAllocation)) - aShmemSection->shmem().get<uint8_t>();
  ShrinkShmemSectionHeap();
  return true;
}

void
ISurfaceAllocator::FreeShmemSection(mozilla::layers::ShmemSection& aShmemSection)
{
  MOZ_ASSERT(aShmemSection.size() == sSupportedBlockSize);
  MOZ_ASSERT(aShmemSection.offset() < sShmemPageSize - sSupportedBlockSize);

  ShmemSectionHeapAllocation* allocHeader =
    reinterpret_cast<ShmemSectionHeapAllocation*>(aShmemSection.shmem().get<char>() +
                                                  aShmemSection.offset() -
                                                  sizeof(ShmemSectionHeapAllocation));

  MOZ_ASSERT(allocHeader->mSize == aShmemSection.size());

  DebugOnly<bool> success = allocHeader->mStatus.compareExchange(STATUS_ALLOCATED, STATUS_FREED);
  
  MOZ_ASSERT(success);

  ShmemSectionHeapHeader* header = aShmemSection.shmem().get<ShmemSectionHeapHeader>();
  header->mAllocatedBlocks--;

  ShrinkShmemSectionHeap();
}


void
ISurfaceAllocator::ShrinkShmemSectionHeap()
{
  
  
  size_t i = 0;
  while (i < mUsedShmems.size()) {
    ShmemSectionHeapHeader* header = mUsedShmems[i].get<ShmemSectionHeapHeader>();
    if (header->mAllocatedBlocks == 0) {
      DeallocShmem(mUsedShmems[i]);

      
      
      if (i < mUsedShmems.size() - 1) {
        mUsedShmems[i] = mUsedShmems[mUsedShmems.size() - 1];
      }
      mUsedShmems.pop_back();
    } else {
      i++;
    }
  }
}

bool
ISurfaceAllocator::AllocGrallocBuffer(const gfx::IntSize& aSize,
                                      uint32_t aFormat,
                                      uint32_t aUsage,
                                      MaybeMagicGrallocBufferHandle* aHandle)
{
  return SharedBufferManagerChild::GetSingleton()->AllocGrallocBuffer(aSize, aFormat, aUsage, aHandle);
}

void
ISurfaceAllocator::DeallocGrallocBuffer(MaybeMagicGrallocBufferHandle* aHandle)
{
  SharedBufferManagerChild::GetSingleton()->DeallocGrallocBuffer(*aHandle);
}

void
ISurfaceAllocator::DropGrallocBuffer(MaybeMagicGrallocBufferHandle* aHandle)
{
  SharedBufferManagerChild::GetSingleton()->DropGrallocBuffer(*aHandle);
}

} 
} 
