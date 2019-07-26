




#ifndef GFX_CLIENTLAYERMANAGER_H
#define GFX_CLIENTLAYERMANAGER_H

#include <stdint.h>                     
#include "Layers.h"
#include "gfxContext.h"                 
#include "mozilla/Attributes.h"         
#include "mozilla/WidgetUtils.h"        
#include "mozilla/gfx/Rect.h"           
#include "mozilla/layers/CompositorTypes.h"
#include "mozilla/layers/LayersTypes.h"  
#include "mozilla/layers/ShadowLayers.h"  
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsRect.h"                     
#include "nsTArray.h"                   
#include "nsTraceRefcnt.h"              
#include "nscore.h"                     

class nsIWidget;

namespace mozilla {
namespace layers {

class ClientThebesLayer;
class CompositorChild;
class ImageLayer;
class PLayerChild;

class ClientLayerManager : public LayerManager,
                           public ShadowLayerForwarder
{
  typedef nsTArray<nsRefPtr<Layer> > LayerRefArray;

public:
  ClientLayerManager(nsIWidget* aWidget);
  virtual ~ClientLayerManager();

  virtual ShadowLayerForwarder* AsShadowForwarder()
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

  virtual LayersBackend GetBackendType() { return LAYERS_CLIENT; }
  virtual void GetBackendName(nsAString& name);
#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() const { return "Client"; }
#endif 

  virtual void SetRoot(Layer* aLayer);

  virtual void Mutated(Layer* aLayer);

  virtual already_AddRefed<ThebesLayer> CreateThebesLayer();
  virtual already_AddRefed<ContainerLayer> CreateContainerLayer();
  virtual already_AddRefed<ImageLayer> CreateImageLayer();
  virtual already_AddRefed<CanvasLayer> CreateCanvasLayer();
  virtual already_AddRefed<ColorLayer> CreateColorLayer();
  virtual already_AddRefed<RefLayer> CreateRefLayer();

  virtual TextureFactoryIdentifier GetTextureFactoryIdentifier() MOZ_OVERRIDE
  {
    return mTextureFactoryIdentifier;
  }

  virtual void FlushRendering() MOZ_OVERRIDE;

  virtual bool NeedsWidgetInvalidation() MOZ_OVERRIDE { return false; }

  ShadowableLayer* Hold(Layer* aLayer);

  bool HasShadowManager() const { return ShadowLayerForwarder::HasShadowManager(); }

  virtual bool IsCompositingCheap();
  virtual bool HasShadowManagerInternal() const { return HasShadowManager(); }

  virtual void SetIsFirstPaint() MOZ_OVERRIDE;

  
  
  virtual void ClearCachedResources(Layer* aSubtree = nullptr) MOZ_OVERRIDE;

  void SetRepeatTransaction() { mRepeatTransaction = true; }
  bool GetRepeatTransaction() { return mRepeatTransaction; }

  bool IsRepeatTransaction() { return mIsRepeatTransaction; }

  void SetTransactionIncomplete() { mTransactionIncomplete = true; }

  bool HasShadowTarget() { return !!mShadowTarget; }

  void SetShadowTarget(gfxContext *aTarget) { mShadowTarget = aTarget; }

  bool CompositorMightResample() { return mCompositorMightResample; } 
  
  DrawThebesLayerCallback GetThebesLayerCallback() const
  { return mThebesLayerCallback; }

  void* GetThebesLayerCallbackData() const
  { return mThebesLayerCallbackData; }

  CompositorChild *GetRemoteRenderer();

  










  bool ProgressiveUpdateCallback(bool aHasPendingNewThebesContent,
                                 gfx::Rect& aViewport,
                                 float& aScaleX,
                                 float& aScaleY,
                                 bool aDrawingCritical);

#ifdef DEBUG
  bool InConstruction() { return mPhase == PHASE_CONSTRUCTION; }
  bool InDrawing() { return mPhase == PHASE_DRAWING; }
  bool InForward() { return mPhase == PHASE_FORWARD; }
#endif
  bool InTransaction() { return mPhase != PHASE_NONE; }

  void SetNeedsComposite(bool aNeedsComposite)
  {
    mNeedsComposite = aNeedsComposite;
  }
  bool NeedsComposite() const { return mNeedsComposite; }

protected:
  enum TransactionPhase {
    PHASE_NONE, PHASE_CONSTRUCTION, PHASE_DRAWING, PHASE_FORWARD
  };
  TransactionPhase mPhase;

private:
  


  void ForwardTransaction();

  



  void MakeSnapshotIfRequired();

  void ClearLayer(Layer* aLayer);

  bool EndTransactionInternal(DrawThebesLayerCallback aCallback,
                              void* aCallbackData,
                              EndTransactionFlags);

  
  nsIntRect mTargetBounds;

  LayerRefArray mKeepAlive;

  nsIWidget* mWidget;
  
  

  DrawThebesLayerCallback mThebesLayerCallback;
  void *mThebesLayerCallbackData;

  
  
  
  
  
  
  
  
  nsRefPtr<gfxContext> mShadowTarget;

  
  
  
  
  ScreenRotation mTargetRotation;

  
  
  bool mRepeatTransaction;
  bool mIsRepeatTransaction;
  bool mTransactionIncomplete;
  bool mCompositorMightResample;
  bool mNeedsComposite;
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
  PLayerChild* shadow = aMgr->ConstructShadowFor(aLayer);
  
  NS_ABORT_IF_FALSE(shadow, "failed to create shadow");

  aLayer->SetShadow(shadow);
  (aMgr->*aMethod)(aLayer);
  aMgr->Hold(aLayer->AsLayer());
}

#define CREATE_SHADOW(_type)                                       \
  CreateShadowFor(layer, this,                                     \
                  &ShadowLayerForwarder::Created ## _type ## Layer)


}
}

#endif
