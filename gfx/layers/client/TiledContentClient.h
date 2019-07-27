




#ifndef MOZILLA_GFX_TILEDCONTENTCLIENT_H
#define MOZILLA_GFX_TILEDCONTENTCLIENT_H

#include <stddef.h>                     
#include <stdint.h>                     
#include <algorithm>                    
#include <limits>
#include "Layers.h"                     
#include "TiledLayerBuffer.h"           
#include "Units.h"                      
#include "gfxTypes.h"
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/ipc/Shmem.h"          
#include "mozilla/ipc/SharedMemory.h"   
#include "mozilla/layers/AsyncCompositionManager.h"  
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
#include "nsExpirationTracker.h"
#include "mozilla/layers/ISurfaceAllocator.h"
#include "gfxReusableSurfaceWrapper.h"
#include "pratom.h"                     
#include "gfxPrefs.h"

namespace mozilla {
namespace layers {

class ClientTiledPaintedLayer;
class ClientLayerManager;



class gfxSharedReadLock {
protected:
  virtual ~gfxSharedReadLock() {}

public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(gfxSharedReadLock)

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

protected:
  ~gfxMemorySharedReadLock();

public:
  virtual int32_t ReadLock() override;

  virtual int32_t ReadUnlock() override;

  virtual int32_t GetReadCount() override;

  virtual gfxSharedReadLockType GetType() override { return TYPE_MEMORY; }

  virtual bool IsValid() const override { return true; };

private:
  int32_t mReadCount;
};

class gfxShmSharedReadLock : public gfxSharedReadLock {
private:
  struct ShmReadLockInfo {
    int32_t readCount;
  };

public:
  explicit gfxShmSharedReadLock(ISurfaceAllocator* aAllocator);

protected:
  ~gfxShmSharedReadLock();

public:
  virtual int32_t ReadLock() override;

  virtual int32_t ReadUnlock() override;

  virtual int32_t GetReadCount() override;

  virtual bool IsValid() const override { return mAllocSuccess; };

  virtual gfxSharedReadLockType GetType() override { return TYPE_SHMEM; }

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
  ~TileClient();

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

  void SetCompositableClient(CompositableClient* aCompositableClient)
  {
    mCompositableClient = aCompositableClient;
  }

  bool IsPlaceholderTile() const
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

  nsExpirationState *GetExpirationState() { return &mExpirationState; }

  TileDescriptor GetTileDescriptor();

  


  void Dump(std::stringstream& aStream);

  


  void Flip();

  void DumpTexture(std::stringstream& aStream) {
    
    CompositableClient::DumpTextureClient(aStream, mFrontBuffer);
  }

  











  TextureClient* GetBackBuffer(const nsIntRegion& aDirtyRegion,
                               gfxContentType aContent, SurfaceMode aMode,
                               bool *aCreatedTextureClient,
                               nsIntRegion& aAddPaintedRegion,
                               RefPtr<TextureClient>* aTextureClientOnWhite);

  void DiscardFrontBuffer();

  void DiscardBackBuffer();

  


  class PrivateProtector {
    public:
      void Set(TileClient * container, RefPtr<TextureClient>);
      void Set(TileClient * container, TextureClient*);
      
      
      operator TextureClient*() const { return mBuffer; }
      RefPtr<TextureClient> operator ->() { return mBuffer; }
    private:
      PrivateProtector& operator=(const PrivateProtector &);
      RefPtr<TextureClient> mBuffer;
  } mBackBuffer;
  RefPtr<TextureClient> mBackBufferOnWhite;
  RefPtr<TextureClient> mFrontBuffer;
  RefPtr<TextureClient> mFrontBufferOnWhite;
  RefPtr<gfxSharedReadLock> mBackLock;
  RefPtr<gfxSharedReadLock> mFrontLock;
  RefPtr<ClientLayerManager> mManager;
  CompositableClient* mCompositableClient;
#ifdef GFX_TILEDLAYER_DEBUG_OVERLAY
  TimeStamp        mLastUpdate;
#endif
  nsIntRegion mInvalidFront;
  nsIntRegion mInvalidBack;
  nsExpirationState mExpirationState;

private:
  
  
  void ValidateBackBufferFromFront(const nsIntRegion &aDirtyRegion,
                                   nsIntRegion& aAddPaintedRegion);
};





struct BasicTiledLayerPaintData {
  



  ParentLayerPoint mScrollOffset;

  




  ParentLayerPoint mLastScrollOffset;

  





  gfx::Matrix4x4 mTransformToCompBounds;

  




  LayerIntRect mCriticalDisplayPort;

  



  CSSToParentLayerScale2D mResolution;

  




  LayerRect mCompositionBounds;

  




  uint16_t mLowPrecisionPaintCount;

  


