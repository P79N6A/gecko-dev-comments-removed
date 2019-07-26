






#include "Layers.h"
#include <algorithm>                    
#include "AnimationCommon.h"            
#include "CompositableHost.h"           
#include "ImageContainer.h"             
#include "ImageLayers.h"                
#include "LayerSorter.h"                
#include "LayersLogging.h"              
#include "ReadbackLayer.h"              
#include "gfxPlatform.h"                
#include "gfxUtils.h"                   
#include "gfx2DGlue.h"
#include "mozilla/DebugOnly.h"          
#include "mozilla/Telemetry.h"          
#include "mozilla/gfx/2D.h"             
#include "mozilla/gfx/BaseSize.h"       
#include "mozilla/gfx/Matrix.h"         
#include "mozilla/layers/AsyncPanZoomController.h"
#include "mozilla/layers/Compositor.h"  
#include "mozilla/layers/CompositorTypes.h"
#include "mozilla/layers/LayerManagerComposite.h"  
#include "mozilla/layers/LayersMessages.h"  
#include "nsAString.h"
#include "nsCSSValue.h"                 
#include "nsPrintfCString.h"            
#include "nsStyleStruct.h"              
#include "gfxPrefs.h"

uint8_t gLayerManagerLayerBuilder;

namespace mozilla {
namespace layers {

FILE*
FILEOrDefault(FILE* aFile)
{
  return aFile ? aFile : stderr;
}

typedef FrameMetrics::ViewID ViewID;
const ViewID FrameMetrics::NULL_SCROLL_ID = 0;

using namespace mozilla::gfx;



Layer*
LayerManager::GetPrimaryScrollableLayer()
{
  if (!mRoot) {
    return nullptr;
  }

  nsTArray<Layer*> queue;
  queue.AppendElement(mRoot);
  while (queue.Length()) {
    ContainerLayer* containerLayer = queue[0]->AsContainerLayer();
    queue.RemoveElementAt(0);
    if (!containerLayer) {
      continue;
    }

    const FrameMetrics& frameMetrics = containerLayer->GetFrameMetrics();
    if (frameMetrics.IsScrollable()) {
      return containerLayer;
    }

    Layer* child = containerLayer->GetFirstChild();
    while (child) {
      queue.AppendElement(child);
      child = child->GetNextSibling();
    }
  }

  return mRoot;
}

void
LayerManager::GetScrollableLayers(nsTArray<Layer*>& aArray)
{
  if (!mRoot) {
    return;
  }

  nsTArray<Layer*> queue;
  queue.AppendElement(mRoot);
  while (!queue.IsEmpty()) {
    ContainerLayer* containerLayer = queue.LastElement()->AsContainerLayer();
    queue.RemoveElementAt(queue.Length() - 1);
    if (!containerLayer) {
      continue;
    }

    const FrameMetrics& frameMetrics = containerLayer->GetFrameMetrics();
    if (frameMetrics.IsScrollable()) {
      aArray.AppendElement(containerLayer);
      continue;
    }

    Layer* child = containerLayer->GetFirstChild();
    while (child) {
      queue.AppendElement(child);
      child = child->GetNextSibling();
    }
  }
}

TemporaryRef<DrawTarget>
LayerManager::CreateOptimalDrawTarget(const gfx::IntSize &aSize,
                                      SurfaceFormat aFormat)
{
  return gfxPlatform::GetPlatform()->CreateOffscreenContentDrawTarget(aSize,
                                                                      aFormat);
}

TemporaryRef<DrawTarget>
LayerManager::CreateOptimalMaskDrawTarget(const gfx::IntSize &aSize)
{
  return CreateOptimalDrawTarget(aSize, SurfaceFormat::A8);
}

TemporaryRef<DrawTarget>
LayerManager::CreateDrawTarget(const IntSize &aSize,
                               SurfaceFormat aFormat)
{
  return gfxPlatform::GetPlatform()->
    CreateOffscreenCanvasDrawTarget(aSize, aFormat);
}

#ifdef DEBUG
void
LayerManager::Mutated(Layer* aLayer)
{
}
#endif  

already_AddRefed<ImageContainer>
LayerManager::CreateImageContainer()
{
  nsRefPtr<ImageContainer> container = new ImageContainer(ImageContainer::DISABLE_ASYNC);
  return container.forget();
}

already_AddRefed<ImageContainer>
LayerManager::CreateAsynchronousImageContainer()
{
  nsRefPtr<ImageContainer> container = new ImageContainer(ImageContainer::ENABLE_ASYNC);
  return container.forget();
}




Layer::Layer(LayerManager* aManager, void* aImplData) :
  mManager(aManager),
  mParent(nullptr),
  mNextSibling(nullptr),
  mPrevSibling(nullptr),
  mImplData(aImplData),
  mMaskLayer(nullptr),
  mPostXScale(1.0f),
  mPostYScale(1.0f),
  mOpacity(1.0),
  mMixBlendMode(CompositionOp::OP_OVER),
  mForceIsolatedGroup(false),
  mContentFlags(0),
  mUseClipRect(false),
  mUseTileSourceRect(false),
  mIsFixedPosition(false),
  mMargins(0, 0, 0, 0),
  mStickyPositionData(nullptr),
  mScrollbarTargetId(FrameMetrics::NULL_SCROLL_ID),
  mScrollbarDirection(ScrollDirection::NONE),
  mDebugColorIndex(0),
  mAnimationGeneration(0)
{}

Layer::~Layer()
{}

Animation*
Layer::AddAnimation()
{
  MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) AddAnimation", this));

  MOZ_ASSERT(!mPendingAnimations, "should have called ClearAnimations first");

  Animation* anim = mAnimations.AppendElement();

  Mutated();
  return anim;
}

void
Layer::ClearAnimations()
{
  mPendingAnimations = nullptr;

  if (mAnimations.IsEmpty() && mAnimationData.IsEmpty()) {
    return;
  }

  MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) ClearAnimations", this));
  mAnimations.Clear();
  mAnimationData.Clear();
  Mutated();
}

Animation*
Layer::AddAnimationForNextTransaction()
{
  MOZ_ASSERT(mPendingAnimations,
             "should have called ClearAnimationsForNextTransaction first");

  Animation* anim = mPendingAnimations->AppendElement();

  return anim;
}

void
Layer::ClearAnimationsForNextTransaction()
{
  
  if (!mPendingAnimations) {
    mPendingAnimations = new AnimationArray;
  }

  mPendingAnimations->Clear();
}

