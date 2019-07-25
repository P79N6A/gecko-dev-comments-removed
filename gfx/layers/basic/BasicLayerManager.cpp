




#include "mozilla/layers/PLayerChild.h"
#include "mozilla/layers/PLayersChild.h"
#include "mozilla/layers/PLayersParent.h"

#include "gfxSharedImageSurface.h"
#include "gfxImageSurface.h"
#include "gfxUtils.h"
#include "nsXULAppAPI.h"
#include "RenderTrace.h"
#include "sampler.h"

#define PIXMAN_DONT_DEFINE_STDINT
#include "pixman.h"

#include "BasicTiledThebesLayer.h"
#include "BasicLayersImpl.h"
#include "BasicThebesLayer.h"
#include "BasicContainerLayer.h"
#include "mozilla/Preferences.h"
#include "nsIWidget.h"

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

BasicLayerManager::BasicLayerManager(nsIWidget* aWidget) :
#ifdef DEBUG
  mPhase(PHASE_NONE),
#endif
  mWidget(aWidget)
  , mDoubleBuffering(BUFFER_NONE), mUsingDefaultTarget(false)
  , mCachedSurfaceInUse(false)
  , mTransactionIncomplete(false)
{
  MOZ_COUNT_CTOR(BasicLayerManager);
  NS_ASSERTION(aWidget, "Must provide a widget");
}

BasicLayerManager::BasicLayerManager() :
#ifdef DEBUG
  mPhase(PHASE_NONE),
#endif
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
#ifdef DEBUG
  mPhase = PHASE_CONSTRUCTION;
#endif
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
                 PRUint32 aFlags)
{
  nsIntRect newClipRect(aClipRect);
  PRUint32 newFlags = aFlags;

  
  
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
          cr += nsIntPoint(PRInt32(tr.x0), PRInt32(tr.y0));
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
#ifdef DEBUG
  mPhase = PHASE_NONE;
#endif
  mUsingDefaultTarget = false;
  mInTransaction = false;
}

bool
BasicLayerManager::EndTransactionInternal(DrawThebesLayerCallback aCallback,
                                          void* aCallbackData,
                                          EndTransactionFlags aFlags)
{
  SAMPLE_LABEL("BasicLayerManager", "EndTranscationInternal");
#ifdef MOZ_LAYERS_HAVE_LOG
  MOZ_LAYERS_LOG(("  ----- (beginning paint)"));
  Log();
#endif

  NS_ASSERTION(InConstruction(), "Should be in construction phase");
#ifdef DEBUG
  mPhase = PHASE_DRAWING;
#endif

  Layer* aLayer = GetRoot();
  RenderTraceLayers(aLayer, "FF00");

  mTransactionIncomplete = false;

  if (aFlags & END_NO_COMPOSITE) {
    
    nsRefPtr<gfxASurface> surf = gfxPlatform::GetPlatform()->CreateOffscreenSurface(gfxIntSize(1, 1), gfxASurface::CONTENT_COLOR);
    mTarget = new gfxContext(surf);
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
    }

    if (!mTransactionIncomplete) {
      
      mTarget = nullptr;
    }
  }

#ifdef MOZ_LAYERS_HAVE_LOG
  Log();
  MOZ_LAYERS_LOG(("]----- EndTransaction"));
#endif

#ifdef DEBUG
  
  
  mPhase = mTransactionIncomplete ? PHASE_CONSTRUCTION : PHASE_NONE;
#endif

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
  static bool sWidgetFlashingEnabled;
  static bool sWidgetFlashingPrefCached = false;

  if (!sWidgetFlashingPrefCached) {
    sWidgetFlashingPrefCached = true;
    mozilla::Preferences::AddBoolVarCache(&sWidgetFlashingEnabled,
                                          "nglayout.debug.widget_update_flashing");
  }

  if (sWidgetFlashingEnabled) {
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
            gfxPoint& aDrawOffset, bool aDontBlit)
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

  
  
  gfxRect destRect = aDest->GetClipExtents();
  destRect.IntersectRect(destRect, offsetRect);

  
  nsRefPtr<gfxASurface> dest = aDest->CurrentSurface();
  nsRefPtr<gfxImageSurface> destImage;
  gfxPoint offset;
  bool blitComplete;
  if (!destImage || aDontBlit || !aDest->ClipContainsRect(destRect)) {
    destImage = new gfxImageSurface(gfxIntSize(destRect.width, destRect.height),
                                    gfxASurface::ImageFormatARGB32);
    offset = destRect.TopLeft();
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

  
  
  aDrawOffset = destRect.TopLeft();
  return destImage.forget(); 
}



