






#include "ISurfaceAllocator.h"
#include <sys/types.h>                  
#include "gfx2DGlue.h"                  
#include "gfxASurface.h"                
#include "gfxPlatform.h"                
#include "gfxSharedImageSurface.h"      
#include "mozilla/Assertions.h"         
#include "mozilla/Atomics.h"            
#include "mozilla/ipc/SharedMemory.h"   
#include "mozilla/layers/LayersSurfaces.h"  
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

NS_IMPL_ISUPPORTS1(GfxMemoryImageReporter, nsIMemoryReporter)

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
  MOZ_ASSERT(aDescriptor.type() == SurfaceDescriptor::TShmem ||
             aDescriptor.type() == SurfaceDescriptor::TMemoryImage);
  if (aDescriptor.type() == SurfaceDescriptor::TShmem) {
    Shmem shmem(aDescriptor.get_Shmem());
    aSize = shmem.Size<uint8_t>();
    return shmem.get<uint8_t>();
  } else {
    const MemoryImage& image = aDescriptor.get_MemoryImage();
    aSize = std::numeric_limits<size_t>::max();
    return reinterpret_cast<uint8_t*>(image.data());
  }
}

TemporaryRef<gfx::DrawTarget>
GetDrawTargetForDescriptor(const SurfaceDescriptor& aDescriptor, gfx::BackendType aBackend)
{
  size_t size;
  uint8_t* data = GetAddressFromDescriptor(aDescriptor, size);
  ImageDataDeserializer image(data, size);
  return image.GetAsDrawTarget(aBackend);
}

TemporaryRef<gfx::DataSourceSurface>
GetSurfaceForDescriptor(const SurfaceDescriptor& aDescriptor)
{
  size_t size;
  uint8_t* data = GetAddressFromDescriptor(aDescriptor, size);
  ImageDataDeserializer image(data, size);
  return image.GetAsSurface();
}

bool
ISurfaceAllocator::AllocSharedImageSurface(const gfx::IntSize& aSize,
                               gfxContentType aContent,
                               gfxSharedImageSurface** aBuffer)
{
  mozilla::ipc::SharedMemory::SharedMemoryType shmemType = OptimalShmemType();
  gfxImageFormat format = gfxPlatform::GetPlatform()->OptimalFormatForContent(aContent);

  nsRefPtr<gfxSharedImageSurface> back =
    gfxSharedImageSurface::CreateUnsafe(this,
                                        gfx::ThebesIntSize(aSize),
                                        format,
                                        shmemType);
  if (!back)
    return false;

  *aBuffer = nullptr;
  back.swap(*aBuffer);
  return true;
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
  bool tryPlatformSurface = true;
#ifdef DEBUG
  tryPlatformSurface = !PR_GetEnv("MOZ_LAYERS_FORCE_SHMEM_SURFACES");
#endif
  if (tryPlatformSurface &&
      PlatformAllocSurfaceDescriptor(aSize, aContent, aCaps, aBuffer)) {
    return true;
  }

  if (XRE_GetProcessType() == GeckoProcessType_Default) {
    gfxImageFormat format =
      gfxPlatform::GetPlatform()->OptimalFormatForContent(aContent);
    int32_t stride = gfxASurface::FormatStrideForWidth(format, aSize.width);
    uint8_t *data = new (std::nothrow) uint8_t[stride * aSize.height];
    if (!data) {
      return false;
    }
    GfxMemoryImageReporter::DidAlloc(data);
#ifdef XP_MACOSX
    
    
    if (format == gfxImageFormat::A8) {
      memset(data, 0, stride * aSize.height);
    }
#endif
    *aBuffer = MemoryImage((uintptr_t)data, aSize, stride, format);
    return true;
  }

  nsRefPtr<gfxSharedImageSurface> buffer;
  if (!AllocSharedImageSurface(aSize, aContent,
                               getter_AddRefs(buffer))) {
    return false;
  }

  *aBuffer = buffer->GetShmem();
  return true;
}

 bool
ISurfaceAllocator::IsShmem(SurfaceDescriptor* aSurface)
{
  return aSurface && (aSurface->type() == SurfaceDescriptor::TShmem ||
                      aSurface->type() == SurfaceDescriptor::TYCbCrImage ||
                      aSurface->type() == SurfaceDescriptor::TRGBImage);
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
  if (PlatformDestroySharedSurface(aSurface)) {
    return;
  }
  switch (aSurface->type()) {
    case SurfaceDescriptor::TShmem:
      DeallocShmem(aSurface->get_Shmem());
      break;
    case SurfaceDescriptor::TYCbCrImage:
      DeallocShmem(aSurface->get_YCbCrImage().data());
      break;
    case SurfaceDescriptor::TRGBImage:
      DeallocShmem(aSurface->get_RGBImage().data());
      break;
    case SurfaceDescriptor::TSurfaceDescriptorD3D9:
    case SurfaceDescriptor::TSurfaceDescriptorDIB:
    case SurfaceDescriptor::TSurfaceDescriptorD3D10:
      break;
    case SurfaceDescriptor::TMemoryImage:
      GfxMemoryImageReporter::WillFree((uint8_t*)aSurface->get_MemoryImage().data());
      delete [] (uint8_t*)aSurface->get_MemoryImage().data();
      break;
    case SurfaceDescriptor::Tnull_t:
    case SurfaceDescriptor::T__None:
      break;
    default:
      NS_RUNTIMEABORT("surface type not implemented!");
  }
  *aSurface = SurfaceDescriptor();
}

#if !defined(MOZ_HAVE_PLATFORM_SPECIFIC_LAYER_BUFFERS)
bool
ISurfaceAllocator::PlatformAllocSurfaceDescriptor(const gfx::IntSize&,
                                                  gfxContentType,
                                                  uint32_t,
                                                  SurfaceDescriptor*)
{
  return false;
}
#endif



const uint32_t sShmemPageSize = 4096;
const uint32_t sSupportedBlockSize = 4;

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
  for (size_t i = 0; i < mUsedShmems.size(); i++) {
    ShmemSectionHeapHeader* header = mUsedShmems[i].get<ShmemSectionHeapHeader>();
    if (header->mAllocatedBlocks == 0) {
      DeallocShmem(mUsedShmems[i]);

      
      
      mUsedShmems[i] = mUsedShmems[mUsedShmems.size() - 1];
      mUsedShmems.pop_back();
      i--;
      break;
    }
  }
}

} 
} 