static nsCSSValueSharedList*
CreateCSSValueList(const InfallibleTArray<TransformFunction>& aFunctions)
{
  nsAutoPtr<nsCSSValueList> result;
  nsCSSValueList** resultTail = getter_Transfers(result);
  for (uint32_t i = 0; i < aFunctions.Length(); i++) {
    nsRefPtr<nsCSSValue::Array> arr;
    switch (aFunctions[i].type()) {
      case TransformFunction::TRotationX:
      {
        float theta = aFunctions[i].get_RotationX().radians();
        arr = nsStyleAnimation::AppendTransformFunction(eCSSKeyword_rotatex, resultTail);
        arr->Item(1).SetFloatValue(theta, eCSSUnit_Radian);
        break;
      }
      case TransformFunction::TRotationY:
      {
        float theta = aFunctions[i].get_RotationY().radians();
        arr = nsStyleAnimation::AppendTransformFunction(eCSSKeyword_rotatey, resultTail);
        arr->Item(1).SetFloatValue(theta, eCSSUnit_Radian);
        break;
      }
      case TransformFunction::TRotationZ:
      {
        float theta = aFunctions[i].get_RotationZ().radians();
        arr = nsStyleAnimation::AppendTransformFunction(eCSSKeyword_rotatez, resultTail);
        arr->Item(1).SetFloatValue(theta, eCSSUnit_Radian);
        break;
      }
      case TransformFunction::TRotation:
      {
        float theta = aFunctions[i].get_Rotation().radians();
        arr = nsStyleAnimation::AppendTransformFunction(eCSSKeyword_rotate, resultTail);
        arr->Item(1).SetFloatValue(theta, eCSSUnit_Radian);
        break;
      }
      case TransformFunction::TRotation3D:
      {
        float x = aFunctions[i].get_Rotation3D().x();
        float y = aFunctions[i].get_Rotation3D().y();
        float z = aFunctions[i].get_Rotation3D().z();
        float theta = aFunctions[i].get_Rotation3D().radians();
        arr = nsStyleAnimation::AppendTransformFunction(eCSSKeyword_rotate3d, resultTail);
        arr->Item(1).SetFloatValue(x, eCSSUnit_Number);
        arr->Item(2).SetFloatValue(y, eCSSUnit_Number);
        arr->Item(3).SetFloatValue(z, eCSSUnit_Number);
        arr->Item(4).SetFloatValue(theta, eCSSUnit_Radian);
        break;
      }
      case TransformFunction::TScale:
      {
        arr = nsStyleAnimation::AppendTransformFunction(eCSSKeyword_scale3d, resultTail);
        arr->Item(1).SetFloatValue(aFunctions[i].get_Scale().x(), eCSSUnit_Number);
        arr->Item(2).SetFloatValue(aFunctions[i].get_Scale().y(), eCSSUnit_Number);
        arr->Item(3).SetFloatValue(aFunctions[i].get_Scale().z(), eCSSUnit_Number);
        break;
      }
      case TransformFunction::TTranslation:
      {
        arr = nsStyleAnimation::AppendTransformFunction(eCSSKeyword_translate3d, resultTail);
        arr->Item(1).SetFloatValue(aFunctions[i].get_Translation().x(), eCSSUnit_Pixel);
        arr->Item(2).SetFloatValue(aFunctions[i].get_Translation().y(), eCSSUnit_Pixel);
        arr->Item(3).SetFloatValue(aFunctions[i].get_Translation().z(), eCSSUnit_Pixel);
        break;
      }
      case TransformFunction::TSkewX:
      {
        float x = aFunctions[i].get_SkewX().x();
        arr = nsStyleAnimation::AppendTransformFunction(eCSSKeyword_skewx, resultTail);
        arr->Item(1).SetFloatValue(x, eCSSUnit_Radian);
        break;
      }
      case TransformFunction::TSkewY:
      {
        float y = aFunctions[i].get_SkewY().y();
        arr = nsStyleAnimation::AppendTransformFunction(eCSSKeyword_skewy, resultTail);
        arr->Item(1).SetFloatValue(y, eCSSUnit_Radian);
        break;
      }
      case TransformFunction::TSkew:
      {
        arr = nsStyleAnimation::AppendTransformFunction(eCSSKeyword_skew, resultTail);
        arr->Item(1).SetFloatValue(aFunctions[i].get_Skew().x(), eCSSUnit_Radian);
        arr->Item(2).SetFloatValue(aFunctions[i].get_Skew().y(), eCSSUnit_Radian);
        break;
      }
      case TransformFunction::TTransformMatrix:
      {
        arr = nsStyleAnimation::AppendTransformFunction(eCSSKeyword_matrix3d, resultTail);
        const gfx::Matrix4x4& matrix = aFunctions[i].get_TransformMatrix().value();
        arr->Item(1).SetFloatValue(matrix._11, eCSSUnit_Number);
        arr->Item(2).SetFloatValue(matrix._12, eCSSUnit_Number);
        arr->Item(3).SetFloatValue(matrix._13, eCSSUnit_Number);
        arr->Item(4).SetFloatValue(matrix._14, eCSSUnit_Number);
        arr->Item(5).SetFloatValue(matrix._21, eCSSUnit_Number);
        arr->Item(6).SetFloatValue(matrix._22, eCSSUnit_Number);
        arr->Item(7).SetFloatValue(matrix._23, eCSSUnit_Number);
        arr->Item(8).SetFloatValue(matrix._24, eCSSUnit_Number);
        arr->Item(9).SetFloatValue(matrix._31, eCSSUnit_Number);
        arr->Item(10).SetFloatValue(matrix._32, eCSSUnit_Number);
        arr->Item(11).SetFloatValue(matrix._33, eCSSUnit_Number);
        arr->Item(12).SetFloatValue(matrix._34, eCSSUnit_Number);
        arr->Item(13).SetFloatValue(matrix._41, eCSSUnit_Number);
        arr->Item(14).SetFloatValue(matrix._42, eCSSUnit_Number);
        arr->Item(15).SetFloatValue(matrix._43, eCSSUnit_Number);
        arr->Item(16).SetFloatValue(matrix._44, eCSSUnit_Number);
        break;
      }
      case TransformFunction::TPerspective:
      {
        float perspective = aFunctions[i].get_Perspective().value();
        arr = nsStyleAnimation::AppendTransformFunction(eCSSKeyword_perspective, resultTail);
        arr->Item(1).SetFloatValue(perspective, eCSSUnit_Pixel);
        break;
      }
      default:
        NS_ASSERTION(false, "All functions should be implemented?");
    }
  }
  if (aFunctions.Length() == 0) {
    result = new nsCSSValueList();
    result->mValue.SetNoneValue();
  }
  return new nsCSSValueSharedList(result.forget());
}

