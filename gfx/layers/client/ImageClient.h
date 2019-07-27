




#ifndef MOZILLA_GFX_IMAGECLIENT_H
#define MOZILLA_GFX_IMAGECLIENT_H

#include <stdint.h>                     
#include <sys/types.h>                  
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/Types.h"          
#include "mozilla/layers/AsyncTransactionTracker.h" 
#include "mozilla/layers/CompositableClient.h"  
#include "mozilla/layers/CompositorTypes.h"  
#include "mozilla/layers/LayersSurfaces.h"  
#include "mozilla/layers/TextureClient.h"  
#include "mozilla/mozalloc.h"           
#include "nsCOMPtr.h"                   
#include "nsRect.h"                     

namespace mozilla {
namespace layers {

class CompositableForwarder;
class AsyncTransactionTracker;
class Image;
class ImageContainer;
class ShadowableLayer;






class ImageClient : public CompositableClient
{
public:
  




  static TemporaryRef<ImageClient> CreateImageClient(CompositableType aImageHostType,
                                                     CompositableForwarder* aFwd,
                                                     TextureFlags aFlags);

  virtual ~ImageClient() {}

  




  virtual bool UpdateImage(ImageContainer* aContainer, uint32_t aContentFlags) = 0;

  



  virtual void UpdatePictureRect(gfx::IntRect aPictureRect);

  virtual already_AddRefed<Image> CreateImage(ImageFormat aFormat) = 0;

  


  virtual TemporaryRef<AsyncTransactionTracker> PrepareFlushAllImages() { return nullptr; }

  



  virtual void FlushAllImages(bool aExceptFront,
                              AsyncTransactionTracker* aAsyncTransactionTracker) {}

  virtual void RemoveTexture(TextureClient* aTexture) override;

  void RemoveTextureWithTracker(TextureClient* aTexture,
                                AsyncTransactionTracker* aAsyncTransactionTracker = nullptr);

protected:
  ImageClient(CompositableForwarder* aFwd, TextureFlags aFlags,
              CompositableType aType);

  CompositableType mType;
  int32_t mLastPaintedImageSerial;
  gfx::IntRect mPictureRect;
};




class ImageClientSingle : public ImageClient
{
public:
  ImageClientSingle(CompositableForwarder* aFwd,
                    TextureFlags aFlags,
                    CompositableType aType);

  virtual bool UpdateImage(ImageContainer* aContainer, uint32_t aContentFlags) override;

  virtual void OnDetach() override;

  virtual bool AddTextureClient(TextureClient* aTexture) override;

  virtual TextureInfo GetTextureInfo() const override;

  virtual already_AddRefed<Image> CreateImage(ImageFormat aFormat) override;

  virtual TemporaryRef<AsyncTransactionTracker> PrepareFlushAllImages() override;

  virtual void FlushAllImages(bool aExceptFront,
                              AsyncTransactionTracker* aAsyncTransactionTracker) override;

protected:
  RefPtr<TextureClient> mFrontBuffer;
};






class ImageClientBridge : public ImageClient
{
public:
  ImageClientBridge(CompositableForwarder* aFwd,
                    TextureFlags aFlags);

  virtual bool UpdateImage(ImageContainer* aContainer, uint32_t aContentFlags) override;
  virtual bool Connect() override { return false; }
  virtual void Updated() {}
  void SetLayer(ShadowableLayer* aLayer)
  {
    mLayer = aLayer;
  }

  virtual TextureInfo GetTextureInfo() const override
  {
    return TextureInfo(mType);
  }

  virtual void SetIPDLActor(CompositableChild* aChild) override
  {
    MOZ_ASSERT(!aChild, "ImageClientBridge should not have IPDL actor");
  }

  virtual already_AddRefed<Image> CreateImage(ImageFormat aFormat) override
  {
    NS_WARNING("Should not create an image through an ImageClientBridge");
    return nullptr;
  }

protected:
  uint64_t mAsyncContainerID;
  ShadowableLayer* mLayer;
};

#ifdef MOZ_WIDGET_GONK







class ImageClientOverlay : public ImageClient
{
public:
  ImageClientOverlay(CompositableForwarder* aFwd,
                     TextureFlags aFlags);

  virtual bool UpdateImage(ImageContainer* aContainer, uint32_t aContentFlags);
  virtual already_AddRefed<Image> CreateImage(ImageFormat aFormat);
  TextureInfo GetTextureInfo() const override
  {
    return TextureInfo(CompositableType::IMAGE_OVERLAY);
  }
};
#endif

}
}

#endif
