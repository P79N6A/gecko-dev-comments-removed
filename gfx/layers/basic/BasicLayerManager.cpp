




#include <stdint.h>                     
#include <stdlib.h>                     
#include <sys/types.h>                  
#include "BasicContainerLayer.h"        
#include "BasicLayersImpl.h"            
#include "GeckoProfiler.h"              
#include "ImageContainer.h"             
#include "Layers.h"                     
#include "ReadbackLayer.h"              
#include "ReadbackProcessor.h"          
#include "RenderTrace.h"                
#include "basic/BasicImplData.h"        
#include "basic/BasicLayers.h"          
#include "gfx3DMatrix.h"                
#include "gfxASurface.h"                
#include "gfxColor.h"                   
#include "gfxContext.h"                 
#include "gfxImageSurface.h"            
#include "gfxMatrix.h"                  
#include "gfxPlatform.h"                
#include "gfxPrefs.h"                   
#include "gfxPoint.h"                   
#include "gfxRect.h"                    
#include "gfxUtils.h"                   
#include "gfx2DGlue.h"                  
#include "mozilla/Assertions.h"         
#include "mozilla/WidgetUtils.h"        
#include "mozilla/gfx/2D.h"             
#include "mozilla/gfx/BasePoint.h"      
#include "mozilla/gfx/BaseRect.h"       
#include "mozilla/gfx/Matrix.h"         
#include "mozilla/gfx/PathHelpers.h"
#include "mozilla/gfx/Rect.h"           
#include "mozilla/layers/LayersTypes.h"  
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsPoint.h"                    
#include "nsRect.h"                     
#include "nsRegion.h"                   
#include "nsTArray.h"                   
#ifdef MOZ_ENABLE_SKIA
#include "skia/SkCanvas.h"              
#include "skia/SkBitmapDevice.h"        
#else
#define PIXMAN_DONT_DEFINE_STDINT
#include "pixman.h"                     
#endif
class nsIWidget;

namespace mozilla {
namespace layers {

using namespace mozilla::gfx;







static bool
ClipToContain(gfxContext* aContext, const nsIntRect& aRect)
{
  gfxRect userRect(aRect.x, aRect.y, aRect.width, aRect.height);
  gfxRect deviceRect = aContext->UserToDevice(userRect);
  deviceRect.RoundOut();

  gfxMatrix currentMatrix = aContext->CurrentMatrix();
  aContext->SetMatrix(gfxMatrix());
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
    aContext->PushGroup(gfxContentType::COLOR);
    result = aContext;
  } else {
    *aNeedsClipToVisibleRegion = false;
    result = aContext;
    if (aLayer->GetContentFlags() & Layer::CONTENT_COMPONENT_ALPHA) {
      aContext->PushGroupAndCopyBackground(gfxContentType::COLOR_ALPHA);
    } else {
      aContext->PushGroup(gfxContentType::COLOR_ALPHA);
    }
  }
  return result.forget();
}

static nsIntRect
ToInsideIntRect(const gfxRect& aRect)
{
  gfxRect r = aRect;
  r.RoundIn();
  return nsIntRect(r.X(), r.Y(), r.Width(), r.Height());
}






class PaintLayerContext {
public:
  PaintLayerContext(gfxContext* aTarget, Layer* aLayer,
                    LayerManager::DrawPaintedLayerCallback aCallback,
                    void* aCallbackData)
   : mTarget(aTarget)
   , mTargetMatrixSR(aTarget)
   , mLayer(aLayer)
   , mCallback(aCallback)
   , mCallbackData(aCallbackData)
   , mPushedOpaqueRect(false)
  {}

  ~PaintLayerContext()
  {
    
    if (mPushedOpaqueRect)
    {
      ClearOpaqueRect();
    }
  }

  
  
  bool Setup2DTransform()
  {
    
    return mLayer->GetEffectiveTransformForBuffer().CanDraw2D(&mTransform);
  }

  
  