void
Layer::SetAnimations(const AnimationArray& aAnimations)
{
  MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) SetAnimations", this));

  mAnimations = aAnimations;
  mAnimationData.Clear();
  for (uint32_t i = 0; i < mAnimations.Length(); i++) {
    AnimData* data = mAnimationData.AppendElement();
    InfallibleTArray<nsAutoPtr<css::ComputedTimingFunction> >& functions = data->mFunctions;
    const InfallibleTArray<AnimationSegment>& segments =
      mAnimations.ElementAt(i).segments();
    for (uint32_t j = 0; j < segments.Length(); j++) {
      TimingFunction tf = segments.ElementAt(j).sampleFn();
      css::ComputedTimingFunction* ctf = new css::ComputedTimingFunction();
      switch (tf.type()) {
        case TimingFunction::TCubicBezierFunction: {
          CubicBezierFunction cbf = tf.get_CubicBezierFunction();
          ctf->Init(nsTimingFunction(cbf.x1(), cbf.y1(), cbf.x2(), cbf.y2()));
          break;
        }
        default: {
          NS_ASSERTION(tf.type() == TimingFunction::TStepFunction,
                       "Function must be bezier or step");
          StepFunction sf = tf.get_StepFunction();
          nsTimingFunction::Type type = sf.type() == 1 ? nsTimingFunction::StepStart
                                                       : nsTimingFunction::StepEnd;
          ctf->Init(nsTimingFunction(type, sf.steps()));
          break;
        }
      }
      functions.AppendElement(ctf);
    }

    
    
    InfallibleTArray<nsStyleAnimation::Value>& startValues = data->mStartValues;
    InfallibleTArray<nsStyleAnimation::Value>& endValues = data->mEndValues;
    for (uint32_t j = 0; j < mAnimations[i].segments().Length(); j++) {
      const AnimationSegment& segment = mAnimations[i].segments()[j];
      nsStyleAnimation::Value* startValue = startValues.AppendElement();
      nsStyleAnimation::Value* endValue = endValues.AppendElement();
      if (segment.endState().type() == Animatable::TArrayOfTransformFunction) {
        const InfallibleTArray<TransformFunction>& startFunctions =
          segment.startState().get_ArrayOfTransformFunction();
        startValue->SetTransformValue(CreateCSSValueList(startFunctions));

        const InfallibleTArray<TransformFunction>& endFunctions =
          segment.endState().get_ArrayOfTransformFunction();
        endValue->SetTransformValue(CreateCSSValueList(endFunctions));
      } else {
        NS_ASSERTION(segment.endState().type() == Animatable::Tfloat,
                     "Unknown Animatable type");
        startValue->SetFloatValue(segment.startState().get_float());
        endValue->SetFloatValue(segment.endState().get_float());
      }
    }
  }

  Mutated();
}

void
ContainerLayer::SetAsyncPanZoomController(AsyncPanZoomController *controller)
{
  mAPZC = controller;
}

AsyncPanZoomController*
ContainerLayer::GetAsyncPanZoomController() const
{
#ifdef DEBUG
  if (mAPZC) {
    MOZ_ASSERT(GetFrameMetrics().IsScrollable());
  }
#endif
  return mAPZC;
}

void
Layer::ApplyPendingUpdatesToSubtree()
{
  ApplyPendingUpdatesForThisTransaction();
  for (Layer* child = GetFirstChild(); child; child = child->GetNextSibling()) {
    child->ApplyPendingUpdatesToSubtree();
  }
}

bool
Layer::CanUseOpaqueSurface()
{
  
  
  if (GetContentFlags() & CONTENT_OPAQUE)
    return true;
  
  
  
  
  ContainerLayer* parent = GetParent();
  return parent && parent->GetFirstChild() == this &&
    parent->CanUseOpaqueSurface();
}



const nsIntRect*
Layer::GetEffectiveClipRect()
{
  if (LayerComposite* shadow = AsLayerComposite()) {
    return shadow->GetShadowClipRect();
  }
  return GetClipRect();
}

const nsIntRegion&
Layer::GetEffectiveVisibleRegion()
{
  if (LayerComposite* shadow = AsLayerComposite()) {
    return shadow->GetShadowVisibleRegion();
  }
  return GetVisibleRegion();
}

Matrix4x4
Layer::SnapTransformTranslation(const Matrix4x4& aTransform,
                                Matrix* aResidualTransform)
{
  if (aResidualTransform) {
    *aResidualTransform = Matrix();
  }

  Matrix matrix2D;
  Matrix4x4 result;
  if (mManager->IsSnappingEffectiveTransforms() &&
      aTransform.Is2D(&matrix2D) &&
      !matrix2D.HasNonTranslation() &&
      matrix2D.HasNonIntegerTranslation()) {
    IntPoint snappedTranslation = RoundedToInt(matrix2D.GetTranslation());
    Matrix snappedMatrix = Matrix::Translation(snappedTranslation.x,
                                               snappedTranslation.y);
    result = Matrix4x4::From2D(snappedMatrix);
    if (aResidualTransform) {
      
      
      
      *aResidualTransform =
        Matrix::Translation(matrix2D._31 - snappedTranslation.x,
                            matrix2D._32 - snappedTranslation.y);
    }
  } else {
    result = aTransform;
  }
  return result;
}

Matrix4x4
Layer::SnapTransform(const Matrix4x4& aTransform,
                     const gfxRect& aSnapRect,
                     Matrix* aResidualTransform)
{
  if (aResidualTransform) {
    *aResidualTransform = Matrix();
  }

  Matrix matrix2D;
  Matrix4x4 result;
  if (mManager->IsSnappingEffectiveTransforms() &&
      aTransform.Is2D(&matrix2D) &&
      gfx::Size(1.0, 1.0) <= ToSize(aSnapRect.Size()) &&
      matrix2D.PreservesAxisAlignedRectangles()) {
    IntPoint transformedTopLeft = RoundedToInt(matrix2D * ToPoint(aSnapRect.TopLeft()));
    IntPoint transformedTopRight = RoundedToInt(matrix2D * ToPoint(aSnapRect.TopRight()));
    IntPoint transformedBottomRight = RoundedToInt(matrix2D * ToPoint(aSnapRect.BottomRight()));

    Matrix snappedMatrix = gfxUtils::TransformRectToRect(aSnapRect,
      transformedTopLeft, transformedTopRight, transformedBottomRight);

    result = Matrix4x4::From2D(snappedMatrix);
    if (aResidualTransform && !snappedMatrix.IsSingular()) {
      
      
      
      Matrix snappedMatrixInverse = snappedMatrix;
      snappedMatrixInverse.Invert();
      *aResidualTransform = matrix2D * snappedMatrixInverse;
    }
  } else {
    result = aTransform;
  }
  return result;
}

static bool
AncestorLayerMayChangeTransform(Layer* aLayer)
{
  for (Layer* l = aLayer; l; l = l->GetParent()) {
    if (l->GetContentFlags() & Layer::CONTENT_MAY_CHANGE_TRANSFORM) {
      return true;
    }
  }
  return false;
}

bool
Layer::MayResample()
{
  Matrix transform2d;
  return !GetEffectiveTransform().Is2D(&transform2d) ||
         ThebesMatrix(transform2d).HasNonIntegerTranslation() ||
         AncestorLayerMayChangeTransform(this);
}

