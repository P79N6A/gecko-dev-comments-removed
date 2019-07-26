




#ifndef MOZILLA_GFX_TILEDCONTENTCLIENT_H
#define MOZILLA_GFX_TILEDCONTENTCLIENT_H

#include <stddef.h>                     
#include <stdint.h>                     
#include <algorithm>                    
#include "Layers.h"                     
#include "TiledLayerBuffer.h"           
#include "Units.h"                      
#include "gfx3DMatrix.h"                
#include "gfxTypes.h"
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/ipc/Shmem.h"          
#include "mozilla/ipc/SharedMemory.h"   
#include "mozilla/layers/CompositableClient.h"  
#include "mozilla/layers/CompositorTypes.h"  
#include "mozilla/layers/LayersMessages.h" 
#include "mozilla/layers/TextureClient.h"
#include "mozilla/layers/TextureClientPool.h"
#include "ClientLayerManager.h"
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsISupportsImpl.h"            
#include "nsPoint.h"                    
#include "nsRect.h"                     
#include "nsRegion.h"                   
#include "nsTArray.h"                   
#include "mozilla/layers/ISurfaceAllocator.h"
#include "gfxReusableSurfaceWrapper.h"
#include "pratom.h"                     
#include "gfxPrefs.h"

class gfxImageSurface;

namespace mozilla {
namespace layers {

class BasicTileDescriptor;
class ClientTiledThebesLayer;
class ClientLayerManager;



class gfxSharedReadLock : public AtomicRefCounted<gfxSharedReadLock> {
public:
  MOZ_DECLARE_REFCOUNTED_TYPENAME(gfxSharedReadLock)
  virtual ~gfxSharedReadLock() {}

  virtual int32_t ReadLock() = 0;
  virtual int32_t ReadUnlock() = 0;
  virtual int32_t GetReadCount() = 0;
  virtual bool IsValid() const = 0;

  enum gfxSharedReadLockType {
    TYPE_MEMORY,
    TYPE_SHMEM
  };
  virtual gfxSharedReadLockType GetType() = 0;

protected:
  NS_DECL_OWNINGTHREAD
};

class gfxMemorySharedReadLock : public gfxSharedReadLock {
public:
  gfxMemorySharedReadLock();

  ~gfxMemorySharedReadLock();

  virtual int32_t ReadLock() MOZ_OVERRIDE;

  virtual int32_t ReadUnlock() MOZ_OVERRIDE;

  virtual int32_t GetReadCount() MOZ_OVERRIDE;

  virtual gfxSharedReadLockType GetType() MOZ_OVERRIDE { return TYPE_MEMORY; }

  virtual bool IsValid() const MOZ_OVERRIDE { return true; };

private:
  int32_t mReadCount;
};

class gfxShmSharedReadLock : public gfxSharedReadLock {
private:
  struct ShmReadLockInfo {
    int32_t readCount;
  };

public:
  gfxShmSharedReadLock(ISurfaceAllocator* aAllocator);

  ~gfxShmSharedReadLock();

  virtual int32_t ReadLock() MOZ_OVERRIDE;

  virtual int32_t ReadUnlock() MOZ_OVERRIDE;

  virtual int32_t GetReadCount() MOZ_OVERRIDE;

  virtual bool IsValid() const MOZ_OVERRIDE { return mAllocSuccess; };

  virtual gfxSharedReadLockType GetType() MOZ_OVERRIDE { return TYPE_SHMEM; }

  mozilla::layers::ShmemSection& GetShmemSection() { return mShmemSection; }

  static already_AddRefed<gfxShmSharedReadLock>
  Open(mozilla::layers::ISurfaceAllocator* aAllocator, const mozilla::layers::ShmemSection& aShmemSection)
  {
    nsRefPtr<gfxShmSharedReadLock> readLock = new gfxShmSharedReadLock(aAllocator, aShmemSection);
    return readLock.forget();
  }

private:
  gfxShmSharedReadLock(ISurfaceAllocator* aAllocator, const mozilla::layers::ShmemSection& aShmemSection)
    : mAllocator(aAllocator)
    , mShmemSection(aShmemSection)
    , mAllocSuccess(true)
  {
    MOZ_COUNT_CTOR(gfxShmSharedReadLock);
  }

