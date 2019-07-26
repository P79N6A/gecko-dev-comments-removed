




#include "LayerManagerComposite.h"
#include <stddef.h>                     
#include <stdint.h>                     
#include "CanvasLayerComposite.h"       
#include "ColorLayerComposite.h"        
#include "Composer2D.h"                 
#include "CompositableHost.h"           
#include "ContainerLayerComposite.h"    
#include "FrameMetrics.h"               
#include "GeckoProfiler.h"              
#include "ImageLayerComposite.h"        
#include "Layers.h"                     
#include "ThebesLayerComposite.h"       
#include "TiledLayerBuffer.h"           
#include "Units.h"                      
#include "gfx2DGlue.h"                  
#include "gfx3DMatrix.h"                
#include "gfxMatrix.h"                  
#include "gfxPlatform.h"                
#ifdef XP_MACOSX
#include "gfxPlatformMac.h"
#endif
#include "gfxPoint.h"                   
#include "gfxRect.h"                    
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

class gfxASurface;
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
{
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
  mComposer2D = mCompositor->GetWidget()->GetComposer2D();
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

    mCompositor->Destroy();

    mDestroyed = true;
  }
}

void
LayerManagerComposite::UpdateRenderBounds(const nsIntRect& aRect)
{
  mRenderBounds = aRect;
}

void
LayerManagerComposite::BeginTransaction()
{
  mInTransaction = true;
}

void
LayerManagerComposite::BeginTransactionWithDrawTarget(DrawTarget* aTarget)
{
  mInTransaction = true;

#ifdef MOZ_LAYERS_HAVE_LOG
  MOZ_LAYERS_LOG(("[----- BeginTransaction"));
  Log();
#endif

  if (mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return;
  }

  mCompositor->SetTargetContext(aTarget);
}

bool
LayerManagerComposite::EndEmptyTransaction(EndTransactionFlags aFlags)
{
  mInTransaction = false;

  if (!mRoot)
    return false;

  EndTransaction(nullptr, nullptr);
  return true;
}

void
LayerManagerComposite::EndTransaction(DrawThebesLayerCallback aCallback,
                                      void* aCallbackData,
                                      EndTransactionFlags aFlags)
{
  mInTransaction = false;

#ifdef MOZ_LAYERS_HAVE_LOG
  MOZ_LAYERS_LOG(("  ----- (beginning paint)"));
  Log();
#endif

  if (mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return;
  }

  if (mRoot && !(aFlags & END_NO_IMMEDIATE_REDRAW)) {
    if (aFlags & END_NO_COMPOSITE) {
      
      
      mRoot->ApplyPendingUpdatesToSubtree();
    }

    
    
    mRoot->ComputeEffectiveTransforms(gfx3DMatrix());

    mThebesLayerCallback = aCallback;
    mThebesLayerCallbackData = aCallbackData;

    Render();

    mThebesLayerCallback = nullptr;
    mThebesLayerCallbackData = nullptr;
  }

  mCompositor->SetTargetContext(nullptr);

#ifdef MOZ_LAYERS_HAVE_LOG
  Log();
  MOZ_LAYERS_LOG(("]----- EndTransaction"));
#endif
}

already_AddRefed<gfxASurface>
LayerManagerComposite::CreateOptimalMaskSurface(const gfxIntSize &aSize)
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

  return static_cast<LayerComposite*>(mRoot->ImplData());
}

static uint16_t sFrameCount = 0;
void
LayerManagerComposite::RenderDebugOverlay(const Rect& aBounds)
{
  if (!gfxPlatform::DrawFrameCounter()) {
    return;
  }

  profiler_set_frame_number(sFrameCount);

  uint16_t frameNumber = sFrameCount;
  const uint16_t bitWidth = 3;
  float opacity = 1.0;
  gfx::Rect clip(0,0, bitWidth*16, bitWidth);
  for (size_t i = 0; i < 16; i++) {

    gfx::Color bitColor;
    if ((frameNumber >> i) & 0x1) {
      bitColor = gfx::Color(0, 0, 0, 1.0);
    } else {
      bitColor = gfx::Color(1.0, 1.0, 1.0, 1.0);
    }
    EffectChain effects;
    effects.mPrimaryEffect = new EffectSolidColor(bitColor);
    mCompositor->DrawQuad(gfx::Rect(bitWidth*i, 0, bitWidth, bitWidth),
                          clip,
                          effects,
                          opacity,
                          gfx::Matrix4x4(),
                          gfx::Point());
  }
  
  sFrameCount++;
}