nsIntRect
Layer::CalculateScissorRect(const nsIntRect& aCurrentScissorRect,
                            const gfx::Matrix* aWorldTransform)
{
  ContainerLayer* container = GetParent();
  NS_ASSERTION(container, "This can't be called on the root!");

  
  
  nsIntRect currentClip;
  if (container->UseIntermediateSurface()) {
    currentClip.SizeTo(container->GetIntermediateSurfaceRect().Size());
  } else {
    currentClip = aCurrentScissorRect;
  }

  const nsIntRect *clipRect = GetEffectiveClipRect();
  if (!clipRect)
    return currentClip;

  if (clipRect->IsEmpty()) {
    
    
    return nsIntRect(currentClip.TopLeft(), nsIntSize(0, 0));
  }

  nsIntRect scissor = *clipRect;
  if (!container->UseIntermediateSurface()) {
    gfx::Matrix matrix;
    DebugOnly<bool> is2D = container->GetEffectiveTransform().Is2D(&matrix);
    
    NS_ASSERTION(is2D && matrix.PreservesAxisAlignedRectangles(),
                 "Non preserves axis aligned transform with clipped child should have forced intermediate surface");
    gfx::Rect r(scissor.x, scissor.y, scissor.width, scissor.height);
    gfxRect trScissor = gfx::ThebesRect(matrix.TransformBounds(r));
    trScissor.Round();
    if (!gfxUtils::GfxRectToIntRect(trScissor, &scissor)) {
      return nsIntRect(currentClip.TopLeft(), nsIntSize(0, 0));
    }

    
    do {
      container = container->GetParent();
    } while (container && !container->UseIntermediateSurface());
  }
  if (container) {
    scissor.MoveBy(-container->GetIntermediateSurfaceRect().TopLeft());
  } else if (aWorldTransform) {
    gfx::Rect r(scissor.x, scissor.y, scissor.width, scissor.height);
    gfx::Rect trScissor = aWorldTransform->TransformBounds(r);
    trScissor.Round();
    if (!gfxUtils::GfxRectToIntRect(ThebesRect(trScissor), &scissor))
      return nsIntRect(currentClip.TopLeft(), nsIntSize(0, 0));
  }
  return currentClip.Intersect(scissor);
}

const Matrix4x4
Layer::GetTransform() const
{
  Matrix4x4 transform = mTransform;
  if (const ContainerLayer* c = AsContainerLayer()) {
    transform.Scale(c->GetPreXScale(), c->GetPreYScale(), 1.0f);
  }
  transform = transform * Matrix4x4().Scale(mPostXScale, mPostYScale, 1.0f);
  return transform;
}

const Matrix4x4
Layer::GetLocalTransform()
{
  Matrix4x4 transform;
  if (LayerComposite* shadow = AsLayerComposite())
    transform = shadow->GetShadowTransform();
  else
    transform = mTransform;
  if (ContainerLayer* c = AsContainerLayer()) {
    transform.Scale(c->GetPreXScale(), c->GetPreYScale(), 1.0f);
  }
  transform = transform * Matrix4x4().Scale(mPostXScale, mPostYScale, 1.0f);

  return transform;
}

void
Layer::ApplyPendingUpdatesForThisTransaction()
{
  if (mPendingTransform && *mPendingTransform != mTransform) {
    MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) PendingUpdatesForThisTransaction", this));
    mTransform = *mPendingTransform;
    Mutated();
  }
  mPendingTransform = nullptr;

  if (mPendingAnimations) {
    MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) PendingUpdatesForThisTransaction", this));
    mPendingAnimations->SwapElements(mAnimations);
    mPendingAnimations = nullptr;
    Mutated();
  }
}

const float
Layer::GetLocalOpacity()
{
   if (LayerComposite* shadow = AsLayerComposite())
    return shadow->GetShadowOpacity();
  return mOpacity;
}

float
Layer::GetEffectiveOpacity()
{
  float opacity = GetLocalOpacity();
  for (ContainerLayer* c = GetParent(); c && !c->UseIntermediateSurface();
       c = c->GetParent()) {
    opacity *= c->GetLocalOpacity();
  }
  return opacity;
}
  
CompositionOp
Layer::GetEffectiveMixBlendMode()
{
  if(mMixBlendMode != CompositionOp::OP_OVER)
    return mMixBlendMode;
  for (ContainerLayer* c = GetParent(); c && !c->UseIntermediateSurface();
    c = c->GetParent()) {
    if(c->mMixBlendMode != CompositionOp::OP_OVER)
      return c->mMixBlendMode;
  }

  return mMixBlendMode;
}

gfxContext::GraphicsOperator
Layer::DeprecatedGetEffectiveMixBlendMode()
{
  return ThebesOp(GetEffectiveMixBlendMode());
}

void
Layer::ComputeEffectiveTransformForMaskLayer(const Matrix4x4& aTransformToSurface)
{
  if (mMaskLayer) {
    mMaskLayer->mEffectiveTransform = aTransformToSurface;

#ifdef DEBUG
    bool maskIs2D = mMaskLayer->GetTransform().CanDraw2D();
    NS_ASSERTION(maskIs2D, "How did we end up with a 3D transform here?!");
#endif
    mMaskLayer->mEffectiveTransform = mMaskLayer->GetTransform() * mMaskLayer->mEffectiveTransform;
  }
}

ContainerLayer::ContainerLayer(LayerManager* aManager, void* aImplData)
  : Layer(aManager, aImplData),
    mFirstChild(nullptr),
    mLastChild(nullptr),
    mScrollHandoffParentId(FrameMetrics::NULL_SCROLL_ID),
    mPreXScale(1.0f),
    mPreYScale(1.0f),
    mInheritedXScale(1.0f),
    mInheritedYScale(1.0f),
    mUseIntermediateSurface(false),
    mSupportsComponentAlphaChildren(false),
    mMayHaveReadbackChild(false)
{
  mContentFlags = 0; 
}

ContainerLayer::~ContainerLayer() {}

bool
ContainerLayer::InsertAfter(Layer* aChild, Layer* aAfter)
{
  if(aChild->Manager() != Manager()) {
    NS_ERROR("Child has wrong manager");
    return false;
  }
  if(aChild->GetParent()) {
    NS_ERROR("aChild already in the tree");
    return false;
  }
  if (aChild->GetNextSibling() || aChild->GetPrevSibling()) {
    NS_ERROR("aChild already has siblings?");
    return false;
  }
  if (aAfter && (aAfter->Manager() != Manager() ||
                 aAfter->GetParent() != this))
  {
    NS_ERROR("aAfter is not our child");
    return false;
  }

  aChild->SetParent(this);
  if (aAfter == mLastChild) {
    mLastChild = aChild;
  }
  if (!aAfter) {
    aChild->SetNextSibling(mFirstChild);
    if (mFirstChild) {
      mFirstChild->SetPrevSibling(aChild);
    }
    mFirstChild = aChild;
    NS_ADDREF(aChild);
    DidInsertChild(aChild);
    return true;
  }

  Layer* next = aAfter->GetNextSibling();
  aChild->SetNextSibling(next);
  aChild->SetPrevSibling(aAfter);
  if (next) {
    next->SetPrevSibling(aChild);
  }
  aAfter->SetNextSibling(aChild);
  NS_ADDREF(aChild);
  DidInsertChild(aChild);
  return true;
}

