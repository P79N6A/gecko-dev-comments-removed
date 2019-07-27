




#ifndef GFX_CLIENTLAYERMANAGER_H
#define GFX_CLIENTLAYERMANAGER_H

#include <stdint.h>                     
#include "Layers.h"
#include "gfxContext.h"                 
#include "mozilla/Attributes.h"         
#include "mozilla/LinkedList.h"         
#include "mozilla/WidgetUtils.h"        
#include "mozilla/gfx/Rect.h"           
#include "mozilla/layers/CompositorTypes.h"
#include "mozilla/layers/LayersTypes.h"  
#include "mozilla/layers/ShadowLayers.h"  
#include "mozilla/layers/APZTestData.h" 
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsDebug.h"                    
#include "nsIObserver.h"                
#include "nsISupportsImpl.h"            
#include "nsRect.h"                     
#include "nsTArray.h"                   
#include "nscore.h"                     
#include "mozilla/layers/TransactionIdAllocator.h"

class nsIWidget;

namespace mozilla {
namespace layers {

class ClientThebesLayer;
class CompositorChild;
class ImageLayer;
class PLayerChild;
class TextureClientPool;
class SimpleTextureClientPool;

class ClientLayerManager MOZ_FINAL : public LayerManager
{
  typedef nsTArray<nsRefPtr<Layer> > LayerRefArray;

public:
  explicit ClientLayerManager(nsIWidget* aWidget);

  virtual void Destroy()
  {
    LayerManager::Destroy();
    ClearCachedResources();
  }

protected:
  virtual ~ClientLayerManager();

public:
  virtual ShadowLayerForwarder* AsShadowForwarder()
  {
    return mForwarder;
  }

  virtual ClientLayerManager* AsClientLayerManager()
  {
    return this;
  }

  virtual int32_t GetMaxTextureSize() const;

  virtual void SetDefaultTargetConfiguration(BufferMode aDoubleBuffering, ScreenRotation aRotation);
  virtual void BeginTransactionWithTarget(gfxContext* aTarget);
  virtual void BeginTransaction();
  virtual bool EndEmptyTransaction(EndTransactionFlags aFlags = END_DEFAULT);
  virtual void EndTransaction(DrawThebesLayerCallback aCallback,
                              void* aCallbackData,
                              EndTransactionFlags aFlags = END_DEFAULT);

  virtual LayersBackend GetBackendType() { return LayersBackend::LAYERS_CLIENT; }
  virtual LayersBackend GetCompositorBackendType() MOZ_OVERRIDE
  {
    return AsShadowForwarder()->GetCompositorBackendType();
  }
  virtual void GetBackendName(nsAString& name);
  virtual const char* Name() const { return "Client"; }

  virtual void SetRoot(Layer* aLayer);

  virtual void Mutated(Layer* aLayer);

  virtual bool IsOptimizedFor(ThebesLayer* aLayer, ThebesLayerCreationHint aHint);

  virtual already_AddRefed<ThebesLayer> CreateThebesLayer();
  virtual already_AddRefed<ThebesLayer> CreateThebesLayerWithHint(ThebesLayerCreationHint aHint);
  virtual already_AddRefed<ContainerLayer> CreateContainerLayer();
  virtual already_AddRefed<ImageLayer> CreateImageLayer();
  virtual already_AddRefed<CanvasLayer> CreateCanvasLayer();
  virtual already_AddRefed<ReadbackLayer> CreateReadbackLayer();
  virtual already_AddRefed<ColorLayer> CreateColorLayer();
  virtual already_AddRefed<RefLayer> CreateRefLayer();

  TextureFactoryIdentifier GetTextureFactoryIdentifier()
  {
    return mForwarder->GetTextureFactoryIdentifier();
  }

  virtual void FlushRendering() MOZ_OVERRIDE;
  void SendInvalidRegion(const nsIntRegion& aRegion);

  virtual uint32_t StartFrameTimeRecording(int32_t aBufferSize) MOZ_OVERRIDE;

  virtual void StopFrameTimeRecording(uint32_t         aStartIndex,
                                      nsTArray<float>& aFrameIntervals) MOZ_OVERRIDE;

