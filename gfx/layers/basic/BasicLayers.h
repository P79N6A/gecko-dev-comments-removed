




#ifndef GFX_BASICLAYERS_H
#define GFX_BASICLAYERS_H

#include <stdint.h>                     
#include "Layers.h"                     
#include "gfxASurface.h"                
#include "gfxCachedTempSurface.h"       
#include "gfxContext.h"                 
#include "mozilla/Attributes.h"         
#include "mozilla/WidgetUtils.h"        
#include "mozilla/layers/LayersTypes.h"  
#include "nsAString.h"
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsISupportsImpl.h"            
#include "nsRegion.h"                   
#include "nscore.h"                     

class gfxPattern;
class nsIWidget;

namespace mozilla {
namespace layers {

class BasicShadowableLayer;
class ImageFactory;
class ImageLayer;
class PaintLayerContext;
class ReadbackLayer;
class ReadbackProcessor;









class BasicLayerManager :
    public LayerManager
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
  virtual bool AreComponentAlphaLayersEnabled() { return HasShadowManager() || !IsWidgetLayerManager(); }

  void AbortTransaction();

  virtual void SetRoot(Layer* aLayer);

  virtual already_AddRefed<ThebesLayer> CreateThebesLayer();
  virtual already_AddRefed<ContainerLayer> CreateContainerLayer();
  virtual already_AddRefed<ImageLayer> CreateImageLayer();
  virtual already_AddRefed<CanvasLayer> CreateCanvasLayer();
  virtual already_AddRefed<ColorLayer> CreateColorLayer();
  virtual already_AddRefed<ReadbackLayer> CreateReadbackLayer();
  virtual ImageFactory *GetImageFactory();

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

  
  virtual void ClearCachedResources(Layer* aSubtree = nullptr) MOZ_OVERRIDE;

  void SetTransactionIncomplete() { mTransactionIncomplete = true; }
  bool IsTransactionIncomplete() { return mTransactionIncomplete; }

  already_AddRefed<gfxContext> PushGroupForLayer(gfxContext* aContext, Layer* aLayer,
                                                 const nsIntRegion& aRegion,
                                                 bool* aNeedsClipToVisibleRegion);
  already_AddRefed<gfxContext> PushGroupWithCachedSurface(gfxContext *aTarget,
                                                          gfxContentType aContent);
  void PopGroupToSourceWithCachedSurface(gfxContext *aTarget, gfxContext *aPushed);

  virtual bool IsCompositingCheap() { return false; }
  virtual int32_t GetMaxTextureSize() const { return INT32_MAX; }
  bool CompositorMightResample() { return mCompositorMightResample; }
  bool HasShadowTarget() { return !!mShadowTarget; }

protected:
  enum TransactionPhase {
    PHASE_NONE, PHASE_CONSTRUCTION, PHASE_DRAWING, PHASE_FORWARD
  };
  TransactionPhase mPhase;

  
  
  
  void PaintSelfOrChildren(PaintLayerContext& aPaintContext, gfxContext* aGroupTarget);

  
  
  void FlushGroup(PaintLayerContext& aPaintContext, bool aNeedsClipToVisibleRegion);

  
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

  void RenderDebugOverlay();

  
  
  nsIWidget* mWidget;
  
  nsRefPtr<gfxContext> mDefaultTarget;
  
  nsRefPtr<gfxContext> mTarget;
  
  
  
  
  
  
  
  
  nsRefPtr<gfxContext> mShadowTarget;
  nsRefPtr<gfxContext> mDummyTarget;
  
  nsRefPtr<ImageFactory> mFactory;

  
  gfxCachedTempSurface mCachedSurface;

  BufferMode mDoubleBuffering;
  bool mUsingDefaultTarget;
  bool mCachedSurfaceInUse;
  bool mTransactionIncomplete;
  bool mCompositorMightResample;
};

void
PaintContext(gfxPattern* aPattern,
             const nsIntRegion& aVisible,
             float aOpacity,
             gfxContext* aContext,
             Layer* aMaskLayer);

}
}

#endif
