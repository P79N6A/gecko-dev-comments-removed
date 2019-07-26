




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
BasicContainerLayer::ComputeEffectiveTransforms(const Matrix4x4& aTransformToSurface)
{
  
  
  
  Matrix residual;
  Matrix4x4 idealTransform = GetLocalTransform() * aTransformToSurface;
  idealTransform.ProjectTo2D();

  if (!idealTransform.CanDraw2D()) {
    mEffectiveTransform = idealTransform;
    ComputeEffectiveTransformsForChildren(Matrix4x4());
    ComputeEffectiveTransformForMaskLayer(Matrix4x4());
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
  gfx3DMatrix effectiveTransform;
  gfx::To3DMatrix(GetEffectiveTransform(), effectiveTransform);
  if (!effectiveTransform.CanDraw2D(&transform) ||
      transform.HasNonIntegerTranslation())
    return false;

  nsIntPoint offset(int32_t(transform.x0), int32_t(transform.y0));
  nsIntRect rect = aInRect.Intersect(GetEffectiveVisibleRegion().GetBounds() + offset);
  nsIntRegion covered;

  for (Layer* l = mFirstChild; l; l = l->GetNextSibling()) {
    if (ToData(l)->IsHidden())
      continue;

    gfxMatrix childTransform;
    gfx3DMatrix effectiveTransform;
    gfx::To3DMatrix(l->GetEffectiveTransform(), effectiveTransform);
    if (!effectiveTransform.CanDraw2D(&childTransform) ||
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

void
BasicContainerLayer::Validate(LayerManager::DrawThebesLayerCallback aCallback,
                              void* aCallbackData)
{
  for (Layer* l = mFirstChild; l; l = l->GetNextSibling()) {
    BasicImplData* data = ToData(l);
    data->Validate(aCallback, aCallbackData);
    if (l->GetMaskLayer()) {
      data = ToData(l->GetMaskLayer());
      data->Validate(aCallback, aCallbackData);
    }
  }
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