  virtual bool NeedsWidgetInvalidation() MOZ_OVERRIDE { return false; }

  ShadowableLayer* Hold(Layer* aLayer);

  bool HasShadowManager() const { return mForwarder->HasShadowManager(); }

  virtual bool IsCompositingCheap();
  virtual bool HasShadowManagerInternal() const { return HasShadowManager(); }

  virtual void SetIsFirstPaint() MOZ_OVERRIDE;

  TextureClientPool* GetTexturePool(gfx::SurfaceFormat aFormat);
  SimpleTextureClientPool* GetSimpleTileTexturePool(gfx::SurfaceFormat aFormat);

  
  
  virtual void ClearCachedResources(Layer* aSubtree = nullptr) MOZ_OVERRIDE;

  void HandleMemoryPressure();

  void SetRepeatTransaction() { mRepeatTransaction = true; }
  bool GetRepeatTransaction() { return mRepeatTransaction; }

  bool IsRepeatTransaction() { return mIsRepeatTransaction; }

  void SetTransactionIncomplete() { mTransactionIncomplete = true; }

  bool HasShadowTarget() { return !!mShadowTarget; }

  void SetShadowTarget(gfxContext* aTarget) { mShadowTarget = aTarget; }

  bool CompositorMightResample() { return mCompositorMightResample; } 
  
  DrawThebesLayerCallback GetThebesLayerCallback() const
  { return mThebesLayerCallback; }

  void* GetThebesLayerCallbackData() const
  { return mThebesLayerCallbackData; }

  CompositorChild* GetRemoteRenderer();

  CompositorChild* GetCompositorChild();

  
  virtual bool ShouldAvoidComponentAlphaLayers() { return !IsCompositingCheap(); }

  











  bool ProgressiveUpdateCallback(bool aHasPendingNewThebesContent,
                                 FrameMetrics& aMetrics,
                                 bool aDrawingCritical);

  bool InConstruction() { return mPhase == PHASE_CONSTRUCTION; }
#ifdef DEBUG
  bool InDrawing() { return mPhase == PHASE_DRAWING; }
  bool InForward() { return mPhase == PHASE_FORWARD; }
#endif
  bool InTransaction() { return mPhase != PHASE_NONE; }

  void SetNeedsComposite(bool aNeedsComposite)
  {
    mNeedsComposite = aNeedsComposite;
  }
  bool NeedsComposite() const { return mNeedsComposite; }

  virtual void Composite() MOZ_OVERRIDE;
  virtual bool RequestOverfill(mozilla::dom::OverfillCallback* aCallback) MOZ_OVERRIDE;
  virtual void RunOverfillCallback(const uint32_t aOverfill) MOZ_OVERRIDE;

  virtual void DidComposite(uint64_t aTransactionId);

  virtual bool SupportsMixBlendModes(EnumSet<gfx::CompositionOp>& aMixBlendModes) MOZ_OVERRIDE
  {
   return (GetTextureFactoryIdentifier().mSupportedBlendModes & aMixBlendModes) == aMixBlendModes;
  }

  virtual bool AreComponentAlphaLayersEnabled() MOZ_OVERRIDE;

  
  
  
  void LogTestDataForCurrentPaint(FrameMetrics::ViewID aScrollId,
                                  const std::string& aKey,
                                  const std::string& aValue)
  {
    mApzTestData.LogTestDataForPaint(mPaintSequenceNumber, aScrollId, aKey, aValue);
  }

  
  
  
  void StartNewRepaintRequest(SequenceNumber aSequenceNumber);

  
  
  
  void LogTestDataForRepaintRequest(SequenceNumber aSequenceNumber,
                                    FrameMetrics::ViewID aScrollId,
                                    const std::string& aKey,
                                    const std::string& aValue)
  {
    mApzTestData.LogTestDataForRepaintRequest(aSequenceNumber, aScrollId, aKey, aValue);
  }

  
  
  const APZTestData& GetAPZTestData() const {
    return mApzTestData;
  }

  
  void GetCompositorSideAPZTestData(APZTestData* aData) const;

