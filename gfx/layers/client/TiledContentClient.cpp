




#include "mozilla/layers/TiledContentClient.h"
#include <math.h>                       
#include <algorithm>
#include "ClientTiledThebesLayer.h"     
#include "GeckoProfiler.h"              
#include "ClientLayerManager.h"         
#include "CompositorChild.h"            
#include "gfxContext.h"                 
#include "gfxPlatform.h"                
#include "gfxPrefs.h"                   
#include "gfxRect.h"                    
#include "mozilla/MathAlgorithms.h"     
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Rect.h"           
#include "mozilla/gfx/Tools.h"          
#include "mozilla/layers/CompositableForwarder.h"
#include "mozilla/layers/LayerMetricsWrapper.h"
#include "mozilla/layers/ShadowLayers.h"  
#include "TextureClientPool.h"
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsSize.h"                     
#include "gfxReusableSharedImageSurfaceWrapper.h"
#include "nsExpirationTracker.h"        
#include "nsMathUtils.h"               
#include "gfx2DGlue.h"
#include "LayersLogging.h"
#include "UnitTransforms.h"             
#include "mozilla/UniquePtr.h"




#define MINIMUM_TILE_COPY_AREA (1.f/16.f)

#ifdef GFX_TILEDLAYER_DEBUG_OVERLAY
#include "cairo.h"
#include <sstream>
using mozilla::layers::Layer;
static void DrawDebugOverlay(mozilla::gfx::DrawTarget* dt, int x, int y, int width, int height)
{
  gfxContext c(dt);

  
  c.NewPath();
  c.SetDeviceColor(gfxRGBA(0.0, 0.0, 0.0, 1.0));
  c.Rectangle(gfxRect(0, 0, width, height));
  c.Stroke();

  
  std::stringstream ss;
  ss << x << ", " << y;

  
  cairo_t* cr = c.GetCairo();
  cairo_set_font_size(cr, 25);
  cairo_text_extents_t extents;
  cairo_text_extents(cr, ss.str().c_str(), &extents);

  int textWidth = extents.width + 6;

  c.NewPath();
  c.SetDeviceColor(gfxRGBA(0.0, 0.0, 0.0, 1.0));
  c.Rectangle(gfxRect(gfxPoint(2,2),gfxSize(textWidth, 30)));
  c.Fill();

  c.NewPath();
  c.SetDeviceColor(gfxRGBA(1.0, 0.0, 0.0, 1.0));
  c.Rectangle(gfxRect(gfxPoint(2,2),gfxSize(textWidth, 30)));
  c.Stroke();

  c.NewPath();
  cairo_move_to(cr, 4, 28);
  cairo_show_text(cr, ss.str().c_str());

}

#endif

namespace mozilla {

using namespace gfx;

namespace layers {


TiledContentClient::TiledContentClient(ClientTiledThebesLayer* aThebesLayer,
                                       ClientLayerManager* aManager)
  : CompositableClient(aManager->AsShadowForwarder())
{
  MOZ_COUNT_CTOR(TiledContentClient);

  mTiledBuffer = ClientTiledLayerBuffer(aThebesLayer, this, aManager,
                                        &mSharedFrameMetricsHelper);
  mLowPrecisionTiledBuffer = ClientTiledLayerBuffer(aThebesLayer, this, aManager,
                                                    &mSharedFrameMetricsHelper);

  mLowPrecisionTiledBuffer.SetResolution(gfxPrefs::LowPrecisionResolution());
}

void
TiledContentClient::ClearCachedResources()
{
  mTiledBuffer.DiscardBuffers();
  mLowPrecisionTiledBuffer.DiscardBuffers();
}

void
TiledContentClient::UseTiledLayerBuffer(TiledBufferType aType)
{
  ClientTiledLayerBuffer* buffer = aType == LOW_PRECISION_TILED_BUFFER
    ? &mLowPrecisionTiledBuffer
    : &mTiledBuffer;

  
  
  
  buffer->ReadLock();

  mForwarder->UseTiledLayerBuffer(this, buffer->GetSurfaceDescriptorTiles());
  buffer->ClearPaintedRegion();
}

SharedFrameMetricsHelper::SharedFrameMetricsHelper()
  : mLastProgressiveUpdateWasLowPrecision(false)
  , mProgressiveUpdateWasInDanger(false)
{
  MOZ_COUNT_CTOR(SharedFrameMetricsHelper);
}

SharedFrameMetricsHelper::~SharedFrameMetricsHelper()
{
  MOZ_COUNT_DTOR(SharedFrameMetricsHelper);
}

static inline bool
FuzzyEquals(float a, float b) {
  return (fabsf(a - b) < 1e-6);
}

static ViewTransform
ComputeViewTransform(const FrameMetrics& aContentMetrics, const FrameMetrics& aCompositorMetrics)
{
  
  
  

  ParentLayerToScreenScale scale = aCompositorMetrics.GetZoom()
                     / aContentMetrics.mDevPixelsPerCSSPixel
                     / aCompositorMetrics.GetParentResolution();
  ScreenPoint translation = (aCompositorMetrics.GetScrollOffset() - aContentMetrics.GetScrollOffset())
                         * aCompositorMetrics.GetZoom();
  return ViewTransform(scale, -translation);
}

bool
SharedFrameMetricsHelper::UpdateFromCompositorFrameMetrics(
    const LayerMetricsWrapper& aLayer,
    bool aHasPendingNewThebesContent,
    bool aLowPrecision,
    ViewTransform& aViewTransform)
{
  MOZ_ASSERT(aLayer);

  CompositorChild* compositor = nullptr;
  if (aLayer.Manager() &&
      aLayer.Manager()->AsClientLayerManager()) {
    compositor = aLayer.Manager()->AsClientLayerManager()->GetCompositorChild();
  }

  if (!compositor) {
    return false;
  }

  const FrameMetrics& contentMetrics = aLayer.Metrics();
  FrameMetrics compositorMetrics;

  if (!compositor->LookupCompositorFrameMetrics(contentMetrics.GetScrollId(),
                                                compositorMetrics)) {
    return false;
  }

  aViewTransform = ComputeViewTransform(contentMetrics, compositorMetrics);

  
  
  if (aLowPrecision && !mLastProgressiveUpdateWasLowPrecision) {
    
    if (!mProgressiveUpdateWasInDanger) {
      TILING_LOG("TILING: Aborting low-precision rendering because not at risk of checkerboarding\n");
      return true;
    }
    mProgressiveUpdateWasInDanger = false;
  }
  mLastProgressiveUpdateWasLowPrecision = aLowPrecision;

  
  
  if (!FuzzyEquals(compositorMetrics.GetZoom().scale, contentMetrics.GetZoom().scale)) {
    TILING_LOG("TILING: Aborting because resolution changed from %f to %f\n",
        contentMetrics.GetZoom().scale, compositorMetrics.GetZoom().scale);
    return true;
  }

  
  
  
  
  if (fabsf(contentMetrics.GetScrollOffset().x - compositorMetrics.GetScrollOffset().x) <= 2 &&
      fabsf(contentMetrics.GetScrollOffset().y - compositorMetrics.GetScrollOffset().y) <= 2 &&
      fabsf(contentMetrics.mDisplayPort.x - compositorMetrics.mDisplayPort.x) <= 2 &&
      fabsf(contentMetrics.mDisplayPort.y - compositorMetrics.mDisplayPort.y) <= 2 &&
      fabsf(contentMetrics.mDisplayPort.width - compositorMetrics.mDisplayPort.width) <= 2 &&
      fabsf(contentMetrics.mDisplayPort.height - compositorMetrics.mDisplayPort.height)) {
    return false;
  }

  
  
  if (!aLowPrecision && !mProgressiveUpdateWasInDanger) {
    bool scrollUpdatePending = contentMetrics.GetScrollOffsetUpdated() &&
        contentMetrics.GetScrollGeneration() != compositorMetrics.GetScrollGeneration();
    
    
    
    
    
    
    
    
    if (!scrollUpdatePending && AboutToCheckerboard(contentMetrics, compositorMetrics)) {
      mProgressiveUpdateWasInDanger = true;
      return true;
    }
  }

  
  
  if (aLowPrecision && !aHasPendingNewThebesContent) {
    TILING_LOG("TILING: Aborting low-precision because of new pending content\n");
    return true;
  }

  return false;
}

bool
SharedFrameMetricsHelper::AboutToCheckerboard(const FrameMetrics& aContentMetrics,
                                              const FrameMetrics& aCompositorMetrics)
{
  
  
  
  
  CSSRect painted = (aContentMetrics.mCriticalDisplayPort.IsEmpty()
                      ? aContentMetrics.mDisplayPort
                      : aContentMetrics.mCriticalDisplayPort)
                    + aContentMetrics.GetScrollOffset();
  painted.Inflate(CSSMargin::FromAppUnits(nsMargin(1, 1, 1, 1)));

  
  
  CSSRect showing = CSSRect(aCompositorMetrics.GetScrollOffset(),
                            aCompositorMetrics.CalculateBoundedCompositedSizeInCssPixels());
  showing.Inflate(LayerSize(gfxPrefs::APZDangerZoneX(), gfxPrefs::APZDangerZoneY())
                  / aCompositorMetrics.LayersPixelsPerCSSPixel());

  
  
  
  painted = painted.Intersect(aContentMetrics.mScrollableRect);
  showing = showing.Intersect(aContentMetrics.mScrollableRect);

  if (!painted.Contains(showing)) {
    TILING_LOG("TILING: About to checkerboard; content %s\n", Stringify(aContentMetrics).c_str());
    TILING_LOG("TILING: About to checkerboard; painted %s\n", Stringify(painted).c_str());
    TILING_LOG("TILING: About to checkerboard; compositor %s\n", Stringify(aCompositorMetrics).c_str());
    TILING_LOG("TILING: About to checkerboard; showing %s\n", Stringify(showing).c_str());
    return true;
  }
  return false;
}

ClientTiledLayerBuffer::ClientTiledLayerBuffer(ClientTiledThebesLayer* aThebesLayer,
                                               CompositableClient* aCompositableClient,
                                               ClientLayerManager* aManager,
                                               SharedFrameMetricsHelper* aHelper)
  : mThebesLayer(aThebesLayer)
  , mCompositableClient(aCompositableClient)
  , mManager(aManager)
  , mLastPaintContentType(gfxContentType::COLOR)
  , mLastPaintSurfaceMode(SurfaceMode::SURFACE_OPAQUE)
  , mSharedFrameMetricsHelper(aHelper)
{
}

bool
ClientTiledLayerBuffer::HasFormatChanged() const
{
  SurfaceMode mode;
  gfxContentType content = GetContentType(&mode);
  return content != mLastPaintContentType ||
         mode != mLastPaintSurfaceMode;
}


gfxContentType
ClientTiledLayerBuffer::GetContentType(SurfaceMode* aMode) const
{
  gfxContentType content =
    mThebesLayer->CanUseOpaqueSurface() ? gfxContentType::COLOR :
                                          gfxContentType::COLOR_ALPHA;
  SurfaceMode mode = mThebesLayer->GetSurfaceMode();

  if (mode == SurfaceMode::SURFACE_COMPONENT_ALPHA) {
#if defined(MOZ_GFX_OPTIMIZE_MOBILE) || defined(MOZ_WIDGET_GONK)
     mode = SurfaceMode::SURFACE_SINGLE_CHANNEL_ALPHA;
  }
#else
      if (!mThebesLayer->GetParent() ||
          !mThebesLayer->GetParent()->SupportsComponentAlphaChildren() ||
          !gfxPrefs::TiledDrawTargetEnabled()) {
        mode = SurfaceMode::SURFACE_SINGLE_CHANNEL_ALPHA;
      } else {
        content = gfxContentType::COLOR;
      }
  } else if (mode == SurfaceMode::SURFACE_OPAQUE) {
    if (mThebesLayer->MayResample()) {
      mode = SurfaceMode::SURFACE_SINGLE_CHANNEL_ALPHA;
      content = gfxContentType::COLOR_ALPHA;
    }
  }
#endif

