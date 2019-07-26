



#include "gfxReusableSurfaceWrapper.h"
#include "gfxSharedImageSurface.h"

using mozilla::ipc::Shmem;
using mozilla::layers::ISurfaceAllocator;

gfxReusableSurfaceWrapper::gfxReusableSurfaceWrapper(ISurfaceAllocator* aAllocator,
                                                     gfxSharedImageSurface* aSurface)
  : mAllocator(aAllocator)
  , mSurface(aSurface)
{
  MOZ_COUNT_CTOR(gfxReusableSurfaceWrapper);
  ReadLock();
}

gfxReusableSurfaceWrapper::~gfxReusableSurfaceWrapper()
{
  MOZ_COUNT_DTOR(gfxReusableSurfaceWrapper);
  ReadUnlock();
}

void
gfxReusableSurfaceWrapper::ReadLock()
{
  NS_CheckThreadSafe(_mOwningThread.GetThread(), "Only the owner thread can call ReadLock");
  mSurface->ReadLock();
}

void
gfxReusableSurfaceWrapper::ReadUnlock()
{
  int32_t readCount = mSurface->ReadUnlock();
  NS_ABORT_IF_FALSE(readCount >= 0, "Read count should not be negative");

  if (readCount == 0) {
    mAllocator->DeallocShmem(mSurface->GetShmem());
  }
}

gfxReusableSurfaceWrapper*
gfxReusableSurfaceWrapper::GetWritable(gfxImageSurface** aSurface)
{
  NS_CheckThreadSafe(_mOwningThread.GetThread(), "Only the owner thread can call GetWritable");

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

  
  gfxReusableSurfaceWrapper* wrapper = new gfxReusableSurfaceWrapper(mAllocator, copySurface);

  
  

  return wrapper;
}

const unsigned char*
gfxReusableSurfaceWrapper::GetReadOnlyData() const
{
  NS_ABORT_IF_FALSE(mSurface->GetReadCount() > 0, "Should have read lock");
  return mSurface->Data();
}

gfxASurface::gfxImageFormat
gfxReusableSurfaceWrapper::Format()
{
  return mSurface->Format();
}

Shmem&
gfxReusableSurfaceWrapper::GetShmem()
{
  return mSurface->GetShmem();
}

 already_AddRefed<gfxReusableSurfaceWrapper>
gfxReusableSurfaceWrapper::Open(ISurfaceAllocator* aAllocator, const Shmem& aShmem)
{
  nsRefPtr<gfxSharedImageSurface> sharedImage = gfxSharedImageSurface::Open(aShmem);
  nsRefPtr<gfxReusableSurfaceWrapper> wrapper = new gfxReusableSurfaceWrapper(aAllocator, sharedImage);
  wrapper->ReadUnlock();
  return wrapper.forget();
}
