




#include "LayerManagerComposite.h"
#include <stddef.h>                     
#include <stdint.h>                     
#include "CanvasLayerComposite.h"       
#include "ColorLayerComposite.h"        
#include "Composer2D.h"                 
#include "CompositableHost.h"           
#include "ContainerLayerComposite.h"    
#include "FPSCounter.h"                 
#include "FrameMetrics.h"               
#include "GeckoProfiler.h"              
#include "ImageLayerComposite.h"        
#include "Layers.h"                     
#include "LayerScope.h"                 
#include "protobuf/LayerScopePacket.pb.h" 
#include "ThebesLayerComposite.h"       
#include "TiledLayerBuffer.h"           
#include "Units.h"                      
#include "gfx2DGlue.h"                  
#include "gfxPrefs.h"                   
#ifdef XP_MACOSX
#include "gfxPlatformMac.h"
#endif
#include "gfxRect.h"                    
#include "gfxUtils.h"                   
#include "mozilla/Assertions.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/2D.h"             
#include "mozilla/gfx/Matrix.h"         
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Rect.h"           
#include "mozilla/gfx/Types.h"          
#include "mozilla/layers/Compositor.h"  
#include "mozilla/layers/CompositorTypes.h"
#include "mozilla/layers/Effects.h"     
#include "mozilla/layers/LayersTypes.h"  
#include "ipc/CompositorBench.h"        
#include "ipc/ShadowLayerUtils.h"
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsIWidget.h"                  
#include "nsPoint.h"                    
#include "nsRect.h"                     
#include "nsRegion.h"                   
#ifdef MOZ_WIDGET_ANDROID
#include <android/log.h>
#endif
#include "GeckoProfiler.h"
#include "TextRenderer.h"               

class gfxContext;
struct nsIntSize;