  if (aMode) {
    *aMode = mode;
  }
  return content;
}

gfxMemorySharedReadLock::gfxMemorySharedReadLock()
  : mReadCount(1)
{
  MOZ_COUNT_CTOR(gfxMemorySharedReadLock);
}

gfxMemorySharedReadLock::~gfxMemorySharedReadLock()
{
  MOZ_ASSERT(mReadCount == 0);
  MOZ_COUNT_DTOR(gfxMemorySharedReadLock);
}

int32_t
gfxMemorySharedReadLock::ReadLock()
{
  NS_ASSERT_OWNINGTHREAD(gfxMemorySharedReadLock);

  return PR_ATOMIC_INCREMENT(&mReadCount);
}

int32_t
gfxMemorySharedReadLock::ReadUnlock()
{
  int32_t readCount = PR_ATOMIC_DECREMENT(&mReadCount);
  NS_ASSERTION(readCount >= 0, "ReadUnlock called without ReadLock.");

  return readCount;
}

int32_t
gfxMemorySharedReadLock::GetReadCount()
{
  NS_ASSERT_OWNINGTHREAD(gfxMemorySharedReadLock);
  return mReadCount;
}

gfxShmSharedReadLock::gfxShmSharedReadLock(ISurfaceAllocator* aAllocator)
  : mAllocator(aAllocator)
  , mAllocSuccess(false)
{
  MOZ_COUNT_CTOR(gfxShmSharedReadLock);
  MOZ_ASSERT(mAllocator);
  if (mAllocator) {
#define MOZ_ALIGN_WORD(x) (((x) + 3) & ~3)
    if (mAllocator->AllocShmemSection(MOZ_ALIGN_WORD(sizeof(ShmReadLockInfo)), &mShmemSection)) {
      ShmReadLockInfo* info = GetShmReadLockInfoPtr();
      info->readCount = 1;
      mAllocSuccess = true;
    }
  }
}

gfxShmSharedReadLock::~gfxShmSharedReadLock()
{
  MOZ_COUNT_DTOR(gfxShmSharedReadLock);
}

int32_t
gfxShmSharedReadLock::ReadLock() {
  NS_ASSERT_OWNINGTHREAD(gfxShmSharedReadLock);
  if (!mAllocSuccess) {
    return 0;
  }
  ShmReadLockInfo* info = GetShmReadLockInfoPtr();
  return PR_ATOMIC_INCREMENT(&info->readCount);
}

int32_t
gfxShmSharedReadLock::ReadUnlock() {
  if (!mAllocSuccess) {
    return 0;
  }
  ShmReadLockInfo* info = GetShmReadLockInfoPtr();
  int32_t readCount = PR_ATOMIC_DECREMENT(&info->readCount);
  NS_ASSERTION(readCount >= 0, "ReadUnlock called without a ReadLock.");
  if (readCount <= 0) {
    mAllocator->FreeShmemSection(mShmemSection);
  }
  return readCount;
}

int32_t
gfxShmSharedReadLock::GetReadCount() {
  NS_ASSERT_OWNINGTHREAD(gfxShmSharedReadLock);
  if (!mAllocSuccess) {
    return 0;
  }
  ShmReadLockInfo* info = GetShmReadLockInfoPtr();
  return info->readCount;
}

class TileExpiry MOZ_FINAL : public nsExpirationTracker<TileClient, 3>
{
  public:
    TileExpiry() : nsExpirationTracker<TileClient, 3>(1000) {}

