




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
    : ImageLayer(aLayerManager,
                 static_cast<ClientLayer*>(MOZ_THIS_IN_INITIALIZER_LIST()))
    , mImageClientTypeContainer(CompositableType::BUFFER_UNKNOWN)
  {
    MOZ_COUNT_CTOR(ClientImageLayer);
  }

protected:
  virtual ~ClientImageLayer()
  {
    DestroyBackBuffer();
    MOZ_COUNT_DTOR(ClientImageLayer);
  }

  virtual void SetContainer(ImageContainer* aContainer) MOZ_OVERRIDE
  {
    ImageLayer::SetContainer(aContainer);
    mImageClientTypeContainer = CompositableType::BUFFER_UNKNOWN;
  }

  virtual void SetVisibleRegion(const nsIntRegion& aRegion)
  {
    NS_ASSERTION(ClientManager()->InConstruction(),
                 "Can only set properties in construction phase");
    ImageLayer::SetVisibleRegion(aRegion);
  }

  virtual void RenderLayer();
  
  virtual void ClearCachedResources() MOZ_OVERRIDE
  {
    DestroyBackBuffer();
  }

  virtual void FillSpecificAttributes(SpecificLayerAttributes& aAttrs)
  {
    aAttrs = ImageLayerAttributes(mFilter, mScaleToSize, mScaleMode);
  }

  virtual Layer* AsLayer() { return this; }
  virtual ShadowableLayer* AsShadowableLayer() { return this; }

  virtual void Disconnect()
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

  virtual CompositableClient* GetCompositableClient() MOZ_OVERRIDE
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
    if (mImageClientTypeContainer != CompositableType::BUFFER_UNKNOWN) {
      return mImageClientTypeContainer;
    }

    if (mContainer->IsAsync()) {
      mImageClientTypeContainer = CompositableType::BUFFER_BRIDGE;
      return mImageClientTypeContainer;
    }

    AutoLockImage autoLock(mContainer);

#ifdef MOZ_WIDGET_GONK
    
    
    mImageClientTypeContainer = autoLock.GetImage() ?
                                  CompositableType::BUFFER_IMAGE_BUFFERED : CompositableType::BUFFER_UNKNOWN;
#else
    mImageClientTypeContainer = autoLock.GetImage() ?
                                  CompositableType::BUFFER_IMAGE_SINGLE : CompositableType::BUFFER_UNKNOWN;
#endif
    return mImageClientTypeContainer;
  }

  RefPtr<ImageClient> mImageClient;
  CompositableType mImageClientTypeContainer;
};

void
ClientImageLayer::RenderLayer()
{
  if (GetMaskLayer()) {
    ToClientLayer(GetMaskLayer())->RenderLayer();
  }

  if (!mContainer) {
     return;
  }

  if (mImageClient) {
    mImageClient->OnTransaction();
  }

  if (!mImageClient ||
      !mImageClient->UpdateImage(mContainer, GetContentFlags())) {
    CompositableType type = GetImageClientType();
    if (type == CompositableType::BUFFER_UNKNOWN) {
      return;
    }
    TextureFlags flags = TextureFlags::FRONT;
    if (mDisallowBigImage) {
      flags |= TextureFlags::DISALLOW_BIGIMAGE;
    }
    mImageClient = ImageClient::CreateImageClient(type,
                                                  ClientManager()->AsShadowForwarder(),
                                                  flags);
    if (type == CompositableType::BUFFER_BRIDGE) {
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
  if (mImageClient) {
    mImageClient->OnTransaction();
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
