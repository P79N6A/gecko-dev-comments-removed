



#include "gfxReusableSharedImageSurfaceWrapper.h"
#include "gfxSharedImageSurface.h"

using mozilla::ipc::Shmem;
using mozilla::layers::ISurfaceAllocator;

gfxReusableSharedImageSurfaceWrapper::gfxReusableSharedImageSurfaceWrapper(ISurfaceAllocator* aAllocator,
                                                                           gfxSharedImageSurface* aSurface)
  : mAllocator(aAllocator)
  , mSurface(aSurface)
{
  MOZ_COUNT_CTOR(gfxReusableSharedImageSurfaceWrapper);
  ReadLock();
}

gfxReusableSharedImageSurfaceWrapper::~gfxReusableSharedImageSurfaceWrapper()
{
  MOZ_COUNT_DTOR(gfxReusableSharedImageSurfaceWrapper);
  ReadUnlock();
}

void
gfxReusableSharedImageSurfaceWrapper::ReadLock()
{
  NS_ASSERT_OWNINGTHREAD(gfxReusableSharedImageSurfaceWrapper);
  mSurface->ReadLock();
}

void
gfxReusableSharedImageSurfaceWrapper::ReadUnlock()
{
  int32_t readCount = mSurface->ReadUnlock();
  NS_ABORT_IF_FALSE(readCount >= 0, "Read count should not be negative");

  if (readCount == 0) {
    mAllocator->DeallocShmem(mSurface->GetShmem());
  }
}

gfxReusableSurfaceWrapper*
gfxReusableSharedImageSurfaceWrapper::GetWritable(gfxImageSurface** aSurface)
{
  NS_ASSERT_OWNINGTHREAD(gfxReusableSharedImageSurfaceWrapper);

  int32_t readCount = mSurface->GetReadCount();
  NS_ABORT_IF_FALSE(readCount > 0, "A ReadLock must be held when calling GetWritable");
  if (readCount == 1) {
    *aSurface = mSurface;
    return this;
  }

  
  nsRefPtr<gfxSharedImageSurface> copySurface =
    gfxSharedImageSurface::CreateUnsafe(mAllocator, mSurface->GetSize(), mSurface->Format());
  copySurface->CopyFrom(mSurface);
  *aSurface = copySurface;

  
  gfxReusableSurfaceWrapper* wrapper = new gfxReusableSharedImageSurfaceWrapper(mAllocator, copySurface);

  
  

  return wrapper;
}

const unsigned char*
gfxReusableSharedImageSurfaceWrapper::GetReadOnlyData() const
{
  NS_ABORT_IF_FALSE(mSurface->GetReadCount() > 0, "Should have read lock");
  return mSurface->Data();
}

gfxASurface::gfxImageFormat
gfxReusableSharedImageSurfaceWrapper::Format()
{
  return mSurface->Format();
}

Shmem&
gfxReusableSharedImageSurfaceWrapper::GetShmem()
{
  return mSurface->GetShmem();
}

 already_AddRefed<gfxReusableSharedImageSurfaceWrapper>
gfxReusableSharedImageSurfaceWrapper::Open(ISurfaceAllocator* aAllocator, const Shmem& aShmem)
{
  nsRefPtr<gfxSharedImageSurface> sharedImage = gfxSharedImageSurface::Open(aShmem);
  nsRefPtr<gfxReusableSharedImageSurfaceWrapper> wrapper = new gfxReusableSharedImageSurfaceWrapper(aAllocator, sharedImage);
  wrapper->ReadUnlock();
  return wrapper.forget();
}
