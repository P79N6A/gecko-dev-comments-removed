




#ifndef MOZILLA_GFX_GRALLOCTEXTURECLIENT_H
#define MOZILLA_GFX_GRALLOCTEXTURECLIENT_H
#ifdef MOZ_WIDGET_GONK

#include "mozilla/layers/TextureClient.h"
#include "ISurfaceAllocator.h" 
#include "mozilla/layers/FenceUtils.h" 
#include "mozilla/layers/ShadowLayerUtilsGralloc.h"
#include <ui/GraphicBuffer.h>

namespace mozilla {
namespace layers {

class GraphicBufferLocked;














class GrallocTextureClientOGL : public BufferTextureClient
{
public:
  GrallocTextureClientOGL(GrallocBufferActor* aActor,
                          gfx::IntSize aSize,
                          TextureFlags aFlags = TEXTURE_FLAGS_DEFAULT);
  GrallocTextureClientOGL(ISurfaceAllocator* aAllocator,
                          gfx::SurfaceFormat aFormat,
                          TextureFlags aFlags = TEXTURE_FLAGS_DEFAULT);

  ~GrallocTextureClientOGL();

  virtual bool Lock(OpenMode aMode) MOZ_OVERRIDE;

  virtual void Unlock() MOZ_OVERRIDE;

  virtual bool ImplementsLocking() const MOZ_OVERRIDE { return true; }

  virtual bool HasInternalBuffer() const MOZ_OVERRIDE { return false; }

  virtual bool IsAllocated() const MOZ_OVERRIDE;

  virtual bool ToSurfaceDescriptor(SurfaceDescriptor& aOutDescriptor) MOZ_OVERRIDE;

  virtual bool UpdateSurface(gfxASurface* aSurface) MOZ_OVERRIDE;

  virtual TextureClientData* DropTextureData() MOZ_OVERRIDE;

  virtual void SetReleaseFenceHandle(FenceHandle aReleaseFenceHandle) MOZ_OVERRIDE;

  const FenceHandle& GetReleaseFenceHandle() const
  {
    return mReleaseFenceHandle;
  }

  void InitWith(GrallocBufferActor* aActor, gfx::IntSize aSize);

  void SetTextureFlags(TextureFlags aFlags) { AddFlags(aFlags); }

  gfx::IntSize GetSize() const MOZ_OVERRIDE { return mSize; }

  android::GraphicBuffer* GetGraphicBuffer()
  {
    return mGraphicBuffer.get();
  }

  android::PixelFormat GetPixelFormat()
  {
    return mGraphicBuffer->getPixelFormat();
  }

  virtual uint8_t* GetBuffer() const MOZ_OVERRIDE;

  virtual TemporaryRef<gfx::DrawTarget> GetAsDrawTarget() MOZ_OVERRIDE;

  virtual already_AddRefed<gfxASurface> GetAsSurface() MOZ_OVERRIDE;

  virtual bool AllocateForSurface(gfx::IntSize aSize,
                                  TextureAllocationFlags aFlags = ALLOC_DEFAULT) MOZ_OVERRIDE;

  virtual bool AllocateForYCbCr(gfx::IntSize aYSize,
                                gfx::IntSize aCbCrSize,
                                StereoMode aStereoMode) MOZ_OVERRIDE;

  bool AllocateForGLRendering(gfx::IntSize aSize);

  bool AllocateGralloc(gfx::IntSize aYSize, uint32_t aAndroidFormat, uint32_t aUsage);

  virtual bool Allocate(uint32_t aSize) MOZ_OVERRIDE;

  virtual size_t GetBufferSize() const MOZ_OVERRIDE;

  void SetGraphicBufferLocked(GraphicBufferLocked* aBufferLocked);

protected:
  


  GrallocBufferActor* mGrallocActor;

  RefPtr<GraphicBufferLocked> mBufferLocked;

  android::sp<android::GraphicBuffer> mGraphicBuffer;

  



  uint8_t* mMappedBuffer;

  RefPtr<gfx::DrawTarget> mDrawTarget;

  




  gfx::IntSize mSize;
};

} 
} 

#endif 
#endif
