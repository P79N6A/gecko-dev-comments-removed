




#include "mozilla/dom/TabChild.h"
#include "mozilla/Hal.h"
#include "mozilla/layers/PLayerChild.h"
#include "mozilla/layers/PLayersChild.h"
#include "mozilla/layers/PLayersParent.h"

#include "gfxSharedImageSurface.h"
#include "gfxImageSurface.h"
#include "gfxUtils.h"
#include "gfxPlatform.h"
#include "nsXULAppAPI.h"
#include "RenderTrace.h"
#include "sampler.h"

#define PIXMAN_DONT_DEFINE_STDINT
#include "pixman.h"

#include "BasicTiledThebesLayer.h"
#include "BasicLayersImpl.h"
#include "BasicThebesLayer.h"
#include "BasicContainerLayer.h"
#include "CompositorChild.h"
#include "mozilla/Preferences.h"
#include "nsIWidget.h"

#ifdef MOZ_WIDGET_ANDROID
#include "AndroidBridge.h"
#endif

using namespace mozilla::dom;
using namespace mozilla::gfx;

namespace mozilla {
namespace layers {







static bool
ClipToContain(gfxContext* aContext, const nsIntRect& aRect)
{
  gfxRect userRect(aRect.x, aRect.y, aRect.width, aRect.height);
  gfxRect deviceRect = aContext->UserToDevice(userRect);
  deviceRect.RoundOut();

  gfxMatrix currentMatrix = aContext->CurrentMatrix();
  aContext->IdentityMatrix();
  aContext->NewPath();
  aContext->Rectangle(deviceRect);
  aContext->Clip();
  aContext->SetMatrix(currentMatrix);

  return aContext->DeviceToUser(deviceRect).IsEqualInterior(userRect);
}

already_AddRefed<gfxContext>
BasicLayerManager::PushGroupForLayer(gfxContext* aContext, Layer* aLayer,
                                     const nsIntRegion& aRegion,
                                     bool* aNeedsClipToVisibleRegion)
{
  
  
  bool didCompleteClip = ClipToContain(aContext, aRegion.GetBounds());

  nsRefPtr<gfxContext> result;
  if (aLayer->CanUseOpaqueSurface() &&
      ((didCompleteClip && aRegion.GetNumRects() == 1) ||
       !aContext->CurrentMatrix().HasNonIntegerTranslation())) {
    
    
    
    
    *aNeedsClipToVisibleRegion = !didCompleteClip || aRegion.GetNumRects() > 1;
    result = PushGroupWithCachedSurface(aContext, gfxASurface::CONTENT_COLOR);
  } else {
    *aNeedsClipToVisibleRegion = false;
    result = aContext;
    if (aLayer->GetContentFlags() & Layer::CONTENT_COMPONENT_ALPHA) {
      aContext->PushGroupAndCopyBackground(gfxASurface::CONTENT_COLOR_ALPHA);
    } else {
      aContext->PushGroup(gfxASurface::CONTENT_COLOR_ALPHA);
    }
  }
  return result.forget();
}

static nsIntRect
ToOutsideIntRect(const gfxRect &aRect)
{
  gfxRect r = aRect;
  r.RoundOut();
  return nsIntRect(r.X(), r.Y(), r.Width(), r.Height());
}

static nsIntRect
ToInsideIntRect(const gfxRect& aRect)
{
  gfxRect r = aRect;
  r.RoundIn();
  return nsIntRect(r.X(), r.Y(), r.Width(), r.Height());
}






class PaintContext {
public:
  PaintContext(gfxContext* aTarget, Layer* aLayer,
               LayerManager::DrawThebesLayerCallback aCallback,
               void* aCallbackData, ReadbackProcessor* aReadback)
   : mTarget(aTarget)
   , mTargetMatrixSR(aTarget)
   , mLayer(aLayer)
   , mCallback(aCallback)
   , mCallbackData(aCallbackData)
   , mReadback(aReadback)
   , mPushedOpaqueRect(false)
  {}

  ~PaintContext()
  {
    
    if (mPushedOpaqueRect)
    {
      ClearOpaqueRect();
    }
  }

  
  
  bool Setup2DTransform()
  {
    const gfx3DMatrix& effectiveTransform = mLayer->GetEffectiveTransform();
    
    return effectiveTransform.CanDraw2D(&mTransform);
  }

  
  
  void Apply2DTransform()
  {
    mTarget->SetMatrix(mTransform);
  }

  
  void AnnotateOpaqueRect()
  {
    const nsIntRegion& visibleRegion = mLayer->GetEffectiveVisibleRegion();
    const nsIntRect& bounds = visibleRegion.GetBounds();
    nsRefPtr<gfxASurface> currentSurface = mTarget->CurrentSurface();

    if (mTarget->IsCairo()) {
      const gfxRect& targetOpaqueRect = currentSurface->GetOpaqueRect();

      
      
      if (targetOpaqueRect.IsEmpty() && visibleRegion.GetNumRects() == 1 &&
          (mLayer->GetContentFlags() & Layer::CONTENT_OPAQUE) &&
          !mTransform.HasNonAxisAlignedTransform()) {
        currentSurface->SetOpaqueRect(
            mTarget->UserToDevice(gfxRect(bounds.x, bounds.y, bounds.width, bounds.height)));
        mPushedOpaqueRect = true;
      }
    } else {
      DrawTarget *dt = mTarget->GetDrawTarget();
      const IntRect& targetOpaqueRect = dt->GetOpaqueRect();

      
      
      if (targetOpaqueRect.IsEmpty() && visibleRegion.GetNumRects() == 1 &&
          (mLayer->GetContentFlags() & Layer::CONTENT_OPAQUE) &&
          !mTransform.HasNonAxisAlignedTransform()) {

        gfx::Rect opaqueRect = dt->GetTransform().TransformBounds(
          gfx::Rect(bounds.x, bounds.y, bounds.width, bounds.height));
        opaqueRect.RoundIn();
        IntRect intOpaqueRect;
        if (opaqueRect.ToIntRect(&intOpaqueRect)) {
          mTarget->GetDrawTarget()->SetOpaqueRect(intOpaqueRect);
          mPushedOpaqueRect = true;
        }
      }
    }
  }

  
  
  
  void ClearOpaqueRect() {
    if (mTarget->IsCairo()) {
      nsRefPtr<gfxASurface> currentSurface = mTarget->CurrentSurface();
      currentSurface->SetOpaqueRect(gfxRect());
    } else {
      mTarget->GetDrawTarget()->SetOpaqueRect(IntRect());
    }
  }