void
BasicLayerManager::PaintLayer(gfxContext* aTarget,
                              Layer* aLayer,
                              DrawThebesLayerCallback aCallback,
                              void* aCallbackData,
                              ReadbackProcessor* aReadback)
{
  RenderTraceScope trace("BasicLayerManager::PaintLayer", "707070");

  const nsIntRect* clipRect = aLayer->GetEffectiveClipRect();
  const gfx3DMatrix& effectiveTransform = aLayer->GetEffectiveTransform();
  
  
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

  
  
  bool needsSaveRestore = needsGroup || clipRect || needsClipToVisibleRegion;
  gfxMatrix savedMatrix;
  if (needsSaveRestore) {
    aTarget->Save();

    if (clipRect) {
      aTarget->NewPath();
      aTarget->Rectangle(gfxRect(clipRect->x, clipRect->y, clipRect->width, clipRect->height), true);
      aTarget->Clip();
    }
  }
  savedMatrix = aTarget->CurrentMatrix();

  gfxMatrix transform;
  
  bool is2D = effectiveTransform.CanDraw2D(&transform);
  NS_ABORT_IF_FALSE(is2D || needsGroup || !aLayer->GetFirstChild(), "Must PushGroup for 3d transforms!");
  if (is2D) {
    aTarget->SetMatrix(transform);
  } else {
    aTarget->SetMatrix(gfxMatrix());
  }

  const nsIntRegion& visibleRegion = aLayer->GetEffectiveVisibleRegion();
  
  if (needsClipToVisibleRegion && !needsGroup) {
    gfxUtils::ClipToRegion(aTarget, visibleRegion);
    
    needsClipToVisibleRegion = false;
  }

  bool pushedTargetOpaqueRect = false;
  nsRefPtr<gfxASurface> currentSurface = aTarget->CurrentSurface();
  DrawTarget *dt = aTarget->GetDrawTarget();
  const nsIntRect& bounds = visibleRegion.GetBounds();
  
  if (is2D) {
    if (aTarget->IsCairo()) {
      const gfxRect& targetOpaqueRect = currentSurface->GetOpaqueRect();

      
      
      if (targetOpaqueRect.IsEmpty() && visibleRegion.GetNumRects() == 1 &&
          (aLayer->GetContentFlags() & Layer::CONTENT_OPAQUE) &&
          !transform.HasNonAxisAlignedTransform()) {
        currentSurface->SetOpaqueRect(
            aTarget->UserToDevice(gfxRect(bounds.x, bounds.y, bounds.width, bounds.height)));
        pushedTargetOpaqueRect = true;
      }
    } else {
      const IntRect& targetOpaqueRect = dt->GetOpaqueRect();

      
      
      if (targetOpaqueRect.IsEmpty() && visibleRegion.GetNumRects() == 1 &&
          (aLayer->GetContentFlags() & Layer::CONTENT_OPAQUE) &&
          !transform.HasNonAxisAlignedTransform()) {

        gfx::Rect opaqueRect = dt->GetTransform().TransformBounds(
          gfx::Rect(bounds.x, bounds.y, bounds.width, bounds.height));
        opaqueRect.RoundIn();
        IntRect intOpaqueRect;
        if (opaqueRect.ToIntRect(&intOpaqueRect)) {
          aTarget->GetDrawTarget()->SetOpaqueRect(intOpaqueRect);
          pushedTargetOpaqueRect = true;
        }
      }
    }
  }

  nsRefPtr<gfxContext> groupTarget;
  nsRefPtr<gfxASurface> untransformedSurface;
  bool clipIsEmpty = !aTarget || aTarget->GetClipExtents().IsEmpty();
  if (!is2D && !clipIsEmpty) {
    untransformedSurface = 
      gfxPlatform::GetPlatform()->CreateOffscreenSurface(gfxIntSize(bounds.width, bounds.height), 
                                                         gfxASurface::CONTENT_COLOR_ALPHA);
    if (!untransformedSurface) {
      if (pushedTargetOpaqueRect) {
        if (aTarget->IsCairo()) {
          currentSurface->SetOpaqueRect(gfxRect(0, 0, 0, 0));
        } else {
          dt->SetOpaqueRect(IntRect());
        }
      }
      NS_ASSERTION(needsSaveRestore, "Should always need to restore with 3d transforms!");
      aTarget->Restore();
      return;
    }
    untransformedSurface->SetDeviceOffset(gfxPoint(-bounds.x, -bounds.y));
    groupTarget = new gfxContext(untransformedSurface);
  } else if (needsGroup && !clipIsEmpty) {
    groupTarget = PushGroupForLayer(aTarget, aLayer, aLayer->GetEffectiveVisibleRegion(),
                                    &needsClipToVisibleRegion);
  } else {
    groupTarget = aTarget;
  }

  if (aLayer->GetFirstChild() &&
      aLayer->GetMaskLayer() &&
      HasShadowManager()) {
    
    static_cast<BasicImplData*>(aLayer->GetMaskLayer()->ImplData())
      ->Paint(nullptr, nullptr);
  }

  
  Layer* child = aLayer->GetFirstChild();
  if (!child) {
#ifdef MOZ_LAYERS_HAVE_LOG
    MOZ_LAYERS_LOG(("%s (0x%p) is covered: %i\n", __FUNCTION__,
                   (void*)aLayer, data->IsHidden()));
#endif
    if (aLayer->AsThebesLayer()) {
      data->PaintThebes(groupTarget,
                        aLayer->GetMaskLayer(),
                        aCallback, aCallbackData,
                        aReadback);
    } else {
      data->Paint(groupTarget, aLayer->GetMaskLayer());
    }
  } else {
    ReadbackProcessor readback;
    ContainerLayer* container = static_cast<ContainerLayer*>(aLayer);
    if (IsRetained()) {
      readback.BuildUpdates(container);
    }
  
    nsAutoTArray<Layer*, 12> children;
    container->SortChildrenBy3DZOrder(children);

    for (PRUint32 i = 0; i < children.Length(); i++) {
      PaintLayer(groupTarget, children.ElementAt(i), aCallback, aCallbackData, &readback);
      if (mTransactionIncomplete)
        break;
    }
  }

  if (needsGroup) {
    bool blitComplete = false;
    if (is2D) {
      PopGroupToSourceWithCachedSurface(aTarget, groupTarget);
    } else {
      
      
      if (!clipIsEmpty) {
        NS_ABORT_IF_FALSE(untransformedSurface, 
                          "We should always allocate an untransformed surface with 3d transforms!");
        gfxPoint offset;
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

        nsRefPtr<gfxASurface> result =
          Transform3D(untransformedSurface, aTarget, bounds,
                      effectiveTransform, offset, dontBlit);

        blitComplete = !result;
        if (result) {
          aTarget->SetSource(result, offset);
        }
      }
    }
    
    
    
    
    
    
    
    
    
    if (!mTransactionIncomplete && !blitComplete) {
      if (needsClipToVisibleRegion) {
        gfxUtils::ClipToRegion(aTarget, aLayer->GetEffectiveVisibleRegion());
      }
      AutoSetOperator setOperator(aTarget, container->GetOperator());
      PaintWithMask(aTarget, aLayer->GetEffectiveOpacity(),
                    HasShadowManager() ? nullptr : aLayer->GetMaskLayer());
    }
  }

  if (pushedTargetOpaqueRect) {
    if (aTarget->IsCairo()) {
      currentSurface->SetOpaqueRect(gfxRect(0, 0, 0, 0));
    } else {
      dt->SetOpaqueRect(IntRect());
    }
  }

  if (needsSaveRestore) {
    aTarget->Restore();
  } else {
    aTarget->SetMatrix(savedMatrix);
  }
}

