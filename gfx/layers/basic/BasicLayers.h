




#ifndef GFX_BASICLAYERS_H
#define GFX_BASICLAYERS_H

#include <stdint.h>                     
#include "Layers.h"                     
#include "gfxTypes.h"
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
  enum BasicLayerManagerType {
    BLM_WIDGET,
    BLM_OFFSCREEN,
    BLM_INACTIVE
  };
  





  BasicLayerManager(BasicLayerManagerType aType);
  














  BasicLayerManager(nsIWidget* aWidget);

protected:
  virtual ~BasicLayerManager();

public:
  










  void SetDefaultTarget(gfxContext* aContext);
  virtual void SetDefaultTargetConfiguration(BufferMode aDoubleBuffering, ScreenRotation aRotation);
  gfxContext* GetDefaultTarget() { return mDefaultTarget; }

  nsIWidget* GetRetainerWidget() { return mWidget; }
  void ClearRetainerWidget() { mWidget = nullptr; }

  virtual bool IsWidgetLayerManager() { return mWidget != nullptr; }
  virtual bool IsInactiveLayerManager() { return mType == BLM_INACTIVE; }

  virtual void BeginTransaction();
  virtual void BeginTransactionWithTarget(gfxContext* aTarget);
  virtual bool EndEmptyTransaction(EndTransactionFlags aFlags = END_DEFAULT);
  virtual void EndTransaction(DrawThebesLayerCallback aCallback,
                              void* aCallbackData,
                              EndTransactionFlags aFlags = END_DEFAULT);
  virtual bool AreComponentAlphaLayersEnabled() { return !IsWidgetLayerManager(); }

  void AbortTransaction();

  virtual void SetRoot(Layer* aLayer);

  virtual already_AddRefed<ThebesLayer> CreateThebesLayer();
  virtual already_AddRefed<ContainerLayer> CreateContainerLayer();
  virtual already_AddRefed<ImageLayer> CreateImageLayer();
  virtual already_AddRefed<CanvasLayer> CreateCanvasLayer();
  virtual already_AddRefed<ColorLayer> CreateColorLayer();
  virtual already_AddRefed<ReadbackLayer> CreateReadbackLayer();
  virtual ImageFactory *GetImageFactory();

  virtual LayersBackend GetBackendType() { return LayersBackend::LAYERS_BASIC; }
  virtual void GetBackendName(nsAString& name) { name.AssignLiteral("Basic"); }

  bool InConstruction() { return mPhase == PHASE_CONSTRUCTION; }
#ifdef DEBUG
  bool InDrawing() { return mPhase == PHASE_DRAWING; }
  bool InForward() { return mPhase == PHASE_FORWARD; }
#endif
  bool InTransaction() { return mPhase != PHASE_NONE; }

  gfxContext* GetTarget() { return mTarget; }
  void SetTarget(gfxContext* aTarget) { mUsingDefaultTarget = false; mTarget = aTarget; }
  bool IsRetained() { return mWidget != nullptr; }

  virtual const char* Name() const { return "Basic"; }

  
  virtual void ClearCachedResources(Layer* aSubtree = nullptr) MOZ_OVERRIDE;

  void SetTransactionIncomplete() { mTransactionIncomplete = true; }
  bool IsTransactionIncomplete() { return mTransactionIncomplete; }

  already_AddRefed<gfxContext> PushGroupForLayer(gfxContext* aContext, Layer* aLayer,
                                                 const nsIntRegion& aRegion,
                                                 bool* aNeedsClipToVisibleRegion);

  virtual bool IsCompositingCheap() { return false; }
  virtual int32_t GetMaxTextureSize() const { return INT32_MAX; }
  bool CompositorMightResample() { return mCompositorMightResample; }

  virtual bool SupportsMixBlendModes(EnumSet<gfx::CompositionOp>& aMixBlendModes) MOZ_OVERRIDE { return true; }

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
                  void* aCallbackData);

  
  void ClearLayer(Layer* aLayer);

  bool EndTransactionInternal(DrawThebesLayerCallback aCallback,
                              void* aCallbackData,
                              EndTransactionFlags aFlags = END_DEFAULT);

  void FlashWidgetUpdateArea(gfxContext* aContext);

  
  
  nsIWidget* mWidget;
  
  nsRefPtr<gfxContext> mDefaultTarget;
  
  nsRefPtr<gfxContext> mTarget;
  
  nsRefPtr<ImageFactory> mFactory;

  BufferMode mDoubleBuffering;
  BasicLayerManagerType mType;
  bool mUsingDefaultTarget;
  bool mTransactionIncomplete;
  bool mCompositorMightResample;
};

}
}

#endif