  ShmReadLockInfo* GetShmReadLockInfoPtr()
  {
    return reinterpret_cast<ShmReadLockInfo*>
      (mShmemSection.shmem().get<char>() + mShmemSection.offset());
  }

  RefPtr<ISurfaceAllocator> mAllocator;
  mozilla::layers::ShmemSection mShmemSection;
  bool mAllocSuccess;
};










struct TileClient
{
  
  TileClient();

  TileClient(const TileClient& o);

  TileClient& operator=(const TileClient& o);

  bool operator== (const TileClient& o) const
  {
    return mFrontBuffer == o.mFrontBuffer;
  }

  bool operator!= (const TileClient& o) const
  {
    return mFrontBuffer != o.mFrontBuffer;
  }

  void SetLayerManager(ClientLayerManager *aManager)
  {
    mManager = aManager;
  }

  bool IsPlaceholderTile()
  {
    return mBackBuffer == nullptr && mFrontBuffer == nullptr;
  }

  void ReadUnlock()
  {
    MOZ_ASSERT(mFrontLock, "ReadLock with no gfxSharedReadLock");
    if (mFrontLock) {
      mFrontLock->ReadUnlock();
    }
  }

  void ReadLock()
  {
    MOZ_ASSERT(mFrontLock, "ReadLock with no gfxSharedReadLock");
    if (mFrontLock) {
      mFrontLock->ReadLock();
    }
  }

  void Release()
  {
    DiscardFrontBuffer();
    DiscardBackBuffer();
  }

  TileDescriptor GetTileDescriptor();

  


  void Flip();

  





  TextureClient* GetBackBuffer(const nsIntRegion& aDirtyRegion,
                               TextureClientPool *aPool,
                               bool *aCreatedTextureClient,
                               bool aCanRerasterizeValidRegion);

  void DiscardFrontBuffer();

  void DiscardBackBuffer();

  RefPtr<TextureClient> mBackBuffer;
  RefPtr<TextureClient> mFrontBuffer;
  RefPtr<gfxSharedReadLock> mBackLock;
  RefPtr<gfxSharedReadLock> mFrontLock;
  RefPtr<ClientLayerManager> mManager;
#ifdef GFX_TILEDLAYER_DEBUG_OVERLAY
  TimeStamp        mLastUpdate;
#endif
  nsIntRegion mInvalidFront;
  nsIntRegion mInvalidBack;

private:
  void ValidateBackBufferFromFront(const nsIntRegion &aDirtyRegion,
                                   bool aCanRerasterizeValidRegion);
};





struct BasicTiledLayerPaintData {
  



  ParentLayerPoint mScrollOffset;

  




  ParentLayerPoint mLastScrollOffset;

  


  gfx3DMatrix mTransformParentLayerToLayoutDevice;

  







  nsIntRect mCriticalDisplayPort;

  



  LayoutDeviceRect mViewport;

  



  CSSToParentLayerScale mResolution;

  




  LayoutDeviceRect mCompositionBounds;

  




  uint16_t mLowPrecisionPaintCount;

  


  bool mFirstPaint : 1;

  




  bool mPaintFinished : 1;
};

class SharedFrameMetricsHelper
{
public:
  SharedFrameMetricsHelper();
  ~SharedFrameMetricsHelper();

  






  bool UpdateFromCompositorFrameMetrics(ContainerLayer* aLayer,
                                        bool aHasPendingNewThebesContent,
                                        bool aLowPrecision,
                                        ParentLayerRect& aCompositionBounds,
                                        CSSToParentLayerScale& aZoom);

  




  void FindFallbackContentFrameMetrics(ContainerLayer* aLayer,
                                       ParentLayerRect& aCompositionBounds,
                                       CSSToParentLayerScale& aZoom);
  