    static void AddTile(TileClient* aTile)
    {
      if (!sTileExpiry) {
        sTileExpiry = MakeUnique<TileExpiry>();
      }

      sTileExpiry->AddObject(aTile);
    }

    static void RemoveTile(TileClient* aTile)
    {
      MOZ_ASSERT(sTileExpiry);
      sTileExpiry->RemoveObject(aTile);
    }

    static void Shutdown() {
      sTileExpiry = nullptr;
    }
  private:
    virtual void NotifyExpired(TileClient* aTile) MOZ_OVERRIDE
    {
      aTile->DiscardBackBuffer();
    }

    static UniquePtr<TileExpiry> sTileExpiry;
};
UniquePtr<TileExpiry> TileExpiry::sTileExpiry;

void ShutdownTileCache()
{
  TileExpiry::Shutdown();
}

void
TileClient::PrivateProtector::Set(TileClient * const aContainer, RefPtr<TextureClient> aNewValue)
{
  if (mBuffer) {
    TileExpiry::RemoveTile(aContainer);
  }
  mBuffer = aNewValue;
  if (mBuffer) {
    TileExpiry::AddTile(aContainer);
  }
}

void
TileClient::PrivateProtector::Set(TileClient * const aContainer, TextureClient* aNewValue)
{
  Set(aContainer, RefPtr<TextureClient>(aNewValue));
}


TileClient::TileClient()
  : mCompositableClient(nullptr)
{
}

TileClient::~TileClient()
{
  if (mExpirationState.IsTracked()) {
    MOZ_ASSERT(mBackBuffer);
    TileExpiry::RemoveTile(this);
  }
}

TileClient::TileClient(const TileClient& o)
{
  mBackBuffer.Set(this, o.mBackBuffer);
  mBackBufferOnWhite = o.mBackBufferOnWhite;
  mFrontBuffer = o.mFrontBuffer;
  mFrontBufferOnWhite = o.mFrontBufferOnWhite;
  mBackLock = o.mBackLock;
  mFrontLock = o.mFrontLock;
  mCompositableClient = nullptr;
#ifdef GFX_TILEDLAYER_DEBUG_OVERLAY
  mLastUpdate = o.mLastUpdate;
#endif
  mManager = o.mManager;
  mInvalidFront = o.mInvalidFront;
  mInvalidBack = o.mInvalidBack;
}

TileClient&
TileClient::operator=(const TileClient& o)
{
  if (this == &o) return *this;
  mBackBuffer.Set(this, o.mBackBuffer);
  mBackBufferOnWhite = o.mBackBufferOnWhite;
  mFrontBuffer = o.mFrontBuffer;
  mFrontBufferOnWhite = o.mFrontBufferOnWhite;
  mBackLock = o.mBackLock;
  mFrontLock = o.mFrontLock;
  mCompositableClient = nullptr;
#ifdef GFX_TILEDLAYER_DEBUG_OVERLAY
  mLastUpdate = o.mLastUpdate;
#endif
  mManager = o.mManager;
  mInvalidFront = o.mInvalidFront;
  mInvalidBack = o.mInvalidBack;
  return *this;
}


void
TileClient::Flip()
{
#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 17
  if (mFrontBuffer && mFrontBuffer->GetIPDLActor() &&
      mCompositableClient && mCompositableClient->GetIPDLActor()) {
    
    RefPtr<AsyncTransactionTracker> tracker = new RemoveTextureFromCompositableTracker();
    
    tracker->SetTextureClient(mFrontBuffer);
    mFrontBuffer->SetRemoveFromCompositableTracker(tracker);
    
    mManager->AsShadowForwarder()->RemoveTextureFromCompositableAsync(tracker,
                                                                      mCompositableClient,
                                                                      mFrontBuffer);
  }
#endif
  RefPtr<TextureClient> frontBuffer = mFrontBuffer;
  RefPtr<TextureClient> frontBufferOnWhite = mFrontBufferOnWhite;
  mFrontBuffer = mBackBuffer;
  mFrontBufferOnWhite = mBackBufferOnWhite;
  mBackBuffer.Set(this, frontBuffer);
  mBackBufferOnWhite = frontBufferOnWhite;
  RefPtr<gfxSharedReadLock> frontLock = mFrontLock;
  mFrontLock = mBackLock;
  mBackLock = frontLock;
  nsIntRegion invalidFront = mInvalidFront;
  mInvalidFront = mInvalidBack;
  mInvalidBack = invalidFront;
}

static bool
CopyFrontToBack(TextureClient* aFront,
                TextureClient* aBack,
                const gfx::IntRect& aRectToCopy)
{
  if (!aFront->Lock(OpenMode::OPEN_READ)) {
    NS_WARNING("Failed to lock the tile's front buffer");
    return false;
  }

  if (!aBack->Lock(OpenMode::OPEN_WRITE)) {
    NS_WARNING("Failed to lock the tile's back buffer");
    return false;
  }

  gfx::IntPoint rectToCopyTopLeft = aRectToCopy.TopLeft();
  aFront->CopyToTextureClient(aBack, &aRectToCopy, &rectToCopyTopLeft);
  return true;
}

void
TileClient::ValidateBackBufferFromFront(const nsIntRegion& aDirtyRegion,
                                        bool aCanRerasterizeValidRegion,
                                        nsIntRegion& aAddPaintedRegion)
{
  if (mBackBuffer && mFrontBuffer) {
    gfx::IntSize tileSize = mFrontBuffer->GetSize();
    const nsIntRect tileRect = nsIntRect(0, 0, tileSize.width, tileSize.height);

    if (aDirtyRegion.Contains(tileRect)) {
      
      
      DiscardFrontBuffer();
    } else {
      
      nsIntRegion regionToCopy = mInvalidBack;

      regionToCopy.Sub(regionToCopy, aDirtyRegion);

      aAddPaintedRegion = regionToCopy;

      if (regionToCopy.IsEmpty() ||
          (aCanRerasterizeValidRegion &&
           regionToCopy.Area() < tileSize.width * tileSize.height * MINIMUM_TILE_COPY_AREA)) {
        
        return;
      }

      
      
      
      const nsIntRect rectToCopy = regionToCopy.GetBounds();
      gfx::IntRect gfxRectToCopy(rectToCopy.x, rectToCopy.y, rectToCopy.width, rectToCopy.height);
      CopyFrontToBack(mFrontBuffer, mBackBuffer, gfxRectToCopy);

      if (mBackBufferOnWhite) {
        MOZ_ASSERT(mFrontBufferOnWhite);
        CopyFrontToBack(mFrontBufferOnWhite, mBackBufferOnWhite, gfxRectToCopy);
      }

      mInvalidBack.SetEmpty();
    }
  }
}

void
TileClient::DiscardFrontBuffer()
{
  if (mFrontBuffer) {
    MOZ_ASSERT(mFrontLock);
#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 17
    MOZ_ASSERT(!mFrontBufferOnWhite);
    if (mFrontBuffer->GetIPDLActor() &&
        mCompositableClient && mCompositableClient->GetIPDLActor()) {
      
      RefPtr<AsyncTransactionTracker> tracker = new RemoveTextureFromCompositableTracker();
      
      tracker->SetTextureClient(mFrontBuffer);
      mFrontBuffer->SetRemoveFromCompositableTracker(tracker);
      
      mManager->AsShadowForwarder()->RemoveTextureFromCompositableAsync(tracker,
                                                                        mCompositableClient,
                                                                        mFrontBuffer);
    }
#endif
    mManager->GetTexturePool(mFrontBuffer->GetFormat())->ReturnTextureClientDeferred(mFrontBuffer);
    if (mFrontBufferOnWhite) {
      mManager->GetTexturePool(mFrontBufferOnWhite->GetFormat())->ReturnTextureClientDeferred(mFrontBufferOnWhite);
    }
    mFrontLock->ReadUnlock();
    if (mFrontBuffer->IsLocked()) {
      mFrontBuffer->Unlock();
    }
    if (mFrontBufferOnWhite && mFrontBufferOnWhite->IsLocked()) {
      mFrontBufferOnWhite->Unlock();
    }
    mFrontBuffer = nullptr;
    mFrontBufferOnWhite = nullptr;
    mFrontLock = nullptr;
  }
}

void
TileClient::DiscardBackBuffer()
{
  if (mBackBuffer) {
    MOZ_ASSERT(mBackLock);
    if (!mBackBuffer->ImplementsLocking() && mBackLock->GetReadCount() > 1) {
      
      
      
     mManager->GetTexturePool(mBackBuffer->GetFormat())->ReportClientLost();
     if (mBackBufferOnWhite) {
       mManager->GetTexturePool(mBackBufferOnWhite->GetFormat())->ReportClientLost();
     }
    } else {
#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 17
      MOZ_ASSERT(!mBackBufferOnWhite);
      if (mBackBuffer->GetIPDLActor() &&
          mCompositableClient && mCompositableClient->GetIPDLActor()) {
        
        RefPtr<AsyncTransactionTracker> tracker = new RemoveTextureFromCompositableTracker();
        
        tracker->SetTextureClient(mBackBuffer);
        mBackBuffer->SetRemoveFromCompositableTracker(tracker);
        
        mManager->AsShadowForwarder()->RemoveTextureFromCompositableAsync(tracker,
                                                                          mCompositableClient,
                                                                          mBackBuffer);
      }
      
      
      mManager->GetTexturePool(mBackBuffer->GetFormat())->ReturnTextureClientDeferred(mBackBuffer);
#else
      mManager->GetTexturePool(mBackBuffer->GetFormat())->ReturnTextureClient(mBackBuffer);
      if (mBackBufferOnWhite) {
        mManager->GetTexturePool(mBackBufferOnWhite->GetFormat())->ReturnTextureClient(mBackBufferOnWhite);
      }
#endif
    }
    mBackLock->ReadUnlock();
    if (mBackBuffer->IsLocked()) {
      mBackBuffer->Unlock();
    }
    if (mBackBufferOnWhite && mBackBufferOnWhite->IsLocked()) {
      mBackBufferOnWhite->Unlock();
    }
    mBackBuffer.Set(this, nullptr);
    mBackBufferOnWhite = nullptr;
    mBackLock = nullptr;
  }
}

TextureClient*
TileClient::GetBackBuffer(const nsIntRegion& aDirtyRegion,
                          gfxContentType aContent,
                          SurfaceMode aMode,
                          bool *aCreatedTextureClient,
                          nsIntRegion& aAddPaintedRegion,
                          bool aCanRerasterizeValidRegion,
                          RefPtr<TextureClient>* aBackBufferOnWhite)
{
  TextureClientPool *pool =
    mManager->GetTexturePool(gfxPlatform::GetPlatform()->Optimal2DFormatForContent(aContent));
  
  if (mFrontBuffer &&
      mFrontBuffer->HasInternalBuffer() &&
      mFrontLock->GetReadCount() == 1) {
    
    
    DiscardBackBuffer();
    Flip();
    *aBackBufferOnWhite = mBackBufferOnWhite;
    return mBackBuffer;
  }

  if (!mBackBuffer ||
      mBackLock->GetReadCount() > 1) {
    if (mBackBuffer) {
      
      
      
      pool->ReportClientLost();
      if (mBackBufferOnWhite) {
        pool->ReportClientLost();
      }
    }
    mBackBuffer.Set(this, pool->GetTextureClient());
    if (aMode == SurfaceMode::SURFACE_COMPONENT_ALPHA) {
      mBackBufferOnWhite = pool->GetTextureClient();
    }

    if (mBackLock) {
      
      mBackLock->ReadUnlock();
    }
    
    if (mManager->AsShadowForwarder()->IsSameProcess()) {
      
      
      mBackLock = new gfxMemorySharedReadLock();
    } else {
      mBackLock = new gfxShmSharedReadLock(mManager->AsShadowForwarder());
    }

    MOZ_ASSERT(mBackLock->IsValid());

    *aCreatedTextureClient = true;
    mInvalidBack = nsIntRect(0, 0, mBackBuffer->GetSize().width, mBackBuffer->GetSize().height);
  }

  ValidateBackBufferFromFront(aDirtyRegion, aCanRerasterizeValidRegion, aAddPaintedRegion);

  *aBackBufferOnWhite = mBackBufferOnWhite;
  return mBackBuffer;
}

TileDescriptor
TileClient::GetTileDescriptor()
{
  if (IsPlaceholderTile()) {
    return PlaceholderTileDescriptor();
  }
  MOZ_ASSERT(mFrontLock);
  if (mFrontLock->GetType() == gfxSharedReadLock::TYPE_MEMORY) {
    
    
    
    mFrontLock->AddRef();
  }

  if (mFrontLock->GetType() == gfxSharedReadLock::TYPE_MEMORY) {
    return TexturedTileDescriptor(nullptr, mFrontBuffer->GetIPDLActor(),
                                  mFrontBufferOnWhite ? MaybeTexture(mFrontBufferOnWhite->GetIPDLActor()) : MaybeTexture(null_t()),
                                  TileLock(uintptr_t(mFrontLock.get())));
  } else {
    gfxShmSharedReadLock *lock = static_cast<gfxShmSharedReadLock*>(mFrontLock.get());
    return TexturedTileDescriptor(nullptr, mFrontBuffer->GetIPDLActor(),
                                  mFrontBufferOnWhite ? MaybeTexture(mFrontBufferOnWhite->GetIPDLActor()) : MaybeTexture(null_t()),
                                  TileLock(lock->GetShmemSection()));
  }
}

void
ClientTiledLayerBuffer::ReadUnlock() {
  for (size_t i = 0; i < mRetainedTiles.Length(); i++) {
    if (mRetainedTiles[i].IsPlaceholderTile()) continue;
    mRetainedTiles[i].ReadUnlock();
  }
}

void
ClientTiledLayerBuffer::ReadLock() {
  for (size_t i = 0; i < mRetainedTiles.Length(); i++) {
    if (mRetainedTiles[i].IsPlaceholderTile()) continue;
    mRetainedTiles[i].ReadLock();
  }
}

void
ClientTiledLayerBuffer::Release()
{
  for (size_t i = 0; i < mRetainedTiles.Length(); i++) {
    if (mRetainedTiles[i].IsPlaceholderTile()) continue;
    mRetainedTiles[i].Release();
  }
}

void
ClientTiledLayerBuffer::DiscardBuffers()
{
  for (size_t i = 0; i < mRetainedTiles.Length(); i++) {
    if (mRetainedTiles[i].IsPlaceholderTile()) continue;
    mRetainedTiles[i].DiscardFrontBuffer();
    mRetainedTiles[i].DiscardBackBuffer();
  }
}

SurfaceDescriptorTiles
ClientTiledLayerBuffer::GetSurfaceDescriptorTiles()
{
  InfallibleTArray<TileDescriptor> tiles;

  for (size_t i = 0; i < mRetainedTiles.Length(); i++) {
    TileDescriptor tileDesc;
    if (mRetainedTiles.SafeElementAt(i, GetPlaceholderTile()) == GetPlaceholderTile()) {
      tileDesc = PlaceholderTileDescriptor();
    } else {
      tileDesc = mRetainedTiles[i].GetTileDescriptor();
    }
    tiles.AppendElement(tileDesc);
  }
  return SurfaceDescriptorTiles(mValidRegion, mPaintedRegion,
                                tiles, mRetainedWidth, mRetainedHeight,
                                mResolution, mFrameResolution.scale);
}

void
ClientTiledLayerBuffer::PaintThebes(const nsIntRegion& aNewValidRegion,
                                   const nsIntRegion& aPaintRegion,
                                   LayerManager::DrawThebesLayerCallback aCallback,
                                   void* aCallbackData)
{
  TILING_LOG("TILING %p: PaintThebes painting region %s\n", mThebesLayer, Stringify(aPaintRegion).c_str());
  TILING_LOG("TILING %p: PaintThebes new valid region %s\n", mThebesLayer, Stringify(aNewValidRegion).c_str());

  mCallback = aCallback;
  mCallbackData = aCallbackData;

#ifdef GFX_TILEDLAYER_PREF_WARNINGS
  long start = PR_IntervalNow();
#endif

  
  NS_ASSERTION(!aPaintRegion.GetBounds().IsEmpty(), "Empty paint region\n");

  bool useSinglePaintBuffer = UseSinglePaintBuffer();
  
  










  if (useSinglePaintBuffer && !gfxPrefs::TiledDrawTargetEnabled()) {
    nsRefPtr<gfxContext> ctxt;

    const nsIntRect bounds = aPaintRegion.GetBounds();
    {
      PROFILER_LABEL("ClientTiledLayerBuffer", "PaintThebesSingleBufferAlloc",
        js::ProfileEntry::Category::GRAPHICS);

      gfxImageFormat format =
        gfxPlatform::GetPlatform()->OptimalFormatForContent(
          GetContentType());

      mSinglePaintDrawTarget =
        gfxPlatform::GetPlatform()->CreateOffscreenContentDrawTarget(
          gfx::IntSize(ceilf(bounds.width * mResolution),
                       ceilf(bounds.height * mResolution)),
          gfx::ImageFormatToSurfaceFormat(format));

      if (!mSinglePaintDrawTarget) {
        return;
      }

      ctxt = new gfxContext(mSinglePaintDrawTarget);

      mSinglePaintBufferOffset = nsIntPoint(bounds.x, bounds.y);
    }
    ctxt->NewPath();
    ctxt->SetMatrix(
      ctxt->CurrentMatrix().Scale(mResolution, mResolution).
                            Translate(-bounds.x, -bounds.y));
#ifdef GFX_TILEDLAYER_PREF_WARNINGS
    if (PR_IntervalNow() - start > 3) {
      printf_stderr("Slow alloc %i\n", PR_IntervalNow() - start);
    }
    start = PR_IntervalNow();
#endif
    PROFILER_LABEL("ClientTiledLayerBuffer", "PaintThebesSingleBufferDraw",
      js::ProfileEntry::Category::GRAPHICS);

    mCallback(mThebesLayer, ctxt, aPaintRegion, DrawRegionClip::CLIP_NONE, nsIntRegion(), mCallbackData);
  }

#ifdef GFX_TILEDLAYER_PREF_WARNINGS
  if (PR_IntervalNow() - start > 30) {
    const nsIntRect bounds = aPaintRegion.GetBounds();
    printf_stderr("Time to draw %i: %i, %i, %i, %i\n", PR_IntervalNow() - start, bounds.x, bounds.y, bounds.width, bounds.height);
    if (aPaintRegion.IsComplex()) {
      printf_stderr("Complex region\n");
      nsIntRegionRectIterator it(aPaintRegion);
      for (const nsIntRect* rect = it.Next(); rect != nullptr; rect = it.Next()) {
        printf_stderr(" rect %i, %i, %i, %i\n", rect->x, rect->y, rect->width, rect->height);
      }
    }
  }
  start = PR_IntervalNow();
#endif

  PROFILER_LABEL("ClientTiledLayerBuffer", "PaintThebesUpdate",
    js::ProfileEntry::Category::GRAPHICS);

  mNewValidRegion = aNewValidRegion;
  Update(aNewValidRegion, aPaintRegion);

#ifdef GFX_TILEDLAYER_PREF_WARNINGS
  if (PR_IntervalNow() - start > 10) {
    const nsIntRect bounds = aPaintRegion.GetBounds();
    printf_stderr("Time to tile %i: %i, %i, %i, %i\n", PR_IntervalNow() - start, bounds.x, bounds.y, bounds.width, bounds.height);
  }
#endif

  mLastPaintContentType = GetContentType(&mLastPaintSurfaceMode);
  mCallback = nullptr;
  mCallbackData = nullptr;
  mSinglePaintDrawTarget = nullptr;
}

void PadDrawTargetOutFromRegion(RefPtr<DrawTarget> drawTarget, nsIntRegion &region)
{
  struct LockedBits {
    uint8_t *data;
    IntSize size;
    int32_t stride;
    SurfaceFormat format;
    static int clamp(int x, int min, int max)
    {
      if (x < min)
	x = min;
      if (x > max)
	x = max;
      return x;
    }

    static void visitor(void *closure, VisitSide side, int x1, int y1, int x2, int y2) {
      LockedBits *lb = static_cast<LockedBits*>(closure);
      uint8_t *bitmap = lb->data;
      const int bpp = gfx::BytesPerPixel(lb->format);
      const int stride = lb->stride;
      const int width = lb->size.width;
      const int height = lb->size.height;

      if (side == VisitSide::TOP) {
	if (y1 > 0) {
	  x1 = clamp(x1, 0, width - 1);
	  x2 = clamp(x2, 0, width - 1);
	  memcpy(&bitmap[x1*bpp + (y1-1) * stride], &bitmap[x1*bpp + y1 * stride], (x2 - x1) * bpp);
	}
      } else if (side == VisitSide::BOTTOM) {
	if (y1 < height) {
	  x1 = clamp(x1, 0, width - 1);
	  x2 = clamp(x2, 0, width - 1);
	  memcpy(&bitmap[x1*bpp + y1 * stride], &bitmap[x1*bpp + (y1-1) * stride], (x2 - x1) * bpp);
	}
      } else if (side == VisitSide::LEFT) {
	if (x1 > 0) {
	  while (y1 != y2) {
	    memcpy(&bitmap[(x1-1)*bpp + y1 * stride], &bitmap[x1*bpp + y1*stride], bpp);
	    y1++;
	  }
	}
      } else if (side == VisitSide::RIGHT) {
	if (x1 < width) {
	  while (y1 != y2) {
	    memcpy(&bitmap[x1*bpp + y1 * stride], &bitmap[(x1-1)*bpp + y1*stride], bpp);
	    y1++;
	  }
	}
      }

    }
  } lb;

  if (drawTarget->LockBits(&lb.data, &lb.size, &lb.stride, &lb.format)) {
    
    region.VisitEdges(lb.visitor, &lb);
    drawTarget->ReleaseBits(lb.data);
  }
}

void
ClientTiledLayerBuffer::PostValidate(const nsIntRegion& aPaintRegion)
{
  if (gfxPrefs::TiledDrawTargetEnabled() && mMoz2DTiles.size() > 0) {
    gfx::TileSet tileset;
    tileset.mTiles = &mMoz2DTiles[0];
    tileset.mTileCount = mMoz2DTiles.size();
    RefPtr<DrawTarget> drawTarget = gfx::Factory::CreateTiledDrawTarget(tileset);
    drawTarget->SetTransform(Matrix());

    RefPtr<gfxContext> ctx = new gfxContext(drawTarget);

    mCallback(mThebesLayer, ctx, aPaintRegion, DrawRegionClip::DRAW, nsIntRegion(), mCallbackData);
    mMoz2DTiles.clear();
  }
}

void
ClientTiledLayerBuffer::UnlockTile(TileClient aTile)
{
  
  if (aTile.mFrontBuffer && aTile.mFrontBuffer->IsLocked()) {
    aTile.mFrontBuffer->Unlock();
  }
  if (aTile.mFrontBufferOnWhite && aTile.mFrontBufferOnWhite->IsLocked()) {
    aTile.mFrontBufferOnWhite->Unlock();
  }
  if (aTile.mBackBuffer && aTile.mBackBuffer->IsLocked()) {
    aTile.mBackBuffer->Unlock();
  }
  if (aTile.mBackBufferOnWhite && aTile.mBackBufferOnWhite->IsLocked()) {
    aTile.mBackBufferOnWhite->Unlock();
  }
}

TileClient
ClientTiledLayerBuffer::ValidateTile(TileClient aTile,
                                    const nsIntPoint& aTileOrigin,
                                    const nsIntRegion& aDirtyRegion)
{
  PROFILER_LABEL("ClientTiledLayerBuffer", "ValidateTile",
    js::ProfileEntry::Category::GRAPHICS);

#ifdef GFX_TILEDLAYER_PREF_WARNINGS
  if (aDirtyRegion.IsComplex()) {
    printf_stderr("Complex region\n");
  }
#endif

  if (aTile.IsPlaceholderTile()) {
    aTile.SetLayerManager(mManager);
  }
  aTile.SetCompositableClient(mCompositableClient);

  
  
  
  if (HasFormatChanged()) {
    aTile.DiscardBackBuffer();
    aTile.DiscardFrontBuffer();
  }

  bool createdTextureClient = false;
  nsIntRegion offsetScaledDirtyRegion = aDirtyRegion.MovedBy(-aTileOrigin);
  offsetScaledDirtyRegion.ScaleRoundOut(mResolution, mResolution);

  bool usingSinglePaintBuffer = !!mSinglePaintDrawTarget;
  SurfaceMode mode;
  gfxContentType content = GetContentType(&mode);
  nsIntRegion extraPainted;
  RefPtr<TextureClient> backBufferOnWhite;
  RefPtr<TextureClient> backBuffer =
    aTile.GetBackBuffer(offsetScaledDirtyRegion,
                        content, mode,
                        &createdTextureClient, extraPainted,
                        !usingSinglePaintBuffer && !gfxPrefs::TiledDrawTargetEnabled(),
                        &backBufferOnWhite);

  extraPainted.MoveBy(aTileOrigin);
  extraPainted.And(extraPainted, mNewValidRegion);
  mPaintedRegion.Or(mPaintedRegion, extraPainted);

  
  if (!backBuffer->IsLocked()) {
    if (!backBuffer->Lock(OpenMode::OPEN_READ_WRITE)) {
      NS_WARNING("Failed to lock tile TextureClient for updating.");
      aTile.DiscardFrontBuffer();
      return aTile;
    }
  }
  if (backBufferOnWhite && !backBufferOnWhite->IsLocked()) {
    if (!backBufferOnWhite->Lock(OpenMode::OPEN_READ_WRITE)) {
      NS_WARNING("Failed to lock tile TextureClient for updating.");
      aTile.DiscardFrontBuffer();
      return aTile;
    }
  }

  if (gfxPrefs::TiledDrawTargetEnabled()) {
    aTile.Flip();

    if (createdTextureClient) {
      if (!mCompositableClient->AddTextureClient(backBuffer)) {
        NS_WARNING("Failed to add tile TextureClient.");
        aTile.DiscardFrontBuffer();
        aTile.DiscardBackBuffer();
        return aTile;
      }
      if (backBufferOnWhite && !mCompositableClient->AddTextureClient(backBufferOnWhite)) {
        NS_WARNING("Failed to add tile TextureClient.");
        aTile.DiscardFrontBuffer();
        aTile.DiscardBackBuffer();
        return aTile;
      }
    }

    
    gfx::Tile moz2DTile;
    RefPtr<DrawTarget> dt = backBuffer->BorrowDrawTarget();
    RefPtr<DrawTarget> dtOnWhite;
    if (backBufferOnWhite) {
      dtOnWhite = backBufferOnWhite->BorrowDrawTarget();
      moz2DTile.mDrawTarget = Factory::CreateDualDrawTarget(dt, dtOnWhite);
    } else {
      moz2DTile.mDrawTarget = dt;
    }
    moz2DTile.mTileOrigin = gfx::IntPoint(aTileOrigin.x, aTileOrigin.y);
    if (!dt || (backBufferOnWhite && !dtOnWhite)) {
      aTile.DiscardFrontBuffer();
      return aTile;
    }

    mMoz2DTiles.push_back(moz2DTile);

    nsIntRegionRectIterator it(aDirtyRegion);
    for (const nsIntRect* dirtyRect = it.Next(); dirtyRect != nullptr; dirtyRect = it.Next()) {
      gfx::Rect drawRect(dirtyRect->x - aTileOrigin.x,
                         dirtyRect->y - aTileOrigin.y,
                         dirtyRect->width,
                         dirtyRect->height);
      drawRect.Scale(mResolution);

      gfx::IntRect copyRect(NS_roundf((dirtyRect->x - mSinglePaintBufferOffset.x) * mResolution),
                            NS_roundf((dirtyRect->y - mSinglePaintBufferOffset.y) * mResolution),
                            drawRect.width,
                            drawRect.height);
      gfx::IntPoint copyTarget(NS_roundf(drawRect.x), NS_roundf(drawRect.y));
      
      aTile.mInvalidBack.Or(aTile.mInvalidBack, nsIntRect(copyTarget.x, copyTarget.y, copyRect.width, copyRect.height));

      if (mode == SurfaceMode::SURFACE_COMPONENT_ALPHA) {
        dt->FillRect(drawRect, ColorPattern(Color(0.0, 0.0, 0.0, 1.0)));
        dtOnWhite->FillRect(drawRect, ColorPattern(Color(1.0, 1.0, 1.0, 1.0)));
      } else if (content == gfxContentType::COLOR_ALPHA) {
        dt->ClearRect(drawRect);
      }
    }

    return aTile;
  } else {
    MOZ_ASSERT(!backBufferOnWhite, "Component alpha only supported with TiledDrawTarget");
  }

  
  
  
  RefPtr<DrawTarget> drawTarget = backBuffer->BorrowDrawTarget();
  drawTarget->SetTransform(Matrix());

  RefPtr<gfxContext> ctxt = new gfxContext(drawTarget);

  if (usingSinglePaintBuffer) {
    
    RefPtr<gfx::SourceSurface> source = mSinglePaintDrawTarget->Snapshot();
    nsIntRegionRectIterator it(aDirtyRegion);
    for (const nsIntRect* dirtyRect = it.Next(); dirtyRect != nullptr; dirtyRect = it.Next()) {
#ifdef GFX_TILEDLAYER_PREF_WARNINGS
      printf_stderr(" break into subdirtyRect %i, %i, %i, %i\n",
                    dirtyRect->x, dirtyRect->y, dirtyRect->width, dirtyRect->height);
#endif
      gfx::Rect drawRect(dirtyRect->x - aTileOrigin.x,
                         dirtyRect->y - aTileOrigin.y,
                         dirtyRect->width,
                         dirtyRect->height);
      drawRect.Scale(mResolution);

      gfx::IntRect copyRect(NS_roundf((dirtyRect->x - mSinglePaintBufferOffset.x) * mResolution),
                            NS_roundf((dirtyRect->y - mSinglePaintBufferOffset.y) * mResolution),
                            drawRect.width,
                            drawRect.height);
      gfx::IntPoint copyTarget(NS_roundf(drawRect.x), NS_roundf(drawRect.y));
      drawTarget->CopySurface(source, copyRect, copyTarget);

      
      aTile.mInvalidFront.Or(aTile.mInvalidFront, nsIntRect(copyTarget.x, copyTarget.y, copyRect.width, copyRect.height));
    }

    
    
    
    if (mResolution == 1) {
      nsIntRect unscaledTile = nsIntRect(aTileOrigin.x,
					 aTileOrigin.y,
					 GetTileSize().width,
					 GetTileSize().height);

      nsIntRegion tileValidRegion = GetValidRegion();
      tileValidRegion.Or(tileValidRegion, aDirtyRegion);
      
      if (!tileValidRegion.Contains(unscaledTile)) {
	tileValidRegion = tileValidRegion.Intersect(unscaledTile);
	
	tileValidRegion.MoveBy(-nsIntPoint(unscaledTile.x, unscaledTile.y));
	PadDrawTargetOutFromRegion(drawTarget, tileValidRegion);
      }
    }

    
    aTile.mInvalidBack.Sub(nsIntRect(0, 0, GetTileSize().width, GetTileSize().height),
                           offsetScaledDirtyRegion);
  } else {
    
    nsIntRegion tileRegion =
      nsIntRect(aTileOrigin.x, aTileOrigin.y,
                GetScaledTileSize().width, GetScaledTileSize().height);

    
    tileRegion = tileRegion.Intersect(aDirtyRegion);

    
    nsIntPoint unscaledTileOrigin = nsIntPoint(aTileOrigin.x * mResolution,
                                               aTileOrigin.y * mResolution);
    nsIntRegion unscaledTileRegion(tileRegion);
    unscaledTileRegion.ScaleRoundOut(mResolution, mResolution);

    
    aTile.mInvalidFront.MoveBy(unscaledTileOrigin);
    aTile.mInvalidBack.MoveBy(unscaledTileOrigin);

    
    
    aTile.mInvalidFront.Or(aTile.mInvalidFront, unscaledTileRegion);

    
    tileRegion.Or(tileRegion, aTile.mInvalidBack);

    
    aTile.mInvalidFront.MoveBy(-unscaledTileOrigin);

    
    aTile.mInvalidBack.SetEmpty();

    nsIntRect bounds = tileRegion.GetBounds();
    bounds.MoveBy(-aTileOrigin);

    if (GetContentType() != gfxContentType::COLOR) {
      drawTarget->ClearRect(Rect(bounds.x, bounds.y, bounds.width, bounds.height));
    }

    ctxt->NewPath();
    ctxt->Clip(gfxRect(bounds.x, bounds.y, bounds.width, bounds.height));
    ctxt->SetMatrix(
      ctxt->CurrentMatrix().Translate(-unscaledTileOrigin.x,
                                      -unscaledTileOrigin.y).
                            Scale(mResolution, mResolution));
    mCallback(mThebesLayer, ctxt,
              tileRegion.GetBounds(),
              DrawRegionClip::CLIP_NONE,
              nsIntRegion(), mCallbackData);

  }

#ifdef GFX_TILEDLAYER_DEBUG_OVERLAY
  DrawDebugOverlay(drawTarget, aTileOrigin.x * mResolution,
                   aTileOrigin.y * mResolution, GetTileLength(), GetTileLength());
#endif

  ctxt = nullptr;
  drawTarget = nullptr;

  nsIntRegion tileRegion =
    nsIntRect(aTileOrigin.x, aTileOrigin.y,
              GetScaledTileSize().width, GetScaledTileSize().height);
  
  tileRegion = tileRegion.Sub(tileRegion, GetValidRegion());
  tileRegion = tileRegion.Sub(tileRegion, aDirtyRegion); 

  backBuffer->SetWaste(tileRegion.Area() * mResolution * mResolution);
  backBuffer->Unlock();

  if (createdTextureClient) {
    if (!mCompositableClient->AddTextureClient(backBuffer)) {
      NS_WARNING("Failed to add tile TextureClient.");
      aTile.DiscardFrontBuffer();
      aTile.DiscardBackBuffer();
      return aTile;
    }
  }

  aTile.Flip();

  
  

  if (backBuffer->HasInternalBuffer()) {
    
    
    aTile.DiscardBackBuffer();
  }

  return aTile;
}












static LayerRect
GetCompositorSideCompositionBounds(const LayerMetricsWrapper& aScrollAncestor,
                                   const Matrix4x4& aTransformToCompBounds,
                                   const ViewTransform& aAPZTransform)
{
  Matrix4x4 nonTransientAPZUntransform = Matrix4x4().Scale(
    aScrollAncestor.Metrics().mResolution.scale,
    aScrollAncestor.Metrics().mResolution.scale,
    1.f);
  nonTransientAPZUntransform.Invert();

  
  
  
  
  Matrix4x4 transform = aTransformToCompBounds
                      * nonTransientAPZUntransform
                      * Matrix4x4(aAPZTransform);
  transform.Invert();

  return TransformTo<LayerPixel>(transform,
            aScrollAncestor.Metrics().mCompositionBounds);
}

bool
ClientTiledLayerBuffer::ComputeProgressiveUpdateRegion(const nsIntRegion& aInvalidRegion,
                                                       const nsIntRegion& aOldValidRegion,
                                                       nsIntRegion& aRegionToPaint,
                                                       BasicTiledLayerPaintData* aPaintData,
                                                       bool aIsRepeated)
{
  aRegionToPaint = aInvalidRegion;

  
  
  
  if (aPaintData->mCompositionBounds.IsEmpty()) {
    aPaintData->mPaintFinished = true;
    return false;
  }

  
  
  
  bool drawingLowPrecision = IsLowPrecision();

  
  nsIntRegion staleRegion;
  staleRegion.And(aInvalidRegion, aOldValidRegion);

  TILING_LOG("TILING %p: Progressive update stale region %s\n", mThebesLayer, Stringify(staleRegion).c_str());

  LayerMetricsWrapper scrollAncestor;
  mThebesLayer->GetAncestorLayers(&scrollAncestor, nullptr);

  
  
  
  ViewTransform viewTransform;
#if defined(MOZ_WIDGET_ANDROID) && !defined(MOZ_ANDROID_APZ)
  FrameMetrics contentMetrics = scrollAncestor.Metrics();
  bool abortPaint = false;
  
  
  
  if (contentMetrics.GetScrollId() == mManager->GetRootScrollableLayerId()) {
    FrameMetrics compositorMetrics = contentMetrics;
    
    abortPaint = mManager->ProgressiveUpdateCallback(!staleRegion.Contains(aInvalidRegion),
                                                     compositorMetrics,
                                                     !drawingLowPrecision);
    viewTransform = ComputeViewTransform(contentMetrics, compositorMetrics);
  }
#else
  MOZ_ASSERT(mSharedFrameMetricsHelper);

  bool abortPaint =
    mSharedFrameMetricsHelper->UpdateFromCompositorFrameMetrics(
      scrollAncestor,
      !staleRegion.Contains(aInvalidRegion),
      drawingLowPrecision,
      viewTransform);
#endif

  TILING_LOG("TILING %p: Progressive update view transform %s zoom %f abort %d\n",
      mThebesLayer, ToString(viewTransform.mTranslation).c_str(), viewTransform.mScale.scale, abortPaint);

  if (abortPaint) {
    
    
    
    if (!aPaintData->mFirstPaint || drawingLowPrecision) {
      PROFILER_LABEL("ClientTiledLayerBuffer", "ComputeProgressiveUpdateRegion",
        js::ProfileEntry::Category::GRAPHICS);

      aRegionToPaint.SetEmpty();
      return aIsRepeated;
    }
  }

  LayerRect transformedCompositionBounds =
    GetCompositorSideCompositionBounds(scrollAncestor,
                                       aPaintData->mTransformToCompBounds,
                                       viewTransform);

  TILING_LOG("TILING %p: Progressive update transformed compositor bounds %s\n", mThebesLayer, Stringify(transformedCompositionBounds).c_str());

  
  
  
  
  
  
  
  
  
  nsIntRect coherentUpdateRect(LayerIntRect::ToUntyped(RoundedOut(
#ifdef MOZ_WIDGET_ANDROID
    transformedCompositionBounds.Intersect(aPaintData->mCompositionBounds)
#else
    transformedCompositionBounds
#endif
  )));

  TILING_LOG("TILING %p: Progressive update final coherency rect %s\n", mThebesLayer, Stringify(coherentUpdateRect).c_str());

  aRegionToPaint.And(aInvalidRegion, coherentUpdateRect);
  aRegionToPaint.Or(aRegionToPaint, staleRegion);
  bool drawingStale = !aRegionToPaint.IsEmpty();
  if (!drawingStale) {
    aRegionToPaint = aInvalidRegion;
  }

  
  bool paintingVisible = false;
  if (aRegionToPaint.Intersects(coherentUpdateRect)) {
    aRegionToPaint.And(aRegionToPaint, coherentUpdateRect);
    paintingVisible = true;
  }

  TILING_LOG("TILING %p: Progressive update final paint region %s\n", mThebesLayer, Stringify(aRegionToPaint).c_str());

  
  
  bool paintInSingleTransaction = paintingVisible && (drawingStale || aPaintData->mFirstPaint);

  TILING_LOG("TILING %p: paintingVisible %d drawingStale %d firstPaint %d singleTransaction %d\n",
    mThebesLayer, paintingVisible, drawingStale, aPaintData->mFirstPaint, paintInSingleTransaction);

  
  
  NS_ASSERTION(!aRegionToPaint.IsEmpty(), "Unexpectedly empty paint region!");
  nsIntRect paintBounds = aRegionToPaint.GetBounds();

  int startX, incX, startY, incY;
  gfx::IntSize scaledTileSize = GetScaledTileSize();
  if (aPaintData->mScrollOffset.x >= aPaintData->mLastScrollOffset.x) {
    startX = RoundDownToTileEdge(paintBounds.x, scaledTileSize.width);
    incX = scaledTileSize.width;
  } else {
    startX = RoundDownToTileEdge(paintBounds.XMost() - 1, scaledTileSize.width);
    incX = -scaledTileSize.width;
  }

  if (aPaintData->mScrollOffset.y >= aPaintData->mLastScrollOffset.y) {
    startY = RoundDownToTileEdge(paintBounds.y, scaledTileSize.height);
    incY = scaledTileSize.height;
  } else {
    startY = RoundDownToTileEdge(paintBounds.YMost() - 1, scaledTileSize.height);
    incY = -scaledTileSize.height;
  }

  
  nsIntRect tileBounds(startX, startY, scaledTileSize.width, scaledTileSize.height);
  int32_t scrollDiffX = aPaintData->mScrollOffset.x - aPaintData->mLastScrollOffset.x;
  int32_t scrollDiffY = aPaintData->mScrollOffset.y - aPaintData->mLastScrollOffset.y;
  
  
  
  while (true) {
    aRegionToPaint.And(aInvalidRegion, tileBounds);
    if (!aRegionToPaint.IsEmpty()) {
      break;
    }
    if (Abs(scrollDiffY) >= Abs(scrollDiffX)) {
      tileBounds.x += incX;
    } else {
      tileBounds.y += incY;
    }
  }

  if (!aRegionToPaint.Contains(aInvalidRegion)) {
    
    

    
    
    
    
    
    
    
    return (!drawingLowPrecision && paintInSingleTransaction);
  }

  
  
  
  aPaintData->mPaintFinished = true;
  return false;
}

bool
ClientTiledLayerBuffer::ProgressiveUpdate(nsIntRegion& aValidRegion,
                                         nsIntRegion& aInvalidRegion,
                                         const nsIntRegion& aOldValidRegion,
                                         BasicTiledLayerPaintData* aPaintData,
                                         LayerManager::DrawThebesLayerCallback aCallback,
                                         void* aCallbackData)
{
  TILING_LOG("TILING %p: Progressive update valid region %s\n", mThebesLayer, Stringify(aValidRegion).c_str());
  TILING_LOG("TILING %p: Progressive update invalid region %s\n", mThebesLayer, Stringify(aInvalidRegion).c_str());
  TILING_LOG("TILING %p: Progressive update old valid region %s\n", mThebesLayer, Stringify(aOldValidRegion).c_str());

  bool repeat = false;
  bool isBufferChanged = false;
  do {
    
    
    nsIntRegion regionToPaint;
    repeat = ComputeProgressiveUpdateRegion(aInvalidRegion,
                                            aOldValidRegion,
                                            regionToPaint,
                                            aPaintData,
                                            repeat);

    TILING_LOG("TILING %p: Progressive update computed paint region %s repeat %d\n", mThebesLayer, Stringify(regionToPaint).c_str(), repeat);

    
    if (regionToPaint.IsEmpty()) {
      break;
    }

    isBufferChanged = true;

    
    aValidRegion.Or(aValidRegion, regionToPaint);

    
    
    
    nsIntRegion validOrStale;
    validOrStale.Or(aValidRegion, aOldValidRegion);

    
    PaintThebes(validOrStale, regionToPaint, aCallback, aCallbackData);
    aInvalidRegion.Sub(aInvalidRegion, regionToPaint);
  } while (repeat);

  TILING_LOG("TILING %p: Progressive update final valid region %s buffer changed %d\n", mThebesLayer, Stringify(aValidRegion).c_str(), isBufferChanged);
  TILING_LOG("TILING %p: Progressive update final invalid region %s\n", mThebesLayer, Stringify(aInvalidRegion).c_str());

  
  
  return isBufferChanged;
}

}
}