void
LayerManagerComposite::Render()
{
  PROFILER_LABEL("LayerManagerComposite", "Render");
  if (mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return;
  }

  if (mComposer2D && mComposer2D->TryRender(mRoot, mWorldMatrix)) {
    mCompositor->EndFrameForExternalComposition(mWorldMatrix);
    return;
  }

  {
    PROFILER_LABEL("LayerManagerComposite", "PreRender");
    if (!mCompositor->GetWidget()->PreRender(this)) {
      return;
    }
  }

  nsIntRect clipRect;
  Rect bounds(mRenderBounds.x, mRenderBounds.y, mRenderBounds.width, mRenderBounds.height);
  Rect actualBounds;
  if (mRoot->GetClipRect()) {
    clipRect = *mRoot->GetClipRect();
    WorldTransformRect(clipRect);
    Rect rect(clipRect.x, clipRect.y, clipRect.width, clipRect.height);
    mCompositor->BeginFrame(&rect, mWorldMatrix, bounds, nullptr, &actualBounds);
  } else {
    gfx::Rect rect;
    mCompositor->BeginFrame(nullptr, mWorldMatrix, bounds, &rect, &actualBounds);
    clipRect = nsIntRect(rect.x, rect.y, rect.width, rect.height);
  }

  if (actualBounds.IsEmpty()) {
    return;
  }

  
  mCompositor->SaveState();
  mCompositor->GetWidget()->DrawWindowUnderlay(this, nsIntRect(actualBounds.x,
                                                               actualBounds.y,
                                                               actualBounds.width,
                                                               actualBounds.height));
  mCompositor->RestoreState();

  
  RootLayer()->RenderLayer(nsIntPoint(0, 0), clipRect);

  
  mCompositor->SaveState();
  mCompositor->GetWidget()->DrawWindowOverlay(this, nsIntRect(actualBounds.x,
                                                              actualBounds.y,
                                                              actualBounds.width,
                                                              actualBounds.height));
  mCompositor->RestoreState();

  
  RenderDebugOverlay(actualBounds);

  {
    PROFILER_LABEL("LayerManagerComposite", "EndFrame");
    mCompositor->EndFrame();
  }
}

void
LayerManagerComposite::SetWorldTransform(const gfxMatrix& aMatrix)
{
  NS_ASSERTION(aMatrix.PreservesAxisAlignedRectangles(),
               "SetWorldTransform only accepts matrices that satisfy PreservesAxisAlignedRectangles");
  NS_ASSERTION(!aMatrix.HasNonIntegerScale(),
               "SetWorldTransform only accepts matrices with integer scale");

  mWorldMatrix = aMatrix;
}

gfxMatrix&
LayerManagerComposite::GetWorldTransform(void)
{
  return mWorldMatrix;
}

void
LayerManagerComposite::WorldTransformRect(nsIntRect& aRect)
{
  gfxRect grect(aRect.x, aRect.y, aRect.width, aRect.height);
  grect = mWorldMatrix.TransformBounds(grect);
  aRect.SetRect(grect.X(), grect.Y(), grect.Width(), grect.Height());
}