  bool mFirstPaint : 1;

  




  bool mPaintFinished : 1;
};

class SharedFrameMetricsHelper
{
public:
  SharedFrameMetricsHelper();
  ~SharedFrameMetricsHelper();

  






  bool UpdateFromCompositorFrameMetrics(const LayerMetricsWrapper& aLayer,
                                        bool aHasPendingNewThebesContent,
                                        bool aLowPrecision,
                                        ViewTransform& aViewTransform);

  






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
  ClientTiledLayerBuffer(ClientTiledPaintedLayer* aPaintedLayer,
                         CompositableClient* aCompositableClient,
                         ClientLayerManager* aManager,
                         SharedFrameMetricsHelper* aHelper);
  ClientTiledLayerBuffer()
    : mPaintedLayer(nullptr)
    , mCompositableClient(nullptr)
    , mManager(nullptr)
    , mLastPaintContentType(gfxContentType::COLOR)
    , mLastPaintSurfaceMode(SurfaceMode::SURFACE_OPAQUE)
    , mSharedFrameMetricsHelper(nullptr)
    , mTilingOrigin(std::numeric_limits<int32_t>::max(),
                    std::numeric_limits<int32_t>::max())
  {}

  void PaintThebes(const nsIntRegion& aNewValidRegion,
                   const nsIntRegion& aPaintRegion,
                   LayerManager::DrawPaintedLayerCallback aCallback,
                   void* aCallbackData);

  void ReadUnlock();

  void ReadLock();

  void Release();

  void DiscardBuffers();

  const CSSToParentLayerScale2D& GetFrameResolution() { return mFrameResolution; }

  void SetFrameResolution(const CSSToParentLayerScale2D& aResolution) { mFrameResolution = aResolution; }

  bool HasFormatChanged() const;

  



  bool ProgressiveUpdate(nsIntRegion& aValidRegion,
                         nsIntRegion& aInvalidRegion,
                         const nsIntRegion& aOldValidRegion,
                         BasicTiledLayerPaintData* aPaintData,
                         LayerManager::DrawPaintedLayerCallback aCallback,
                         void* aCallbackData);

  SurfaceDescriptorTiles GetSurfaceDescriptorTiles();

protected:
  TileClient ValidateTile(TileClient aTile,
                          const nsIntPoint& aTileRect,
                          const nsIntRegion& dirtyRect);

  void PostValidate(const nsIntRegion& aPaintRegion);

  void UnlockTile(TileClient aTile);

  void ReleaseTile(TileClient aTile) { aTile.Release(); }

  void SwapTiles(TileClient& aTileA, TileClient& aTileB) { std::swap(aTileA, aTileB); }

  TileClient GetPlaceholderTile() const { return TileClient(); }

private:
  gfxContentType GetContentType(SurfaceMode* aMode = nullptr) const;
  ClientTiledPaintedLayer* mPaintedLayer;
  CompositableClient* mCompositableClient;
  ClientLayerManager* mManager;
  LayerManager::DrawPaintedLayerCallback mCallback;
  void* mCallbackData;
  CSSToParentLayerScale2D mFrameResolution;
  gfxContentType mLastPaintContentType;
  SurfaceMode mLastPaintSurfaceMode;

  
  
  nsIntRegion mNewValidRegion;

  
  RefPtr<gfx::DrawTarget>       mSinglePaintDrawTarget;
  nsIntPoint                    mSinglePaintBufferOffset;
  SharedFrameMetricsHelper*  mSharedFrameMetricsHelper;
  
  std::vector<gfx::Tile> mMoz2DTiles;
  








  gfx::IntPoint mTilingOrigin;
  
















  bool ComputeProgressiveUpdateRegion(const nsIntRegion& aInvalidRegion,
                                      const nsIntRegion& aOldValidRegion,
                                      nsIntRegion& aRegionToPaint,
                                      BasicTiledLayerPaintData* aPaintData,
                                      bool aIsRepeated);
};

class TiledContentClient : public CompositableClient
{
  
  
  
  
  friend class ClientTiledPaintedLayer;

public:
  TiledContentClient(ClientTiledPaintedLayer* aPaintedLayer,
                     ClientLayerManager* aManager);

protected:
  ~TiledContentClient()
  {
    MOZ_COUNT_DTOR(TiledContentClient);

    mTiledBuffer.Release();
    mLowPrecisionTiledBuffer.Release();
  }

  virtual void PrintInfo(std::stringstream& aStream, const char* aPrefix);

  virtual void Dump(std::stringstream& aStream,
                    const char* aPrefix="",
                    bool aDumpHtml=false);

public:
  virtual TextureInfo GetTextureInfo() const override
  {
    return TextureInfo(CompositableType::CONTENT_TILED);
  }

  virtual void ClearCachedResources() override;

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
