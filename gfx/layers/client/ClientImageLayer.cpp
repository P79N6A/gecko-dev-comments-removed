




#include "ClientLayerManager.h"         
#include "ImageContainer.h"             
#include "ImageLayers.h"                
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/layers/CompositorTypes.h"
#include "mozilla/layers/ImageClient.h"  
#include "mozilla/layers/LayersMessages.h"  
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsRegion.h"                   

namespace mozilla {
namespace layers {

using namespace mozilla::gfx;

class ClientImageLayer : public ImageLayer,
                         public ClientLayer {
public:
  explicit ClientImageLayer(ClientLayerManager* aLayerManager)
    : ImageLayer(aLayerManager, static_cast<ClientLayer*>(this))
    , mImageClientTypeContainer(CompositableType::UNKNOWN)
  {
    MOZ_COUNT_CTOR(ClientImageLayer);
  }

protected:
  virtual ~ClientImageLayer()
  {
    DestroyBackBuffer();
    MOZ_COUNT_DTOR(ClientImageLayer);
  }

  virtual void SetContainer(ImageContainer* aContainer) override
  {
    ImageLayer::SetContainer(aContainer);
    mImageClientTypeContainer = CompositableType::UNKNOWN;
  }

  virtual void SetVisibleRegion(const nsIntRegion& aRegion) override
  {
    NS_ASSERTION(ClientManager()->InConstruction(),
                 "Can only set properties in construction phase");
    ImageLayer::SetVisibleRegion(aRegion);
  }

  virtual void RenderLayer() override;
  
  virtual void ClearCachedResources() override
  {
    DestroyBackBuffer();
  }

  virtual void FillSpecificAttributes(SpecificLayerAttributes& aAttrs) override
  {
    aAttrs = ImageLayerAttributes(mFilter, mScaleToSize, mScaleMode);
  }

  virtual Layer* AsLayer() override { return this; }
  virtual ShadowableLayer* AsShadowableLayer() override { return this; }

  virtual void Disconnect() override
  {
    DestroyBackBuffer();
    ClientLayer::Disconnect();
  }

  void DestroyBackBuffer()
  {
    if (mImageClient) {
      mImageClient->OnDetach();
      mImageClient = nullptr;
    }
  }

  virtual CompositableClient* GetCompositableClient() override
  {
    return mImageClient;
  }

protected:
  ClientLayerManager* ClientManager()
  {
    return static_cast<ClientLayerManager*>(mManager);
  }

  CompositableType GetImageClientType()
  {
    if (mImageClientTypeContainer != CompositableType::UNKNOWN) {
      return mImageClientTypeContainer;
    }

    if (mContainer->IsAsync()) {
      mImageClientTypeContainer = CompositableType::IMAGE_BRIDGE;
      return mImageClientTypeContainer;
    }

    AutoLockImage autoLock(mContainer);

#ifdef MOZ_WIDGET_GONK
    if (autoLock.GetImage()->GetFormat() == ImageFormat::OVERLAY_IMAGE) {
      mImageClientTypeContainer = CompositableType::IMAGE_OVERLAY;
      return mImageClientTypeContainer;
    }
#endif

  	mImageClientTypeContainer = autoLock.GetImage()
							  ? CompositableType::IMAGE
							  : CompositableType::UNKNOWN;
    return mImageClientTypeContainer;
  }

  RefPtr<ImageClient> mImageClient;
  CompositableType mImageClientTypeContainer;
};

void
ClientImageLayer::RenderLayer()
{
  RenderMaskLayers(this);

  if (!mContainer) {
     return;
  }

  if (!mImageClient ||
      !mImageClient->UpdateImage(mContainer, GetContentFlags())) {
    CompositableType type = GetImageClientType();
    if (type == CompositableType::UNKNOWN) {
      return;
    }
    TextureFlags flags = TextureFlags::DEFAULT;
    if (mDisallowBigImage) {
      flags |= TextureFlags::DISALLOW_BIGIMAGE;
    }
    mImageClient = ImageClient::CreateImageClient(type,
                                                  ClientManager()->AsShadowForwarder(),
                                                  flags);
    if (type == CompositableType::IMAGE_BRIDGE) {
      static_cast<ImageClientBridge*>(mImageClient.get())->SetLayer(this);
    }

    if (!mImageClient) {
      return;
    }
    if (HasShadow() && !mContainer->IsAsync()) {
      mImageClient->Connect();
      ClientManager()->AsShadowForwarder()->Attach(mImageClient, this);
    }
    if (!mImageClient->UpdateImage(mContainer, GetContentFlags())) {
      return;
    }
  }
  ClientManager()->Hold(this);
}

already_AddRefed<ImageLayer>
ClientLayerManager::CreateImageLayer()
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  nsRefPtr<ClientImageLayer> layer =
    new ClientImageLayer(this);
  CREATE_SHADOW(Image);
  return layer.forget();
}

}
}