bool
ContainerLayer::RemoveChild(Layer *aChild)
{
  if (aChild->Manager() != Manager()) {
    NS_ERROR("Child has wrong manager");
    return false;
  }
  if (aChild->GetParent() != this) {
    NS_ERROR("aChild not our child");
    return false;
  }

  Layer* prev = aChild->GetPrevSibling();
  Layer* next = aChild->GetNextSibling();
  if (prev) {
    prev->SetNextSibling(next);
  } else {
    this->mFirstChild = next;
  }
  if (next) {
    next->SetPrevSibling(prev);
  } else {
    this->mLastChild = prev;
  }

  aChild->SetNextSibling(nullptr);
  aChild->SetPrevSibling(nullptr);
  aChild->SetParent(nullptr);

  this->DidRemoveChild(aChild);
  NS_RELEASE(aChild);
  return true;
}


bool
ContainerLayer::RepositionChild(Layer* aChild, Layer* aAfter)
{
  if (aChild->Manager() != Manager()) {
    NS_ERROR("Child has wrong manager");
    return false;
  }
  if (aChild->GetParent() != this) {
    NS_ERROR("aChild not our child");
    return false;
  }
  if (aAfter && (aAfter->Manager() != Manager() ||
                 aAfter->GetParent() != this))
  {
    NS_ERROR("aAfter is not our child");
    return false;
  }
  if (aChild == aAfter) {
    NS_ERROR("aChild cannot be the same as aAfter");
    return false;
  }

  Layer* prev = aChild->GetPrevSibling();
  Layer* next = aChild->GetNextSibling();
  if (prev == aAfter) {
    
    return true;
  }
  if (prev) {
    prev->SetNextSibling(next);
  } else {
    mFirstChild = next;
  }
  if (next) {
    next->SetPrevSibling(prev);
  } else {
    mLastChild = prev;
  }
  if (!aAfter) {
    aChild->SetPrevSibling(nullptr);
    aChild->SetNextSibling(mFirstChild);
    if (mFirstChild) {
      mFirstChild->SetPrevSibling(aChild);
    }
    mFirstChild = aChild;
    return true;
  }

  Layer* afterNext = aAfter->GetNextSibling();
  if (afterNext) {
    afterNext->SetPrevSibling(aChild);
  } else {
    mLastChild = aChild;
  }
  aAfter->SetNextSibling(aChild);
  aChild->SetPrevSibling(aAfter);
  aChild->SetNextSibling(afterNext);
  return true;
}

void
ContainerLayer::FillSpecificAttributes(SpecificLayerAttributes& aAttrs)
{
  aAttrs = ContainerLayerAttributes(GetFrameMetrics(), mScrollHandoffParentId,
                                    mPreXScale, mPreYScale,
                                    mInheritedXScale, mInheritedYScale);
}

bool
ContainerLayer::HasMultipleChildren()
{
  uint32_t count = 0;
  for (Layer* child = GetFirstChild(); child; child = child->GetNextSibling()) {
    const nsIntRect *clipRect = child->GetEffectiveClipRect();
    if (clipRect && clipRect->IsEmpty())
      continue;
    if (child->GetVisibleRegion().IsEmpty())
      continue;
    ++count;
    if (count > 1)
      return true;
  }

  return false;
}

void
ContainerLayer::SortChildrenBy3DZOrder(nsTArray<Layer*>& aArray)
{
  nsAutoTArray<Layer*, 10> toSort;

  for (Layer* l = GetFirstChild(); l; l = l->GetNextSibling()) {
    ContainerLayer* container = l->AsContainerLayer();
    if (container && container->GetContentFlags() & CONTENT_PRESERVE_3D) {
      toSort.AppendElement(l);
    } else {
      if (toSort.Length() > 0) {
        SortLayersBy3DZOrder(toSort);
        aArray.MoveElementsFrom(toSort);
      }
      aArray.AppendElement(l);
    }
  }
  if (toSort.Length() > 0) {
    SortLayersBy3DZOrder(toSort);
    aArray.MoveElementsFrom(toSort);
  }
}

void
ContainerLayer::DefaultComputeEffectiveTransforms(const Matrix4x4& aTransformToSurface)
{
  Matrix residual;
  Matrix4x4 idealTransform = GetLocalTransform() * aTransformToSurface;
  idealTransform.ProjectTo2D();
  mEffectiveTransform = SnapTransformTranslation(idealTransform, &residual);

  bool useIntermediateSurface;
  if (GetMaskLayer() ||
      GetForceIsolatedGroup()) {
    useIntermediateSurface = true;
#ifdef MOZ_DUMP_PAINTING
  } else if (gfxUtils::sDumpPainting) {
    useIntermediateSurface = true;
#endif
  } else {
    float opacity = GetEffectiveOpacity();
    CompositionOp blendMode = GetEffectiveMixBlendMode();
    if ((opacity != 1.0f || blendMode != CompositionOp::OP_OVER) && HasMultipleChildren()) {
      useIntermediateSurface = true;
    } else {
      useIntermediateSurface = false;
      gfx::Matrix contTransform;
      if (!mEffectiveTransform.Is2D(&contTransform) ||
#ifdef MOZ_GFX_OPTIMIZE_MOBILE
        !contTransform.PreservesAxisAlignedRectangles()) {
#else
        gfx::ThebesMatrix(contTransform).HasNonIntegerTranslation()) {
#endif
        for (Layer* child = GetFirstChild(); child; child = child->GetNextSibling()) {
          const nsIntRect *clipRect = child->GetEffectiveClipRect();
          




          if ((clipRect && !clipRect->IsEmpty() && !child->GetVisibleRegion().IsEmpty()) ||
              child->GetMaskLayer()) {
            useIntermediateSurface = true;
            break;
          }
        }
      }
    }
  }

  mUseIntermediateSurface = useIntermediateSurface;
  if (useIntermediateSurface) {
    ComputeEffectiveTransformsForChildren(Matrix4x4::From2D(residual));
  } else {
    ComputeEffectiveTransformsForChildren(idealTransform);
  }

  if (idealTransform.CanDraw2D()) {
    ComputeEffectiveTransformForMaskLayer(aTransformToSurface);
  } else {
    ComputeEffectiveTransformForMaskLayer(Matrix4x4());
  }
}

void
ContainerLayer::DefaultComputeSupportsComponentAlphaChildren(bool* aNeedsSurfaceCopy)
{
  bool supportsComponentAlphaChildren = false;
  bool needsSurfaceCopy = false;
  CompositionOp blendMode = GetEffectiveMixBlendMode();
  if (UseIntermediateSurface()) {
    if (GetEffectiveVisibleRegion().GetNumRects() == 1 &&
        (GetContentFlags() & Layer::CONTENT_OPAQUE))
    {
      supportsComponentAlphaChildren = true;
    } else {
      gfx::Matrix transform;
      if (HasOpaqueAncestorLayer(this) &&
          GetEffectiveTransform().Is2D(&transform) &&
          !gfx::ThebesMatrix(transform).HasNonIntegerTranslation() &&
          blendMode == gfx::CompositionOp::OP_OVER) {
        supportsComponentAlphaChildren = true;
        needsSurfaceCopy = true;
      }
    }
  } else if (blendMode == gfx::CompositionOp::OP_OVER) {
    supportsComponentAlphaChildren =
      (GetContentFlags() & Layer::CONTENT_OPAQUE) ||
      (GetParent() && GetParent()->SupportsComponentAlphaChildren());
  }

  mSupportsComponentAlphaChildren = supportsComponentAlphaChildren &&
                                    gfxPrefs::ComponentAlphaEnabled();
  if (aNeedsSurfaceCopy) {
    *aNeedsSurfaceCopy = mSupportsComponentAlphaChildren && needsSurfaceCopy;
  }
}

