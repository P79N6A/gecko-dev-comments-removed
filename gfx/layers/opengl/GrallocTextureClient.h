




#ifndef MOZILLA_GFX_GRALLOCTEXTURECLIENT_H
#define MOZILLA_GFX_GRALLOCTEXTURECLIENT_H
#ifdef MOZ_WIDGET_GONK

#include "mozilla/layers/TextureClient.h"
#include "mozilla/layers/ISurfaceAllocator.h" 
#include "mozilla/layers/FenceUtils.h" 
#include "mozilla/layers/ShadowLayerUtilsGralloc.h"
#include <ui/GraphicBuffer.h>


namespace android {
class MediaBuffer;
};

namespace mozilla {
namespace layers {














class GrallocTextureClientOGL : public BufferTextureClient
{
public:
  GrallocTextureClientOGL(ISurfaceAllocator* aAllocator,
                          gfx::SurfaceFormat aFormat,
                          gfx::BackendType aMoz2dBackend,
                          TextureFlags aFlags = TextureFlags::DEFAULT);

  ~GrallocTextureClientOGL();

  virtual bool Lock(OpenMode aMode) MOZ_OVERRIDE;

  virtual void Unlock() MOZ_OVERRIDE;

  virtual bool ImplementsLocking() const MOZ_OVERRIDE { return true; }

  virtual bool HasInternalBuffer() const MOZ_OVERRIDE { return false; }

  virtual bool IsAllocated() const MOZ_OVERRIDE;

  virtual bool ToSurfaceDescriptor(SurfaceDescriptor& aOutDescriptor) MOZ_OVERRIDE;

  virtual void SetRemoveFromCompositableTracker(AsyncTransactionTracker* aTracker) MOZ_OVERRIDE;

  virtual void WaitForBufferOwnership(bool aWaitReleaseFence = true) MOZ_OVERRIDE;

  void InitWith(MaybeMagicGrallocBufferHandle aDesc, gfx::IntSize aSize);

  void SetTextureFlags(TextureFlags aFlags) { AddFlags(aFlags); }

  gfx::IntSize GetSize() const MOZ_OVERRIDE { return mSize; }

  android::sp<android::GraphicBuffer> GetGraphicBuffer()
  {
    return mGraphicBuffer;
  }

  android::PixelFormat GetPixelFormat()
  {
    return mGraphicBuffer->getPixelFormat();
  }

  virtual uint8_t* GetBuffer() const MOZ_OVERRIDE;

  virtual gfx::DrawTarget* BorrowDrawTarget() MOZ_OVERRIDE;

  virtual bool AllocateForSurface(gfx::IntSize aSize,
                                  TextureAllocationFlags aFlags = ALLOC_DEFAULT) MOZ_OVERRIDE;

  virtual bool AllocateForYCbCr(gfx::IntSize aYSize,
                                gfx::IntSize aCbCrSize,
                                StereoMode aStereoMode) MOZ_OVERRIDE;

  bool AllocateForGLRendering(gfx::IntSize aSize);

  bool AllocateGralloc(gfx::IntSize aYSize, uint32_t aAndroidFormat, uint32_t aUsage);

  void SetIsOpaque(bool aIsOpaque) { mIsOpaque = aIsOpaque; }

  virtual bool Allocate(uint32_t aSize) MOZ_OVERRIDE;

  virtual size_t GetBufferSize() const MOZ_OVERRIDE;

  




  void SetMediaBuffer(android::MediaBuffer* aMediaBuffer)
  {
    mMediaBuffer = aMediaBuffer;
  }

  android::MediaBuffer* GetMediaBuffer()
  {
    return mMediaBuffer;
  }

  virtual TemporaryRef<TextureClient>
  CreateSimilar(TextureFlags aFlags = TextureFlags::DEFAULT,
                TextureAllocationFlags aAllocFlags = ALLOC_DEFAULT) const MOZ_OVERRIDE;

  static TemporaryRef<TextureClient> FromSharedSurface(gl::SharedSurface* surf,
                                                       TextureFlags flags);

protected:
  


  MaybeMagicGrallocBufferHandle mGrallocHandle;

  RefPtr<AsyncTransactionTracker> mRemoveFromCompositableTracker;

  android::sp<android::GraphicBuffer> mGraphicBuffer;

  



  uint8_t* mMappedBuffer;

  RefPtr<gfx::DrawTarget> mDrawTarget;

  




  gfx::IntSize mSize;

  android::MediaBuffer* mMediaBuffer;

  bool mIsOpaque;
};

} 
} 

#endif 
#endif