namespace mozilla {
namespace layers {

class ImageLayer;

using namespace mozilla::gfx;
using namespace mozilla::gl;

static LayerComposite*
ToLayerComposite(Layer* aLayer)
{
  return static_cast<LayerComposite*>(aLayer->ImplData());
}

static void ClearSubtree(Layer* aLayer)
{
  ToLayerComposite(aLayer)->CleanupResources();
  for (Layer* child = aLayer->GetFirstChild(); child;
       child = child->GetNextSibling()) {
    ClearSubtree(child);
  }
}

void
LayerManagerComposite::ClearCachedResources(Layer* aSubtree)
{
  MOZ_ASSERT(!aSubtree || aSubtree->Manager() == this);
  Layer* subtree = aSubtree ? aSubtree : mRoot.get();
  if (!subtree) {
    return;
  }

  ClearSubtree(subtree);
  
  
  
}




LayerManagerComposite::LayerManagerComposite(Compositor* aCompositor)
: mCompositor(aCompositor)
, mInTransaction(false)
, mIsCompositorReady(false)
, mDebugOverlayWantsNextFrame(false)
, mGeometryChanged(true)
{
  mTextRenderer = new TextRenderer(aCompositor);
  MOZ_ASSERT(aCompositor);
}

LayerManagerComposite::~LayerManagerComposite()
{
  Destroy();
}


bool
LayerManagerComposite::Initialize()
{
  bool result = mCompositor->Initialize();
  return result;
}

void
LayerManagerComposite::Destroy()
{
  if (!mDestroyed) {
    mCompositor->GetWidget()->CleanupWindowEffects();
    if (mRoot) {
      RootLayer()->Destroy();
    }
    mRoot = nullptr;
    mDestroyed = true;
  }
}

void
LayerManagerComposite::UpdateRenderBounds(const nsIntRect& aRect)
{
  mRenderBounds = aRect;
}

bool
LayerManagerComposite::AreComponentAlphaLayersEnabled()
{
  return Compositor::GetBackend() != LayersBackend::LAYERS_BASIC &&
         LayerManager::AreComponentAlphaLayersEnabled();
}

void
LayerManagerComposite::BeginTransaction()
{
  mInTransaction = true;
  
  if (!mCompositor->Ready()) {
    return;
  }
  
  mIsCompositorReady = true;

  if (Compositor::GetBackend() == LayersBackend::LAYERS_OPENGL ||
      Compositor::GetBackend() == LayersBackend::LAYERS_BASIC) {
    mClonedLayerTreeProperties = LayerProperties::CloneFrom(GetRoot());
  }
}

void
LayerManagerComposite::BeginTransactionWithDrawTarget(DrawTarget* aTarget, const nsIntRect& aRect)
{
  mInTransaction = true;
  
  if (!mCompositor->Ready()) {
    return;
  }

#ifdef MOZ_LAYERS_HAVE_LOG
  MOZ_LAYERS_LOG(("[----- BeginTransaction"));
  Log();
#endif

  if (mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return;
  }

  mIsCompositorReady = true;
  mCompositor->SetTargetContext(aTarget, aRect);
  mTarget = aTarget;
  mTargetBounds = aRect;
}

bool
LayerManagerComposite::EndEmptyTransaction(EndTransactionFlags aFlags)
{
  NS_ASSERTION(mInTransaction, "Didn't call BeginTransaction?");
  if (!mRoot) {
    mInTransaction = false;
    mIsCompositorReady = false;
    return false;
  }

  EndTransaction(nullptr, nullptr);
  return true;
}

void
LayerManagerComposite::EndTransaction(DrawThebesLayerCallback aCallback,
                                      void* aCallbackData,
                                      EndTransactionFlags aFlags)
{
  NS_ASSERTION(mInTransaction, "Didn't call BeginTransaction?");
  NS_ASSERTION(!aCallback && !aCallbackData, "Not expecting callbacks here");
  mInTransaction = false;

  if (!mIsCompositorReady) {
    return;
  }
  mIsCompositorReady = false;

#ifdef MOZ_LAYERS_HAVE_LOG
  MOZ_LAYERS_LOG(("  ----- (beginning paint)"));
  Log();
#endif

  if (mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return;
  }

  if (mRoot && mClonedLayerTreeProperties) {
    MOZ_ASSERT(!mTarget);
    nsIntRegion invalid =
      mClonedLayerTreeProperties->ComputeDifferences(mRoot, nullptr, &mGeometryChanged);
    mClonedLayerTreeProperties = nullptr;

    mInvalidRegion.Or(mInvalidRegion, invalid);
  } else if (!mTarget) {
    mInvalidRegion.Or(mInvalidRegion, mRenderBounds);
  }

  if (mRoot && !(aFlags & END_NO_IMMEDIATE_REDRAW)) {
    if (aFlags & END_NO_COMPOSITE) {
      
      
      mRoot->ApplyPendingUpdatesToSubtree();
    }

    
    
    mRoot->ComputeEffectiveTransforms(gfx::Matrix4x4());

    Render();
    mGeometryChanged = false;
  } else {
    
    mGeometryChanged = true;
  }

  mCompositor->ClearTargetContext();
  mTarget = nullptr;

#ifdef MOZ_LAYERS_HAVE_LOG
  Log();
  MOZ_LAYERS_LOG(("]----- EndTransaction"));
#endif
}

TemporaryRef<DrawTarget>
LayerManagerComposite::CreateOptimalMaskDrawTarget(const IntSize &aSize)
{
  NS_RUNTIMEABORT("Should only be called on the drawing side");
  return nullptr;
}

already_AddRefed<ThebesLayer>
LayerManagerComposite::CreateThebesLayer()
{
  NS_RUNTIMEABORT("Should only be called on the drawing side");
  return nullptr;
}

already_AddRefed<ContainerLayer>
LayerManagerComposite::CreateContainerLayer()
{
  NS_RUNTIMEABORT("Should only be called on the drawing side");
  return nullptr;
}

already_AddRefed<ImageLayer>
LayerManagerComposite::CreateImageLayer()
{
  NS_RUNTIMEABORT("Should only be called on the drawing side");
  return nullptr;
}

already_AddRefed<ColorLayer>
LayerManagerComposite::CreateColorLayer()
{
  NS_RUNTIMEABORT("Should only be called on the drawing side");
  return nullptr;
}

already_AddRefed<CanvasLayer>
LayerManagerComposite::CreateCanvasLayer()
{
  NS_RUNTIMEABORT("Should only be called on the drawing side");
  return nullptr;
}

LayerComposite*
LayerManagerComposite::RootLayer() const
{
  if (mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return nullptr;
  }

  return ToLayerComposite(mRoot);
}

#ifdef MOZ_PROFILING




#include "qrcode_table.h"
#endif

static uint16_t sFrameCount = 0;
void
LayerManagerComposite::RenderDebugOverlay(const Rect& aBounds)
{
  bool drawFps = gfxPrefs::LayersDrawFPS();
  bool drawFrameCounter = gfxPrefs::DrawFrameCounter();
  bool drawFrameColorBars = gfxPrefs::CompositorDrawColorBars();

  if (drawFps) {
    if (!mFPS) {
      mFPS = new FPSState();
    }

    float fillRatio = mCompositor->GetFillRatio();
    mFPS->DrawFPS(TimeStamp::Now(), drawFrameColorBars ? 10 : 0, 0, unsigned(fillRatio), mCompositor);
  } else {
    mFPS = nullptr;
  }

  if (drawFrameColorBars) {
    gfx::Rect sideRect(0, 0, 10, aBounds.height);

    EffectChain effects;
    effects.mPrimaryEffect = new EffectSolidColor(gfxUtils::GetColorForFrameNumber(sFrameCount));
    mCompositor->DrawQuad(sideRect,
                          sideRect,
                          effects,
                          1.0,
                          gfx::Matrix4x4());
  }

#ifdef MOZ_PROFILING
  if (drawFrameCounter) {
    profiler_set_frame_number(sFrameCount);
    const char* qr = sQRCodeTable[sFrameCount%256];

    int size = 21;
    int padding = 2;
    float opacity = 1.0;
    const uint16_t bitWidth = 5;
    gfx::Rect clip(0,0, bitWidth*640, bitWidth*640);

    
    gfx::Color bitColor(1.0, 1.0, 1.0, 1.0);
    EffectChain effects;
    effects.mPrimaryEffect = new EffectSolidColor(bitColor);
    int totalSize = (size + padding * 2) * bitWidth;
    mCompositor->DrawQuad(gfx::Rect(0, 0, totalSize, totalSize),
                          clip,
                          effects,
                          opacity,
                          gfx::Matrix4x4());

    
    effects.mPrimaryEffect = new EffectSolidColor(gfx::Color(0, 0, 0, 1.0));
    for (int y = 0; y < size; y++) {
      for (int x = 0; x < size; x++) {
        
        int currBit = 128 >> ((x + y * 21) % 8);
        int i = (x + y * 21) / 8;
        if (qr[i] & currBit) {
          mCompositor->DrawQuad(gfx::Rect(bitWidth * (x + padding),
                                          bitWidth * (y + padding),
                                          bitWidth, bitWidth),
                                clip,
                                effects,
                                opacity,
                                gfx::Matrix4x4());
        }
      }
    }
  }
#endif

  if (drawFrameColorBars || drawFrameCounter) {
    
    sFrameCount++;
  }
}

RefPtr<CompositingRenderTarget>
LayerManagerComposite::PushGroup()
{
  RefPtr<CompositingRenderTarget> previousTarget = mCompositor->GetCurrentRenderTarget();
  
  
  IntRect rect(previousTarget->GetOrigin(), previousTarget->GetSize());
  
  MOZ_ASSERT(rect.x == 0 && rect.y == 0);
  if (!mTwoPassTmpTarget ||
      mTwoPassTmpTarget->GetSize() != previousTarget->GetSize() ||
      mTwoPassTmpTarget->GetOrigin() != previousTarget->GetOrigin()) {
    mTwoPassTmpTarget = mCompositor->CreateRenderTarget(rect, INIT_MODE_NONE);
  }
  mCompositor->SetRenderTarget(mTwoPassTmpTarget);
  return previousTarget;
}
void LayerManagerComposite::PopGroup(RefPtr<CompositingRenderTarget> aPreviousTarget, nsIntRect aClipRect)
{
  mCompositor->SetRenderTarget(aPreviousTarget);

  EffectChain effectChain(RootLayer());
  Matrix5x4 matrix;
  if (gfxPrefs::Grayscale()) {
    matrix._11 = matrix._12 = matrix._13 = 0.2126f;
    matrix._21 = matrix._22 = matrix._23 = 0.7152f;
    matrix._31 = matrix._32 = matrix._33 = 0.0722f;
  }

  if (gfxPrefs::Invert()) {
    matrix._11 = -matrix._11;
    matrix._12 = -matrix._12;
    matrix._13 = -matrix._13;
    matrix._21 = -matrix._21;
    matrix._22 = -matrix._22;
    matrix._23 = -matrix._23;
    matrix._31 = -matrix._31;
    matrix._32 = -matrix._32;
    matrix._33 = -matrix._33;
    matrix._51 = 1;
    matrix._52 = 1;
    matrix._53 = 1;
  }

  effectChain.mPrimaryEffect = new EffectRenderTarget(mTwoPassTmpTarget);
  effectChain.mSecondaryEffects[EffectTypes::COLOR_MATRIX] = new EffectColorMatrix(matrix);

  gfx::Rect clipRectF(aClipRect.x, aClipRect.y, aClipRect.width, aClipRect.height);
  mCompositor->DrawQuad(Rect(Point(0, 0), Size(mTwoPassTmpTarget->GetSize())), clipRectF, effectChain, 1.,
                        Matrix4x4());
}

void
LayerManagerComposite::Render()
{
  PROFILER_LABEL("LayerManagerComposite", "Render",
    js::ProfileEntry::Category::GRAPHICS);

  if (mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return;
  }

  
  nsRefPtr<Composer2D> composer2D = mCompositor->GetWidget()->GetComposer2D();

  
  LayerScopeAutoFrame frame(PR_Now());

  
  if (gfxPrefs::LayersDump()) {
    this->Dump();
  }

  
  if (LayerScope::CheckSendable()) {
    
    
    auto packet = MakeUnique<layerscope::Packet>();
    layerscope::LayersPacket* layersPacket = packet->mutable_layers();
    this->Dump(layersPacket);
    LayerScope::SendLayerDump(Move(packet));
  }

  if (gfxPrefs::Invert() || gfxPrefs::Grayscale()) {
    composer2D = nullptr;
  }

  if (!mTarget && composer2D && composer2D->TryRender(mRoot, mWorldMatrix, mGeometryChanged)) {
    if (mFPS) {
      double fps = mFPS->mCompositionFps.AddFrameAndGetFps(TimeStamp::Now());
      if (gfxPrefs::LayersDrawFPS()) {
        printf_stderr("HWComposer: FPS is %g\n", fps);
      }
    }
    mCompositor->EndFrameForExternalComposition(mWorldMatrix);
    
    mInvalidRegion.SetEmpty();
    return;
  }

  {
    PROFILER_LABEL("LayerManagerComposite", "PreRender",
      js::ProfileEntry::Category::GRAPHICS);

    if (!mCompositor->GetWidget()->PreRender(this)) {
      return;
    }
  }

  nsIntRegion invalid;
  if (mTarget) {
    invalid = mTargetBounds;
  } else {
    invalid = mInvalidRegion;
    
    mInvalidRegion.SetEmpty();
  }

  nsIntRect clipRect;
  Rect bounds(mRenderBounds.x, mRenderBounds.y, mRenderBounds.width, mRenderBounds.height);
  Rect actualBounds;

  CompositorBench(mCompositor, bounds);

  if (mRoot->GetClipRect()) {
    clipRect = *mRoot->GetClipRect();
    WorldTransformRect(clipRect);
    Rect rect(clipRect.x, clipRect.y, clipRect.width, clipRect.height);
    mCompositor->BeginFrame(invalid, &rect, mWorldMatrix, bounds, nullptr, &actualBounds);
  } else {
    gfx::Rect rect;
    mCompositor->BeginFrame(invalid, nullptr, mWorldMatrix, bounds, &rect, &actualBounds);
    clipRect = nsIntRect(rect.x, rect.y, rect.width, rect.height);
  }

  if (actualBounds.IsEmpty()) {
    mCompositor->GetWidget()->PostRender(this);
    return;
  }

  
  mCompositor->GetWidget()->DrawWindowUnderlay(this, nsIntRect(actualBounds.x,
                                                               actualBounds.y,
                                                               actualBounds.width,
                                                               actualBounds.height));

  RefPtr<CompositingRenderTarget> previousTarget;
  if (gfxPrefs::Invert() || gfxPrefs::Grayscale()) {
    previousTarget = PushGroup();
  } else {
    mTwoPassTmpTarget = nullptr;
  }

  
  RootLayer()->Prepare(RenderTargetPixel::FromUntyped(clipRect));
  RootLayer()->RenderLayer(clipRect);

  if (!mRegionToClear.IsEmpty()) {
    nsIntRegionRectIterator iter(mRegionToClear);
    const nsIntRect *r;
    while ((r = iter.Next())) {
      mCompositor->ClearRect(Rect(r->x, r->y, r->width, r->height));
    }
  }

  if (mTwoPassTmpTarget) {
    PopGroup(previousTarget, clipRect);
  }

  
  mCompositor->GetWidget()->DrawWindowOverlay(this, nsIntRect(actualBounds.x,
                                                              actualBounds.y,
                                                              actualBounds.width,
                                                              actualBounds.height));

  
  RenderDebugOverlay(actualBounds);

  {
    PROFILER_LABEL("LayerManagerComposite", "EndFrame",
      js::ProfileEntry::Category::GRAPHICS);

    mCompositor->EndFrame();
    mCompositor->SetFBAcquireFence(mRoot);
  }

  mCompositor->GetWidget()->PostRender(this);

  RecordFrame();
}

void
LayerManagerComposite::SetWorldTransform(const gfx::Matrix& aMatrix)
{
  NS_ASSERTION(aMatrix.PreservesAxisAlignedRectangles(),
               "SetWorldTransform only accepts matrices that satisfy PreservesAxisAlignedRectangles");
  NS_ASSERTION(!aMatrix.HasNonIntegerScale(),
               "SetWorldTransform only accepts matrices with integer scale");

  mWorldMatrix = aMatrix;
}

gfx::Matrix&
LayerManagerComposite::GetWorldTransform(void)
{
  return mWorldMatrix;
}

void
LayerManagerComposite::WorldTransformRect(nsIntRect& aRect)
{
  gfx::Rect grect(aRect.x, aRect.y, aRect.width, aRect.height);
  grect = mWorldMatrix.TransformBounds(grect);
  aRect.SetRect(grect.X(), grect.Y(), grect.Width(), grect.Height());
}

static void
SubtractTransformedRegion(nsIntRegion& aRegion,
                          const nsIntRegion& aRegionToSubtract,
                          const Matrix4x4& aTransform)
{
  if (aRegionToSubtract.IsEmpty()) {
    return;
  }

  
  
  nsIntRegionRectIterator it(aRegionToSubtract);
  while (const nsIntRect* rect = it.Next()) {
    Rect incompleteRect = aTransform.TransformBounds(ToRect(*rect));
    aRegion.Sub(aRegion, nsIntRect(incompleteRect.x,
                                   incompleteRect.y,
                                   incompleteRect.width,
                                   incompleteRect.height));
  }
}

