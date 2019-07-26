




#ifndef GFX_CLIENTCONTAINERLAYER_H
#define GFX_CLIENTCONTAINERLAYER_H

#include <stdint.h>                     
#include "ClientLayerManager.h"         
#include "Layers.h"                     
#include "gfxPrefs.h"                   
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsISupportsUtils.h"           
#include "nsRegion.h"                   
#include "nsTArray.h"                   

namespace mozilla {
namespace layers {

class ShadowableLayer;

class ClientContainerLayer : public ContainerLayer,
                             public ClientLayer
{
public:
  ClientContainerLayer(ClientLayerManager* aManager) :
    ContainerLayer(aManager,
                   static_cast<ClientLayer*>(MOZ_THIS_IN_INITIALIZER_LIST()))
  {
    MOZ_COUNT_CTOR(ClientContainerLayer);
    mSupportsComponentAlphaChildren = true;
  }
  virtual ~ClientContainerLayer()
  {
    while (mFirstChild) {
      ContainerLayer::RemoveChild(mFirstChild);
    }

    MOZ_COUNT_DTOR(ClientContainerLayer);
  }

  virtual void RenderLayer()
  {
    if (GetMaskLayer()) {
      ToClientLayer(GetMaskLayer())->RenderLayer();
    }
    
    
    
    if (UseIntermediateSurface()) {
      if (GetEffectiveVisibleRegion().GetNumRects() != 1 ||
          !(GetContentFlags() & Layer::CONTENT_OPAQUE))
      {
        gfx::Matrix transform;
        if (HasOpaqueAncestorLayer(this) &&
            GetEffectiveTransform().Is2D(&transform) &&
            !gfx::ThebesMatrix(transform).HasNonIntegerTranslation()) {
          SetSupportsComponentAlphaChildren(
            gfxPrefs::ComponentAlphaEnabled());
        }
      }
    } else {
      SetSupportsComponentAlphaChildren(
        (GetContentFlags() & Layer::CONTENT_OPAQUE) ||
        (GetParent() && GetParent()->SupportsComponentAlphaChildren()));
    }

    nsAutoTArray<Layer*, 12> children;
    SortChildrenBy3DZOrder(children);

    for (uint32_t i = 0; i < children.Length(); i++) {
      Layer* child = children.ElementAt(i);
      if (child->GetEffectiveVisibleRegion().IsEmpty()) {
        continue;
      }

      if (!child->GetInvalidRegion().IsEmpty()) {
        child->Mutated();
      }

      ToClientLayer(child)->RenderLayer();
    }
  }

  virtual void SetVisibleRegion(const nsIntRegion& aRegion)
  {
    NS_ASSERTION(ClientManager()->InConstruction(),
                 "Can only set properties in construction phase");
    ContainerLayer::SetVisibleRegion(aRegion);
  }
  virtual bool InsertAfter(Layer* aChild, Layer* aAfter) MOZ_OVERRIDE
  {
    if(!ClientManager()->InConstruction()) {
      NS_ERROR("Can only set properties in construction phase");
      return false;
    }

    if (!ContainerLayer::InsertAfter(aChild, aAfter)) {
      return false;
    }

    ClientManager()->AsShadowForwarder()->InsertAfter(ClientManager()->Hold(this),
                                                      ClientManager()->Hold(aChild),
                                                      aAfter ? ClientManager()->Hold(aAfter) : nullptr);
    return true;
  }

  virtual bool RemoveChild(Layer* aChild) MOZ_OVERRIDE
  {
    if (!ClientManager()->InConstruction()) {
      NS_ERROR("Can only set properties in construction phase");
      return false;
    }
    
    ShadowableLayer *heldChild = ClientManager()->Hold(aChild);
    if (!ContainerLayer::RemoveChild(aChild)) {
      return false;
    }
    ClientManager()->AsShadowForwarder()->RemoveChild(ClientManager()->Hold(this), heldChild);
    return true;
  }

  virtual bool RepositionChild(Layer* aChild, Layer* aAfter) MOZ_OVERRIDE
  {
    if (!ClientManager()->InConstruction()) {
      NS_ERROR("Can only set properties in construction phase");
      return false;
    }
    if (!ContainerLayer::RepositionChild(aChild, aAfter)) {
      return false;
    }
    ClientManager()->AsShadowForwarder()->RepositionChild(ClientManager()->Hold(this),
                                                          ClientManager()->Hold(aChild),
                                                          aAfter ? ClientManager()->Hold(aAfter) : nullptr);
    return true;
  }

  virtual Layer* AsLayer() { return this; }
  virtual ShadowableLayer* AsShadowableLayer() { return this; }

  virtual void ComputeEffectiveTransforms(const gfx::Matrix4x4& aTransformToSurface)
  {
    DefaultComputeEffectiveTransforms(aTransformToSurface);
  }

  void ForceIntermediateSurface() { mUseIntermediateSurface = true; }

  void SetSupportsComponentAlphaChildren(bool aSupports) { mSupportsComponentAlphaChildren = aSupports; }

protected:
  ClientLayerManager* ClientManager()
  {
    return static_cast<ClientLayerManager*>(mManager);
  }
};

class ClientRefLayer : public RefLayer,
                       public ClientLayer {
public:
  ClientRefLayer(ClientLayerManager* aManager) :
    RefLayer(aManager,
             static_cast<ClientLayer*>(MOZ_THIS_IN_INITIALIZER_LIST()))
  {
    MOZ_COUNT_CTOR(ClientRefLayer);
  }
  virtual ~ClientRefLayer()
  {
    MOZ_COUNT_DTOR(ClientRefLayer);
  }

  virtual Layer* AsLayer() { return this; }
  virtual ShadowableLayer* AsShadowableLayer() { return this; }

  virtual void Disconnect()
  {
    ClientLayer::Disconnect();
  }

  virtual void RenderLayer() { }

  virtual void ComputeEffectiveTransforms(const gfx::Matrix4x4& aTransformToSurface)
  {
    DefaultComputeEffectiveTransforms(aTransformToSurface);
  }

private:
  ClientLayerManager* ClientManager()
  {
    return static_cast<ClientLayerManager*>(mManager);
  }
};

}
}

#endif
