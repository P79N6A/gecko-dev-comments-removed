




#include "ClientLayerManager.h"
#include "CompositorChild.h"            
#include "GeckoProfiler.h"              
#include "gfx3DMatrix.h"                
#include "gfxASurface.h"                
#include "ipc/AutoOpenSurface.h"        
#include "mozilla/Assertions.h"         
#include "mozilla/Hal.h"
#include "mozilla/dom/ScreenOrientation.h"  
#include "mozilla/dom/TabChild.h"       
#include "mozilla/hal_sandbox/PHal.h"   
#include "mozilla/layers/CompositableClient.h"  
#include "mozilla/layers/ContentClient.h"  
#include "mozilla/layers/ISurfaceAllocator.h"
#include "mozilla/layers/LayersMessages.h"  
#include "mozilla/layers/LayersSurfaces.h"  
#include "mozilla/layers/PLayerChild.h"  
#include "mozilla/layers/LayerTransactionChild.h"
#include "nsAString.h"
#include "nsIWidget.h"                  
#include "nsTArray.h"                   
#include "nsXULAppAPI.h"                
#ifdef MOZ_WIDGET_ANDROID
#include "AndroidBridge.h"
#endif

using namespace mozilla::dom;
using namespace mozilla::gfx;

namespace mozilla {
namespace layers {

ClientLayerManager::ClientLayerManager(nsIWidget* aWidget)
  : mPhase(PHASE_NONE)
  , mWidget(aWidget) 
  , mTargetRotation(ROTATION_0)
  , mRepeatTransaction(false)
  , mIsRepeatTransaction(false)
  , mTransactionIncomplete(false)
  , mCompositorMightResample(false)
  , mNeedsComposite(false)
  , mForwarder(new ShadowLayerForwarder)
{
  MOZ_COUNT_CTOR(ClientLayerManager);
}

ClientLayerManager::~ClientLayerManager()
{
  mRoot = nullptr;

  MOZ_COUNT_DTOR(ClientLayerManager);
}

int32_t
ClientLayerManager::GetMaxTextureSize() const
{
  return mForwarder->GetMaxTextureSize();
}

void
ClientLayerManager::SetDefaultTargetConfiguration(BufferMode aDoubleBuffering,
                                                  ScreenRotation aRotation)
{
  mTargetRotation = aRotation;
  if (mWidget) {
    mTargetBounds = mWidget->GetNaturalBounds();
   }
 }

void
ClientLayerManager::SetRoot(Layer* aLayer)
{
  if (mRoot != aLayer) {
    
    
    
    
    if (mRoot) {
      Hold(mRoot);
    }
    mForwarder->SetRoot(Hold(aLayer));
    NS_ASSERTION(aLayer, "Root can't be null");
    NS_ASSERTION(aLayer->Manager() == this, "Wrong manager");
    NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
    mRoot = aLayer;
  }
}

void
ClientLayerManager::Mutated(Layer* aLayer)
{
  LayerManager::Mutated(aLayer);

  NS_ASSERTION(InConstruction() || InDrawing(), "wrong phase");
  mForwarder->Mutated(Hold(aLayer));
}

void
ClientLayerManager::BeginTransactionWithTarget(gfxContext* aTarget)
{
  mInTransaction = true;

#ifdef MOZ_LAYERS_HAVE_LOG
  MOZ_LAYERS_LOG(("[----- BeginTransaction"));
  Log();
#endif

  NS_ASSERTION(!InTransaction(), "Nested transactions not allowed");
  mPhase = PHASE_CONSTRUCTION;

  NS_ABORT_IF_FALSE(mKeepAlive.IsEmpty(), "uncommitted txn?");
  nsRefPtr<gfxContext> targetContext = aTarget;

  
  
  
  ScreenOrientation orientation;
  if (TabChild* window = mWidget->GetOwningTabChild()) {
    orientation = window->GetOrientation();
  } else {
    hal::ScreenConfiguration currentConfig;
    hal::GetCurrentScreenConfiguration(&currentConfig);
    orientation = currentConfig.orientation();
  }
  nsIntRect clientBounds;
  mWidget->GetClientBounds(clientBounds);
  clientBounds.x = clientBounds.y = 0;
  mForwarder->BeginTransaction(mTargetBounds, mTargetRotation, clientBounds, orientation);

  
  
  
  
  
  if (mWidget) {
    if (TabChild* window = mWidget->GetOwningTabChild()) {
      mCompositorMightResample = window->IsAsyncPanZoomEnabled();
    }
  }

  
  
  if (aTarget && XRE_GetProcessType() == GeckoProcessType_Default) {
    mShadowTarget = aTarget;
  }
}

void
ClientLayerManager::BeginTransaction()
{
  mInTransaction = true;
  BeginTransactionWithTarget(nullptr);
}

bool
ClientLayerManager::EndTransactionInternal(DrawThebesLayerCallback aCallback,
                                           void* aCallbackData,
                                           EndTransactionFlags)
{
  PROFILER_LABEL("ClientLayerManager", "EndTransactionInternal");
#ifdef MOZ_LAYERS_HAVE_LOG
  MOZ_LAYERS_LOG(("  ----- (beginning paint)"));
  Log();
#endif
  profiler_tracing("Paint", "Rasterize", TRACING_INTERVAL_START);

  NS_ASSERTION(InConstruction(), "Should be in construction phase");
  mPhase = PHASE_DRAWING;

  ClientLayer* root = ClientLayer::ToClientLayer(GetRoot());

  mTransactionIncomplete = false;
      
  
  
  GetRoot()->ApplyPendingUpdatesToSubtree();
    
  mThebesLayerCallback = aCallback;
  mThebesLayerCallbackData = aCallbackData;

  GetRoot()->ComputeEffectiveTransforms(gfx3DMatrix());

  root->RenderLayer();
  
  mThebesLayerCallback = nullptr;
  mThebesLayerCallbackData = nullptr;

  
  
  mPhase = mTransactionIncomplete ? PHASE_CONSTRUCTION : PHASE_NONE;

  NS_ASSERTION(!aCallback || !mTransactionIncomplete,
               "If callback is not null, transaction must be complete");

  return !mTransactionIncomplete;
}

void
ClientLayerManager::EndTransaction(DrawThebesLayerCallback aCallback,
                                   void* aCallbackData,
                                   EndTransactionFlags aFlags)
{
  if (mWidget) {
    mWidget->PrepareWindowEffects();
  }
  EndTransactionInternal(aCallback, aCallbackData, aFlags);
  ForwardTransaction(!(aFlags & END_NO_REMOTE_COMPOSITE));

  if (mRepeatTransaction) {
    mRepeatTransaction = false;
    mIsRepeatTransaction = true;
    BeginTransaction();
    ClientLayerManager::EndTransaction(aCallback, aCallbackData, aFlags);
    mIsRepeatTransaction = false;
  } else {
    MakeSnapshotIfRequired();
  }
}

bool
ClientLayerManager::EndEmptyTransaction(EndTransactionFlags aFlags)
{
  mInTransaction = false;

  if (!mRoot) {
    return false;
  }
  if (!EndTransactionInternal(nullptr, nullptr, aFlags)) {
    
    
    
    return false;
  }
  if (mWidget) {
    mWidget->PrepareWindowEffects();
  }
  ForwardTransaction(!(aFlags & END_NO_REMOTE_COMPOSITE));
  MakeSnapshotIfRequired();
  return true;
}

CompositorChild *
ClientLayerManager::GetRemoteRenderer()
{
  if (!mWidget) {
    return nullptr;
  }

  return mWidget->GetRemoteRenderer();
}

void
ClientLayerManager::Composite()
{
  if (CompositorChild* remoteRenderer = GetRemoteRenderer()) {
    remoteRenderer->SendForceComposite();
  }
}

void 
ClientLayerManager::MakeSnapshotIfRequired()
{
  if (!mShadowTarget) {
    return;
  }
  if (mWidget) {
    if (CompositorChild* remoteRenderer = GetRemoteRenderer()) {
      nsIntRect bounds;
      mWidget->GetBounds(bounds);
      SurfaceDescriptor inSnapshot, snapshot;
      if (mForwarder->AllocSurfaceDescriptor(bounds.Size(),
                                             GFX_CONTENT_COLOR_ALPHA,
                                             &inSnapshot) &&
          
          
          
          remoteRenderer->SendMakeSnapshot(inSnapshot, &snapshot)) {
        AutoOpenSurface opener(OPEN_READ_ONLY, snapshot);
        gfxASurface* source = opener.Get();

        mShadowTarget->DrawSurface(source, source->GetSize());
      }
      if (IsSurfaceDescriptorValid(snapshot)) {
        mForwarder->DestroySharedSurface(&snapshot);
      }
    }
  }
  mShadowTarget = nullptr;
}

void
ClientLayerManager::FlushRendering()
{
  if (mWidget) {
    if (CompositorChild* remoteRenderer = mWidget->GetRemoteRenderer()) {
      remoteRenderer->SendFlushRendering();
    }
  }
}

void
ClientLayerManager::SendInvalidRegion(const nsIntRegion& aRegion)
{
  if (mWidget) {
    if (CompositorChild* remoteRenderer = mWidget->GetRemoteRenderer()) {
      remoteRenderer->SendNotifyRegionInvalidated(aRegion);
    }
  }
}

uint32_t
ClientLayerManager::StartFrameTimeRecording(int32_t aBufferSize)
{
  CompositorChild* renderer = GetRemoteRenderer();
  if (renderer) {
    uint32_t startIndex;
    renderer->SendStartFrameTimeRecording(aBufferSize, &startIndex);
    return startIndex;
  }
  return -1;
}

void
ClientLayerManager::StopFrameTimeRecording(uint32_t         aStartIndex,
                                           nsTArray<float>& aFrameIntervals)
{
  CompositorChild* renderer = GetRemoteRenderer();
  if (renderer) {
    renderer->SendStopFrameTimeRecording(aStartIndex, &aFrameIntervals);
  }
}

void
ClientLayerManager::ForwardTransaction(bool aScheduleComposite)
{
  mPhase = PHASE_FORWARD;

  
  bool sent;
  AutoInfallibleTArray<EditReply, 10> replies;
  if (HasShadowManager() && mForwarder->EndTransaction(&replies, aScheduleComposite, &sent)) {
    for (nsTArray<EditReply>::size_type i = 0; i < replies.Length(); ++i) {
      const EditReply& reply = replies[i];

      switch (reply.type()) {
      case EditReply::TOpContentBufferSwap: {
        MOZ_LAYERS_LOG(("[LayersForwarder] DoubleBufferSwap"));

        const OpContentBufferSwap& obs = reply.get_OpContentBufferSwap();

        CompositableChild* compositableChild =
          static_cast<CompositableChild*>(obs.compositableChild());
        ContentClientRemote* contentClient =
          static_cast<ContentClientRemote*>(compositableChild->GetCompositableClient());
        MOZ_ASSERT(contentClient);

        contentClient->SwapBuffers(obs.frontUpdatedRegion());

        break;
      }
      case EditReply::TOpTextureSwap: {
        MOZ_LAYERS_LOG(("[LayersForwarder] TextureSwap"));

        const OpTextureSwap& ots = reply.get_OpTextureSwap();

        CompositableChild* compositableChild =
          static_cast<CompositableChild*>(ots.compositableChild());
        MOZ_ASSERT(compositableChild);

        compositableChild->GetCompositableClient()
          ->SetDescriptorFromReply(ots.textureId(), ots.image());
        break;
      }

      default:
        NS_RUNTIMEABORT("not reached");
      }
    }

    if (sent) {
      mNeedsComposite = false;
    }
  } else if (HasShadowManager()) {
    NS_WARNING("failed to forward Layers transaction");
  }

  mPhase = PHASE_NONE;

  
  
  mKeepAlive.Clear();
}

ShadowableLayer*
ClientLayerManager::Hold(Layer* aLayer)
{
  NS_ABORT_IF_FALSE(HasShadowManager(),
                    "top-level tree, no shadow tree to remote to");

  ShadowableLayer* shadowable = ClientLayer::ToClientLayer(aLayer);
  NS_ABORT_IF_FALSE(shadowable, "trying to remote an unshadowable layer");

  mKeepAlive.AppendElement(aLayer);
  return shadowable;
}

bool
ClientLayerManager::IsCompositingCheap()
{
  
  return mForwarder->mShadowManager &&
         LayerManager::IsCompositingCheap(mForwarder->GetCompositorBackendType());
}

void
ClientLayerManager::SetIsFirstPaint()
{
  mForwarder->SetIsFirstPaint();
}

void
ClientLayerManager::ClearCachedResources(Layer* aSubtree)
{
  MOZ_ASSERT(!HasShadowManager() || !aSubtree);
  if (LayerTransactionChild* manager = mForwarder->GetShadowManager()) {
    manager->SendClearCachedResources();
  }
  if (aSubtree) {
    ClearLayer(aSubtree);
  } else if (mRoot) {
    ClearLayer(mRoot);
  }
}

void
ClientLayerManager::ClearLayer(Layer* aLayer)
{
  ClientLayer::ToClientLayer(aLayer)->ClearCachedResources();
  for (Layer* child = aLayer->GetFirstChild(); child;
       child = child->GetNextSibling()) {
    ClearLayer(child);
  }
}

void
ClientLayerManager::GetBackendName(nsAString& aName)
{
  switch (mForwarder->GetCompositorBackendType()) {
    case LAYERS_BASIC: aName.AssignLiteral("Basic"); return;
    case LAYERS_OPENGL: aName.AssignLiteral("OpenGL"); return;
    case LAYERS_D3D9: aName.AssignLiteral("Direct3D 9"); return;
    case LAYERS_D3D10: aName.AssignLiteral("Direct3D 10"); return;
    case LAYERS_D3D11: aName.AssignLiteral("Direct3D 11"); return;
    default: NS_RUNTIMEABORT("Invalid backend");
  }
}

bool
ClientLayerManager::ProgressiveUpdateCallback(bool aHasPendingNewThebesContent,
                                              ScreenRect& aCompositionBounds,
                                              CSSToScreenScale& aZoom,
                                              bool aDrawingCritical)
{
  aZoom.scale = 1.0;
#ifdef MOZ_WIDGET_ANDROID
  Layer* primaryScrollable = GetPrimaryScrollableLayer();
  if (primaryScrollable) {
    const FrameMetrics& metrics = primaryScrollable->AsContainerLayer()->GetFrameMetrics();

    
    
    CSSToLayerScale paintScale = metrics.LayersPixelsPerCSSPixel();
    const CSSRect& metricsDisplayPort =
      (aDrawingCritical && !metrics.mCriticalDisplayPort.IsEmpty()) ?
        metrics.mCriticalDisplayPort : metrics.mDisplayPort;
    LayerRect displayPort = (metricsDisplayPort + metrics.mScrollOffset) * paintScale;

    return AndroidBridge::Bridge()->ProgressiveUpdateCallback(
      aHasPendingNewThebesContent, displayPort, paintScale.scale, aDrawingCritical,
      aCompositionBounds, aZoom);
  }
#endif

  return false;
}

ClientLayer::~ClientLayer()
{
  if (HasShadow()) {
    PLayerChild::Send__delete__(GetShadow());
  }
  MOZ_COUNT_DTOR(ClientLayer);
}

}
}
