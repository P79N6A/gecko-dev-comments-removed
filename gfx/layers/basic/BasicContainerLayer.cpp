




#include "BasicContainerLayer.h"
#include <sys/types.h>                  
#include "BasicLayersImpl.h"            
#include "basic/BasicImplData.h"        
#include "basic/BasicLayers.h"          
#include "mozilla/gfx/BaseRect.h"       
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsISupportsImpl.h"            
#include "nsPoint.h"                    
#include "nsRect.h"                     
#include "gfx3DMatrix.h"                
#include "gfxMatrix.h"                  
#include "nsRegion.h"                   

using namespace mozilla::gfx;

namespace mozilla {
namespace layers {

BasicContainerLayer::~BasicContainerLayer()
{
  while (mFirstChild) {
    ContainerLayer::RemoveChild(mFirstChild);
  }

  MOZ_COUNT_DTOR(BasicContainerLayer);
}

void
BasicContainerLayer::ComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface)
{
  
  
  
  gfxMatrix residual;
  gfx3DMatrix idealTransform = GetLocalTransform()*aTransformToSurface;
  idealTransform.ProjectTo2D();

  if (!idealTransform.CanDraw2D()) {
    mEffectiveTransform = idealTransform;
    ComputeEffectiveTransformsForChildren(gfx3DMatrix());
    ComputeEffectiveTransformForMaskLayer(gfx3DMatrix());
    mUseIntermediateSurface = true;
    return;
  }

  mEffectiveTransform = SnapTransformTranslation(idealTransform, &residual);
  
  
  ComputeEffectiveTransformsForChildren(idealTransform);

  ComputeEffectiveTransformForMaskLayer(aTransformToSurface);
  
  Layer* child = GetFirstChild();
  bool hasSingleBlendingChild = false;
  if (!HasMultipleChildren() && child) {
    hasSingleBlendingChild = child->GetMixBlendMode() != gfxContext::OPERATOR_OVER;
  }

  






  mUseIntermediateSurface =
    GetMaskLayer() ||
    GetForceIsolatedGroup() ||
    (GetMixBlendMode() != gfxContext::OPERATOR_OVER && HasMultipleChildren()) ||
    (GetEffectiveOpacity() != 1.0 && (HasMultipleChildren() || hasSingleBlendingChild));
}

bool
BasicContainerLayer::ChildrenPartitionVisibleRegion(const nsIntRect& aInRect)
{
  gfxMatrix transform;
  if (!GetEffectiveTransform().CanDraw2D(&transform) ||
      transform.HasNonIntegerTranslation())
    return false;

  nsIntPoint offset(int32_t(transform.x0), int32_t(transform.y0));
  nsIntRect rect = aInRect.Intersect(GetEffectiveVisibleRegion().GetBounds() + offset);
  nsIntRegion covered;

  for (Layer* l = mFirstChild; l; l = l->GetNextSibling()) {
    if (ToData(l)->IsHidden())
      continue;

    gfxMatrix childTransform;
    if (!l->GetEffectiveTransform().CanDraw2D(&childTransform) ||
        childTransform.HasNonIntegerTranslation() ||
        l->GetEffectiveOpacity() != 1.0)
      return false;
    nsIntRegion childRegion = l->GetEffectiveVisibleRegion();
    childRegion.MoveBy(int32_t(childTransform.x0), int32_t(childTransform.y0));
    childRegion.And(childRegion, rect);
    if (l->GetClipRect()) {
      childRegion.And(childRegion, *l->GetClipRect() + offset);
    }
    nsIntRegion intersection;
    intersection.And(covered, childRegion);
    if (!intersection.IsEmpty())
      return false;
    covered.Or(covered, childRegion);
  }

  return covered.Contains(rect);
}

already_AddRefed<ContainerLayer>
BasicLayerManager::CreateContainerLayer()
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  nsRefPtr<ContainerLayer> layer = new BasicContainerLayer(this);
  return layer.forget();
}

}
}
