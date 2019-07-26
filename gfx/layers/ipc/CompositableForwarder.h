





#ifndef MOZILLA_LAYERS_COMPOSITABLEFORWARDER
#define MOZILLA_LAYERS_COMPOSITABLEFORWARDER

#include <stdint.h>                     
#include "gfxTypes.h"
#include "mozilla/Attributes.h"         
#include "mozilla/layers/CompositorTypes.h"
#include "mozilla/layers/ISurfaceAllocator.h"  
#include "mozilla/layers/LayersTypes.h"  
#include "nsRegion.h"                   

struct nsIntPoint;
struct nsIntRect;

namespace mozilla {
namespace layers {

class CompositableClient;
class TextureFactoryIdentifier;
class SurfaceDescriptor;
class SurfaceDescriptorTiles;
class ThebesBufferData;
class DeprecatedTextureClient;
class TextureClient;
class BasicTiledLayerBuffer;











class CompositableForwarder : public ISurfaceAllocator
{
  friend class AutoOpenSurface;
  friend class DeprecatedTextureClientShmem;
public:

  CompositableForwarder()
    : mMultiProcess(false)
  {}

  



  virtual void Connect(CompositableClient* aCompositable) = 0;

  








  virtual void CreatedSingleBuffer(CompositableClient* aCompositable,
                                   const SurfaceDescriptor& aDescriptor,
                                   const TextureInfo& aTextureInfo,
                                   const SurfaceDescriptor* aDescriptorOnWhite = nullptr) = 0;
  virtual void CreatedDoubleBuffer(CompositableClient* aCompositable,
                                   const SurfaceDescriptor& aFrontDescriptor,
                                   const SurfaceDescriptor& aBackDescriptor,
                                   const TextureInfo& aTextureInfo,
                                   const SurfaceDescriptor* aFrontDescriptorOnWhite = nullptr,
                                   const SurfaceDescriptor* aBackDescriptorOnWhite = nullptr) = 0;

  



  virtual void CreatedIncrementalBuffer(CompositableClient* aCompositable,
                                        const TextureInfo& aTextureInfo,
                                        const nsIntRect& aBufferRect) = 0;

  



  virtual void DestroyThebesBuffer(CompositableClient* aCompositable) = 0;

  virtual void PaintedTiledLayerBuffer(CompositableClient* aCompositable,
                                       const SurfaceDescriptorTiles& aTiledDescriptor) = 0;

  



  virtual void UpdateTexture(CompositableClient* aCompositable,
                             TextureIdentifier aTextureId,
                             SurfaceDescriptor* aDescriptor) = 0;

  


  virtual void UpdateTextureNoSwap(CompositableClient* aCompositable,
                                   TextureIdentifier aTextureId,
                                   SurfaceDescriptor* aDescriptor) = 0;

  



  virtual void UpdateTextureRegion(CompositableClient* aCompositable,
                                   const ThebesBufferData& aThebesBufferData,
                                   const nsIntRegion& aUpdatedRegion) = 0;

  









  virtual void UpdateTextureIncremental(CompositableClient* aCompositable,
                                        TextureIdentifier aTextureId,
                                        SurfaceDescriptor& aDescriptor,
                                        const nsIntRegion& aUpdatedRegion,
                                        const nsIntRect& aBufferRect,
                                        const nsIntPoint& aBufferRotation) = 0;

  


  virtual void UpdatePictureRect(CompositableClient* aCompositable,
                                 const nsIntRect& aRect) = 0;

  







  virtual void DestroyedThebesBuffer(const SurfaceDescriptor& aBackBufferToDestroy) = 0;

  



  virtual bool AddTexture(CompositableClient* aCompositable,
                          TextureClient* aClient) = 0;

  









  virtual void RemoveTexture(CompositableClient* aCompositable,
                             uint64_t aTextureID,
                             TextureFlags aFlags) = 0;

  



  virtual void UseTexture(CompositableClient* aCompositable,
                          TextureClient* aClient) = 0;

  



  virtual void UpdatedTexture(CompositableClient* aCompositable,
                              TextureClient* aTexture,
                              nsIntRegion* aRegion) = 0;

  void IdentifyTextureHost(const TextureFactoryIdentifier& aIdentifier);

  


  virtual int32_t GetMaxTextureSize() const { return mTextureFactoryIdentifier.mMaxTextureSize; }

  bool IsOnCompositorSide() const MOZ_OVERRIDE { return false; }

  




  LayersBackend GetCompositorBackendType() const
  {
    return mTextureFactoryIdentifier.mParentBackend;
  }

  bool SupportsTextureBlitting() const
  {
    return mTextureFactoryIdentifier.mSupportsTextureBlitting;
  }

  bool SupportsPartialUploads() const
  {
    return mTextureFactoryIdentifier.mSupportsPartialUploads;
  }

  bool ForwardsToDifferentProcess() const
  {
    return mMultiProcess;
  }

protected:
  TextureFactoryIdentifier mTextureFactoryIdentifier;
  bool mMultiProcess;
};

} 
} 

#endif
