




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

class ClientLayer;
class CompositableForwarder;
class AsyncTransactionTracker;
class Image;
class ImageContainer;
class ShadowableLayer;






class ImageClient : public CompositableClient
{
public:
  




  static already_AddRefed<ImageClient> CreateImageClient(CompositableType aImageHostType,
                                                     CompositableForwarder* aFwd,
                                                     TextureFlags aFlags);

  virtual ~ImageClient() {}

  




  virtual bool UpdateImage(ImageContainer* aContainer, uint32_t aContentFlags) = 0;

  virtual already_AddRefed<Image> CreateImage(ImageFormat aFormat) = 0;

  void SetLayer(ClientLayer* aLayer) { mLayer = aLayer; }
  ClientLayer* GetLayer() const { return mLayer; }

  



  virtual void FlushAllImages(AsyncTransactionWaiter* aAsyncTransactionWaiter) {}

  virtual void RemoveTexture(TextureClient* aTexture) override;

  void RemoveTextureWithWaiter(TextureClient* aTexture,
                               AsyncTransactionWaiter* aAsyncTransactionWaiter = nullptr);

protected:
  ImageClient(CompositableForwarder* aFwd, TextureFlags aFlags,
              CompositableType aType);

  ClientLayer* mLayer;
  CompositableType mType;
  uint32_t mLastUpdateGenerationCounter;
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

  virtual void FlushAllImages(AsyncTransactionWaiter* aAsyncTransactionWaiter) override;

protected:
  struct Buffer {
    RefPtr<TextureClient> mTextureClient;
    int32_t mImageSerial;
  };
  nsTArray<Buffer> mBuffers;
};






class ImageClientBridge : public ImageClient
{
public:
  ImageClientBridge(CompositableForwarder* aFwd,
                    TextureFlags aFlags);

  virtual bool UpdateImage(ImageContainer* aContainer, uint32_t aContentFlags) override;
  virtual bool Connect(ImageContainer* aImageContainer) override { return false; }

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
