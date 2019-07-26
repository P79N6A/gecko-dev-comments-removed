




#include "mozilla/layers/TiledContentClient.h"
#include <math.h>                       
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
#include "mozilla/layers/CompositableForwarder.h"
#include "mozilla/layers/ShadowLayers.h"  
#include "TextureClientPool.h"
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsSize.h"                     
#include "gfxReusableSharedImageSurfaceWrapper.h"
#include "nsMathUtils.h"               
#include "gfx2DGlue.h"




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

  mLowPrecisionTiledBuffer.SetResolution(gfxPrefs::LowPrecisionResolution()/1000.f);
}

void
TiledContentClient::ClearCachedResources()
{
  mTiledBuffer.DiscardBackBuffers();
  mLowPrecisionTiledBuffer.DiscardBackBuffers();
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

bool
SharedFrameMetricsHelper::UpdateFromCompositorFrameMetrics(
    ContainerLayer* aLayer,
    bool aHasPendingNewThebesContent,
    bool aLowPrecision,
    ParentLayerRect& aCompositionBounds,
    CSSToParentLayerScale& aZoom)
{
  MOZ_ASSERT(aLayer);

  CompositorChild* compositor = CompositorChild::Get();

  if (!compositor) {
    FindFallbackContentFrameMetrics(aLayer, aCompositionBounds, aZoom);
    return false;
  }

  const FrameMetrics& contentMetrics = aLayer->GetFrameMetrics();
  FrameMetrics compositorMetrics;

  if (!compositor->LookupCompositorFrameMetrics(contentMetrics.GetScrollId(),
                                                compositorMetrics)) {
    FindFallbackContentFrameMetrics(aLayer, aCompositionBounds, aZoom);
    return false;
  }

  aCompositionBounds = ParentLayerRect(compositorMetrics.mCompositionBounds);
  aZoom = compositorMetrics.GetZoomToParent();

  
  
  if (aLowPrecision && !mLastProgressiveUpdateWasLowPrecision) {
    
    if (!mProgressiveUpdateWasInDanger) {
      return true;
    }
    mProgressiveUpdateWasInDanger = false;
  }
  mLastProgressiveUpdateWasLowPrecision = aLowPrecision;

  
  
  if (!FuzzyEquals(compositorMetrics.GetZoom().scale, contentMetrics.GetZoom().scale)) {
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
    if (AboutToCheckerboard(contentMetrics, compositorMetrics)) {
      mProgressiveUpdateWasInDanger = true;
      return true;
    }
  }

  
  
  if (aLowPrecision && !aHasPendingNewThebesContent) {
    return true;
  }

  return false;
}

void
SharedFrameMetricsHelper::FindFallbackContentFrameMetrics(ContainerLayer* aLayer,
                                                          ParentLayerRect& aCompositionBounds,
                                                          CSSToParentLayerScale& aZoom) {
  if (!aLayer) {
    return;
  }

  ContainerLayer* layer = aLayer;
  const FrameMetrics* contentMetrics = &(layer->GetFrameMetrics());

  
  while (layer && contentMetrics->mCompositionBounds.IsEmpty()) {
    layer = layer->GetParent();
    contentMetrics = layer ? &(layer->GetFrameMetrics()) : contentMetrics;
  }

  MOZ_ASSERT(!contentMetrics->mCompositionBounds.IsEmpty());

  aCompositionBounds = ParentLayerRect(contentMetrics->mCompositionBounds);
  aZoom = contentMetrics->GetZoomToParent();  
  return;
}

bool
SharedFrameMetricsHelper::AboutToCheckerboard(const FrameMetrics& aContentMetrics,
                                              const FrameMetrics& aCompositorMetrics)
{
  CSSRect painted =
        (aContentMetrics.mCriticalDisplayPort.IsEmpty() ? aContentMetrics.mDisplayPort : aContentMetrics.mCriticalDisplayPort)
        + aContentMetrics.GetScrollOffset();
  CSSRect showing = CSSRect(aCompositorMetrics.GetScrollOffset(), aCompositorMetrics.CalculateBoundedCompositedSizeInCssPixels());
  return !painted.Contains(showing);
}

ClientTiledLayerBuffer::ClientTiledLayerBuffer(ClientTiledThebesLayer* aThebesLayer,
                                             CompositableClient* aCompositableClient,
                                             ClientLayerManager* aManager,
                                             SharedFrameMetricsHelper* aHelper)
  : mThebesLayer(aThebesLayer)
  , mCompositableClient(aCompositableClient)
  , mManager(aManager)
  , mLastPaintOpaque(false)
  , mSharedFrameMetricsHelper(aHelper)
{
}

bool
ClientTiledLayerBuffer::HasFormatChanged() const
{
  return mThebesLayer->CanUseOpaqueSurface() != mLastPaintOpaque;
}


gfxContentType
ClientTiledLayerBuffer::GetContentType() const
{
  if (mThebesLayer->CanUseOpaqueSurface()) {
    return gfxContentType::COLOR;
  } else {
    return gfxContentType::COLOR_ALPHA;
  }
}

gfxMemorySharedReadLock::gfxMemorySharedReadLock()
  : mReadCount(1)
{
  MOZ_COUNT_CTOR(gfxMemorySharedReadLock);
}

gfxMemorySharedReadLock::~gfxMemorySharedReadLock()
{
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


TileClient::TileClient()
  : mBackBuffer(nullptr)
  , mFrontBuffer(nullptr)
  , mBackLock(nullptr)
  , mFrontLock(nullptr)
{
}

TileClient::TileClient(const TileClient& o)
{
  mBackBuffer = o.mBackBuffer;
  mFrontBuffer = o.mFrontBuffer;
  mBackLock = o.mBackLock;
  mFrontLock = o.mFrontLock;
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
  mBackBuffer = o.mBackBuffer;
  mFrontBuffer = o.mFrontBuffer;
  mBackLock = o.mBackLock;
  mFrontLock = o.mFrontLock;
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
  RefPtr<TextureClient> frontBuffer = mFrontBuffer;
  mFrontBuffer = mBackBuffer;
  mBackBuffer = frontBuffer;
  RefPtr<gfxSharedReadLock> frontLock = mFrontLock;
  mFrontLock = mBackLock;
  mBackLock = frontLock;
  nsIntRegion invalidFront = mInvalidFront;
  mInvalidFront = mInvalidBack;
  mInvalidBack = invalidFront;
}

void
TileClient::ValidateBackBufferFromFront(const nsIntRegion& aDirtyRegion,
                                        bool aCanRerasterizeValidRegion)
{
  if (mBackBuffer && mFrontBuffer) {
    gfx::IntSize tileSize = mFrontBuffer->GetSize();
    const nsIntRect tileRect = nsIntRect(0, 0, tileSize.width, tileSize.height);

    if (aDirtyRegion.Contains(tileRect)) {
      
      
      DiscardFrontBuffer();
    } else {
      
      nsIntRegion regionToCopy = mInvalidBack;

      regionToCopy.Sub(regionToCopy, aDirtyRegion);

      if (regionToCopy.IsEmpty() ||
          (aCanRerasterizeValidRegion &&
           regionToCopy.Area() < tileSize.width * tileSize.height * MINIMUM_TILE_COPY_AREA)) {
        
        return;
      }

      if (!mFrontBuffer->Lock(OpenMode::OPEN_READ)) {
        NS_WARNING("Failed to lock the tile's front buffer");
        return;
      }
      TextureClientAutoUnlock autoFront(mFrontBuffer);

      if (!mBackBuffer->Lock(OpenMode::OPEN_WRITE)) {
        NS_WARNING("Failed to lock the tile's back buffer");
        return;
      }
      TextureClientAutoUnlock autoBack(mBackBuffer);

      
      
      
      const nsIntRect rectToCopy = regionToCopy.GetBounds();
      gfx::IntRect gfxRectToCopy(rectToCopy.x, rectToCopy.y, rectToCopy.width, rectToCopy.height);
      gfx::IntPoint gfxRectToCopyTopLeft = gfxRectToCopy.TopLeft();
      mFrontBuffer->CopyToTextureClient(mBackBuffer, &gfxRectToCopy, &gfxRectToCopyTopLeft);

      mInvalidBack.SetEmpty();
    }
  }
}

void
TileClient::DiscardFrontBuffer()
{
  if (mFrontBuffer) {
    MOZ_ASSERT(mFrontLock);
    mManager->GetTexturePool(mFrontBuffer->GetFormat())->ReturnTextureClientDeferred(mFrontBuffer);
    mFrontLock->ReadUnlock();
    mFrontBuffer = nullptr;
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
    } else {
      mManager->GetTexturePool(mBackBuffer->GetFormat())->ReturnTextureClient(mBackBuffer);
    }
    mBackLock->ReadUnlock();
    mBackBuffer = nullptr;
    mBackLock = nullptr;
  }
}

TextureClient*
TileClient::GetBackBuffer(const nsIntRegion& aDirtyRegion, TextureClientPool *aPool, bool *aCreatedTextureClient, bool aCanRerasterizeValidRegion)
{
  
  if (mFrontBuffer &&
      mFrontBuffer->HasInternalBuffer() &&
      mFrontLock->GetReadCount() == 1) {
    
    
    DiscardBackBuffer();
    Flip();
    return mBackBuffer;
  }

  if (!mBackBuffer ||
      mBackLock->GetReadCount() > 1) {
    if (mBackBuffer) {
      
      
      
      aPool->ReportClientLost();
    }
    mBackBuffer = aPool->GetTextureClient();
    
    if (mManager->AsShadowForwarder()->IsSameProcess()) {
      
      
      mBackLock = new gfxMemorySharedReadLock();
    } else {
      mBackLock = new gfxShmSharedReadLock(mManager->AsShadowForwarder());
    }

    MOZ_ASSERT(mBackLock->IsValid());

    *aCreatedTextureClient = true;
    mInvalidBack = nsIntRect(0, 0, mBackBuffer->GetSize().width, mBackBuffer->GetSize().height);
  }

  ValidateBackBufferFromFront(aDirtyRegion, aCanRerasterizeValidRegion);

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
                                  TileLock(uintptr_t(mFrontLock.get())));
  } else {
    gfxShmSharedReadLock *lock = static_cast<gfxShmSharedReadLock*>(mFrontLock.get());
    return TexturedTileDescriptor(nullptr, mFrontBuffer->GetIPDLActor(),
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
ClientTiledLayerBuffer::DiscardBackBuffers()
{
  for (size_t i = 0; i < mRetainedTiles.Length(); i++) {
    if (mRetainedTiles[i].IsPlaceholderTile()) continue;
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
  mCallback = aCallback;
  mCallbackData = aCallbackData;

#ifdef GFX_TILEDLAYER_PREF_WARNINGS
  long start = PR_IntervalNow();
#endif

  
  NS_ASSERTION(!aPaintRegion.GetBounds().IsEmpty(), "Empty paint region\n");

  bool useSinglePaintBuffer = UseSinglePaintBuffer();
  
  











  if (useSinglePaintBuffer) {
    nsRefPtr<gfxContext> ctxt;

    const nsIntRect bounds = aPaintRegion.GetBounds();
    {
      PROFILER_LABEL("ClientTiledLayerBuffer", "PaintThebesSingleBufferAlloc");
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
    ctxt->Scale(mResolution, mResolution);
    ctxt->Translate(gfxPoint(-bounds.x, -bounds.y));
#ifdef GFX_TILEDLAYER_PREF_WARNINGS
    if (PR_IntervalNow() - start > 3) {
      printf_stderr("Slow alloc %i\n", PR_IntervalNow() - start);
    }
    start = PR_IntervalNow();
#endif
    PROFILER_LABEL("ClientTiledLayerBuffer", "PaintThebesSingleBufferDraw");

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

  PROFILER_LABEL("ClientTiledLayerBuffer", "PaintThebesUpdate");
  Update(aNewValidRegion, aPaintRegion);

#ifdef GFX_TILEDLAYER_PREF_WARNINGS
  if (PR_IntervalNow() - start > 10) {
    const nsIntRect bounds = aPaintRegion.GetBounds();
    printf_stderr("Time to tile %i: %i, %i, %i, %i\n", PR_IntervalNow() - start, bounds.x, bounds.y, bounds.width, bounds.height);
  }
#endif

  mLastPaintOpaque = mThebesLayer->CanUseOpaqueSurface();
  mCallback = nullptr;
  mCallbackData = nullptr;
  mSinglePaintDrawTarget = nullptr;
}

TileClient
ClientTiledLayerBuffer::ValidateTile(TileClient aTile,
                                    const nsIntPoint& aTileOrigin,
                                    const nsIntRegion& aDirtyRegion)
{
  PROFILER_LABEL("ClientTiledLayerBuffer", "ValidateTile");

#ifdef GFX_TILEDLAYER_PREF_WARNINGS
  if (aDirtyRegion.IsComplex()) {
    printf_stderr("Complex region\n");
  }
#endif

  if (aTile.IsPlaceholderTile()) {
    aTile.SetLayerManager(mManager);
  }

  
  
  
  if (HasFormatChanged()) {
    aTile.DiscardBackBuffer();
    aTile.DiscardFrontBuffer();
  }

  bool createdTextureClient = false;
  nsIntRegion offsetScaledDirtyRegion = aDirtyRegion.MovedBy(-aTileOrigin);
  offsetScaledDirtyRegion.ScaleRoundOut(mResolution, mResolution);

  bool usingSinglePaintBuffer = !!mSinglePaintDrawTarget;
  RefPtr<TextureClient> backBuffer =
    aTile.GetBackBuffer(offsetScaledDirtyRegion,
                        mManager->GetTexturePool(gfxPlatform::GetPlatform()->Optimal2DFormatForContent(GetContentType())),
                        &createdTextureClient, !usingSinglePaintBuffer);

  if (!backBuffer->Lock(OpenMode::OPEN_READ_WRITE)) {
    NS_WARNING("Failed to lock tile TextureClient for updating.");
    aTile.DiscardFrontBuffer();
    return aTile;
  }

  
  
  
  RefPtr<DrawTarget> drawTarget = backBuffer->GetAsDrawTarget();
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
    ctxt->Translate(gfxPoint(-unscaledTileOrigin.x, -unscaledTileOrigin.y));
    ctxt->Scale(mResolution, mResolution);
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

  backBuffer->Unlock();

  aTile.Flip();

  if (createdTextureClient) {
    if (!mCompositableClient->AddTextureClient(backBuffer)) {
      NS_WARNING("Failed to add tile TextureClient.");
      aTile.DiscardFrontBuffer();
      aTile.DiscardBackBuffer();
      return aTile;
    }
  }

  
  

  if (backBuffer->HasInternalBuffer()) {
    
    
    aTile.DiscardBackBuffer();
  }

  return aTile;
}

static LayoutDeviceRect
TransformCompositionBounds(const ParentLayerRect& aCompositionBounds,
                           const CSSToParentLayerScale& aZoom,
                           const ParentLayerPoint& aScrollOffset,
                           const CSSToParentLayerScale& aResolution,
                           const gfx3DMatrix& aTransformParentLayerToLayoutDevice)
{
  
  
  
  ParentLayerRect offsetViewportRect = (aCompositionBounds / aZoom) * aResolution;
  offsetViewportRect.MoveBy(-aScrollOffset);

  gfxRect transformedViewport =
    aTransformParentLayerToLayoutDevice.TransformBounds(
      gfxRect(offsetViewportRect.x, offsetViewportRect.y,
              offsetViewportRect.width, offsetViewportRect.height));

  return LayoutDeviceRect(transformedViewport.x,
                          transformedViewport.y,
                          transformedViewport.width,
                          transformedViewport.height);
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

  
  
  
  ParentLayerRect compositionBounds;
  CSSToParentLayerScale zoom;
#if defined(MOZ_WIDGET_ANDROID)
  bool abortPaint = mManager->ProgressiveUpdateCallback(!staleRegion.Contains(aInvalidRegion),
                                                        compositionBounds, zoom,
                                                        !drawingLowPrecision);
#else
  MOZ_ASSERT(mSharedFrameMetricsHelper);

  ContainerLayer* parent = mThebesLayer->AsLayer()->GetParent();

  bool abortPaint =
    mSharedFrameMetricsHelper->UpdateFromCompositorFrameMetrics(
      parent,
      !staleRegion.Contains(aInvalidRegion),
      drawingLowPrecision,
      compositionBounds,
      zoom);
#endif

  if (abortPaint) {
    
    
    
    if (!aPaintData->mFirstPaint || drawingLowPrecision) {
      PROFILER_LABEL("ContentClient", "Abort painting");
      aRegionToPaint.SetEmpty();
      return aIsRepeated;
    }
  }

  
  
  
  LayoutDeviceRect transformedCompositionBounds =
    TransformCompositionBounds(compositionBounds, zoom, aPaintData->mScrollOffset,
                               aPaintData->mResolution, aPaintData->mTransformParentLayerToLayoutDevice);

  
  
  
  
  LayoutDeviceRect typedCoherentUpdateRect =
    transformedCompositionBounds.Intersect(aPaintData->mCompositionBounds);

  
  
  typedCoherentUpdateRect.MoveBy(aPaintData->mViewport.TopLeft());

  
  nsIntRect roundedCoherentUpdateRect =
    LayoutDeviceIntRect::ToUntyped(RoundedOut(typedCoherentUpdateRect));

  aRegionToPaint.And(aInvalidRegion, roundedCoherentUpdateRect);
  aRegionToPaint.Or(aRegionToPaint, staleRegion);
  bool drawingStale = !aRegionToPaint.IsEmpty();
  if (!drawingStale) {
    aRegionToPaint = aInvalidRegion;
  }

  
  bool paintVisible = false;
  if (aRegionToPaint.Intersects(roundedCoherentUpdateRect)) {
    aRegionToPaint.And(aRegionToPaint, roundedCoherentUpdateRect);
    paintVisible = true;
  }

  
  
  bool paintInSingleTransaction = paintVisible && (drawingStale || aPaintData->mFirstPaint);

  
  
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
    
    

    
    
    
    
    
    if (!drawingLowPrecision && paintInSingleTransaction) {
      return true;
    }

    mManager->SetRepeatTransaction();
    return false;
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
  bool repeat = false;
  bool isBufferChanged = false;
  do {
    
    
    nsIntRegion regionToPaint;
    repeat = ComputeProgressiveUpdateRegion(aInvalidRegion,
                                            aOldValidRegion,
                                            regionToPaint,
                                            aPaintData,
                                            repeat);

    
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

  
  
  return isBufferChanged;
}

}
}