  void SetTransactionIdAllocator(TransactionIdAllocator* aAllocator) { mTransactionIdAllocator = aAllocator; }

  float RequestProperty(const nsAString& aProperty) MOZ_OVERRIDE;
protected:
  enum TransactionPhase {
    PHASE_NONE, PHASE_CONSTRUCTION, PHASE_DRAWING, PHASE_FORWARD
  };
  TransactionPhase mPhase;

private:
  
  class MemoryPressureObserver MOZ_FINAL : public nsIObserver
  {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

    explicit MemoryPressureObserver(ClientLayerManager* aClientLayerManager)
      : mClientLayerManager(aClientLayerManager)
    {
      RegisterMemoryPressureEvent();
    }

    void Destroy();

  private:
    virtual ~MemoryPressureObserver() {}
    void RegisterMemoryPressureEvent();
    void UnregisterMemoryPressureEvent();

    ClientLayerManager* mClientLayerManager;
  };

  


  void ForwardTransaction(bool aScheduleComposite);

  



  void MakeSnapshotIfRequired();

  void ClearLayer(Layer* aLayer);

  bool EndTransactionInternal(DrawThebesLayerCallback aCallback,
                              void* aCallbackData,
                              EndTransactionFlags);

  LayerRefArray mKeepAlive;

  nsIWidget* mWidget;
  
  

  DrawThebesLayerCallback mThebesLayerCallback;
  void *mThebesLayerCallbackData;

  
  
  
  
  
  
  
  
  nsRefPtr<gfxContext> mShadowTarget;

  nsRefPtr<TransactionIdAllocator> mTransactionIdAllocator;
  uint64_t mLatestTransactionId;

  
  
  
  
  ScreenRotation mTargetRotation;

  
  
  bool mRepeatTransaction;
  bool mIsRepeatTransaction;
  bool mTransactionIncomplete;
  bool mCompositorMightResample;
  bool mNeedsComposite;

  
  
  uint32_t mPaintSequenceNumber;

  APZTestData mApzTestData;

  RefPtr<ShadowLayerForwarder> mForwarder;
  nsAutoTArray<RefPtr<TextureClientPool>,2> mTexturePools;
  nsAutoTArray<dom::OverfillCallback*,0> mOverfillCallbacks;
  mozilla::TimeStamp mTransactionStart;

  
  nsTArray<RefPtr<SimpleTextureClientPool> > mSimpleTilePools;

  nsRefPtr<MemoryPressureObserver> mMemoryPressureObserver;
};

class ClientLayer : public ShadowableLayer
{
public:
  ClientLayer()
  {
    MOZ_COUNT_CTOR(ClientLayer);
  }

  ~ClientLayer();

  void SetShadow(PLayerChild* aShadow)
  {
    NS_ABORT_IF_FALSE(!mShadow, "can't have two shadows (yet)");
    mShadow = aShadow;
  }

  virtual void Disconnect()
  {
    
    
    
    
    
    mShadow = nullptr;
  }

  virtual void ClearCachedResources() { }

  virtual void RenderLayer() = 0;
  virtual void RenderLayerWithReadback(ReadbackProcessor *aReadback) { RenderLayer(); }

  virtual ClientThebesLayer* AsThebes() { return nullptr; }

  static inline ClientLayer *
  ToClientLayer(Layer* aLayer)
  {
    return static_cast<ClientLayer*>(aLayer->ImplData());
  }
};




template<typename CreatedMethod> void
CreateShadowFor(ClientLayer* aLayer,
                ClientLayerManager* aMgr,
                CreatedMethod aMethod)
{
  PLayerChild* shadow = aMgr->AsShadowForwarder()->ConstructShadowFor(aLayer);
  
  NS_ABORT_IF_FALSE(shadow, "failed to create shadow");

  aLayer->SetShadow(shadow);
  (aMgr->AsShadowForwarder()->*aMethod)(aLayer);
  aMgr->Hold(aLayer->AsLayer());
}

#define CREATE_SHADOW(_type)                                       \
  CreateShadowFor(layer, this,                                     \
                  &ShadowLayerForwarder::Created ## _type ## Layer)


}
}

#endif
