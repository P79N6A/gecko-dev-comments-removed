




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
#include "nsIObserver.h"                
#include "nsISupportsImpl.h"            
#include "nsRect.h"                     
#include "nsTArray.h"                   
#include "nscore.h"                     
#include "mozilla/layers/TransactionIdAllocator.h"
#include "nsIWidget.h"                  

namespace mozilla {
namespace layers {

class ClientPaintedLayer;
class CompositorChild;
class ImageLayer;
class PLayerChild;
class TextureClientPool;

class ClientLayerManager final : public LayerManager
{
  typedef nsTArray<nsRefPtr<Layer> > LayerRefArray;

public:
  explicit ClientLayerManager(nsIWidget* aWidget);

  virtual void Destroy() override
  {
    
    
    ClearCachedResources();
    LayerManager::Destroy();
  }

protected:
  virtual ~ClientLayerManager();

public:
  virtual ShadowLayerForwarder* AsShadowForwarder() override
  {
    return mForwarder;
  }

  virtual ClientLayerManager* AsClientLayerManager() override
  {
    return this;
  }

  virtual int32_t GetMaxTextureSize() const override;

  virtual void SetDefaultTargetConfiguration(BufferMode aDoubleBuffering, ScreenRotation aRotation);
  virtual void BeginTransactionWithTarget(gfxContext* aTarget) override;
  virtual void BeginTransaction() override;
  virtual bool EndEmptyTransaction(EndTransactionFlags aFlags = END_DEFAULT) override;
  virtual void EndTransaction(DrawPaintedLayerCallback aCallback,
                              void* aCallbackData,
                              EndTransactionFlags aFlags = END_DEFAULT) override;

  virtual LayersBackend GetBackendType() override { return LayersBackend::LAYERS_CLIENT; }
  virtual LayersBackend GetCompositorBackendType() override
  {
    return AsShadowForwarder()->GetCompositorBackendType();
  }
  virtual void GetBackendName(nsAString& name) override;
  virtual const char* Name() const override { return "Client"; }

  virtual void SetRoot(Layer* aLayer) override;

  virtual void Mutated(Layer* aLayer) override;

  virtual bool IsOptimizedFor(PaintedLayer* aLayer, PaintedLayerCreationHint aHint) override;

  virtual already_AddRefed<PaintedLayer> CreatePaintedLayer() override;
  virtual already_AddRefed<PaintedLayer> CreatePaintedLayerWithHint(PaintedLayerCreationHint aHint) override;
  virtual already_AddRefed<ContainerLayer> CreateContainerLayer() override;
  virtual already_AddRefed<ImageLayer> CreateImageLayer() override;
  virtual already_AddRefed<CanvasLayer> CreateCanvasLayer() override;
  virtual already_AddRefed<ReadbackLayer> CreateReadbackLayer() override;
  virtual already_AddRefed<ColorLayer> CreateColorLayer() override;
  virtual already_AddRefed<RefLayer> CreateRefLayer() override;

  TextureFactoryIdentifier GetTextureFactoryIdentifier()
  {
    return mForwarder->GetTextureFactoryIdentifier();
  }

  virtual void FlushRendering() override;
  void SendInvalidRegion(const nsIntRegion& aRegion);

  virtual uint32_t StartFrameTimeRecording(int32_t aBufferSize) override;

  virtual void StopFrameTimeRecording(uint32_t         aStartIndex,
                                      nsTArray<float>& aFrameIntervals) override;

  virtual bool NeedsWidgetInvalidation() override { return false; }

  ShadowableLayer* Hold(Layer* aLayer);

  bool HasShadowManager() const { return mForwarder->HasShadowManager(); }

  virtual bool IsCompositingCheap() override;
  virtual bool HasShadowManagerInternal() const override { return HasShadowManager(); }

  virtual void SetIsFirstPaint() override;

  TextureClientPool* GetTexturePool(gfx::SurfaceFormat aFormat);

  
  void ReturnTextureClientDeferred(TextureClient& aClient);
  void ReturnTextureClient(TextureClient& aClient);
  void ReportClientLost(TextureClient& aClient);

  





  void StorePluginWidgetConfigurations(const nsTArray<nsIWidget::Configuration>&
                                       aConfigurations) override;

  
  
  virtual void ClearCachedResources(Layer* aSubtree = nullptr) override;

  void HandleMemoryPressure();

  void SetRepeatTransaction() { mRepeatTransaction = true; }
  bool GetRepeatTransaction() { return mRepeatTransaction; }

  bool IsRepeatTransaction() { return mIsRepeatTransaction; }

  void SetTransactionIncomplete() { mTransactionIncomplete = true; }

  bool HasShadowTarget() { return !!mShadowTarget; }

  void SetShadowTarget(gfxContext* aTarget) { mShadowTarget = aTarget; }

  bool CompositorMightResample() { return mCompositorMightResample; } 
  
  DrawPaintedLayerCallback GetPaintedLayerCallback() const
  { return mPaintedLayerCallback; }

  void* GetPaintedLayerCallbackData() const
  { return mPaintedLayerCallbackData; }

  CompositorChild* GetRemoteRenderer();

  CompositorChild* GetCompositorChild();

  
  virtual bool ShouldAvoidComponentAlphaLayers() override { return !IsCompositingCheap(); }

  











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

  virtual void Composite() override;
  virtual bool RequestOverfill(mozilla::dom::OverfillCallback* aCallback) override;
  virtual void RunOverfillCallback(const uint32_t aOverfill) override;

  virtual void DidComposite(uint64_t aTransactionId);

  virtual bool SupportsMixBlendModes(EnumSet<gfx::CompositionOp>& aMixBlendModes) override
  {
   return (GetTextureFactoryIdentifier().mSupportedBlendModes & aMixBlendModes) == aMixBlendModes;
  }

  virtual bool AreComponentAlphaLayersEnabled() override;

  
  
  
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

  float RequestProperty(const nsAString& aProperty) override;

  bool AsyncPanZoomEnabled() const override;

protected:
  enum TransactionPhase {
    PHASE_NONE, PHASE_CONSTRUCTION, PHASE_DRAWING, PHASE_FORWARD
  };
  TransactionPhase mPhase;

private:
  
  class MemoryPressureObserver final : public nsIObserver
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

  bool EndTransactionInternal(DrawPaintedLayerCallback aCallback,
                              void* aCallbackData,
                              EndTransactionFlags);

  LayerRefArray mKeepAlive;

  nsIWidget* mWidget;
  
  

  DrawPaintedLayerCallback mPaintedLayerCallback;
  void *mPaintedLayerCallbackData;

  
  
  
  
  
  
  
  
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
    MOZ_ASSERT(!mShadow, "can't have two shadows (yet)");
    mShadow = aShadow;
  }

  virtual void Disconnect()
  {
    
    
    
    
    
    mShadow = nullptr;
  }

  virtual void ClearCachedResources() { }

  virtual void RenderLayer() = 0;
  virtual void RenderLayerWithReadback(ReadbackProcessor *aReadback) { RenderLayer(); }

  virtual ClientPaintedLayer* AsThebes() { return nullptr; }

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
  
  MOZ_ASSERT(shadow, "failed to create shadow");

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