void
ContainerLayer::ComputeEffectiveTransformsForChildren(const Matrix4x4& aTransformToSurface)
{
  for (Layer* l = mFirstChild; l; l = l->GetNextSibling()) {
    l->ComputeEffectiveTransforms(aTransformToSurface);
  }
}

 bool
ContainerLayer::HasOpaqueAncestorLayer(Layer* aLayer)
{
  for (Layer* l = aLayer->GetParent(); l; l = l->GetParent()) {
    if (l->GetContentFlags() & Layer::CONTENT_OPAQUE)
      return true;
  }
  return false;
}

void
ContainerLayer::DidRemoveChild(Layer* aLayer)
{
  ThebesLayer* tl = aLayer->AsThebesLayer();
  if (tl && tl->UsedForReadback()) {
    for (Layer* l = mFirstChild; l; l = l->GetNextSibling()) {
      if (l->GetType() == TYPE_READBACK) {
        static_cast<ReadbackLayer*>(l)->NotifyThebesLayerRemoved(tl);
      }
    }
  }
  if (aLayer->GetType() == TYPE_READBACK) {
    static_cast<ReadbackLayer*>(aLayer)->NotifyRemoved();
  }
}

void
ContainerLayer::DidInsertChild(Layer* aLayer)
{
  if (aLayer->GetType() == TYPE_READBACK) {
    mMayHaveReadbackChild = true;
  }
}

void
RefLayer::FillSpecificAttributes(SpecificLayerAttributes& aAttrs)
{
  aAttrs = RefLayerAttributes(GetReferentId());
}



























uint32_t
LayerManager::StartFrameTimeRecording(int32_t aBufferSize)
{
  if (mRecording.mIsPaused) {
    mRecording.mIsPaused = false;

    if (!mRecording.mIntervals.Length()) { 
      mRecording.mIntervals.SetLength(aBufferSize);
    }

    
    mRecording.mLastFrameTime = TimeStamp::Now();

    
    mRecording.mCurrentRunStartIndex = mRecording.mNextIndex;
  }

  
  
  mRecording.mLatestStartIndex = mRecording.mNextIndex;
  return mRecording.mNextIndex;
}

void
LayerManager::RecordFrame()
{
  if (!mRecording.mIsPaused) {
    TimeStamp now = TimeStamp::Now();
    uint32_t i = mRecording.mNextIndex % mRecording.mIntervals.Length();
    mRecording.mIntervals[i] = static_cast<float>((now - mRecording.mLastFrameTime)
                                                  .ToMilliseconds());
    mRecording.mNextIndex++;
    mRecording.mLastFrameTime = now;

    if (mRecording.mNextIndex > (mRecording.mLatestStartIndex + mRecording.mIntervals.Length())) {
      
      mRecording.mIsPaused = true;
    }
  }
}

void
LayerManager::PostPresent()
{
  if (!mTabSwitchStart.IsNull()) {
    Telemetry::Accumulate(Telemetry::FX_TAB_SWITCH_TOTAL_MS,
                          uint32_t((TimeStamp::Now() - mTabSwitchStart).ToMilliseconds()));
    mTabSwitchStart = TimeStamp();
  }
}

void
LayerManager::StopFrameTimeRecording(uint32_t         aStartIndex,
                                     nsTArray<float>& aFrameIntervals)
{
  uint32_t bufferSize = mRecording.mIntervals.Length();
  uint32_t length = mRecording.mNextIndex - aStartIndex;
  if (mRecording.mIsPaused || length > bufferSize || aStartIndex < mRecording.mCurrentRunStartIndex) {
    
    
    length = 0;
  }

  if (!length) {
    aFrameIntervals.Clear();
    return; 
  }
  
  aFrameIntervals.SetLength(length);

  uint32_t cyclicPos = aStartIndex % bufferSize;
  for (uint32_t i = 0; i < length; i++, cyclicPos++) {
    if (cyclicPos == bufferSize) {
      cyclicPos = 0;
    }
    aFrameIntervals[i] = mRecording.mIntervals[cyclicPos];
  }
}

void
LayerManager::BeginTabSwitch()
{
  mTabSwitchStart = TimeStamp::Now();
}

static nsACString& PrintInfo(nsACString& aTo, LayerComposite* aLayerComposite);

#ifdef MOZ_DUMP_PAINTING
template <typename T>
void WriteSnapshotLinkToDumpFile(T* aObj, FILE* aFile)
{
  if (!aObj) {
    return;
  }
  nsCString string(aObj->Name());
  string.Append('-');
  string.AppendInt((uint64_t)aObj);
  fprintf_stderr(aFile, "href=\"javascript:ViewImage('%s')\"", string.BeginReading());
}

template <typename T>
void WriteSnapshotToDumpFile_internal(T* aObj, DataSourceSurface* aSurf)
{
  nsRefPtr<gfxImageSurface> deprecatedSurf =
    new gfxImageSurface(aSurf->GetData(),
                        ThebesIntSize(aSurf->GetSize()),
                        aSurf->Stride(),
                        SurfaceFormatToImageFormat(aSurf->GetFormat()));
  nsCString string(aObj->Name());
  string.Append('-');
  string.AppendInt((uint64_t)aObj);
  if (gfxUtils::sDumpPaintFile) {
    fprintf_stderr(gfxUtils::sDumpPaintFile, "array[\"%s\"]=\"", string.BeginReading());
  }
  deprecatedSurf->DumpAsDataURL(gfxUtils::sDumpPaintFile);
  if (gfxUtils::sDumpPaintFile) {
    fprintf_stderr(gfxUtils::sDumpPaintFile, "\";");
  }
}

void WriteSnapshotToDumpFile(Layer* aLayer, DataSourceSurface* aSurf)
{
  WriteSnapshotToDumpFile_internal(aLayer, aSurf);
}

void WriteSnapshotToDumpFile(LayerManager* aManager, DataSourceSurface* aSurf)
{
  WriteSnapshotToDumpFile_internal(aManager, aSurf);
}

void WriteSnapshotToDumpFile(Compositor* aCompositor, DrawTarget* aTarget)
{
  RefPtr<SourceSurface> surf = aTarget->Snapshot();
  RefPtr<DataSourceSurface> dSurf = surf->GetDataSurface();
  WriteSnapshotToDumpFile_internal(aCompositor, dSurf);
}
#endif