static void
SubtractTransformedRegion(nsIntRegion& aRegion,
                          const nsIntRegion& aRegionToSubtract,
                          const gfx3DMatrix& aTransform)
{
  if (aRegionToSubtract.IsEmpty()) {
    return;
  }

  
  
  nsIntRegionRectIterator it(aRegionToSubtract);
  while (const nsIntRect* rect = it.Next()) {
    gfxRect incompleteRect = aTransform.TransformBounds(gfxRect(*rect));
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
                                                      const gfx3DMatrix& aTransform)
{
  if (aLayer->GetOpacity() <= 0.f ||
      (aScreenRegion.IsEmpty() && aLowPrecisionScreenRegion.IsEmpty())) {
    return;
  }

  
  ContainerLayer* container = aLayer->AsContainerLayer();
  if (container) {
    
    gfx3DMatrix transform = aTransform;
    if (container->UseIntermediateSurface()) {
      transform = aLayer->GetEffectiveTransform();
      transform.PreMultiply(aTransform);
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
    
    gfx3DMatrix transformToScreen = aLayer->GetEffectiveTransform();
    transformToScreen.PreMultiply(aTransform);

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
                       const gfx3DMatrix& aTransformToScreen,
                       const nsIntRect& aScreenRect)
{
  gfxRect transformedDisplayport =
    aTransformToScreen.TransformBounds(gfxRect(aDisplayPort.x,
                                               aDisplayPort.y,
                                               aDisplayPort.width,
                                               aDisplayPort.height));
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
  if (!gfxPlatform::UseProgressiveTilePainting() || !root) {
    return 1.f;
  }

  const FrameMetrics& rootMetrics = root->AsContainerLayer()->GetFrameMetrics();
  nsIntRect screenRect(rootMetrics.mCompositionBounds.x,
                       rootMetrics.mCompositionBounds.y,
                       rootMetrics.mCompositionBounds.width,
                       rootMetrics.mCompositionBounds.height);

  float lowPrecisionMultiplier = 1.0f;
  float highPrecisionMultiplier = 1.0f;

#ifdef MOZ_ANDROID_OMTC
  
  
  Layer* primaryScrollable = GetPrimaryScrollableLayer();
  if (primaryScrollable) {
    
    
    const gfx3DMatrix& rootTransform = root->GetTransform();
    float devPixelRatioX = 1 / rootTransform.GetXScale();
    float devPixelRatioY = 1 / rootTransform.GetYScale();

    gfx3DMatrix transform = primaryScrollable->GetEffectiveTransform();
    transform.ScalePost(devPixelRatioX, devPixelRatioY, 1);
    const FrameMetrics& metrics = primaryScrollable->AsContainerLayer()->GetFrameMetrics();

    
    gfxRect documentBounds =
      transform.TransformBounds(gfxRect(metrics.mScrollableRect.x - metrics.mScrollOffset.x,
                                        metrics.mScrollableRect.y - metrics.mScrollOffset.y,
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
  gfx3DMatrix transform;
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
  : mCompositable(nullptr)
{
  if (!aMaskLayer) {
    return;
  }

  mCompositable = static_cast<LayerComposite*>(aMaskLayer->ImplData())->GetCompositableHost();
  if (!mCompositable) {
    NS_WARNING("Mask layer with no compositable host");
    return;
  }

  gfx::Matrix4x4 transform;
  ToMatrix4x4(aMaskLayer->GetEffectiveTransform(), transform);
  mCompositable->AddMaskEffect(aEffects, transform, aIs3D);
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
    return Factory::CreateDrawTarget(BACKEND_COREGRAPHICS_ACCELERATED,
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

const nsIntSize&
LayerManagerComposite::GetWidgetSize()
{
  return mCompositor->GetWidgetSize();
}

void
LayerManagerComposite::SetCompositorID(uint32_t aID)
{
  NS_ASSERTION(mCompositor, "No compositor");
  mCompositor->SetCompositorID(aID);
}

void
LayerManagerComposite::NotifyShadowTreeTransaction()
{
  mCompositor->NotifyLayersTransaction();
}

bool
LayerManagerComposite::CanUseCanvasLayerForSize(const gfxIntSize &aSize)
{
  return mCompositor->CanUseCanvasLayerForSize(gfx::IntSize(aSize.width,
                                                            aSize.height));
}

TextureFactoryIdentifier
LayerManagerComposite::GetTextureFactoryIdentifier()
{
  return mCompositor->GetTextureFactoryIdentifier();
}

int32_t
LayerManagerComposite::GetMaxTextureSize() const
{
  return mCompositor->GetMaxTextureSize();
}

#ifndef MOZ_HAVE_PLATFORM_SPECIFIC_LAYER_BUFFERS

 already_AddRefed<TextureImage>
LayerManagerComposite::OpenDescriptorForDirectTexturing(GLContext*,
                                                        const SurfaceDescriptor&,
                                                        GLenum)
{
  return nullptr;
}

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
