




#ifndef GFX_BASICCONTAINERLAYER_H
#define GFX_BASICCONTAINERLAYER_H

#include "BasicImplData.h"              
#include "BasicLayers.h"                
#include "Layers.h"                     
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsISupportsUtils.h"           
#include "mozilla/gfx/Rect.h"

namespace mozilla {
namespace layers {

class BasicContainerLayer : public ContainerLayer, public BasicImplData {
public:
  explicit BasicContainerLayer(BasicLayerManager* aManager) :
    ContainerLayer(aManager, static_cast<BasicImplData*>(this))
  {
    MOZ_COUNT_CTOR(BasicContainerLayer);
    mSupportsComponentAlphaChildren = true;
  }
protected:
  virtual ~BasicContainerLayer();

public:
  virtual void SetVisibleRegion(const nsIntRegion& aRegion) override
  {
    NS_ASSERTION(BasicManager()->InConstruction(),
                 "Can only set properties in construction phase");
    ContainerLayer::SetVisibleRegion(aRegion);
  }
  virtual bool InsertAfter(Layer* aChild, Layer* aAfter) override
  {
    if (!BasicManager()->InConstruction()) {
      NS_ERROR("Can only set properties in construction phase");
      return false;
    }
    return ContainerLayer::InsertAfter(aChild, aAfter);
  }

  virtual bool RemoveChild(Layer* aChild) override
  { 
    if (!BasicManager()->InConstruction()) {
      NS_ERROR("Can only set properties in construction phase");
      return false;
    }
    return ContainerLayer::RemoveChild(aChild);
  }

  virtual bool RepositionChild(Layer* aChild, Layer* aAfter) override
  {
    if (!BasicManager()->InConstruction()) {
      NS_ERROR("Can only set properties in construction phase");
      return false;
    }
    return ContainerLayer::RepositionChild(aChild, aAfter);
  }

  virtual void ComputeEffectiveTransforms(const gfx::Matrix4x4& aTransformToSurface) override;

  












  bool ChildrenPartitionVisibleRegion(const gfx::IntRect& aInRect);

  void ForceIntermediateSurface() { mUseIntermediateSurface = true; }

  void SetSupportsComponentAlphaChildren(bool aSupports) { mSupportsComponentAlphaChildren = aSupports; }

  virtual void Validate(LayerManager::DrawPaintedLayerCallback aCallback,
                        void* aCallbackData,
                        ReadbackProcessor* aReadback) override;

  



  virtual int32_t GetMaxLayerSize() override { return 4096; }

protected:
  BasicLayerManager* BasicManager()
  {
    return static_cast<BasicLayerManager*>(mManager);
  }
};
}
}

#endif
