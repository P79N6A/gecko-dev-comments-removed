




#ifndef MOZILLA_GFX_GRALLOCTEXTURECLIENT_H
#define MOZILLA_GFX_GRALLOCTEXTURECLIENT_H
#ifdef MOZ_WIDGET_GONK

#include "mozilla/layers/TextureClient.h"
#include "ISurfaceAllocator.h" 
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
  GrallocTextureClientOGL(CompositableClient* aCompositable,
                          gfx::SurfaceFormat aFormat,
                          TextureFlags aFlags = TEXTURE_FLAGS_DEFAULT);

  ~GrallocTextureClientOGL();

  virtual bool Lock(OpenMode aMode) MOZ_OVERRIDE;

  virtual void Unlock() MOZ_OVERRIDE;

  virtual bool ImplementsLocking() const MOZ_OVERRIDE { return true; }

  virtual bool IsAllocated() const MOZ_OVERRIDE;

  virtual bool ToSurfaceDescriptor(SurfaceDescriptor& aOutDescriptor) MOZ_OVERRIDE;

  virtual TextureClientData* DropTextureData() MOZ_OVERRIDE;

  void InitWith(GrallocBufferActor* aActor, gfx::IntSize aSize);

  gfx::IntSize GetSize() const MOZ_OVERRIDE { return mSize; }

  android::GraphicBuffer* GetGraphicBuffer()
  {
    return mGraphicBuffer.get();
  }

  android::PixelFormat GetPixelFormat()
  {
    return mGraphicBuffer->getPixelFormat();
  }

  




  void SetGrallocOpenFlags(uint32_t aFlags)
  {
    mGrallocFlags = aFlags;
  }

  virtual uint8_t* GetBuffer() const MOZ_OVERRIDE;

  virtual bool AllocateForSurface(gfx::IntSize aSize) MOZ_OVERRIDE;

  virtual bool AllocateForYCbCr(LayerIntSize aYSize,
                                LayerIntSize aCbCrSize,
                                StereoMode aStereoMode) MOZ_OVERRIDE;

  bool AllocateGralloc(gfx::IntSize aYSize, uint32_t aAndroidFormat, uint32_t aUsage);

  virtual bool Allocate(uint32_t aSize) MOZ_OVERRIDE;

  virtual size_t GetBufferSize() const MOZ_OVERRIDE;

  void SetGraphicBufferLocked(GraphicBufferLocked* aBufferLocked);

protected:

  


  GrallocBufferActor* mGrallocActor;

  RefPtr<GraphicBufferLocked> mBufferLocked;

  android::sp<android::GraphicBuffer> mGraphicBuffer;

  


  uint32_t mGrallocFlags;
  



  uint8_t* mMappedBuffer;
  




  gfx::IntSize mSize;
};

} 
} 

#endif 
#endif
