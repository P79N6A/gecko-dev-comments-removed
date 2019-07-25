




#ifndef GFX_BASICLAYERS_H
#define GFX_BASICLAYERS_H

#include "Layers.h"

#include "gfxContext.h"
#include "gfxCachedTempSurface.h"
#include "mozilla/layers/ShadowLayers.h"
#include "mozilla/WidgetUtils.h"
#include "nsAutoRef.h"
#include "nsThreadUtils.h"

class nsIWidget;

namespace mozilla {
namespace layers {

class BasicShadowableLayer;
class ShadowThebesLayer;
class ShadowContainerLayer;
class ShadowImageLayer;
class ShadowCanvasLayer;
class ShadowColorLayer;
class ReadbackProcessor;
class ImageFactory;









class THEBES_API BasicLayerManager :
    public ShadowLayerManager
{
public:
  





  BasicLayerManager();
  














  BasicLayerManager(nsIWidget* aWidget);
  virtual ~BasicLayerManager();

  










  void SetDefaultTarget(gfxContext* aContext);
  virtual void SetDefaultTargetConfiguration(BufferMode aDoubleBuffering, ScreenRotation aRotation);
  gfxContext* GetDefaultTarget() { return mDefaultTarget; }

  nsIWidget* GetRetainerWidget() { return mWidget; }
  void ClearRetainerWidget() { mWidget = nullptr; }

  virtual bool IsWidgetLayerManager() { return mWidget != nullptr; }

  virtual void BeginTransaction();
  virtual void BeginTransactionWithTarget(gfxContext* aTarget);
  virtual bool EndEmptyTransaction(EndTransactionFlags aFlags = END_DEFAULT);
  virtual void EndTransaction(DrawThebesLayerCallback aCallback,
                              void* aCallbackData,
                              EndTransactionFlags aFlags = END_DEFAULT);
  virtual bool AreComponentAlphaLayersEnabled() { return HasShadowManager(); }

  void AbortTransaction();

  virtual void SetRoot(Layer* aLayer);

  virtual already_AddRefed<ThebesLayer> CreateThebesLayer();
  virtual already_AddRefed<ContainerLayer> CreateContainerLayer();
  virtual already_AddRefed<ImageLayer> CreateImageLayer();
  virtual already_AddRefed<CanvasLayer> CreateCanvasLayer();
  virtual already_AddRefed<ColorLayer> CreateColorLayer();
  virtual already_AddRefed<ReadbackLayer> CreateReadbackLayer();
  virtual ImageFactory *GetImageFactory();

  virtual already_AddRefed<ShadowThebesLayer> CreateShadowThebesLayer()
  { return nullptr; }
  virtual already_AddRefed<ShadowContainerLayer> CreateShadowContainerLayer()
  { return nullptr; }
  virtual already_AddRefed<ShadowImageLayer> CreateShadowImageLayer()
  { return nullptr; }
  virtual already_AddRefed<ShadowColorLayer> CreateShadowColorLayer()
  { return nullptr; }
  virtual already_AddRefed<ShadowCanvasLayer> CreateShadowCanvasLayer()
  { return nullptr; }
  virtual already_AddRefed<ShadowRefLayer> CreateShadowRefLayer()
  { return nullptr; }

  virtual LayersBackend GetBackendType() { return LAYERS_BASIC; }
  virtual void GetBackendName(nsAString& name) { name.AssignLiteral("Basic"); }

#ifdef DEBUG
  bool InConstruction() { return mPhase == PHASE_CONSTRUCTION; }
  bool InDrawing() { return mPhase == PHASE_DRAWING; }
  bool InForward() { return mPhase == PHASE_FORWARD; }
#endif
  bool InTransaction() { return mPhase != PHASE_NONE; }

  gfxContext* GetTarget() { return mTarget; }
  void SetTarget(gfxContext* aTarget) { mUsingDefaultTarget = false; mTarget = aTarget; }
  bool IsRetained() { return mWidget != nullptr; }

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() const { return "Basic"; }
#endif 

  
  void ClearCachedResources();

  void SetTransactionIncomplete() { mTransactionIncomplete = true; }
  bool IsTransactionIncomplete() { return mTransactionIncomplete; }

  already_AddRefed<gfxContext> PushGroupForLayer(gfxContext* aContext, Layer* aLayer,
                                                 const nsIntRegion& aRegion,
                                                 bool* aNeedsClipToVisibleRegion);
  already_AddRefed<gfxContext> PushGroupWithCachedSurface(gfxContext *aTarget,
                                                          gfxASurface::gfxContentType aContent);
  void PopGroupToSourceWithCachedSurface(gfxContext *aTarget, gfxContext *aPushed);

  virtual bool IsCompositingCheap() { return false; }
  virtual int32_t GetMaxTextureSize() const { return PR_INT32_MAX; }

protected:
  enum TransactionPhase {
    PHASE_NONE, PHASE_CONSTRUCTION, PHASE_DRAWING, PHASE_FORWARD
  };
  TransactionPhase mPhase;

  
  void PaintLayer(gfxContext* aTarget,
                  Layer* aLayer,
                  DrawThebesLayerCallback aCallback,
                  void* aCallbackData,
                  ReadbackProcessor* aReadback);

  
  void ClearLayer(Layer* aLayer);

  bool EndTransactionInternal(DrawThebesLayerCallback aCallback,
                              void* aCallbackData,
                              EndTransactionFlags aFlags = END_DEFAULT);

  void FlashWidgetUpdateArea(gfxContext* aContext);

  
  
  nsIWidget* mWidget;
  
  nsRefPtr<gfxContext> mDefaultTarget;
  
  nsRefPtr<gfxContext> mTarget;
  
  nsRefPtr<gfxContext> mShadowTarget;
  
  nsRefPtr<ImageFactory> mFactory;

  
  gfxCachedTempSurface mCachedSurface;

  BufferMode   mDoubleBuffering;
  bool mUsingDefaultTarget;
  bool mCachedSurfaceInUse;
  bool         mTransactionIncomplete;
};
 

class BasicShadowLayerManager : public BasicLayerManager,
                                public ShadowLayerForwarder
{
  typedef nsTArray<nsRefPtr<Layer> > LayerRefArray;

public:
  BasicShadowLayerManager(nsIWidget* aWidget);
  virtual ~BasicShadowLayerManager();

  virtual ShadowLayerForwarder* AsShadowForwarder()
  {
    return this;
  }
  virtual ShadowLayerManager* AsShadowManager()
  {
    return this;
  }

  virtual int32_t GetMaxTextureSize() const;

  virtual void SetDefaultTargetConfiguration(BufferMode aDoubleBuffering, ScreenRotation aRotation) MOZ_OVERRIDE;
  virtual void BeginTransactionWithTarget(gfxContext* aTarget);
  virtual bool EndEmptyTransaction(EndTransactionFlags aFlags = END_DEFAULT);
  virtual void EndTransaction(DrawThebesLayerCallback aCallback,
                              void* aCallbackData,
                              EndTransactionFlags aFlags = END_DEFAULT);

  virtual void SetRoot(Layer* aLayer);

  virtual void Mutated(Layer* aLayer);

  virtual already_AddRefed<ThebesLayer> CreateThebesLayer();
  virtual already_AddRefed<ContainerLayer> CreateContainerLayer();
  virtual already_AddRefed<ImageLayer> CreateImageLayer();
  virtual already_AddRefed<CanvasLayer> CreateCanvasLayer();
  virtual already_AddRefed<ColorLayer> CreateColorLayer();
  virtual already_AddRefed<RefLayer> CreateRefLayer();
  virtual already_AddRefed<ShadowThebesLayer> CreateShadowThebesLayer();
  virtual already_AddRefed<ShadowContainerLayer> CreateShadowContainerLayer();
  virtual already_AddRefed<ShadowImageLayer> CreateShadowImageLayer();
  virtual already_AddRefed<ShadowColorLayer> CreateShadowColorLayer();
  virtual already_AddRefed<ShadowCanvasLayer> CreateShadowCanvasLayer();
  virtual already_AddRefed<ShadowRefLayer> CreateShadowRefLayer();

  ShadowableLayer* Hold(Layer* aLayer);

  bool HasShadowManager() const { return ShadowLayerForwarder::HasShadowManager(); }

  virtual bool IsCompositingCheap();
  virtual bool HasShadowManagerInternal() const { return HasShadowManager(); }

  virtual void SetIsFirstPaint() MOZ_OVERRIDE;

  void SetRepeatTransaction() { mRepeatTransaction = true; }

private:
  


  void ForwardTransaction();

  
  nsIntRect mTargetBounds;

  LayerRefArray mKeepAlive;

  
  
  
  
  ScreenRotation mTargetRotation;

  
  
  bool mRepeatTransaction;
};

class BasicShadowableThebesLayer;
class BasicShadowableLayer : public ShadowableLayer
{
public:
  BasicShadowableLayer()
  {
    MOZ_COUNT_CTOR(BasicShadowableLayer);
  }

  ~BasicShadowableLayer();

  void SetShadow(PLayerChild* aShadow)
  {
    NS_ABORT_IF_FALSE(!mShadow, "can't have two shadows (yet)");
    mShadow = aShadow;
  }

  virtual void SetBackBuffer(const SurfaceDescriptor& aBuffer)
  {
    NS_RUNTIMEABORT("if this default impl is called, |aBuffer| leaks");
  }
  
  virtual void SetBackBufferYUVImage(const SurfaceDescriptor& aYBuffer,
                                     const SurfaceDescriptor& aUBuffer,
                                     const SurfaceDescriptor& aVBuffer)
  {
    NS_RUNTIMEABORT("if this default impl is called, the buffers leak");
  }

  virtual void Disconnect()
  {
    
    
    
    
    
    mShadow = nullptr;
  }

  virtual BasicShadowableThebesLayer* AsThebes() { return nullptr; }
};


}
}

#endif