void
Layer::Dump(FILE* aFile, const char* aPrefix, bool aDumpHtml)
{
  if (aDumpHtml) {
    fprintf_stderr(aFile, "<li><a id=\"%p\" ", this);
#ifdef MOZ_DUMP_PAINTING
    if (GetType() == TYPE_CONTAINER || GetType() == TYPE_THEBES) {
      WriteSnapshotLinkToDumpFile(this, aFile);
    }
#endif
    fprintf_stderr(aFile, ">");
  }
  DumpSelf(aFile, aPrefix);

#ifdef MOZ_DUMP_PAINTING
  if (gfxUtils::sDumpPainting && AsLayerComposite() && AsLayerComposite()->GetCompositableHost()) {
    AsLayerComposite()->GetCompositableHost()->Dump(aFile, aPrefix, aDumpHtml);
  }
#endif

  if (aDumpHtml) {
    fprintf_stderr(aFile, "</a>");
  }

  if (Layer* mask = GetMaskLayer()) {
    fprintf_stderr(aFile, "%s  Mask layer:\n", aPrefix);
    nsAutoCString pfx(aPrefix);
    pfx += "    ";
    mask->Dump(aFile, pfx.get(), aDumpHtml);
  }

  if (Layer* kid = GetFirstChild()) {
    nsAutoCString pfx(aPrefix);
    pfx += "  ";
    if (aDumpHtml) {
      fprintf_stderr(aFile, "<ul>");
    }
    kid->Dump(aFile, pfx.get(), aDumpHtml);
    if (aDumpHtml) {
      fprintf_stderr(aFile, "</ul>");
    }
  }

  if (aDumpHtml) {
    fprintf_stderr(aFile, "</li>");
  }
  if (Layer* next = GetNextSibling())
    next->Dump(aFile, aPrefix, aDumpHtml);
}

void
Layer::DumpSelf(FILE* aFile, const char* aPrefix)
{
  nsAutoCString str;
  PrintInfo(str, aPrefix);
  fprintf_stderr(aFile, "%s\n", str.get());
}

void
Layer::Log(const char* aPrefix)
{
  if (!IsLogEnabled())
    return;

  LogSelf(aPrefix);

  if (Layer* kid = GetFirstChild()) {
    nsAutoCString pfx(aPrefix);
    pfx += "  ";
    kid->Log(pfx.get());
  }

  if (Layer* next = GetNextSibling())
    next->Log(aPrefix);
}

void
Layer::LogSelf(const char* aPrefix)
{
  if (!IsLogEnabled())
    return;

  nsAutoCString str;
  PrintInfo(str, aPrefix);
  MOZ_LAYERS_LOG(("%s", str.get()));

  if (mMaskLayer) {
    nsAutoCString pfx(aPrefix);
    pfx += "   \\ MaskLayer ";
    mMaskLayer->LogSelf(pfx.get());
  }
}

nsACString&
Layer::PrintInfo(nsACString& aTo, const char* aPrefix)
{
  aTo += aPrefix;
  aTo += nsPrintfCString("%s%s (0x%p)", mManager->Name(), Name(), this);

  layers::PrintInfo(aTo, AsLayerComposite());

  if (mUseClipRect) {
    AppendToString(aTo, mClipRect, " [clip=", "]");
  }
  if (1.0 != mPostXScale || 1.0 != mPostYScale) {
    aTo.AppendPrintf(" [postScale=%g, %g]", mPostXScale, mPostYScale);
  }
  if (!mTransform.IsIdentity()) {
    AppendToString(aTo, mTransform, " [transform=", "]");
  }
  if (!mVisibleRegion.IsEmpty()) {
    AppendToString(aTo, mVisibleRegion, " [visible=", "]");
  } else {
    aTo += " [not visible]";
  }
  if (!mEventRegions.mHitRegion.IsEmpty()) {
    AppendToString(aTo, mEventRegions.mHitRegion, " [hitregion=", "]");
  }
  if (!mEventRegions.mDispatchToContentHitRegion.IsEmpty()) {
    AppendToString(aTo, mEventRegions.mDispatchToContentHitRegion, " [dispatchtocontentregion=", "]");
  }
  if (1.0 != mOpacity) {
    aTo.AppendPrintf(" [opacity=%g]", mOpacity);
  }
  if (GetContentFlags() & CONTENT_OPAQUE) {
    aTo += " [opaqueContent]";
  }
  if (GetContentFlags() & CONTENT_COMPONENT_ALPHA) {
    aTo += " [componentAlpha]";
  }
  if (GetScrollbarDirection() == VERTICAL) {
    aTo.AppendPrintf(" [vscrollbar=%lld]", GetScrollbarTargetContainerId());
  }
  if (GetScrollbarDirection() == HORIZONTAL) {
    aTo.AppendPrintf(" [hscrollbar=%lld]", GetScrollbarTargetContainerId());
  }
  if (GetIsFixedPosition()) {
    aTo.AppendPrintf(" [isFixedPosition anchor=%f,%f margin=%f,%f,%f,%f]", mAnchor.x, mAnchor.y,
                     mMargins.top, mMargins.right, mMargins.bottom, mMargins.left);
  }
  if (GetIsStickyPosition()) {
    aTo.AppendPrintf(" [isStickyPosition scrollId=%d outer=%f,%f %fx%f "
                     "inner=%f,%f %fx%f]", mStickyPositionData->mScrollId,
                     mStickyPositionData->mOuter.x, mStickyPositionData->mOuter.y,
                     mStickyPositionData->mOuter.width, mStickyPositionData->mOuter.height,
                     mStickyPositionData->mInner.x, mStickyPositionData->mInner.y,
                     mStickyPositionData->mInner.width, mStickyPositionData->mInner.height);
  }
  if (mMaskLayer) {
    aTo.AppendPrintf(" [mMaskLayer=%p]", mMaskLayer.get());
  }

  return aTo;
}

nsACString&
ThebesLayer::PrintInfo(nsACString& aTo, const char* aPrefix)
{
  Layer::PrintInfo(aTo, aPrefix);
  if (!mValidRegion.IsEmpty()) {
    AppendToString(aTo, mValidRegion, " [valid=", "]");
  }
  return aTo;
}

nsACString&
ContainerLayer::PrintInfo(nsACString& aTo, const char* aPrefix)
{
  Layer::PrintInfo(aTo, aPrefix);
  if (!mFrameMetrics.IsDefault()) {
    AppendToString(aTo, mFrameMetrics, " [metrics=", "]");
  }
  if (mScrollHandoffParentId != FrameMetrics::NULL_SCROLL_ID) {
    aTo.AppendPrintf(" [scrollParent=%llu]", mScrollHandoffParentId);
  }
  if (UseIntermediateSurface()) {
    aTo += " [usesTmpSurf]";
  }
  if (1.0 != mPreXScale || 1.0 != mPreYScale) {
    aTo.AppendPrintf(" [preScale=%g, %g]", mPreXScale, mPreYScale);
  }
  return aTo;
}

nsACString&
ColorLayer::PrintInfo(nsACString& aTo, const char* aPrefix)
{
  Layer::PrintInfo(aTo, aPrefix);
  AppendToString(aTo, mColor, " [color=", "]");
  return aTo;
}

nsACString&
CanvasLayer::PrintInfo(nsACString& aTo, const char* aPrefix)
{
  Layer::PrintInfo(aTo, aPrefix);
  if (mFilter != GraphicsFilter::FILTER_GOOD) {
    AppendToString(aTo, mFilter, " [filter=", "]");
  }
  return aTo;
}