void
BasicLayerManager::ClearCachedResources()
{
  if (mRoot) {
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
  mRepeatTransaction(false)
{
  MOZ_COUNT_CTOR(BasicShadowLayerManager);
}

BasicShadowLayerManager::~BasicShadowLayerManager()
{
  MOZ_COUNT_DTOR(BasicShadowLayerManager);
}

PRInt32
BasicShadowLayerManager::GetMaxTextureSize() const
{
  if (HasShadowManager()) {
    return ShadowLayerForwarder::GetMaxTextureSize();
  }

  return PR_INT32_MAX;
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
    ShadowLayerForwarder::BeginTransaction(mTargetBounds, mTargetRotation);

    
    
    if (aTarget && (aTarget != mDefaultTarget) &&
        XRE_GetProcessType() == GeckoProcessType_Default) {
      mShadowTarget = aTarget;

      
      
      nsRefPtr<gfxASurface> targetSurface = gfxPlatform::GetPlatform()->
        CreateOffscreenSurface(aTarget->OriginalSurface()->GetSize(),
                               aTarget->OriginalSurface()->GetContentType());
      targetContext = new gfxContext(targetSurface);
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
    BasicLayerManager::BeginTransaction();
    BasicShadowLayerManager::EndTransaction(aCallback, aCallbackData, aFlags);
  } else if (mShadowTarget) {
    
    ShadowLayerForwarder::ShadowDrawToTarget(mShadowTarget);
    mShadowTarget = nullptr;
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
#ifdef DEBUG
  mPhase = PHASE_FORWARD;
#endif

  
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

#ifdef DEBUG
  mPhase = PHASE_NONE;
#endif

  
  
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
