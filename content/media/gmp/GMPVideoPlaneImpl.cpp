




#include "GMPVideoPlaneImpl.h"
#include "mozilla/gmp/GMPTypes.h"
#include "GMPVideoHost.h"
#include "GMPSharedMemManager.h"

namespace mozilla {
namespace gmp {

GMPPlaneImpl::GMPPlaneImpl(GMPVideoHostImpl* aHost)
: mSize(0),
  mStride(0),
  mHost(aHost)
{
  MOZ_ASSERT(mHost);
  mHost->PlaneCreated(this);
}

GMPPlaneImpl::GMPPlaneImpl(const GMPPlaneData& aPlaneData, GMPVideoHostImpl* aHost)
: mBuffer(aPlaneData.mBuffer()),
  mSize(aPlaneData.mSize()),
  mStride(aPlaneData.mStride()),
  mHost(aHost)
{
  MOZ_ASSERT(mHost);
  mHost->PlaneCreated(this);
}

GMPPlaneImpl::~GMPPlaneImpl()
{
  DestroyBuffer();
  if (mHost) {
    mHost->PlaneDestroyed(this);
  }
}

void
GMPPlaneImpl::DoneWithAPI()
{
  DestroyBuffer();

  
  
  mHost = nullptr;
}

void
GMPPlaneImpl::ActorDestroyed()
{
  
  
  mBuffer = ipc::Shmem();
  
  mHost = nullptr;
}

bool
GMPPlaneImpl::InitPlaneData(GMPPlaneData& aPlaneData)
{
  aPlaneData.mBuffer() = mBuffer;
  aPlaneData.mSize() = mSize;
  aPlaneData.mStride() = mStride;

  
  
  
  mBuffer = ipc::Shmem();

  return true;
}

GMPErr
GMPPlaneImpl::MaybeResize(int32_t aNewSize) {
  if (aNewSize <= AllocatedSize()) {
    return GMPNoErr;
  }

  if (!mHost) {
    return GMPGenericErr;
  }

  ipc::Shmem new_mem;
  if (!mHost->SharedMemMgr()->MgrAllocShmem(GMPSharedMem::kGMPFrameData, aNewSize,
                                            ipc::SharedMemory::TYPE_BASIC, &new_mem) ||
      !new_mem.get<uint8_t>()) {
    return GMPAllocErr;
  }

  if (mBuffer.IsReadable()) {
    memcpy(new_mem.get<uint8_t>(), Buffer(), mSize);
  }

  DestroyBuffer();

  mBuffer = new_mem;

  return GMPNoErr;
}

void
GMPPlaneImpl::DestroyBuffer()
{
  if (mHost && mBuffer.IsWritable()) {
    mHost->SharedMemMgr()->MgrDeallocShmem(GMPSharedMem::kGMPFrameData, mBuffer);
  }
  mBuffer = ipc::Shmem();
}

GMPErr
GMPPlaneImpl::CreateEmptyPlane(int32_t aAllocatedSize, int32_t aStride, int32_t aPlaneSize)
{
  if (aAllocatedSize < 1 || aStride < 1 || aPlaneSize < 1) {
    return GMPGenericErr;
  }

  GMPErr err = MaybeResize(aAllocatedSize);
  if (err != GMPNoErr) {
    return err;
  }

  mSize = aPlaneSize;
  mStride = aStride;

  return GMPNoErr;
}

GMPErr
GMPPlaneImpl::Copy(const GMPPlane& aPlane)
{
  auto& planeimpl = static_cast<const GMPPlaneImpl&>(aPlane);

  GMPErr err = MaybeResize(planeimpl.mSize);
  if (err != GMPNoErr) {
    return err;
  }

  if (planeimpl.Buffer() && planeimpl.mSize > 0) {
    memcpy(Buffer(), planeimpl.Buffer(), mSize);
  }

  mSize = planeimpl.mSize;
  mStride = planeimpl.mStride;

  return GMPNoErr;
}

GMPErr
GMPPlaneImpl::Copy(int32_t aSize, int32_t aStride, const uint8_t* aBuffer)
{
  GMPErr err = MaybeResize(aSize);
  if (err != GMPNoErr) {
    return err;
  }

  if (aBuffer && aSize > 0) {
    memcpy(Buffer(), aBuffer, aSize);
  }

  mSize = aSize;
  mStride = aStride;

  return GMPNoErr;
}

void
GMPPlaneImpl::Swap(GMPPlane& aPlane)
{
  auto& planeimpl = static_cast<GMPPlaneImpl&>(aPlane);

  std::swap(mStride, planeimpl.mStride);
  std::swap(mSize, planeimpl.mSize);
  std::swap(mBuffer, planeimpl.mBuffer);
}

int32_t
GMPPlaneImpl::AllocatedSize() const
{
  if (mBuffer.IsWritable()) {
    return mBuffer.Size<uint8_t>();
  }
  return 0;
}

void
GMPPlaneImpl::ResetSize()
{
  mSize = 0;
}

bool
GMPPlaneImpl::IsZeroSize() const
{
  return (mSize == 0);
}

int32_t
GMPPlaneImpl::Stride() const
{
  return mStride;
}

const uint8_t*
GMPPlaneImpl::Buffer() const
{
  return mBuffer.get<uint8_t>();
}

uint8_t*
GMPPlaneImpl::Buffer()
{
  return mBuffer.get<uint8_t>();
}

void
GMPPlaneImpl::Destroy()
{
  delete this;
}

} 
} 
