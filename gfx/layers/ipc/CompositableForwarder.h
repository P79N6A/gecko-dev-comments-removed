





#ifndef MOZILLA_LAYERS_COMPOSITABLEFORWARDER
#define MOZILLA_LAYERS_COMPOSITABLEFORWARDER

#include <stdint.h>                     
#include "gfxTypes.h"
#include "mozilla/Attributes.h"         
#include "mozilla/layers/CompositorTypes.h"
#include "mozilla/layers/ISurfaceAllocator.h"  
#include "mozilla/layers/LayersTypes.h"  
#include "mozilla/layers/TextureClient.h"  
#include "nsRegion.h"                   

struct nsIntPoint;
struct nsIntRect;

namespace mozilla {
namespace layers {

class CompositableClient;
class AsyncTransactionTracker;
struct TextureFactoryIdentifier;
class SurfaceDescriptor;
class SurfaceDescriptorTiles;
class ThebesBufferData;
class ClientTiledLayerBuffer;
class PTextureChild;











class CompositableForwarder : public ISurfaceAllocator
{
public:

  CompositableForwarder()
    : mSerial(++sSerialCounter)
  {}

  



  virtual void Connect(CompositableClient* aCompositable) = 0;

  



  virtual void CreatedIncrementalBuffer(CompositableClient* aCompositable,
                                        const TextureInfo& aTextureInfo,
                                        const nsIntRect& aBufferRect) = 0;

  



  virtual void UseTiledLayerBuffer(CompositableClient* aCompositable,
                                   const SurfaceDescriptorTiles& aTiledDescriptor) = 0;

  


  virtual PTextureChild* CreateTexture(const SurfaceDescriptor& aSharedData, TextureFlags aFlags) = 0;

  



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

#ifdef MOZ_WIDGET_GONK
  virtual void UseOverlaySource(CompositableClient* aCompositabl,
                                const OverlaySource& aOverlay) = 0;
#endif

  







  virtual void RemoveTextureFromCompositable(CompositableClient* aCompositable,
                                             TextureClient* aTexture) = 0;

  








  virtual void RemoveTextureFromCompositableAsync(AsyncTransactionTracker* aAsyncTransactionTracker,
                                                  CompositableClient* aCompositable,
                                                  TextureClient* aTexture) {}

  



  virtual void RemoveTexture(TextureClient* aTexture) = 0;

  



  virtual void HoldUntilTransaction(TextureClient* aClient)
  {
    if (aClient) {
      mTexturesToRemove.AppendElement(aClient);
    }
  }

  



  virtual void RemoveTexturesIfNecessary()
  {
    mTexturesToRemove.Clear();
  }

  virtual void HoldTransactionsToRespond(uint64_t aTransactionId)
  {
    mTransactionsToRespond.push_back(aTransactionId);
  }

  virtual void ClearTransactionsToRespond()
  {
    mTransactionsToRespond.clear();
  }

  



  virtual void UseTexture(CompositableClient* aCompositable,
                          TextureClient* aClient) = 0;
  virtual void UseComponentAlphaTextures(CompositableClient* aCompositable,
                                         TextureClient* aClientOnBlack,
                                         TextureClient* aClientOnWhite) = 0;

  



  virtual void UpdatedTexture(CompositableClient* aCompositable,
                              TextureClient* aTexture,
                              nsIntRegion* aRegion) = 0;


  virtual void SendFenceHandle(AsyncTransactionTracker* aTracker,
                               PTextureChild* aTexture,
                               const FenceHandle& aFence) = 0;

  void IdentifyTextureHost(const TextureFactoryIdentifier& aIdentifier);

  virtual int32_t GetMaxTextureSize() const MOZ_OVERRIDE
  {
    return mTextureFactoryIdentifier.mMaxTextureSize;
  }

  bool IsOnCompositorSide() const MOZ_OVERRIDE { return false; }

  




  virtual LayersBackend GetCompositorBackendType() const MOZ_OVERRIDE
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

  const TextureFactoryIdentifier& GetTextureFactoryIdentifier() const
  {
    return mTextureFactoryIdentifier;
  }

  int32_t GetSerial() { return mSerial; }

protected:
  TextureFactoryIdentifier mTextureFactoryIdentifier;
  nsTArray<RefPtr<TextureClient> > mTexturesToRemove;
  std::vector<uint64_t> mTransactionsToRespond;
  const int32_t mSerial;
  static mozilla::Atomic<int32_t> sSerialCounter;
};

} 
} 

#endif
