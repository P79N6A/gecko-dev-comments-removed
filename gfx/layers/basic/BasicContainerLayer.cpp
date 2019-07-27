




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
#include "nsRegion.h"                   
#include "ReadbackProcessor.h"

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
    hasSingleBlendingChild = child->GetMixBlendMode() != CompositionOp::OP_OVER;
  }

  






  mUseIntermediateSurface =
    GetMaskLayer() ||
    GetForceIsolatedGroup() ||
    (GetMixBlendMode() != CompositionOp::OP_OVER && HasMultipleChildren()) ||
    (GetEffectiveOpacity() != 1.0 && (HasMultipleChildren() || hasSingleBlendingChild));
}

bool
BasicContainerLayer::ChildrenPartitionVisibleRegion(const nsIntRect& aInRect)
{
  Matrix transform;
  if (!GetEffectiveTransform().CanDraw2D(&transform) ||
      ThebesMatrix(transform).HasNonIntegerTranslation())
    return false;

  nsIntPoint offset(int32_t(transform._31), int32_t(transform._32));
  nsIntRect rect = aInRect.Intersect(GetEffectiveVisibleRegion().GetBounds() + offset);
  nsIntRegion covered;

  for (Layer* l = mFirstChild; l; l = l->GetNextSibling()) {
    if (ToData(l)->IsHidden())
      continue;

    Matrix childTransform;
    if (!l->GetEffectiveTransform().CanDraw2D(&childTransform) ||
        ThebesMatrix(childTransform).HasNonIntegerTranslation() ||
        l->GetEffectiveOpacity() != 1.0)
      return false;
    nsIntRegion childRegion = l->GetEffectiveVisibleRegion();
    childRegion.MoveBy(int32_t(childTransform._31), int32_t(childTransform._32));
    childRegion.And(childRegion, rect);
    if (l->GetClipRect()) {
      childRegion.And(childRegion, ParentLayerIntRect::ToUntyped(*l->GetClipRect()) + offset);
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
BasicContainerLayer::Validate(LayerManager::DrawPaintedLayerCallback aCallback,
                              void* aCallbackData,
                              ReadbackProcessor* aReadback)
{
  ReadbackProcessor readback;
  if (BasicManager()->IsRetained()) {
    readback.BuildUpdates(this);
  }
  for (Layer* l = mFirstChild; l; l = l->GetNextSibling()) {
    BasicImplData* data = ToData(l);
    data->Validate(aCallback, aCallbackData, &readback);
    if (l->GetMaskLayer()) {
      data = ToData(l->GetMaskLayer());
      data->Validate(aCallback, aCallbackData, nullptr);
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