  void Apply2DTransform()
  {
    mTarget->SetMatrix(ThebesMatrix(mTransform));
  }

  
  void AnnotateOpaqueRect()
  {
    const nsIntRegion& visibleRegion = mLayer->GetEffectiveVisibleRegion();
    const nsIntRect& bounds = visibleRegion.GetBounds();

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

  
  
  
  void ClearOpaqueRect() {
    mTarget->GetDrawTarget()->SetOpaqueRect(IntRect());
  }

  gfxContext* mTarget;
  gfxContextMatrixAutoSaveRestore mTargetMatrixSR;
  Layer* mLayer;
  LayerManager::DrawPaintedLayerCallback mCallback;
  void* mCallbackData;
  Matrix mTransform;
  bool mPushedOpaqueRect;
};

BasicLayerManager::BasicLayerManager(nsIWidget* aWidget)
  : mPhase(PHASE_NONE)
  , mWidget(aWidget)
  , mDoubleBuffering(BufferMode::BUFFER_NONE)
  , mType(BLM_WIDGET)
  , mUsingDefaultTarget(false)
  , mTransactionIncomplete(false)
  , mCompositorMightResample(false)
{
  MOZ_COUNT_CTOR(BasicLayerManager);
  NS_ASSERTION(aWidget, "Must provide a widget");
}

BasicLayerManager::BasicLayerManager(BasicLayerManagerType aType)
  : mPhase(PHASE_NONE)
  , mWidget(nullptr)
  , mDoubleBuffering(BufferMode::BUFFER_NONE)
  , mType(aType)
  , mUsingDefaultTarget(false)
  , mTransactionIncomplete(false)
{
  MOZ_COUNT_CTOR(BasicLayerManager);
  MOZ_ASSERT(mType != BLM_WIDGET);
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
TransformIntRect(nsIntRect& aRect, const Matrix& aMatrix,
                 nsIntRect (*aRoundMethod)(const gfxRect&))
{
  Rect gr = Rect(aRect.x, aRect.y, aRect.width, aRect.height);
  gr = aMatrix.TransformBounds(gr);
  aRect = (*aRoundMethod)(ThebesRect(gr));
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
        Matrix tr;
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
  data->SetOperator(CompositionOp::OP_OVER);
  data->SetClipToVisibleRegion(false);
  data->SetDrawAtomically(false);

  if (!aLayer->AsContainerLayer()) {
    Matrix transform;
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
        Matrix tr;
        if (aLayer->GetParent()->GetEffectiveTransform().CanDraw2D(&tr)) {
          NS_ASSERTION(!ThebesMatrix(tr).HasNonIntegerTranslation(),
                       "Parent can only have an integer translation");
          cr += nsIntPoint(int32_t(tr._31), int32_t(tr._32));
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
    data->SetOperator(CompositionOp::OP_SOURCE);
    data->SetDrawAtomically(true);
  } else {
    if (container->UseIntermediateSurface() ||
        !container->ChildrenPartitionVisibleRegion(newVisibleRect)) {
      
      data->SetOperator(CompositionOp::OP_SOURCE);
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
BasicLayerManager::EndTransaction(DrawPaintedLayerCallback aCallback,
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
BasicLayerManager::EndTransactionInternal(DrawPaintedLayerCallback aCallback,
                                          void* aCallbackData,
                                          EndTransactionFlags aFlags)
{
  PROFILER_LABEL("BasicLayerManager", "EndTransactionInternal",
    js::ProfileEntry::Category::GRAPHICS);

#ifdef MOZ_LAYERS_HAVE_LOG
  MOZ_LAYERS_LOG(("  ----- (beginning paint)"));
  Log();
#endif

  NS_ASSERTION(InConstruction(), "Should be in construction phase");
  mPhase = PHASE_DRAWING;

  RenderTraceLayers(mRoot, "FF00");

  mTransactionIncomplete = false;

  if (mRoot) {
    if (aFlags & END_NO_COMPOSITE) {
      
      
      mRoot->ApplyPendingUpdatesToSubtree();
    }

    
    
    if (mTarget) {
      mSnapEffectiveTransforms =
        !mTarget->GetDrawTarget()->GetUserData(&sDisablePixelSnapping);
    } else {
      mSnapEffectiveTransforms = true;
    }
    mRoot->ComputeEffectiveTransforms(mTarget ? Matrix4x4::From2D(ToMatrix(mTarget->CurrentMatrix())) : Matrix4x4());

    ToData(mRoot)->Validate(aCallback, aCallbackData, nullptr);
    if (mRoot->GetMaskLayer()) {
      ToData(mRoot->GetMaskLayer())->Validate(aCallback, aCallbackData, nullptr);
    }
  }

  if (mTarget && mRoot &&
      !(aFlags & END_NO_IMMEDIATE_REDRAW) &&
      !(aFlags & END_NO_COMPOSITE)) {
    nsIntRect clipRect;

    {
      gfxContextMatrixAutoSaveRestore save(mTarget);
      mTarget->SetMatrix(gfxMatrix());
      clipRect = ToOutsideIntRect(mTarget->GetClipExtents());
    }

    if (IsRetained()) {
      nsIntRegion region;
      MarkLayersHidden(mRoot, clipRect, clipRect, region, ALLOW_OPAQUE);
      if (mUsingDefaultTarget && mDoubleBuffering != BufferMode::BUFFER_NONE) {
        ApplyDoubleBuffering(mRoot, clipRect);
      }
    }

    PaintLayer(mTarget, mRoot, aCallback, aCallbackData);
    if (!mRegionToClear.IsEmpty()) {
      nsIntRegionRectIterator iter(mRegionToClear);
      const nsIntRect *r;
      while ((r = iter.Next())) {
        mTarget->GetDrawTarget()->ClearRect(Rect(r->x, r->y, r->width, r->height));
      }
    }
    if (mWidget) {
      FlashWidgetUpdateArea(mTarget);
    }
    RecordFrame();
    PostPresent();

    if (!mTransactionIncomplete) {
      
      mTarget = nullptr;
    }
  }

  if (mRoot) {
    mAnimationReadyTime = TimeStamp::Now();
    mRoot->StartPendingAnimations(mAnimationReadyTime);
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
  if (gfxPrefs::WidgetUpdateFlashing()) {
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

#ifdef MOZ_ENABLE_SKIA
static SkMatrix
BasicLayerManager_Matrix3DToSkia(const gfx3DMatrix& aMatrix)
{
  SkMatrix transform;
  transform.setAll(aMatrix._11,
                   aMatrix._21,
                   aMatrix._41,
                   aMatrix._12,
                   aMatrix._22,
                   aMatrix._42,
                   aMatrix._14,
                   aMatrix._24,
                   aMatrix._44);

  return transform;
}

static void
Transform(const gfxImageSurface* aDest,
          RefPtr<DataSourceSurface> aSrc,
          const gfx3DMatrix& aTransform,
          gfxPoint aDestOffset)
{
  if (aTransform.IsSingular()) {
    return;
  }

  IntSize destSize = aDest->GetSize();
  SkImageInfo destInfo = SkImageInfo::Make(destSize.width,
                                           destSize.height,
                                           kBGRA_8888_SkColorType,
                                           kPremul_SkAlphaType);
  SkBitmap destBitmap;
  destBitmap.setInfo(destInfo, aDest->Stride());
  destBitmap.setPixels((uint32_t*)aDest->Data());
  SkCanvas destCanvas(destBitmap);

  IntSize srcSize = aSrc->GetSize();
  SkImageInfo srcInfo = SkImageInfo::Make(srcSize.width,
                                          srcSize.height,
                                          kBGRA_8888_SkColorType,
                                          kPremul_SkAlphaType);
  SkBitmap src;
  src.setInfo(srcInfo, aSrc->Stride());
  src.setPixels((uint32_t*)aSrc->GetData());

  gfx3DMatrix transform = aTransform;
  transform.TranslatePost(Point3D(-aDestOffset.x, -aDestOffset.y, 0));
  destCanvas.setMatrix(BasicLayerManager_Matrix3DToSkia(transform));

  SkPaint paint;
  paint.setXfermodeMode(SkXfermode::kSrc_Mode);
  paint.setAntiAlias(true);
  paint.setFilterLevel(SkPaint::kLow_FilterLevel);
  SkRect destRect = SkRect::MakeXYWH(0, 0, srcSize.width, srcSize.height);
  destCanvas.drawBitmapRectToRect(src, nullptr, destRect, &paint);
}
#else
static pixman_transform
BasicLayerManager_Matrix3DToPixman(const gfx3DMatrix& aMatrix)
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
Transform(const gfxImageSurface* aDest,
          RefPtr<DataSourceSurface> aSrc,
          const gfx3DMatrix& aTransform,
          gfxPoint aDestOffset)
{
  IntSize destSize = aDest->GetSize();
  pixman_image_t* dest = pixman_image_create_bits(aDest->Format() == gfxImageFormat::ARGB32 ? PIXMAN_a8r8g8b8 : PIXMAN_x8r8g8b8,
                                                  destSize.width,
                                                  destSize.height,
                                                  (uint32_t*)aDest->Data(),
                                                  aDest->Stride());

  IntSize srcSize = aSrc->GetSize();
  pixman_image_t* src = pixman_image_create_bits(aSrc->GetFormat() == SurfaceFormat::B8G8R8A8 ? PIXMAN_a8r8g8b8 : PIXMAN_x8r8g8b8,
                                                 srcSize.width,
                                                 srcSize.height,
                                                 (uint32_t*)aSrc->GetData(),
                                                 aSrc->Stride());

  NS_ABORT_IF_FALSE(src && dest, "Failed to create pixman images?");

  pixman_transform pixTransform = BasicLayerManager_Matrix3DToPixman(aTransform);
  pixman_transform pixTransformInverted;

  
  if (!pixman_transform_invert(&pixTransformInverted, &pixTransform)) {
    pixman_image_unref(dest);
    pixman_image_unref(src);
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
#endif














static already_AddRefed<gfxASurface>
Transform3D(RefPtr<SourceSurface> aSource,
            gfxContext* aDest,
            const gfxRect& aBounds,
            const gfx3DMatrix& aTransform,
            gfxRect& aDestRect)
{
  
  gfxRect offsetRect = aTransform.TransformBounds(aBounds);

  
  
  aDestRect = aDest->GetClipExtents();
  aDestRect.IntersectRect(aDestRect, offsetRect);
  aDestRect.RoundOut();

  
  nsRefPtr<gfxASurface> dest = aDest->CurrentSurface();
  nsRefPtr<gfxImageSurface> destImage = new gfxImageSurface(gfxIntSize(aDestRect.width,
                                                                       aDestRect.height),
                                                            gfxImageFormat::ARGB32);
  gfxPoint offset = aDestRect.TopLeft();

  
  gfx3DMatrix translation = gfx3DMatrix::Translation(aBounds.x, aBounds.y, 0);

  
  Transform(destImage, aSource->GetDataSurface(), translation * aTransform, offset);

  
  
  return destImage.forget();
}

void
BasicLayerManager::PaintSelfOrChildren(PaintLayerContext& aPaintContext,
                                       gfxContext* aGroupTarget)
{
  BasicImplData* data = ToData(aPaintContext.mLayer);

  
  Layer* child = aPaintContext.mLayer->GetFirstChild();
  if (!child) {
    if (aPaintContext.mLayer->AsPaintedLayer()) {
      data->PaintThebes(aGroupTarget, aPaintContext.mLayer->GetMaskLayer(),
          aPaintContext.mCallback, aPaintContext.mCallbackData);
    } else {
      data->Paint(aGroupTarget->GetDrawTarget(),
                  aGroupTarget->GetDeviceOffset(),
                  aPaintContext.mLayer->GetMaskLayer());
    }
  } else {
    ContainerLayer* container =
        static_cast<ContainerLayer*>(aPaintContext.mLayer);
    nsAutoTArray<Layer*, 12> children;
    container->SortChildrenBy3DZOrder(children);
    for (uint32_t i = 0; i < children.Length(); i++) {
      PaintLayer(aGroupTarget, children.ElementAt(i), aPaintContext.mCallback,
          aPaintContext.mCallbackData);
      if (mTransactionIncomplete)
        break;
    }
  }
}

void
BasicLayerManager::FlushGroup(PaintLayerContext& aPaintContext, bool aNeedsClipToVisibleRegion)
{
  
  
  
  
  
  
  
  
  
  if (!mTransactionIncomplete) {
    if (aNeedsClipToVisibleRegion) {
      gfxUtils::ClipToRegion(aPaintContext.mTarget,
                             aPaintContext.mLayer->GetEffectiveVisibleRegion());
    }

    CompositionOp op = GetEffectiveOperator(aPaintContext.mLayer);
    AutoSetOperator setOperator(aPaintContext.mTarget, ThebesOp(op));

    PaintWithMask(aPaintContext.mTarget, aPaintContext.mLayer->GetEffectiveOpacity(),
                  aPaintContext.mLayer->GetMaskLayer());
  }
}

void
BasicLayerManager::PaintLayer(gfxContext* aTarget,
                              Layer* aLayer,
                              DrawPaintedLayerCallback aCallback,
                              void* aCallbackData)
{
  PROFILER_LABEL("BasicLayerManager", "PaintLayer",
    js::ProfileEntry::Category::GRAPHICS);

  PaintLayerContext paintLayerContext(aTarget, aLayer, aCallback, aCallbackData);

  
  
  if (aLayer->GetEffectiveTransform().IsSingular()) {
    return;
  }

  RenderTraceScope trace("BasicLayerManager::PaintLayer", "707070");

  const nsIntRect* clipRect = aLayer->GetEffectiveClipRect();
  BasicContainerLayer* container =
    static_cast<BasicContainerLayer*>(aLayer->AsContainerLayer());
  bool needsGroup = container && container->UseIntermediateSurface();
  BasicImplData* data = ToData(aLayer);
  bool needsClipToVisibleRegion =
    data->GetClipToVisibleRegion() && !aLayer->AsPaintedLayer();
  NS_ASSERTION(needsGroup || !container ||
               container->GetOperator() == CompositionOp::OP_OVER,
               "non-OVER operator should have forced UseIntermediateSurface");
  NS_ASSERTION(!container || !aLayer->GetMaskLayer() ||
               container->UseIntermediateSurface(),
               "ContainerLayer with mask layer should force UseIntermediateSurface");

  gfxContextAutoSaveRestore contextSR;
  gfxMatrix transform;
  
  bool is2D = paintLayerContext.Setup2DTransform();
  MOZ_ASSERT(is2D || needsGroup || !container, "Must PushGroup for 3d transforms!");

  bool needsSaveRestore =
    needsGroup || clipRect || needsClipToVisibleRegion || !is2D;
  if (needsSaveRestore) {
    contextSR.SetContext(aTarget);

    if (clipRect) {
      aTarget->NewPath();
      aTarget->SnappedRectangle(gfxRect(clipRect->x, clipRect->y, clipRect->width, clipRect->height));
      aTarget->Clip();
    }
  }

  paintLayerContext.Apply2DTransform();

  const nsIntRegion& visibleRegion = aLayer->GetEffectiveVisibleRegion();
  
  if (needsClipToVisibleRegion && !needsGroup) {
    gfxUtils::ClipToRegion(aTarget, visibleRegion);
    
    needsClipToVisibleRegion = false;
  }
  
  if (is2D) {
    paintLayerContext.AnnotateOpaqueRect();
  }

  bool clipIsEmpty = !aTarget || aTarget->GetClipExtents().IsEmpty();
  if (clipIsEmpty) {
    PaintSelfOrChildren(paintLayerContext, aTarget);
    return;
  }

  if (is2D) {
    if (needsGroup) {
      nsRefPtr<gfxContext> groupTarget = PushGroupForLayer(aTarget, aLayer, aLayer->GetEffectiveVisibleRegion(),
                                      &needsClipToVisibleRegion);
      PaintSelfOrChildren(paintLayerContext, groupTarget);
      aTarget->PopGroupToSource();
      FlushGroup(paintLayerContext, needsClipToVisibleRegion);
    } else {
      PaintSelfOrChildren(paintLayerContext, aTarget);
    }
  } else {
    const nsIntRect& bounds = visibleRegion.GetBounds();
    RefPtr<DrawTarget> untransformedDT =
      gfxPlatform::GetPlatform()->CreateOffscreenContentDrawTarget(IntSize(bounds.width, bounds.height),
                                                                   SurfaceFormat::B8G8R8A8);
    if (!untransformedDT) {
      return;
    }

    nsRefPtr<gfxContext> groupTarget = new gfxContext(untransformedDT,
                                                      Point(bounds.x, bounds.y));

    PaintSelfOrChildren(paintLayerContext, groupTarget);

    
    
    MOZ_ASSERT(untransformedDT,
               "We should always allocate an untransformed surface with 3d transforms!");
    gfxRect destRect;
#ifdef DEBUG
    if (aLayer->GetDebugColorIndex() != 0) {
      gfxRGBA  color((aLayer->GetDebugColorIndex() & 1) ? 1.0 : 0.0,
                     (aLayer->GetDebugColorIndex() & 2) ? 1.0 : 0.0,
                     (aLayer->GetDebugColorIndex() & 4) ? 1.0 : 0.0,
                     1.0);

      nsRefPtr<gfxContext> temp = new gfxContext(untransformedDT, Point(bounds.x, bounds.y));
      temp->SetColor(color);
      temp->Paint();
    }
#endif
    gfx3DMatrix effectiveTransform;
    effectiveTransform = gfx::To3DMatrix(aLayer->GetEffectiveTransform());
    nsRefPtr<gfxASurface> result =
      Transform3D(untransformedDT->Snapshot(), aTarget, bounds,
                  effectiveTransform, destRect);

    if (result) {
      aTarget->SetSource(result, destRect.TopLeft());
      
      
      
      aTarget->NewPath();
      aTarget->SnappedRectangle(destRect);
      aTarget->Clip();
      FlushGroup(paintLayerContext, needsClipToVisibleRegion);
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

}
}
