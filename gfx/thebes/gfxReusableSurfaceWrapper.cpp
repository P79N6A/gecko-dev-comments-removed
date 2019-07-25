



#include "gfxReusableSurfaceWrapper.h"
#include "gfxImageSurface.h"
#include "pratom.h"

gfxReusableSurfaceWrapper::gfxReusableSurfaceWrapper(gfxImageSurface* aSurface)
  : mSurface(aSurface)
  , mSurfaceData(aSurface->Data())
  , mFormat(aSurface->Format())
  , mReadCount(0)
{
  MOZ_COUNT_CTOR(gfxReusableSurfaceWrapper);
}

gfxReusableSurfaceWrapper::~gfxReusableSurfaceWrapper()
{
  NS_ABORT_IF_FALSE(mReadCount == 0, "Should not be locked when released");
  MOZ_COUNT_DTOR(gfxReusableSurfaceWrapper);
  if (!NS_IsMainThread()) {
    class DeleteImageOnMainThread : public nsRunnable {
    public:
      DeleteImageOnMainThread(gfxImageSurface *aImage)
        : mImage(aImage)
      {}

      NS_IMETHOD Run()
      {
        return NS_OK;
      }
    private:
      nsRefPtr<gfxImageSurface> mImage;
    };
    NS_DispatchToMainThread(new DeleteImageOnMainThread(mSurface));
  }
}

void
gfxReusableSurfaceWrapper::ReadLock()
{
  NS_CheckThreadSafe(_mOwningThread.GetThread(), "Only the owner thread can call ReadOnlyLock");
  PR_ATOMIC_INCREMENT(&mReadCount);
}

void
gfxReusableSurfaceWrapper::ReadUnlock()
{
  PR_ATOMIC_DECREMENT(&mReadCount);
}

gfxReusableSurfaceWrapper*
gfxReusableSurfaceWrapper::GetWritable(gfxImageSurface** aSurface)
{
  NS_CheckThreadSafe(_mOwningThread.GetThread(), "Only the owner thread can call GetWritable");

  if (mReadCount == 0) {
    *aSurface = mSurface;
    return this;
  }

  
  gfxImageSurface* copySurface = new gfxImageSurface(mSurface->GetSize(), mSurface->Format());
  copySurface->CopyFrom(mSurface);
  *aSurface = copySurface;

  
  gfxReusableSurfaceWrapper* wrapper = new gfxReusableSurfaceWrapper(copySurface);
  return wrapper;
}

