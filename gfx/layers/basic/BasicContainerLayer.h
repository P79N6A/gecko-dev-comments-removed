




#ifndef GFX_BASICCONTAINERLAYER_H
#define GFX_BASICCONTAINERLAYER_H

#include "BasicImplData.h"              
#include "BasicLayers.h"                
#include "Layers.h"                     
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsISupportsUtils.h"           
struct nsIntRect;

namespace mozilla {
namespace layers {

class BasicContainerLayer : public ContainerLayer, public BasicImplData {
public:
  explicit BasicContainerLayer(BasicLayerManager* aManager) :
    ContainerLayer(aManager,
                   static_cast<BasicImplData*>(MOZ_THIS_IN_INITIALIZER_LIST()))
  {
    MOZ_COUNT_CTOR(BasicContainerLayer);
    mSupportsComponentAlphaChildren = true;
  }
protected:
  virtual ~BasicContainerLayer();

public:
  virtual void SetVisibleRegion(const nsIntRegion& aRegion)
  {
    NS_ASSERTION(BasicManager()->InConstruction(),
                 "Can only set properties in construction phase");
    ContainerLayer::SetVisibleRegion(aRegion);
  }
  virtual bool InsertAfter(Layer* aChild, Layer* aAfter)
  {
    if (!BasicManager()->InConstruction()) {
      NS_ERROR("Can only set properties in construction phase");
      return false;
    }
    return ContainerLayer::InsertAfter(aChild, aAfter);
  }

  virtual bool RemoveChild(Layer* aChild)
  { 
    if (!BasicManager()->InConstruction()) {
      NS_ERROR("Can only set properties in construction phase");
      return false;
    }
    return ContainerLayer::RemoveChild(aChild);
  }

  virtual bool RepositionChild(Layer* aChild, Layer* aAfter)
  {
    if (!BasicManager()->InConstruction()) {
      NS_ERROR("Can only set properties in construction phase");
      return false;
    }
    return ContainerLayer::RepositionChild(aChild, aAfter);
  }

  virtual void ComputeEffectiveTransforms(const gfx::Matrix4x4& aTransformToSurface);

  












  bool ChildrenPartitionVisibleRegion(const nsIntRect& aInRect);

  void ForceIntermediateSurface() { mUseIntermediateSurface = true; }

  void SetSupportsComponentAlphaChildren(bool aSupports) { mSupportsComponentAlphaChildren = aSupports; }

  virtual void Validate(LayerManager::DrawThebesLayerCallback aCallback,
                        void* aCallbackData,
                        ReadbackProcessor* aReadback) MOZ_OVERRIDE;

  



  virtual int32_t GetMaxLayerSize() MOZ_OVERRIDE { return 4096; }

protected:
  BasicLayerManager* BasicManager()
  {
    return static_cast<BasicLayerManager*>(mManager);
  }
};
}
}

#endif