nsACString&
ImageLayer::PrintInfo(nsACString& aTo, const char* aPrefix)
{
  Layer::PrintInfo(aTo, aPrefix);
  if (mFilter != GraphicsFilter::FILTER_GOOD) {
    AppendToString(aTo, mFilter, " [filter=", "]");
  }
  return aTo;
}

nsACString&
RefLayer::PrintInfo(nsACString& aTo, const char* aPrefix)
{
  ContainerLayer::PrintInfo(aTo, aPrefix);
  if (0 != mId) {
    AppendToString(aTo, mId, " [id=", "]");
  }
  return aTo;
}

nsACString&
ReadbackLayer::PrintInfo(nsACString& aTo, const char* aPrefix)
{
  Layer::PrintInfo(aTo, aPrefix);
  AppendToString(aTo, mSize, " [size=", "]");
  if (mBackgroundLayer) {
    AppendToString(aTo, mBackgroundLayer, " [backgroundLayer=", "]");
    AppendToString(aTo, mBackgroundLayerOffset, " [backgroundOffset=", "]");
  } else if (mBackgroundColor.a == 1.0) {
    AppendToString(aTo, mBackgroundColor, " [backgroundColor=", "]");
  } else {
    aTo += " [nobackground]";
  }
  return aTo;
}




void
LayerManager::Dump(FILE* aFile, const char* aPrefix, bool aDumpHtml)
{
  FILE* file = FILEOrDefault(aFile);

#ifdef MOZ_DUMP_PAINTING
  if (aDumpHtml) {
    fprintf_stderr(file, "<ul><li><a ");
    WriteSnapshotLinkToDumpFile(this, file);
    fprintf_stderr(file, ">");
  }
#endif
  DumpSelf(file, aPrefix);
#ifdef MOZ_DUMP_PAINTING
  if (aDumpHtml) {
    fprintf_stderr(file, "</a>");
  }
#endif

  nsAutoCString pfx(aPrefix);
  pfx += "  ";
  if (!GetRoot()) {
    fprintf_stderr(file, "%s(null)", pfx.get());
    if (aDumpHtml) {
      fprintf_stderr(file, "</li></ul>");
    }
    return;
  }

  if (aDumpHtml) {
    fprintf_stderr(file, "<ul>");
  }
  GetRoot()->Dump(file, pfx.get(), aDumpHtml);
  if (aDumpHtml) {
    fprintf_stderr(file, "</ul></li></ul>");
  }
  fprintf_stderr(file, "\n");
}

void
LayerManager::DumpSelf(FILE* aFile, const char* aPrefix)
{
  nsAutoCString str;
  PrintInfo(str, aPrefix);
  fprintf_stderr(FILEOrDefault(aFile), "%s\n", str.get());
}

void
LayerManager::Log(const char* aPrefix)
{
  if (!IsLogEnabled())
    return;

  LogSelf(aPrefix);

  nsAutoCString pfx(aPrefix);
  pfx += "  ";
  if (!GetRoot()) {
    MOZ_LAYERS_LOG(("%s(null)", pfx.get()));
    return;
  }

  GetRoot()->Log(pfx.get());
}

void
LayerManager::LogSelf(const char* aPrefix)
{
  nsAutoCString str;
  PrintInfo(str, aPrefix);
  MOZ_LAYERS_LOG(("%s", str.get()));
}

nsACString&
LayerManager::PrintInfo(nsACString& aTo, const char* aPrefix)
{
  aTo += aPrefix;
  return aTo += nsPrintfCString("%sLayerManager (0x%p)", Name(), this);
}

 void
LayerManager::InitLog()
{
  if (!sLog)
    sLog = PR_NewLogModule("Layers");
}

 bool
LayerManager::IsLogEnabled()
{
  NS_ABORT_IF_FALSE(!!sLog,
                    "layer manager must be created before logging is allowed");
  return PR_LOG_TEST(sLog, PR_LOG_DEBUG);
}

static nsACString&
PrintInfo(nsACString& aTo, LayerComposite* aLayerComposite)
{
  if (!aLayerComposite) {
    return aTo;
  }
  if (const nsIntRect* clipRect = aLayerComposite->GetShadowClipRect()) {
    AppendToString(aTo, *clipRect, " [shadow-clip=", "]");
  }
  if (!aLayerComposite->GetShadowTransform().IsIdentity()) {
    AppendToString(aTo, aLayerComposite->GetShadowTransform(), " [shadow-transform=", "]");
  }
  if (!aLayerComposite->GetShadowVisibleRegion().IsEmpty()) {
    AppendToString(aTo, aLayerComposite->GetShadowVisibleRegion(), " [shadow-visible=", "]");
  }
  return aTo;
}

void
SetAntialiasingFlags(Layer* aLayer, DrawTarget* aTarget)
{
  bool permitSubpixelAA = !(aLayer->GetContentFlags() & Layer::CONTENT_DISABLE_SUBPIXEL_AA);
  if (aTarget->GetFormat() != SurfaceFormat::B8G8R8A8) {
    aTarget->SetPermitSubpixelAA(permitSubpixelAA);
    return;
  }

  const nsIntRect& bounds = aLayer->GetVisibleRegion().GetBounds();
  gfx::Rect transformedBounds = aTarget->GetTransform().TransformBounds(gfx::Rect(Float(bounds.x), Float(bounds.y),
                                                                                  Float(bounds.width), Float(bounds.height)));
  transformedBounds.RoundOut();
  IntRect intTransformedBounds;
  transformedBounds.ToIntRect(&intTransformedBounds);
  permitSubpixelAA &= !(aLayer->GetContentFlags() & Layer::CONTENT_COMPONENT_ALPHA) ||
                      aTarget->GetOpaqueRect().Contains(intTransformedBounds);
  aTarget->SetPermitSubpixelAA(permitSubpixelAA);
}

void
SetAntialiasingFlags(Layer* aLayer, gfxContext* aTarget)
{
  if (!aTarget->IsCairo()) {
    SetAntialiasingFlags(aLayer, aTarget->GetDrawTarget());
    return;
  }

  bool permitSubpixelAA = !(aLayer->GetContentFlags() & Layer::CONTENT_DISABLE_SUBPIXEL_AA);
  nsRefPtr<gfxASurface> surface = aTarget->CurrentSurface();
  if (surface->GetContentType() != gfxContentType::COLOR_ALPHA) {
    
    surface->SetSubpixelAntialiasingEnabled(permitSubpixelAA);
    return;
  }

  const nsIntRect& bounds = aLayer->GetVisibleRegion().GetBounds();
  permitSubpixelAA &= !(aLayer->GetContentFlags() & Layer::CONTENT_COMPONENT_ALPHA) ||
      surface->GetOpaqueRect().Contains(
      aTarget->UserToDevice(gfxRect(bounds.x, bounds.y, bounds.width, bounds.height)));
  surface->SetSubpixelAntialiasingEnabled(permitSubpixelAA);
}

PRLogModuleInfo* LayerManager::sLog;

} 
} 
