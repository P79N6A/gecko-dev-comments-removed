




#ifndef GFX_BASICCONTAINERLAYER_H
#define GFX_BASICCONTAINERLAYER_H

#include "BasicImplData.h"              
#include "BasicLayers.h"                
#include "Layers.h"                     
#include "nsDebug.h"                    
#include "nsISupportsUtils.h"           
#include "nsTraceRefcnt.h"              
struct nsIntRect;

namespace mozilla {
namespace layers {

class BasicContainerLayer : public ContainerLayer, public BasicImplData {
public:
  BasicContainerLayer(BasicLayerManager* aManager) :
    ContainerLayer(aManager,
                   static_cast<BasicImplData*>(MOZ_THIS_IN_INITIALIZER_LIST()))
  {
    MOZ_COUNT_CTOR(BasicContainerLayer);
    mSupportsComponentAlphaChildren = true;
  }
  virtual ~BasicContainerLayer();

  virtual void SetVisibleRegion(const nsIntRegion& aRegion)
  {
    NS_ASSERTION(BasicManager()->InConstruction(),
                 "Can only set properties in construction phase");
    ContainerLayer::SetVisibleRegion(aRegion);
  }
  virtual void InsertAfter(Layer* aChild, Layer* aAfter)
  {
    NS_ASSERTION(BasicManager()->InConstruction(),
                 "Can only set properties in construction phase");
    ContainerLayer::InsertAfter(aChild, aAfter);
  }

  virtual void RemoveChild(Layer* aChild)
  { 
    NS_ASSERTION(BasicManager()->InConstruction(),
                 "Can only set properties in construction phase");
    ContainerLayer::RemoveChild(aChild);
  }

  virtual void RepositionChild(Layer* aChild, Layer* aAfter)
  {
    NS_ASSERTION(BasicManager()->InConstruction(),
                 "Can only set properties in construction phase");
    ContainerLayer::RepositionChild(aChild, aAfter);
  }

  virtual void ComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface);

  












  bool ChildrenPartitionVisibleRegion(const nsIntRect& aInRect);

  void ForceIntermediateSurface() { mUseIntermediateSurface = true; }

  void SetSupportsComponentAlphaChildren(bool aSupports) { mSupportsComponentAlphaChildren = aSupports; }

  virtual void Validate(LayerManager::DrawThebesLayerCallback aCallback,
                        void* aCallbackData) MOZ_OVERRIDE;

protected:
  BasicLayerManager* BasicManager()
  {
    return static_cast<BasicLayerManager*>(mManager);
  }
};
}
}

#endif
