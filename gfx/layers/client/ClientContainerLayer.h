




#ifndef GFX_CLIENTCONTAINERLAYER_H
#define GFX_CLIENTCONTAINERLAYER_H

#include <stdint.h>                     
#include "ClientLayerManager.h"         
#include "Layers.h"                     
#include "gfx3DMatrix.h"                
#include "gfxMatrix.h"                  
#include "gfxPlatform.h"                
#include "nsDebug.h"                    
#include "nsISupportsUtils.h"           
#include "nsRegion.h"                   
#include "nsTArray.h"                   
#include "nsTraceRefcnt.h"              

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
        gfx3DMatrix transform3D;
        gfx::To3DMatrix(GetEffectiveTransform(), transform3D);
        gfxMatrix transform;
        if (HasOpaqueAncestorLayer(this) &&
            transform3D.Is2D(&transform) && 
            !transform.HasNonIntegerTranslation()) {
          SetSupportsComponentAlphaChildren(
            gfxPlatform::ComponentAlphaEnabled());
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
      if (children.ElementAt(i)->GetEffectiveVisibleRegion().IsEmpty()) {
        continue;
      }

      ToClientLayer(children.ElementAt(i))->RenderLayer();
    }
  }

  virtual void SetVisibleRegion(const nsIntRegion& aRegion)
  {
    NS_ASSERTION(ClientManager()->InConstruction(),
                 "Can only set properties in construction phase");
    ContainerLayer::SetVisibleRegion(aRegion);
  }
  virtual void InsertAfter(Layer* aChild, Layer* aAfter) MOZ_OVERRIDE
  {
    NS_ASSERTION(ClientManager()->InConstruction(),
                 "Can only set properties in construction phase");
    ClientManager()->AsShadowForwarder()->InsertAfter(ClientManager()->Hold(this),
                                                      ClientManager()->Hold(aChild),
                                                      aAfter ? ClientManager()->Hold(aAfter) : nullptr);
    ContainerLayer::InsertAfter(aChild, aAfter);
  }

  virtual void RemoveChild(Layer* aChild) MOZ_OVERRIDE
  { 
    NS_ASSERTION(ClientManager()->InConstruction(),
                 "Can only set properties in construction phase");
    ClientManager()->AsShadowForwarder()->RemoveChild(ClientManager()->Hold(this),
                                                      ClientManager()->Hold(aChild));
    ContainerLayer::RemoveChild(aChild);
  }

  virtual void RepositionChild(Layer* aChild, Layer* aAfter) MOZ_OVERRIDE
  {
    NS_ASSERTION(ClientManager()->InConstruction(),
                 "Can only set properties in construction phase");
    ClientManager()->AsShadowForwarder()->RepositionChild(ClientManager()->Hold(this),
                                                          ClientManager()->Hold(aChild),
                                                          aAfter ? ClientManager()->Hold(aAfter) : nullptr);
    ContainerLayer::RepositionChild(aChild, aAfter);
  }
  
  virtual Layer* AsLayer() { return this; }
  virtual ShadowableLayer* AsShadowableLayer() { return this; }

  virtual void ComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface)
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

  virtual void ComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface)
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