  bool AboutToCheckerboard(const FrameMetrics& aContentMetrics,
                           const FrameMetrics& aCompositorMetrics);
private:
  bool mLastProgressiveUpdateWasLowPrecision;
  bool mProgressiveUpdateWasInDanger;
};








class ClientTiledLayerBuffer
  : public TiledLayerBuffer<ClientTiledLayerBuffer, TileClient>
{
  friend class TiledLayerBuffer<ClientTiledLayerBuffer, TileClient>;

public:
  ClientTiledLayerBuffer(ClientTiledThebesLayer* aThebesLayer,
                         CompositableClient* aCompositableClient,
                         ClientLayerManager* aManager,
                         SharedFrameMetricsHelper* aHelper);
  ClientTiledLayerBuffer()
    : mThebesLayer(nullptr)
    , mCompositableClient(nullptr)
    , mManager(nullptr)
    , mLastPaintOpaque(false)
    , mSharedFrameMetricsHelper(nullptr)
  {}

  void PaintThebes(const nsIntRegion& aNewValidRegion,
                   const nsIntRegion& aPaintRegion,
                   LayerManager::DrawThebesLayerCallback aCallback,
                   void* aCallbackData);

  void ReadUnlock();

  void ReadLock();

  void Release();

  void DiscardBackBuffers();

  const CSSToParentLayerScale& GetFrameResolution() { return mFrameResolution; }

  void SetFrameResolution(const CSSToParentLayerScale& aResolution) { mFrameResolution = aResolution; }

  bool HasFormatChanged() const;

  



  bool ProgressiveUpdate(nsIntRegion& aValidRegion,
                         nsIntRegion& aInvalidRegion,
                         const nsIntRegion& aOldValidRegion,
                         BasicTiledLayerPaintData* aPaintData,
                         LayerManager::DrawThebesLayerCallback aCallback,
                         void* aCallbackData);

  SurfaceDescriptorTiles GetSurfaceDescriptorTiles();

protected:
  TileClient ValidateTile(TileClient aTile,
                          const nsIntPoint& aTileRect,
                          const nsIntRegion& dirtyRect);

  
  
  
  
  bool UseSinglePaintBuffer() { return !gfxPrefs::PerTileDrawing(); }

  void ReleaseTile(TileClient aTile) { aTile.Release(); }

  void SwapTiles(TileClient& aTileA, TileClient& aTileB) { std::swap(aTileA, aTileB); }

  TileClient GetPlaceholderTile() const { return TileClient(); }

private:
  gfxContentType GetContentType() const;
  ClientTiledThebesLayer* mThebesLayer;
  CompositableClient* mCompositableClient;
  ClientLayerManager* mManager;
  LayerManager::DrawThebesLayerCallback mCallback;
  void* mCallbackData;
  CSSToParentLayerScale mFrameResolution;
  bool mLastPaintOpaque;

  
  RefPtr<gfx::DrawTarget>       mSinglePaintDrawTarget;
  nsIntPoint                    mSinglePaintBufferOffset;
  SharedFrameMetricsHelper*  mSharedFrameMetricsHelper;

  
















  bool ComputeProgressiveUpdateRegion(const nsIntRegion& aInvalidRegion,
                                      const nsIntRegion& aOldValidRegion,
                                      nsIntRegion& aRegionToPaint,
                                      BasicTiledLayerPaintData* aPaintData,
                                      bool aIsRepeated);
};

class TiledContentClient : public CompositableClient
{
  
  
  
  
  friend class ClientTiledThebesLayer;

public:
  TiledContentClient(ClientTiledThebesLayer* aThebesLayer,
                     ClientLayerManager* aManager);

  ~TiledContentClient()
  {
    MOZ_COUNT_DTOR(TiledContentClient);

    mTiledBuffer.Release();
    mLowPrecisionTiledBuffer.Release();
  }

  virtual TextureInfo GetTextureInfo() const MOZ_OVERRIDE
  {
    return TextureInfo(BUFFER_TILED);
  }

  virtual void ClearCachedResources() MOZ_OVERRIDE;

  enum TiledBufferType {
    TILED_BUFFER,
    LOW_PRECISION_TILED_BUFFER
  };
  void UseTiledLayerBuffer(TiledBufferType aType);

private:
  SharedFrameMetricsHelper mSharedFrameMetricsHelper;
  ClientTiledLayerBuffer mTiledBuffer;
  ClientTiledLayerBuffer mLowPrecisionTiledBuffer;
};

}
}

#endif
