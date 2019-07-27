





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
#include "mozilla/gfx/Rect.h"

namespace mozilla {
namespace layers {

class CompositableClient;
class AsyncTransactionTracker;
struct TextureFactoryIdentifier;
class SurfaceDescriptor;
class SurfaceDescriptorTiles;
class ThebesBufferData;
class PTextureChild;











class CompositableForwarder : public ISurfaceAllocator
{
public:

  CompositableForwarder()
    : mSerial(++sSerialCounter)
  {}

  



  virtual void Connect(CompositableClient* aCompositable) = 0;

  



  virtual void UseTiledLayerBuffer(CompositableClient* aCompositable,
                                   const SurfaceDescriptorTiles& aTiledDescriptor) = 0;

  


  virtual PTextureChild* CreateTexture(const SurfaceDescriptor& aSharedData, TextureFlags aFlags) = 0;

  



  virtual void UpdateTextureRegion(CompositableClient* aCompositable,
                                   const ThebesBufferData& aThebesBufferData,
                                   const nsIntRegion& aUpdatedRegion) = 0;

  


  virtual void UpdatePictureRect(CompositableClient* aCompositable,
                                 const gfx::IntRect& aRect) = 0;

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

  



  virtual void UseTexture(CompositableClient* aCompositable,
                          TextureClient* aClient) = 0;
  virtual void UseComponentAlphaTextures(CompositableClient* aCompositable,
                                         TextureClient* aClientOnBlack,
                                         TextureClient* aClientOnWhite) = 0;

  virtual void SendPendingAsyncMessges() = 0;

  void IdentifyTextureHost(const TextureFactoryIdentifier& aIdentifier);

  virtual int32_t GetMaxTextureSize() const override
  {
    return mTextureFactoryIdentifier.mMaxTextureSize;
  }

  bool IsOnCompositorSide() const override { return false; }

  




  virtual LayersBackend GetCompositorBackendType() const override
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

  SyncObject* GetSyncObject() { return mSyncObject; }

protected:
  TextureFactoryIdentifier mTextureFactoryIdentifier;
  nsTArray<RefPtr<TextureClient> > mTexturesToRemove;
  RefPtr<SyncObject> mSyncObject;
  const int32_t mSerial;
  static mozilla::Atomic<int32_t> sSerialCounter;
};

} 
} 

#endif
