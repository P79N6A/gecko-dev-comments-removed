




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









class BasicLayerManager MOZ_FINAL :
    public LayerManager
{
public:
  enum BasicLayerManagerType {
    BLM_WIDGET,
    BLM_OFFSCREEN,
    BLM_INACTIVE
  };
  





  explicit BasicLayerManager(BasicLayerManagerType aType);
  














  explicit BasicLayerManager(nsIWidget* aWidget);

protected:
  virtual ~BasicLayerManager();

public:
  










  void SetDefaultTarget(gfxContext* aContext);
  virtual void SetDefaultTargetConfiguration(BufferMode aDoubleBuffering, ScreenRotation aRotation);
  gfxContext* GetDefaultTarget() { return mDefaultTarget; }

  nsIWidget* GetRetainerWidget() { return mWidget; }
  void ClearRetainerWidget() { mWidget = nullptr; }

  virtual bool IsWidgetLayerManager() MOZ_OVERRIDE { return mWidget != nullptr; }
  virtual bool IsInactiveLayerManager() MOZ_OVERRIDE { return mType == BLM_INACTIVE; }

  virtual void BeginTransaction() MOZ_OVERRIDE;
  virtual void BeginTransactionWithTarget(gfxContext* aTarget) MOZ_OVERRIDE;
  virtual bool EndEmptyTransaction(EndTransactionFlags aFlags = END_DEFAULT) MOZ_OVERRIDE;
  virtual void EndTransaction(DrawPaintedLayerCallback aCallback,
                              void* aCallbackData,
                              EndTransactionFlags aFlags = END_DEFAULT) MOZ_OVERRIDE;
  virtual bool ShouldAvoidComponentAlphaLayers() MOZ_OVERRIDE { return IsWidgetLayerManager(); }

  void AbortTransaction();

  virtual void SetRoot(Layer* aLayer) MOZ_OVERRIDE;

  virtual already_AddRefed<PaintedLayer> CreatePaintedLayer() MOZ_OVERRIDE;
  virtual already_AddRefed<ContainerLayer> CreateContainerLayer() MOZ_OVERRIDE;
  virtual already_AddRefed<ImageLayer> CreateImageLayer() MOZ_OVERRIDE;
  virtual already_AddRefed<CanvasLayer> CreateCanvasLayer() MOZ_OVERRIDE;
  virtual already_AddRefed<ColorLayer> CreateColorLayer() MOZ_OVERRIDE;
  virtual already_AddRefed<ReadbackLayer> CreateReadbackLayer() MOZ_OVERRIDE;
  virtual ImageFactory *GetImageFactory();

  virtual LayersBackend GetBackendType() MOZ_OVERRIDE { return LayersBackend::LAYERS_BASIC; }
  virtual void GetBackendName(nsAString& name) MOZ_OVERRIDE { name.AssignLiteral("Basic"); }

  bool InConstruction() { return mPhase == PHASE_CONSTRUCTION; }
#ifdef DEBUG
  bool InDrawing() { return mPhase == PHASE_DRAWING; }
  bool InForward() { return mPhase == PHASE_FORWARD; }
#endif
  bool InTransaction() { return mPhase != PHASE_NONE; }

  gfxContext* GetTarget() { return mTarget; }
  void SetTarget(gfxContext* aTarget) { mUsingDefaultTarget = false; mTarget = aTarget; }
  bool IsRetained() { return mWidget != nullptr; }

  virtual const char* Name() const MOZ_OVERRIDE { return "Basic"; }

  
  virtual void ClearCachedResources(Layer* aSubtree = nullptr) MOZ_OVERRIDE;

  void SetTransactionIncomplete() { mTransactionIncomplete = true; }
  bool IsTransactionIncomplete() { return mTransactionIncomplete; }

  already_AddRefed<gfxContext> PushGroupForLayer(gfxContext* aContext, Layer* aLayer,
                                                 const nsIntRegion& aRegion,
                                                 bool* aNeedsClipToVisibleRegion);

  virtual bool IsCompositingCheap() MOZ_OVERRIDE { return false; }
  virtual int32_t GetMaxTextureSize() const MOZ_OVERRIDE { return INT32_MAX; }
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
                  DrawPaintedLayerCallback aCallback,
                  void* aCallbackData);

  
  void ClearLayer(Layer* aLayer);

  bool EndTransactionInternal(DrawPaintedLayerCallback aCallback,
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
