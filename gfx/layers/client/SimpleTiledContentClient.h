




#ifndef MOZILLA_GFX_SIMPLETILEDCONTENTCLIENT_H
#define MOZILLA_GFX_SIMPLETILEDCONTENTCLIENT_H



#include "TiledContentClient.h"

#include "SharedBuffer.h"

namespace mozilla {
namespace layers {

class ClientTiledThebesLayer;

struct SimpleTiledLayerTile;
class SimpleTiledLayerBuffer;
class SimpleClientTiledThebesLayer;
class SimpleTiledLayerBuffer;

#define GFX_SIMP_TILEDLAYER_DEBUG_OVERLAY

struct SimpleTiledLayerTile
{
  RefPtr<TextureClient> mTileBuffer;
  RefPtr<ClientLayerManager> mManager;
  nsRefPtr<SharedBuffer> mCachedBuffer;
  TimeStamp mLastUpdate;

  SimpleTiledLayerTile() { }

  SimpleTiledLayerTile(ClientLayerManager *aManager, TextureClient *aBuffer)
    : mTileBuffer(aBuffer)
    , mManager(aManager)
  { }

  bool operator== (const SimpleTiledLayerTile& o) const
  {
    return mTileBuffer == o.mTileBuffer;
  }

  bool operator!= (const SimpleTiledLayerTile& o) const
  {
    return mTileBuffer != o.mTileBuffer;
  }

  void SetLayerManager(ClientLayerManager *aManager)
  {
    mManager = aManager;
  }

  bool IsPlaceholderTile()
  {
    return mTileBuffer == nullptr;
  }

  TileDescriptor GetTileDescriptor()
  {
    if (mTileBuffer)
      return TexturedTileDescriptor(nullptr, mTileBuffer->GetIPDLActor(), 0);

    NS_NOTREACHED("Unhandled SimpleTiledLayerTile type");
    return PlaceholderTileDescriptor();
  }

  void Release()
  {
    mTileBuffer = nullptr;
    mCachedBuffer = nullptr;
  }
};

class SimpleTiledLayerBuffer
  : public TiledLayerBuffer<SimpleTiledLayerBuffer, SimpleTiledLayerTile>
{
  friend class TiledLayerBuffer<SimpleTiledLayerBuffer, SimpleTiledLayerTile>;

public:
  SimpleTiledLayerBuffer(SimpleClientTiledThebesLayer* aThebesLayer,
                         CompositableClient* aCompositableClient,
                         ClientLayerManager* aManager)
    : mThebesLayer(aThebesLayer)
    , mCompositableClient(aCompositableClient)
    , mManager(aManager)
    , mLastPaintOpaque(false)
  {}

  SimpleTiledLayerBuffer()
    : mLastPaintOpaque(false)
  {}

  void PaintThebes(const nsIntRegion& aNewValidRegion,
                   const nsIntRegion& aPaintRegion,
                   LayerManager::DrawThebesLayerCallback aCallback,
                   void* aCallbackData);

  SurfaceDescriptorTiles GetSurfaceDescriptorTiles();

  void Release() {
    for (size_t i = 0; i < mRetainedTiles.Length(); i++) {
      mRetainedTiles[i].Release();
    }
  }

  const CSSToParentLayerScale& GetFrameResolution() const { return mFrameResolution; }
  void SetFrameResolution(const CSSToParentLayerScale& aResolution) { mFrameResolution = aResolution; }

  bool HasFormatChanged() const;
private:
  SimpleClientTiledThebesLayer* mThebesLayer;
  CompositableClient* mCompositableClient;
  ClientLayerManager* mManager;
  LayerManager::DrawThebesLayerCallback mCallback;
  void* mCallbackData;
  CSSToParentLayerScale mFrameResolution;
  bool mLastPaintOpaque;

  gfxContentType GetContentType() const;

  SimpleTiledLayerTile ValidateTile(SimpleTiledLayerTile aTile,
                                    const nsIntPoint& aTileOrigin,
                                    const nsIntRegion& aDirtyRect);

  SimpleTiledLayerTile GetPlaceholderTile() const { return SimpleTiledLayerTile(); }

  void ReleaseTile(SimpleTiledLayerTile aTile) { aTile.Release(); }

  void SwapTiles(SimpleTiledLayerTile& aTileA, SimpleTiledLayerTile& aTileB) { std::swap(aTileA, aTileB); }

  void PostValidate(const nsIntRegion& aPaintRegion) {}
  void UnlockTile(SimpleTiledLayerTile aTile) {}
};

class SimpleTiledContentClient : public CompositableClient
{
  friend class SimpleClientTiledThebesLayer;

public:
  SimpleTiledContentClient(SimpleClientTiledThebesLayer* aThebesLayer,
                           ClientLayerManager* aManager);

private:
  ~SimpleTiledContentClient();

  virtual TextureInfo GetTextureInfo() const MOZ_OVERRIDE
  {
    return TextureInfo(CompositableType::BUFFER_SIMPLE_TILED);
  }

  void UseTiledLayerBuffer();

private:
  SimpleTiledLayerBuffer mTiledBuffer;
};

class SimpleClientTiledThebesLayer : public ThebesLayer,
                                     public ClientLayer
{
  typedef ThebesLayer Base;

public:
  SimpleClientTiledThebesLayer(ClientLayerManager* const aManager,
                               ClientLayerManager::ThebesLayerCreationHint aCreationHint = LayerManager::NONE);
protected:
  ~SimpleClientTiledThebesLayer();

public:
  
  virtual Layer* AsLayer() { return this; }
  virtual void InvalidateRegion(const nsIntRegion& aRegion) {
    mInvalidRegion.Or(mInvalidRegion, aRegion);
    mValidRegion.Sub(mValidRegion, aRegion);
  }

  
  virtual void FillSpecificAttributes(SpecificLayerAttributes& aAttrs);
  virtual ShadowableLayer* AsShadowableLayer() { return this; }

  virtual void Disconnect() { ClientLayer::Disconnect(); }

  virtual void RenderLayer();

protected:
  ClientLayerManager* ClientManager() { return static_cast<ClientLayerManager*>(mManager); }

  RefPtr<SimpleTiledContentClient> mContentClient;
};

} 
} 

#endif
