




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
#include "nsIWidget.h"                  

namespace mozilla {
namespace layers {

class ClientPaintedLayer;
class CompositorChild;
class ImageLayer;
class PLayerChild;
class TextureClientPool;

class ClientLayerManager MOZ_FINAL : public LayerManager
{
  typedef nsTArray<nsRefPtr<Layer> > LayerRefArray;

public:
  explicit ClientLayerManager(nsIWidget* aWidget);

  virtual void Destroy() MOZ_OVERRIDE
  {
    LayerManager::Destroy();
    ClearCachedResources();
  }

protected:
  virtual ~ClientLayerManager();

public:
  virtual ShadowLayerForwarder* AsShadowForwarder() MOZ_OVERRIDE
  {
    return mForwarder;
  }

  virtual ClientLayerManager* AsClientLayerManager() MOZ_OVERRIDE
  {
    return this;
  }

  virtual int32_t GetMaxTextureSize() const MOZ_OVERRIDE;

  virtual void SetDefaultTargetConfiguration(BufferMode aDoubleBuffering, ScreenRotation aRotation);
  virtual void BeginTransactionWithTarget(gfxContext* aTarget) MOZ_OVERRIDE;
  virtual void BeginTransaction() MOZ_OVERRIDE;
  virtual bool EndEmptyTransaction(EndTransactionFlags aFlags = END_DEFAULT) MOZ_OVERRIDE;
  virtual void EndTransaction(DrawPaintedLayerCallback aCallback,
                              void* aCallbackData,
                              EndTransactionFlags aFlags = END_DEFAULT) MOZ_OVERRIDE;

  virtual LayersBackend GetBackendType() MOZ_OVERRIDE { return LayersBackend::LAYERS_CLIENT; }
  virtual LayersBackend GetCompositorBackendType() MOZ_OVERRIDE
  {
    return AsShadowForwarder()->GetCompositorBackendType();
  }
  virtual void GetBackendName(nsAString& name) MOZ_OVERRIDE;
  virtual const char* Name() const MOZ_OVERRIDE { return "Client"; }

  virtual void SetRoot(Layer* aLayer) MOZ_OVERRIDE;

  virtual void Mutated(Layer* aLayer) MOZ_OVERRIDE;

  virtual bool IsOptimizedFor(PaintedLayer* aLayer, PaintedLayerCreationHint aHint) MOZ_OVERRIDE;

  virtual already_AddRefed<PaintedLayer> CreatePaintedLayer() MOZ_OVERRIDE;
  virtual already_AddRefed<PaintedLayer> CreatePaintedLayerWithHint(PaintedLayerCreationHint aHint) MOZ_OVERRIDE;
  virtual already_AddRefed<ContainerLayer> CreateContainerLayer() MOZ_OVERRIDE;
  virtual already_AddRefed<ImageLayer> CreateImageLayer() MOZ_OVERRIDE;
  virtual already_AddRefed<CanvasLayer> CreateCanvasLayer() MOZ_OVERRIDE;
  virtual already_AddRefed<ReadbackLayer> CreateReadbackLayer() MOZ_OVERRIDE;
  virtual already_AddRefed<ColorLayer> CreateColorLayer() MOZ_OVERRIDE;
  virtual already_AddRefed<RefLayer> CreateRefLayer() MOZ_OVERRIDE;

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

  virtual bool IsCompositingCheap() MOZ_OVERRIDE;
  virtual bool HasShadowManagerInternal() const MOZ_OVERRIDE { return HasShadowManager(); }

  virtual void SetIsFirstPaint() MOZ_OVERRIDE;

  TextureClientPool* GetTexturePool(gfx::SurfaceFormat aFormat);

  
  void ReturnTextureClientDeferred(TextureClient& aClient);
  void ReturnTextureClient(TextureClient& aClient);
  void ReportClientLost(TextureClient& aClient);

  





  void StorePluginWidgetConfigurations(const nsTArray<nsIWidget::Configuration>&
                                       aConfigurations) MOZ_OVERRIDE;

  
  
  virtual void ClearCachedResources(Layer* aSubtree = nullptr) MOZ_OVERRIDE;

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

  
  virtual bool ShouldAvoidComponentAlphaLayers() MOZ_OVERRIDE { return !IsCompositingCheap(); }

  











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
