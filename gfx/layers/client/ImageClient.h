




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

  



  virtual void UpdatePictureRect(nsIntRect aPictureRect);

  virtual already_AddRefed<Image> CreateImage(ImageFormat aFormat) = 0;

  


  virtual TemporaryRef<AsyncTransactionTracker> PrepareFlushAllImages() { return nullptr; }

  



  virtual void FlushAllImages(bool aExceptFront,
                              AsyncTransactionTracker* aAsyncTransactionTracker) {}

  virtual void RemoveTexture(TextureClient* aTexture) MOZ_OVERRIDE;

  void RemoveTextureWithTracker(TextureClient* aTexture,
                                AsyncTransactionTracker* aAsyncTransactionTracker = nullptr);

protected:
  ImageClient(CompositableForwarder* aFwd, TextureFlags aFlags,
              CompositableType aType);

  CompositableType mType;
  int32_t mLastPaintedImageSerial;
  nsIntRect mPictureRect;
};




class ImageClientSingle : public ImageClient
{
public:
  ImageClientSingle(CompositableForwarder* aFwd,
                    TextureFlags aFlags,
                    CompositableType aType);

  virtual bool UpdateImage(ImageContainer* aContainer, uint32_t aContentFlags);

  virtual void OnDetach() MOZ_OVERRIDE;

  virtual bool AddTextureClient(TextureClient* aTexture) MOZ_OVERRIDE;

  virtual TextureInfo GetTextureInfo() const MOZ_OVERRIDE;

  virtual already_AddRefed<Image> CreateImage(ImageFormat aFormat) MOZ_OVERRIDE;

  virtual TemporaryRef<AsyncTransactionTracker> PrepareFlushAllImages() MOZ_OVERRIDE;

  virtual void FlushAllImages(bool aExceptFront,
                              AsyncTransactionTracker* aAsyncTransactionTracker) MOZ_OVERRIDE;

protected:
  virtual bool UpdateImageInternal(ImageContainer* aContainer, uint32_t aContentFlags, bool* aIsSwapped);

protected:
  RefPtr<TextureClient> mFrontBuffer;
};




class ImageClientBuffered : public ImageClientSingle
{
public:
  ImageClientBuffered(CompositableForwarder* aFwd,
                      TextureFlags aFlags,
                      CompositableType aType);

  virtual bool UpdateImage(ImageContainer* aContainer, uint32_t aContentFlags);

  virtual void OnDetach() MOZ_OVERRIDE;

  virtual void FlushAllImages(bool aExceptFront,
                              AsyncTransactionTracker* aAsyncTransactionTracker) MOZ_OVERRIDE;

protected:
  RefPtr<TextureClient> mBackBuffer;
};






class ImageClientBridge : public ImageClient
{
public:
  ImageClientBridge(CompositableForwarder* aFwd,
                    TextureFlags aFlags);

  virtual bool UpdateImage(ImageContainer* aContainer, uint32_t aContentFlags);
  virtual bool Connect() { return false; }
  virtual void Updated() {}
  void SetLayer(ShadowableLayer* aLayer)
  {
    mLayer = aLayer;
  }

  virtual TextureInfo GetTextureInfo() const MOZ_OVERRIDE
  {
    return TextureInfo(mType);
  }

  virtual void SetIPDLActor(CompositableChild* aChild) MOZ_OVERRIDE
  {
    MOZ_ASSERT(!aChild, "ImageClientBridge should not have IPDL actor");
  }

  virtual already_AddRefed<Image> CreateImage(ImageFormat aFormat) MOZ_OVERRIDE
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
  TextureInfo GetTextureInfo() const MOZ_OVERRIDE
  {
    return TextureInfo(CompositableType::IMAGE_OVERLAY);
  }
};
#endif

}
}

#endif