 void
LayerManagerComposite::ComputeRenderIntegrityInternal(Layer* aLayer,
                                                      nsIntRegion& aScreenRegion,
                                                      nsIntRegion& aLowPrecisionScreenRegion,
                                                      const Matrix4x4& aTransform)
{
  if (aLayer->GetOpacity() <= 0.f ||
      (aScreenRegion.IsEmpty() && aLowPrecisionScreenRegion.IsEmpty())) {
    return;
  }

  
  ContainerLayer* container = aLayer->AsContainerLayer();
  if (container) {
    
    Matrix4x4 transform = aTransform;
    if (container->UseIntermediateSurface()) {
      transform = aLayer->GetEffectiveTransform();
      transform = aTransform * transform;
    }
    for (Layer* child = aLayer->GetFirstChild(); child;
         child = child->GetNextSibling()) {
      ComputeRenderIntegrityInternal(child, aScreenRegion, aLowPrecisionScreenRegion, transform);
    }
    return;
  }

  
  ThebesLayer* thebesLayer = aLayer->AsThebesLayer();
  if (!thebesLayer) {
    return;
  }

  
  nsIntRegion incompleteRegion = aLayer->GetEffectiveVisibleRegion();
  incompleteRegion.Sub(incompleteRegion, thebesLayer->GetValidRegion());

  if (!incompleteRegion.IsEmpty()) {
    
    Matrix4x4 transformToScreen = aLayer->GetEffectiveTransform();
    transformToScreen = aTransform * transformToScreen;

    SubtractTransformedRegion(aScreenRegion, incompleteRegion, transformToScreen);

    
    TiledLayerComposer* composer = nullptr;
    LayerComposite* shadow = aLayer->AsLayerComposite();
    if (shadow) {
      composer = shadow->GetTiledLayerComposer();
      if (composer) {
        incompleteRegion.Sub(incompleteRegion, composer->GetValidLowPrecisionRegion());
        if (!incompleteRegion.IsEmpty()) {
          SubtractTransformedRegion(aLowPrecisionScreenRegion, incompleteRegion, transformToScreen);
        }
      }
    }

    
    
    if (!composer) {
      SubtractTransformedRegion(aLowPrecisionScreenRegion, incompleteRegion, transformToScreen);
    }
  }
}

#ifdef MOZ_ANDROID_OMTC
static float
GetDisplayportCoverage(const CSSRect& aDisplayPort,
                       const Matrix4x4& aTransformToScreen,
                       const nsIntRect& aScreenRect)
{
  Rect transformedDisplayport =
    aTransformToScreen.TransformBounds(aDisplayPort.ToUnknownRect());

  transformedDisplayport.RoundOut();
  nsIntRect displayport = nsIntRect(transformedDisplayport.x,
                                    transformedDisplayport.y,
                                    transformedDisplayport.width,
                                    transformedDisplayport.height);
  if (!displayport.Contains(aScreenRect)) {
    nsIntRegion coveredRegion;
    coveredRegion.And(aScreenRect, displayport);
    return coveredRegion.Area() / (float)(aScreenRect.width * aScreenRect.height);
  }

  return 1.0f;
}
#endif 

float
LayerManagerComposite::ComputeRenderIntegrity()
{
  
  Layer* root = GetRoot();
  if (!gfxPrefs::UseProgressiveTilePainting() || !root) {
    return 1.f;
  }

  const FrameMetrics& rootMetrics = root->GetFrameMetrics();
  ParentLayerIntRect bounds = RoundedToInt(rootMetrics.mCompositionBounds);
  nsIntRect screenRect(bounds.x,
                       bounds.y,
                       bounds.width,
                       bounds.height);

  float lowPrecisionMultiplier = 1.0f;
  float highPrecisionMultiplier = 1.0f;

#ifdef MOZ_ANDROID_OMTC
  
  
  Layer* primaryScrollable = GetPrimaryScrollableLayer();
  if (primaryScrollable) {
    
    
    const FrameMetrics& metrics = primaryScrollable->GetFrameMetrics();
    Matrix4x4 transform = primaryScrollable->GetEffectiveTransform();
    transform.ScalePost(metrics.mResolution.scale, metrics.mResolution.scale, 1);

    
    Rect documentBounds =
      transform.TransformBounds(Rect(metrics.mScrollableRect.x - metrics.GetScrollOffset().x,
                                     metrics.mScrollableRect.y - metrics.GetScrollOffset().y,
                                     metrics.mScrollableRect.width,
                                     metrics.mScrollableRect.height));
    documentBounds.RoundOut();
    screenRect = screenRect.Intersect(nsIntRect(documentBounds.x, documentBounds.y,
                                                documentBounds.width, documentBounds.height));

    
    
    if (screenRect.IsEmpty()) {
      return 1.0f;
    }

    
    bool hasLowPrecision = false;
    if (!metrics.mCriticalDisplayPort.IsEmpty()) {
      hasLowPrecision = true;
      highPrecisionMultiplier =
        GetDisplayportCoverage(metrics.mCriticalDisplayPort, transform, screenRect);
    }

    
    if (!metrics.mDisplayPort.IsEmpty()) {
      if (hasLowPrecision) {
        lowPrecisionMultiplier =
          GetDisplayportCoverage(metrics.mDisplayPort, transform, screenRect);
      } else {
        lowPrecisionMultiplier = highPrecisionMultiplier =
          GetDisplayportCoverage(metrics.mDisplayPort, transform, screenRect);
      }
    }
  }

  
  if (highPrecisionMultiplier <= 0.0f && lowPrecisionMultiplier <= 0.0f) {
    return 0.0f;
  }
#endif 

  nsIntRegion screenRegion(screenRect);
  nsIntRegion lowPrecisionScreenRegion(screenRect);
  Matrix4x4 transform;
  ComputeRenderIntegrityInternal(root, screenRegion,
                                 lowPrecisionScreenRegion, transform);

  if (!screenRegion.IsEqual(screenRect)) {
    
    
    float screenArea = screenRect.width * screenRect.height;
    float highPrecisionIntegrity = screenRegion.Area() / screenArea;
    float lowPrecisionIntegrity = 1.f;
    if (!lowPrecisionScreenRegion.IsEqual(screenRect)) {
      lowPrecisionIntegrity = lowPrecisionScreenRegion.Area() / screenArea;
    }

    return ((highPrecisionIntegrity * highPrecisionMultiplier) +
            (lowPrecisionIntegrity * lowPrecisionMultiplier)) / 2;
  }

  return 1.f;
}

already_AddRefed<ThebesLayerComposite>
LayerManagerComposite::CreateThebesLayerComposite()
{
  if (mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return nullptr;
  }
  return nsRefPtr<ThebesLayerComposite>(new ThebesLayerComposite(this)).forget();
}

already_AddRefed<ContainerLayerComposite>
LayerManagerComposite::CreateContainerLayerComposite()
{
  if (mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return nullptr;
  }
  return nsRefPtr<ContainerLayerComposite>(new ContainerLayerComposite(this)).forget();
}

already_AddRefed<ImageLayerComposite>
LayerManagerComposite::CreateImageLayerComposite()
{
  if (mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return nullptr;
  }
  return nsRefPtr<ImageLayerComposite>(new ImageLayerComposite(this)).forget();
}

already_AddRefed<ColorLayerComposite>
LayerManagerComposite::CreateColorLayerComposite()
{
  if (LayerManagerComposite::mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return nullptr;
  }
  return nsRefPtr<ColorLayerComposite>(new ColorLayerComposite(this)).forget();
}

already_AddRefed<CanvasLayerComposite>
LayerManagerComposite::CreateCanvasLayerComposite()
{
  if (LayerManagerComposite::mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return nullptr;
  }
  return nsRefPtr<CanvasLayerComposite>(new CanvasLayerComposite(this)).forget();
}

already_AddRefed<RefLayerComposite>
LayerManagerComposite::CreateRefLayerComposite()
{
  if (LayerManagerComposite::mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return nullptr;
  }
  return nsRefPtr<RefLayerComposite>(new RefLayerComposite(this)).forget();
}

LayerManagerComposite::AutoAddMaskEffect::AutoAddMaskEffect(Layer* aMaskLayer,
                                                            EffectChain& aEffects,
                                                            bool aIs3D)
  : mCompositable(nullptr), mFailed(false)
{
  if (!aMaskLayer) {
    return;
  }

  mCompositable = ToLayerComposite(aMaskLayer)->GetCompositableHost();
  if (!mCompositable) {
    NS_WARNING("Mask layer with no compositable host");
    mFailed = true;
    return;
  }

  if (!mCompositable->AddMaskEffect(aEffects, aMaskLayer->GetEffectiveTransform(), aIs3D)) {
    mCompositable = nullptr;
    mFailed = true;
  }
}

LayerManagerComposite::AutoAddMaskEffect::~AutoAddMaskEffect()
{
  if (!mCompositable) {
    return;
  }

  mCompositable->RemoveMaskEffect();
}

TemporaryRef<DrawTarget>
LayerManagerComposite::CreateDrawTarget(const IntSize &aSize,
                                        SurfaceFormat aFormat)
{
#ifdef XP_MACOSX
  
  
  
  
  bool useAcceleration = aSize.width <= 4096 && aSize.height <= 4096 &&
                         aSize.width > 64 && aSize.height > 64 &&
                         gfxPlatformMac::GetPlatform()->UseAcceleratedCanvas();
  if (useAcceleration) {
    return Factory::CreateDrawTarget(BackendType::COREGRAPHICS_ACCELERATED,
                                     aSize, aFormat);
  }
#endif
  return LayerManager::CreateDrawTarget(aSize, aFormat);
}

LayerComposite::LayerComposite(LayerManagerComposite *aManager)
  : mCompositeManager(aManager)
  , mCompositor(aManager->GetCompositor())
  , mShadowOpacity(1.0)
  , mUseShadowClipRect(false)
  , mShadowTransformSetByAnimation(false)
  , mDestroyed(false)
  , mLayerComposited(false)
{ }

LayerComposite::~LayerComposite()
{
}

void
LayerComposite::Destroy()
{
  if (!mDestroyed) {
    mDestroyed = true;
    CleanupResources();
  }
}

void
LayerComposite::AddBlendModeEffect(EffectChain& aEffectChain)
{
  gfx::CompositionOp blendMode = GetLayer()->GetEffectiveMixBlendMode();
  if (blendMode == gfx::CompositionOp::OP_OVER) {
    return;
  }

  aEffectChain.mSecondaryEffects[EffectTypes::BLEND_MODE] = new EffectBlendMode(blendMode);
  return;
}

bool
LayerManagerComposite::CanUseCanvasLayerForSize(const IntSize &aSize)
{
  return mCompositor->CanUseCanvasLayerForSize(gfx::IntSize(aSize.width,
                                                            aSize.height));
}

void
LayerManagerComposite::NotifyShadowTreeTransaction()
{
  if (mFPS) {
    mFPS->NotifyShadowTreeTransaction();
  }
}

#ifndef MOZ_HAVE_PLATFORM_SPECIFIC_LAYER_BUFFERS

 bool
LayerManagerComposite::SupportsDirectTexturing()
{
  return false;
}

 void
LayerManagerComposite::PlatformSyncBeforeReplyUpdate()
{
}

#endif  

} 
} 