  gfxContext* mTarget;
  gfxContextMatrixAutoSaveRestore mTargetMatrixSR;
  Layer* mLayer;
  LayerManager::DrawThebesLayerCallback mCallback;
  void* mCallbackData;
  ReadbackProcessor* mReadback;
  gfxMatrix mTransform;
  bool mPushedOpaqueRect;
};

BasicLayerManager::BasicLayerManager(nsIWidget* aWidget) :
  mPhase(PHASE_NONE),
  mWidget(aWidget)
  , mDoubleBuffering(BUFFER_NONE), mUsingDefaultTarget(false)
  , mCachedSurfaceInUse(false)
  , mTransactionIncomplete(false)
  , mCompositorMightResample(false)
{
  MOZ_COUNT_CTOR(BasicLayerManager);
  NS_ASSERTION(aWidget, "Must provide a widget");
}

BasicLayerManager::BasicLayerManager() :
  mPhase(PHASE_NONE),
  mWidget(nullptr)
  , mDoubleBuffering(BUFFER_NONE), mUsingDefaultTarget(false)
  , mCachedSurfaceInUse(false)
  , mTransactionIncomplete(false)
{
  MOZ_COUNT_CTOR(BasicLayerManager);
}

BasicLayerManager::~BasicLayerManager()
{
  NS_ASSERTION(!InTransaction(), "Died during transaction?");

  ClearCachedResources();

  mRoot = nullptr;

  MOZ_COUNT_DTOR(BasicLayerManager);
}

void
BasicLayerManager::SetDefaultTarget(gfxContext* aContext)
{
  NS_ASSERTION(!InTransaction(),
               "Must set default target outside transaction");
  mDefaultTarget = aContext;
}

void
BasicLayerManager::SetDefaultTargetConfiguration(BufferMode aDoubleBuffering, ScreenRotation aRotation)
{
  mDoubleBuffering = aDoubleBuffering;
}

void
BasicLayerManager::BeginTransaction()
{
  mInTransaction = true;
  mUsingDefaultTarget = true;
  BeginTransactionWithTarget(mDefaultTarget);
}

already_AddRefed<gfxContext>
BasicLayerManager::PushGroupWithCachedSurface(gfxContext *aTarget,
                                              gfxASurface::gfxContentType aContent)
{
  nsRefPtr<gfxContext> ctx;
  
  if (!mCachedSurfaceInUse && aTarget->IsCairo()) {
    gfxContextMatrixAutoSaveRestore saveMatrix(aTarget);
    aTarget->IdentityMatrix();

    nsRefPtr<gfxASurface> currentSurf = aTarget->CurrentSurface();
    gfxRect clip = aTarget->GetClipExtents();
    clip.RoundOut();

    ctx = mCachedSurface.Get(aContent, clip, currentSurf);

    if (ctx) {
      mCachedSurfaceInUse = true;
      
      ctx->SetMatrix(saveMatrix.Matrix());
      return ctx.forget();
    }
  }

  ctx = aTarget;
  ctx->PushGroup(aContent);
  return ctx.forget();
}

void
BasicLayerManager::PopGroupToSourceWithCachedSurface(gfxContext *aTarget, gfxContext *aPushed)
{
  if (!aTarget)
    return;
  nsRefPtr<gfxASurface> current = aPushed->CurrentSurface();
  if (aTarget->IsCairo() && mCachedSurface.IsSurface(current)) {
    gfxContextMatrixAutoSaveRestore saveMatrix(aTarget);
    aTarget->IdentityMatrix();
    aTarget->SetSource(current);
    mCachedSurfaceInUse = false;
  } else {
    aTarget->PopGroupToSource();
  }
}

void
BasicLayerManager::BeginTransactionWithTarget(gfxContext* aTarget)
{
  mInTransaction = true;

#ifdef MOZ_LAYERS_HAVE_LOG
  MOZ_LAYERS_LOG(("[----- BeginTransaction"));
  Log();
#endif

  NS_ASSERTION(!InTransaction(), "Nested transactions not allowed");
  mPhase = PHASE_CONSTRUCTION;
  mTarget = aTarget;
}

static void
TransformIntRect(nsIntRect& aRect, const gfxMatrix& aMatrix,
                 nsIntRect (*aRoundMethod)(const gfxRect&))
{
  gfxRect gr = gfxRect(aRect.x, aRect.y, aRect.width, aRect.height);
  gr = aMatrix.TransformBounds(gr);
  aRect = (*aRoundMethod)(gr);
}

















enum {
    ALLOW_OPAQUE = 0x01,
};
static void
MarkLayersHidden(Layer* aLayer, const nsIntRect& aClipRect,
                 const nsIntRect& aDirtyRect,
                 nsIntRegion& aOpaqueRegion,
                 uint32_t aFlags)
{
  nsIntRect newClipRect(aClipRect);
  uint32_t newFlags = aFlags;

  
  
  if (aLayer->GetOpacity() != 1.0f) {
    newFlags &= ~ALLOW_OPAQUE;
  }

  {
    const nsIntRect* clipRect = aLayer->GetEffectiveClipRect();
    if (clipRect) {
      nsIntRect cr = *clipRect;
      
      
      if (aLayer->GetParent()) {
        gfxMatrix tr;
        if (aLayer->GetParent()->GetEffectiveTransform().CanDraw2D(&tr)) {
          
          
          TransformIntRect(cr, tr, ToInsideIntRect);
        } else {
          cr.SetRect(0, 0, 0, 0);
        }
      }
      newClipRect.IntersectRect(newClipRect, cr);
    }
  }

  BasicImplData* data = ToData(aLayer);
  data->SetOperator(gfxContext::OPERATOR_OVER);
  data->SetClipToVisibleRegion(false);
  data->SetDrawAtomically(false);

  if (!aLayer->AsContainerLayer()) {
    gfxMatrix transform;
    if (!aLayer->GetEffectiveTransform().CanDraw2D(&transform)) {
      data->SetHidden(false);
      return;
    }

    nsIntRegion region = aLayer->GetEffectiveVisibleRegion();
    nsIntRect r = region.GetBounds();
    TransformIntRect(r, transform, ToOutsideIntRect);
    r.IntersectRect(r, aDirtyRect);
    data->SetHidden(aOpaqueRegion.Contains(r));

    
    
    if ((aLayer->GetContentFlags() & Layer::CONTENT_OPAQUE) &&
        (newFlags & ALLOW_OPAQUE)) {
      nsIntRegionRectIterator it(region);
      while (const nsIntRect* sr = it.Next()) {
        r = *sr;
        TransformIntRect(r, transform, ToInsideIntRect);

        r.IntersectRect(r, newClipRect);
        aOpaqueRegion.Or(aOpaqueRegion, r);
      }
    }
  } else {
    Layer* child = aLayer->GetLastChild();
    bool allHidden = true;
    for (; child; child = child->GetPrevSibling()) {
      MarkLayersHidden(child, newClipRect, aDirtyRect, aOpaqueRegion, newFlags);
      if (!ToData(child)->IsHidden()) {
        allHidden = false;
      }
    }
    data->SetHidden(allHidden);
  }
}








static void
ApplyDoubleBuffering(Layer* aLayer, const nsIntRect& aVisibleRect)
{
  BasicImplData* data = ToData(aLayer);
  if (data->IsHidden())
    return;

  nsIntRect newVisibleRect(aVisibleRect);

  {
    const nsIntRect* clipRect = aLayer->GetEffectiveClipRect();
    if (clipRect) {
      nsIntRect cr = *clipRect;
      
      
      if (aLayer->GetParent()) {
        gfxMatrix tr;
        if (aLayer->GetParent()->GetEffectiveTransform().CanDraw2D(&tr)) {
          NS_ASSERTION(!tr.HasNonIntegerTranslation(),
                       "Parent can only have an integer translation");
          cr += nsIntPoint(int32_t(tr.x0), int32_t(tr.y0));
        } else {
          NS_ERROR("Parent can only have an integer translation");
        }
      }
      newVisibleRect.IntersectRect(newVisibleRect, cr);
    }
  }

  BasicContainerLayer* container =
    static_cast<BasicContainerLayer*>(aLayer->AsContainerLayer());
  
  
  
  if (!container) {
    data->SetOperator(gfxContext::OPERATOR_SOURCE);
    data->SetDrawAtomically(true);
  } else {
    if (container->UseIntermediateSurface() ||
        !container->ChildrenPartitionVisibleRegion(newVisibleRect)) {
      
      data->SetOperator(gfxContext::OPERATOR_SOURCE);
      container->ForceIntermediateSurface();
    } else {
      
      
      for (Layer* child = aLayer->GetFirstChild(); child;
           child = child->GetNextSibling()) {
        ToData(child)->SetClipToVisibleRegion(true);
        ApplyDoubleBuffering(child, newVisibleRect);
      }
    }
  }
}

void
BasicLayerManager::EndTransaction(DrawThebesLayerCallback aCallback,
                                  void* aCallbackData,
                                  EndTransactionFlags aFlags)
{
  mInTransaction = false;

  EndTransactionInternal(aCallback, aCallbackData, aFlags);
}

void
BasicLayerManager::AbortTransaction()
{
  NS_ASSERTION(InConstruction(), "Should be in construction phase");
  mPhase = PHASE_NONE;
  mUsingDefaultTarget = false;
  mInTransaction = false;
}

bool
BasicLayerManager::EndTransactionInternal(DrawThebesLayerCallback aCallback,
                                          void* aCallbackData,
                                          EndTransactionFlags aFlags)
{
  PROFILER_LABEL("BasicLayerManager", "EndTranscationInternal");
#ifdef MOZ_LAYERS_HAVE_LOG
  MOZ_LAYERS_LOG(("  ----- (beginning paint)"));
  Log();
#endif

  NS_ASSERTION(InConstruction(), "Should be in construction phase");
  mPhase = PHASE_DRAWING;

  Layer* aLayer = GetRoot();
  RenderTraceLayers(aLayer, "FF00");

  mTransactionIncomplete = false;

  if (aFlags & END_NO_COMPOSITE) {
    if (!mDummyTarget) {
      
      
      nsRefPtr<gfxASurface> surf = gfxPlatform::GetPlatform()->CreateOffscreenSurface(gfxIntSize(1, 1), gfxASurface::CONTENT_COLOR);
      mDummyTarget = new gfxContext(surf);
    }
    mTarget = mDummyTarget;
  }

  if (mTarget && mRoot && !(aFlags & END_NO_IMMEDIATE_REDRAW)) {
    nsIntRect clipRect;
    if (HasShadowManager()) {
      
      
      const nsIntRect& bounds = mRoot->GetVisibleRegion().GetBounds();
      gfxRect deviceRect =
          mTarget->UserToDevice(gfxRect(bounds.x, bounds.y, bounds.width, bounds.height));
      clipRect = ToOutsideIntRect(deviceRect);
    } else {
      gfxContextMatrixAutoSaveRestore save(mTarget);
      mTarget->SetMatrix(gfxMatrix());
      clipRect = ToOutsideIntRect(mTarget->GetClipExtents());
    }

    if (aFlags & END_NO_COMPOSITE) {
      
      
      aLayer->ApplyPendingUpdatesToSubtree();
    }

    
    
    mSnapEffectiveTransforms =
      !(mTarget->GetFlags() & gfxContext::FLAG_DISABLE_SNAPPING);
    mRoot->ComputeEffectiveTransforms(gfx3DMatrix::From2D(mTarget->CurrentMatrix()));

    if (IsRetained()) {
      nsIntRegion region;
      MarkLayersHidden(mRoot, clipRect, clipRect, region, ALLOW_OPAQUE);
      if (mUsingDefaultTarget && mDoubleBuffering != BUFFER_NONE) {
        ApplyDoubleBuffering(mRoot, clipRect);
      }
    }

    if (aFlags & END_NO_COMPOSITE) {
      if (IsRetained()) {
        
        
        mTarget->Clip(gfxRect(0, 0, 0, 0));
        PaintLayer(mTarget, mRoot, aCallback, aCallbackData, nullptr);
      }
      
    } else {
      PaintLayer(mTarget, mRoot, aCallback, aCallbackData, nullptr);
      if (mWidget) {
        FlashWidgetUpdateArea(mTarget);
      }
      LayerManager::PostPresent();
    }

    if (!mTransactionIncomplete) {
      
      mTarget = nullptr;
    }
  }

#ifdef MOZ_LAYERS_HAVE_LOG
  Log();
  MOZ_LAYERS_LOG(("]----- EndTransaction"));
#endif

  
  
  mPhase = mTransactionIncomplete ? PHASE_CONSTRUCTION : PHASE_NONE;

  if (!mTransactionIncomplete) {
    
    mUsingDefaultTarget = false;
  }

  NS_ASSERTION(!aCallback || !mTransactionIncomplete,
               "If callback is not null, transaction must be complete");

  
  

  return !mTransactionIncomplete;
}

void
BasicLayerManager::FlashWidgetUpdateArea(gfxContext *aContext)
{
  if (gfxPlatform::GetPlatform()->WidgetUpdateFlashing()) {
    float r = float(rand()) / RAND_MAX;
    float g = float(rand()) / RAND_MAX;
    float b = float(rand()) / RAND_MAX;
    aContext->SetColor(gfxRGBA(r, g, b, 0.2));
    aContext->Paint();
  }
}

bool
BasicLayerManager::EndEmptyTransaction(EndTransactionFlags aFlags)
{
  mInTransaction = false;

  if (!mRoot) {
    return false;
  }

  return EndTransactionInternal(nullptr, nullptr, aFlags);
}

void
BasicLayerManager::SetRoot(Layer* aLayer)
{
  NS_ASSERTION(aLayer, "Root can't be null");
  NS_ASSERTION(aLayer->Manager() == this, "Wrong manager");
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  mRoot = aLayer;
}

static pixman_transform
Matrix3DToPixman(const gfx3DMatrix& aMatrix)
{
  pixman_f_transform transform;

  transform.m[0][0] = aMatrix._11;
  transform.m[0][1] = aMatrix._21;
  transform.m[0][2] = aMatrix._41;
  transform.m[1][0] = aMatrix._12;
  transform.m[1][1] = aMatrix._22;
  transform.m[1][2] = aMatrix._42;
  transform.m[2][0] = aMatrix._14;
  transform.m[2][1] = aMatrix._24;
  transform.m[2][2] = aMatrix._44;

  pixman_transform result;
  pixman_transform_from_pixman_f_transform(&result, &transform);

  return result;
}

static void
PixmanTransform(const gfxImageSurface *aDest, 
                const gfxImageSurface *aSrc, 
                const gfx3DMatrix& aTransform, 
                gfxPoint aDestOffset)
{
  gfxIntSize destSize = aDest->GetSize();
  pixman_image_t* dest = pixman_image_create_bits(aDest->Format() == gfxASurface::ImageFormatARGB32 ? PIXMAN_a8r8g8b8 : PIXMAN_x8r8g8b8,
                                                  destSize.width,
                                                  destSize.height,
                                                  (uint32_t*)aDest->Data(),
                                                  aDest->Stride());

  gfxIntSize srcSize = aSrc->GetSize();
  pixman_image_t* src = pixman_image_create_bits(aSrc->Format() == gfxASurface::ImageFormatARGB32 ? PIXMAN_a8r8g8b8 : PIXMAN_x8r8g8b8,
                                                 srcSize.width,
                                                 srcSize.height,
                                                 (uint32_t*)aSrc->Data(),
                                                 aSrc->Stride());

  NS_ABORT_IF_FALSE(src && dest, "Failed to create pixman images?");

  pixman_transform pixTransform = Matrix3DToPixman(aTransform);
  pixman_transform pixTransformInverted;

  
  if (!pixman_transform_invert(&pixTransformInverted, &pixTransform)) {
    return;
  }
  pixman_image_set_transform(src, &pixTransformInverted);

  pixman_image_composite32(PIXMAN_OP_SRC,
                           src,
                           nullptr,
                           dest,
                           aDestOffset.x,
                           aDestOffset.y,
                           0,
                           0,
                           0,
                           0,
                           destSize.width,
                           destSize.height);

  pixman_image_unref(dest);
  pixman_image_unref(src);
}















static already_AddRefed<gfxASurface> 
Transform3D(gfxASurface* aSource, gfxContext* aDest, 
            const gfxRect& aBounds, const gfx3DMatrix& aTransform, 
            gfxRect& aDestRect, bool aDontBlit)
{
  nsRefPtr<gfxImageSurface> sourceImage = aSource->GetAsImageSurface();
  if (!sourceImage) {
    sourceImage = new gfxImageSurface(gfxIntSize(aBounds.width, aBounds.height), gfxPlatform::GetPlatform()->OptimalFormatForContent(aSource->GetContentType()));
    nsRefPtr<gfxContext> ctx = new gfxContext(sourceImage);

    aSource->SetDeviceOffset(gfxPoint(0, 0));
    ctx->SetSource(aSource);
    ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
    ctx->Paint();
  }

  
  gfxRect offsetRect = aTransform.TransformBounds(aBounds);

  
  
  aDestRect = aDest->GetClipExtents();
  aDestRect.IntersectRect(aDestRect, offsetRect);
  aDestRect.RoundOut();

  
  nsRefPtr<gfxASurface> dest = aDest->CurrentSurface();
  nsRefPtr<gfxImageSurface> destImage;
  gfxPoint offset;
  bool blitComplete;
  if (!destImage || aDontBlit || !aDest->ClipContainsRect(aDestRect)) {
    destImage = new gfxImageSurface(gfxIntSize(aDestRect.width, aDestRect.height),
                                    gfxASurface::ImageFormatARGB32);
    offset = aDestRect.TopLeft();
    blitComplete = false;
  } else {
    offset = -dest->GetDeviceOffset();
    blitComplete = true;
  }

  
  gfx3DMatrix translation = gfx3DMatrix::Translation(aBounds.x, aBounds.y, 0);

  
  PixmanTransform(destImage, sourceImage, translation * aTransform, offset);

  if (blitComplete) {
    return nullptr;
  }

  
  
  return destImage.forget(); 
}

void
BasicLayerManager::PaintSelfOrChildren(PaintContext& aPaintContext,
                                       gfxContext* aGroupTarget)
{
  BasicImplData* data = ToData(aPaintContext.mLayer);

  if (aPaintContext.mLayer->GetFirstChild() &&
      aPaintContext.mLayer->GetMaskLayer() &&
      HasShadowManager()) {
    
    static_cast<BasicImplData*>(aPaintContext.mLayer->GetMaskLayer()->ImplData())
      ->Paint(nullptr, nullptr);
  }

  
  Layer* child = aPaintContext.mLayer->GetFirstChild();
  if (!child) {
    if (aPaintContext.mLayer->AsThebesLayer()) {
      data->PaintThebes(aGroupTarget, aPaintContext.mLayer->GetMaskLayer(),
          aPaintContext.mCallback, aPaintContext.mCallbackData,
          aPaintContext.mReadback);
    } else {
      data->Paint(aGroupTarget, aPaintContext.mLayer->GetMaskLayer());
    }
  } else {
    ReadbackProcessor readback;
    ContainerLayer* container =
        static_cast<ContainerLayer*>(aPaintContext.mLayer);
    if (IsRetained()) {
      readback.BuildUpdates(container);
    }
    nsAutoTArray<Layer*, 12> children;
    container->SortChildrenBy3DZOrder(children);
    for (uint32_t i = 0; i < children.Length(); i++) {
      PaintLayer(aGroupTarget, children.ElementAt(i), aPaintContext.mCallback,
          aPaintContext.mCallbackData, &readback);
      if (mTransactionIncomplete)
        break;
    }
  }
}

void
BasicLayerManager::FlushGroup(PaintContext& aPaintContext, bool aNeedsClipToVisibleRegion)
{
  
  
  
  
  
  
  
  
  
  if (!mTransactionIncomplete) {
    if (aNeedsClipToVisibleRegion) {
      gfxUtils::ClipToRegion(aPaintContext.mTarget,
                             aPaintContext.mLayer->GetEffectiveVisibleRegion());
    }
    BasicContainerLayer* container = static_cast<BasicContainerLayer*>(aPaintContext.mLayer);
    AutoSetOperator setOperator(aPaintContext.mTarget, container->GetOperator());
    PaintWithMask(aPaintContext.mTarget, aPaintContext.mLayer->GetEffectiveOpacity(),
                  HasShadowManager() ? nullptr : aPaintContext.mLayer->GetMaskLayer());
  }
}

void
BasicLayerManager::PaintLayer(gfxContext* aTarget,
                              Layer* aLayer,
                              DrawThebesLayerCallback aCallback,
                              void* aCallbackData,
                              ReadbackProcessor* aReadback)
{
  PaintContext paintContext(aTarget, aLayer, aCallback, aCallbackData, aReadback);

  
  
  if (aLayer->GetEffectiveTransform().IsSingular()) {
    return;
  }

  RenderTraceScope trace("BasicLayerManager::PaintLayer", "707070");

  const nsIntRect* clipRect = aLayer->GetEffectiveClipRect();
  
  
  BasicContainerLayer* container = static_cast<BasicContainerLayer*>(aLayer);
  bool needsGroup = aLayer->GetFirstChild() &&
                    container->UseIntermediateSurface();
  BasicImplData* data = ToData(aLayer);
  bool needsClipToVisibleRegion =
    data->GetClipToVisibleRegion() && !aLayer->AsThebesLayer();
  NS_ASSERTION(needsGroup || !aLayer->GetFirstChild() ||
               container->GetOperator() == gfxContext::OPERATOR_OVER,
               "non-OVER operator should have forced UseIntermediateSurface");
  NS_ASSERTION(!aLayer->GetFirstChild() || !aLayer->GetMaskLayer() ||
               container->UseIntermediateSurface(),
               "ContainerLayer with mask layer should force UseIntermediateSurface");

  gfxContextAutoSaveRestore contextSR;
  gfxMatrix transform;
  
  bool is2D = paintContext.Setup2DTransform();
  NS_ABORT_IF_FALSE(is2D || needsGroup || !aLayer->GetFirstChild(), "Must PushGroup for 3d transforms!");

  bool needsSaveRestore =
    needsGroup || clipRect || needsClipToVisibleRegion || !is2D;
  if (needsSaveRestore) {
    contextSR.SetContext(aTarget);

    if (clipRect) {
      aTarget->NewPath();
      aTarget->Rectangle(gfxRect(clipRect->x, clipRect->y, clipRect->width, clipRect->height), true);
      aTarget->Clip();
    }
  }

  paintContext.Apply2DTransform();

  const nsIntRegion& visibleRegion = aLayer->GetEffectiveVisibleRegion();
  
  if (needsClipToVisibleRegion && !needsGroup) {
    gfxUtils::ClipToRegion(aTarget, visibleRegion);
    
    needsClipToVisibleRegion = false;
  }
  
  if (is2D) {
    paintContext.AnnotateOpaqueRect();
  }

  bool clipIsEmpty = !aTarget || aTarget->GetClipExtents().IsEmpty();
  if (clipIsEmpty) {
    PaintSelfOrChildren(paintContext, aTarget);
    return;
  }

  if (is2D) {
    if (needsGroup) {
      nsRefPtr<gfxContext> groupTarget = PushGroupForLayer(aTarget, aLayer, aLayer->GetEffectiveVisibleRegion(),
                                      &needsClipToVisibleRegion);
      PaintSelfOrChildren(paintContext, groupTarget);
      PopGroupToSourceWithCachedSurface(aTarget, groupTarget);
      FlushGroup(paintContext, needsClipToVisibleRegion);
    } else {
      PaintSelfOrChildren(paintContext, aTarget);
    }
  } else {
    const nsIntRect& bounds = visibleRegion.GetBounds();
    nsRefPtr<gfxASurface> untransformedSurface =
      gfxPlatform::GetPlatform()->CreateOffscreenSurface(gfxIntSize(bounds.width, bounds.height),
                                                         gfxASurface::CONTENT_COLOR_ALPHA);
    if (!untransformedSurface) {
      return;
    }
    untransformedSurface->SetDeviceOffset(gfxPoint(-bounds.x, -bounds.y));
    nsRefPtr<gfxContext> groupTarget = new gfxContext(untransformedSurface);

    PaintSelfOrChildren(paintContext, groupTarget);

    
    
    NS_ABORT_IF_FALSE(untransformedSurface,
                      "We should always allocate an untransformed surface with 3d transforms!");
    gfxRect destRect;
    bool dontBlit = needsClipToVisibleRegion || mTransactionIncomplete ||
                      aLayer->GetEffectiveOpacity() != 1.0f;
#ifdef DEBUG
    if (aLayer->GetDebugColorIndex() != 0) {
      gfxRGBA  color((aLayer->GetDebugColorIndex() & 1) ? 1.0 : 0.0,
                     (aLayer->GetDebugColorIndex() & 2) ? 1.0 : 0.0,
                     (aLayer->GetDebugColorIndex() & 4) ? 1.0 : 0.0,
                     1.0);

      nsRefPtr<gfxContext> temp = new gfxContext(untransformedSurface);
      temp->SetColor(color);
      temp->Paint();
    }
#endif
    const gfx3DMatrix& effectiveTransform = aLayer->GetEffectiveTransform();
    nsRefPtr<gfxASurface> result =
      Transform3D(untransformedSurface, aTarget, bounds,
                  effectiveTransform, destRect, dontBlit);

    if (result) {
      aTarget->SetSource(result, destRect.TopLeft());
      
      
      
      aTarget->NewPath();
      aTarget->Rectangle(destRect, true);
      aTarget->Clip();
      FlushGroup(paintContext, needsClipToVisibleRegion);
    }
  }
}

void
BasicLayerManager::ClearCachedResources(Layer* aSubtree)
{
  MOZ_ASSERT(!aSubtree || aSubtree->Manager() == this);
  if (aSubtree) {
    ClearLayer(aSubtree);
  } else if (mRoot) {
    ClearLayer(mRoot);
  }
  mCachedSurface.Expire();
}
void
BasicLayerManager::ClearLayer(Layer* aLayer)
{
  ToData(aLayer)->ClearCachedResources();
  for (Layer* child = aLayer->GetFirstChild(); child;
       child = child->GetNextSibling()) {
    ClearLayer(child);
  }
}

already_AddRefed<ReadbackLayer>
BasicLayerManager::CreateReadbackLayer()
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  nsRefPtr<ReadbackLayer> layer = new BasicReadbackLayer(this);
  return layer.forget();
}

BasicShadowLayerManager::BasicShadowLayerManager(nsIWidget* aWidget) :
  BasicLayerManager(aWidget), mTargetRotation(ROTATION_0),
  mRepeatTransaction(false), mIsRepeatTransaction(false)
{
  MOZ_COUNT_CTOR(BasicShadowLayerManager);
}

BasicShadowLayerManager::~BasicShadowLayerManager()
{
  MOZ_COUNT_DTOR(BasicShadowLayerManager);
}

int32_t
BasicShadowLayerManager::GetMaxTextureSize() const
{
  if (HasShadowManager()) {
    return ShadowLayerForwarder::GetMaxTextureSize();
  }

  return INT32_MAX;
}

void
BasicShadowLayerManager::SetDefaultTargetConfiguration(BufferMode aDoubleBuffering, ScreenRotation aRotation)
{
  BasicLayerManager::SetDefaultTargetConfiguration(aDoubleBuffering, aRotation);
  mTargetRotation = aRotation;
  if (mWidget) {
    mTargetBounds = mWidget->GetNaturalBounds();
  }
}

void
BasicShadowLayerManager::SetRoot(Layer* aLayer)
{
  if (mRoot != aLayer) {
    if (HasShadowManager()) {
      
      
      
      
      if (mRoot) {
        Hold(mRoot);
      }
      ShadowLayerForwarder::SetRoot(Hold(aLayer));
    }
    BasicLayerManager::SetRoot(aLayer);
  }
}

void
BasicShadowLayerManager::Mutated(Layer* aLayer)
{
  BasicLayerManager::Mutated(aLayer);

  NS_ASSERTION(InConstruction() || InDrawing(), "wrong phase");
  if (HasShadowManager() && ShouldShadow(aLayer)) {
    ShadowLayerForwarder::Mutated(Hold(aLayer));
  }
}

void
BasicShadowLayerManager::BeginTransactionWithTarget(gfxContext* aTarget)
{
  NS_ABORT_IF_FALSE(mKeepAlive.IsEmpty(), "uncommitted txn?");
  nsRefPtr<gfxContext> targetContext = aTarget;

  
  
  
  if (HasShadowManager()) {
    ScreenOrientation orientation;
    nsIntRect clientBounds;
    if (TabChild* window = mWidget->GetOwningTabChild()) {
      orientation = window->GetOrientation();
    } else {
      hal::ScreenConfiguration currentConfig;
      hal::GetCurrentScreenConfiguration(&currentConfig);
      orientation = currentConfig.orientation();
    }
    mWidget->GetClientBounds(clientBounds);
    ShadowLayerForwarder::BeginTransaction(mTargetBounds, mTargetRotation, clientBounds, orientation);

    
    
    
    
    
    if (mWidget) {
      if (TabChild* window = mWidget->GetOwningTabChild()) {
        mCompositorMightResample = window->IsAsyncPanZoomEnabled();
      }
    }

    
    
    if (aTarget && (aTarget != mDefaultTarget) &&
        XRE_GetProcessType() == GeckoProcessType_Default) {
      mShadowTarget = aTarget;
      
      
      nsRefPtr<gfxASurface> dummy =
        gfxPlatform::GetPlatform()->CreateOffscreenSurface(
          gfxIntSize(1, 1),
          aTarget->OriginalSurface()->GetContentType());
      mDummyTarget = new gfxContext(dummy);
      aTarget = mDummyTarget;
    }
  }
  BasicLayerManager::BeginTransactionWithTarget(aTarget);
}

void
BasicShadowLayerManager::EndTransaction(DrawThebesLayerCallback aCallback,
                                        void* aCallbackData,
                                        EndTransactionFlags aFlags)
{
  BasicLayerManager::EndTransaction(aCallback, aCallbackData, aFlags);
  ForwardTransaction();

  if (mRepeatTransaction) {
    mRepeatTransaction = false;
    mIsRepeatTransaction = true;
    BasicLayerManager::BeginTransaction();
    BasicShadowLayerManager::EndTransaction(aCallback, aCallbackData, aFlags);
    mIsRepeatTransaction = false;
  } else if (mShadowTarget) {
    if (mWidget) {
      if (CompositorChild* remoteRenderer = mWidget->GetRemoteRenderer()) {
        nsRefPtr<gfxASurface> target = mShadowTarget->OriginalSurface();
        SurfaceDescriptor inSnapshot, snapshot;
        if (AllocBuffer(target->GetSize(), target->GetContentType(),
                        &inSnapshot) &&
            
            
            
            remoteRenderer->SendMakeSnapshot(inSnapshot, &snapshot)) {
          AutoOpenSurface opener(OPEN_READ_ONLY, snapshot);
          gfxASurface* source = opener.Get();

          gfxContextAutoSaveRestore restore(mShadowTarget);
          mShadowTarget->SetOperator(gfxContext::OPERATOR_OVER);
          mShadowTarget->DrawSurface(source, source->GetSize());
        }
        if (IsSurfaceDescriptorValid(snapshot)) {
          ShadowLayerForwarder::DestroySharedSurface(&snapshot);
        }
      }
    }
    mShadowTarget = nullptr;
    mDummyTarget = nullptr;
  }
}

bool
BasicShadowLayerManager::EndEmptyTransaction(EndTransactionFlags aFlags)
{
  if (!BasicLayerManager::EndEmptyTransaction(aFlags)) {
    
    
    
    return false;
  }
  ForwardTransaction();
  return true;
}

void
BasicShadowLayerManager::ForwardTransaction()
{
  RenderTraceScope rendertrace("Foward Transaction", "000090");
  mPhase = PHASE_FORWARD;

  
  AutoInfallibleTArray<EditReply, 10> replies;
  if (HasShadowManager() && ShadowLayerForwarder::EndTransaction(&replies)) {
    for (nsTArray<EditReply>::size_type i = 0; i < replies.Length(); ++i) {
      const EditReply& reply = replies[i];

      switch (reply.type()) {
      case EditReply::TOpThebesBufferSwap: {
        MOZ_LAYERS_LOG(("[LayersForwarder] ThebesBufferSwap"));

        const OpThebesBufferSwap& obs = reply.get_OpThebesBufferSwap();
        BasicShadowableThebesLayer* thebes = GetBasicShadowable(obs)->AsThebes();
        thebes->SetBackBufferAndAttrs(
          obs.newBackBuffer(), obs.newValidRegion(),
          obs.readOnlyFrontBuffer(), obs.frontUpdatedRegion());
        break;
      }
      case EditReply::TOpBufferSwap: {
        MOZ_LAYERS_LOG(("[LayersForwarder] BufferSwap"));

        const OpBufferSwap& obs = reply.get_OpBufferSwap();
        const CanvasSurface& newBack = obs.newBackBuffer();
        if (newBack.type() == CanvasSurface::TSurfaceDescriptor) {
          GetBasicShadowable(obs)->SetBackBuffer(newBack.get_SurfaceDescriptor());
        } else if (newBack.type() == CanvasSurface::Tnull_t) {
          GetBasicShadowable(obs)->SetBackBuffer(SurfaceDescriptor());
        } else {
          NS_RUNTIMEABORT("Unknown back image type");
        }
        break;
      }

      case EditReply::TOpImageSwap: {
        MOZ_LAYERS_LOG(("[LayersForwarder] YUVBufferSwap"));

        const OpImageSwap& ois = reply.get_OpImageSwap();
        BasicShadowableLayer* layer = GetBasicShadowable(ois);
        const SharedImage& newBack = ois.newBackImage();

        if (newBack.type() == SharedImage::TSurfaceDescriptor) {
          layer->SetBackBuffer(newBack.get_SurfaceDescriptor());
        } else if (newBack.type() == SharedImage::TYUVImage) {
          const YUVImage& yuv = newBack.get_YUVImage();
          layer->SetBackBufferYUVImage(yuv.Ydata(), yuv.Udata(), yuv.Vdata());
        } else {
          layer->SetBackBuffer(SurfaceDescriptor());
          layer->SetBackBufferYUVImage(SurfaceDescriptor(),
                                       SurfaceDescriptor(),
                                       SurfaceDescriptor());
        }

        break;
      }

      default:
        NS_RUNTIMEABORT("not reached");
      }
    }
  } else if (HasShadowManager()) {
    NS_WARNING("failed to forward Layers transaction");
  }

  mPhase = PHASE_NONE;

  
  
  mKeepAlive.Clear();
}

ShadowableLayer*
BasicShadowLayerManager::Hold(Layer* aLayer)
{
  NS_ABORT_IF_FALSE(HasShadowManager(),
                    "top-level tree, no shadow tree to remote to");

  ShadowableLayer* shadowable = ToShadowable(aLayer);
  NS_ABORT_IF_FALSE(shadowable, "trying to remote an unshadowable layer");

  mKeepAlive.AppendElement(aLayer);
  return shadowable;
}

bool
BasicShadowLayerManager::IsCompositingCheap()
{
  
  return mShadowManager &&
         LayerManager::IsCompositingCheap(GetParentBackendType());
}

void
BasicShadowLayerManager::SetIsFirstPaint()
{
  ShadowLayerForwarder::SetIsFirstPaint();
}

void
BasicShadowLayerManager::ClearCachedResources(Layer* aSubtree)
{
  MOZ_ASSERT(!HasShadowManager() || !aSubtree);
  if (PLayersChild* manager = GetShadowManager()) {
    manager->SendClearCachedResources();
  }
  BasicLayerManager::ClearCachedResources(aSubtree);
}

bool
BasicShadowLayerManager::ProgressiveUpdateCallback(bool aHasPendingNewThebesContent,
                                                   gfx::Rect& aViewport,
                                                   float& aScaleX,
                                                   float& aScaleY,
                                                   bool aDrawingCritical)
{
#ifdef MOZ_WIDGET_ANDROID
  Layer* primaryScrollable = GetPrimaryScrollableLayer();
  if (primaryScrollable) {
    const FrameMetrics& metrics = primaryScrollable->AsContainerLayer()->GetFrameMetrics();

    
    
    const gfx3DMatrix& rootTransform = GetRoot()->GetTransform();
    float devPixelRatioX = 1 / rootTransform.GetXScale();
    float devPixelRatioY = 1 / rootTransform.GetYScale();
    const gfx::Rect& metricsDisplayPort =
      (aDrawingCritical && !metrics.mCriticalDisplayPort.IsEmpty()) ?
        metrics.mCriticalDisplayPort : metrics.mDisplayPort;
    gfx::Rect displayPort((metricsDisplayPort.x + metrics.mScrollOffset.x) * devPixelRatioX,
                          (metricsDisplayPort.y + metrics.mScrollOffset.y) * devPixelRatioY,
                          metricsDisplayPort.width * devPixelRatioX,
                          metricsDisplayPort.height * devPixelRatioY);

    return AndroidBridge::Bridge()->ProgressiveUpdateCallback(
      aHasPendingNewThebesContent, displayPort, devPixelRatioX, aDrawingCritical,
      aViewport, aScaleX, aScaleY);
  }
#endif

  return false;
}

already_AddRefed<ThebesLayer>
BasicShadowLayerManager::CreateThebesLayer()
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
#ifdef FORCE_BASICTILEDTHEBESLAYER
  if (HasShadowManager() && GetParentBackendType() == LAYERS_OPENGL) {
    
    
    
    nsRefPtr<BasicTiledThebesLayer> layer =
      new BasicTiledThebesLayer(this);
    MAYBE_CREATE_SHADOW(Thebes);
    return layer.forget();
  } else
#endif
  {
    nsRefPtr<BasicShadowableThebesLayer> layer =
      new BasicShadowableThebesLayer(this);
    MAYBE_CREATE_SHADOW(Thebes);
    return layer.forget();
  }
}


BasicShadowableLayer::~BasicShadowableLayer()
{
  if (HasShadow()) {
    PLayerChild::Send__delete__(GetShadow());
  }
  MOZ_COUNT_DTOR(BasicShadowableLayer);
}

}
}
