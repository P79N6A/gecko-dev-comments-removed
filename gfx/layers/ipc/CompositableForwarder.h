





#ifndef MOZILLA_LAYERS_COMPOSITABLEFORWARDER
#define MOZILLA_LAYERS_COMPOSITABLEFORWARDER

#include "mozilla/StandardInteger.h"
#include "gfxASurface.h"
#include "GLDefs.h"
#include "mozilla/layers/ISurfaceAllocator.h"

namespace mozilla {
namespace layers {

class CompositableClient;
class TextureFactoryIdentifier;
class SurfaceDescriptor;
class ThebesBufferData;
class TextureClient;
class BasicTiledLayerBuffer;











class CompositableForwarder : public ISurfaceAllocator
{
  friend class AutoOpenSurface;
  friend class TextureClientShmem;
public:
  CompositableForwarder()
  : mMaxTextureSize(0), mCompositorBackend(layers::LAYERS_NONE)
  {}

  



  virtual void Connect(CompositableClient* aCompositable) = 0;

  








  virtual void CreatedSingleBuffer(CompositableClient* aCompositable,
                                   const SurfaceDescriptor& aDescriptor,
                                   const TextureInfo& aTextureInfo) = 0;
  virtual void CreatedDoubleBuffer(CompositableClient* aCompositable,
                                   const SurfaceDescriptor& aFrontDescriptor,
                                   const SurfaceDescriptor& aBackDescriptor,
                                   const TextureInfo& aTextureInfo) = 0;

  



  virtual void DestroyThebesBuffer(CompositableClient* aCompositable) = 0;

  virtual void PaintedTiledLayerBuffer(CompositableClient* aCompositable,
                                       BasicTiledLayerBuffer* aTiledLayerBuffer) = 0;

  



  virtual void UpdateTexture(CompositableClient* aCompositable,
                             TextureIdentifier aTextureId,
                             SurfaceDescriptor* aDescriptor) = 0;

  



  virtual void UpdateTextureRegion(CompositableClient* aCompositable,
                                   const ThebesBufferData& aThebesBufferData,
                                   const nsIntRegion& aUpdatedRegion) = 0;

  


  virtual void UpdatePictureRect(CompositableClient* aCompositable,
                                 const nsIntRect& aRect) = 0;

  







  virtual void DestroyedThebesBuffer(const SurfaceDescriptor& aBackBufferToDestroy) = 0;

  void IdentifyTextureHost(const TextureFactoryIdentifier& aIdentifier);

  


  virtual int32_t GetMaxTextureSize() const { return mMaxTextureSize; }

  bool IsOnCompositorSide() const MOZ_OVERRIDE { return false; }

  




  LayersBackend GetCompositorBackendType() const
  {
    return mCompositorBackend;
  }

protected:
  uint32_t mMaxTextureSize;
  LayersBackend mCompositorBackend;
};

} 
} 

#endif
