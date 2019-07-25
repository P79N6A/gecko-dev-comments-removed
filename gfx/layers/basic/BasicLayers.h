




































#ifndef GFX_BASICLAYERS_H
#define GFX_BASICLAYERS_H

#include "Layers.h"

#include "gfxContext.h"
#include "gfxCachedTempSurface.h"
#include "nsAutoRef.h"
#include "nsThreadUtils.h"

#ifdef MOZ_IPC
#include "mozilla/layers/ShadowLayers.h"
#endif

class nsIWidget;

namespace mozilla {
namespace layers {

class BasicShadowableLayer;
class ShadowThebesLayer;
class ShadowImageLayer;
class ShadowCanvasLayer;









class THEBES_API BasicLayerManager :
#ifdef MOZ_IPC
    public ShadowLayerManager
#else
    public LayerManager
#endif
{
public:
  





  BasicLayerManager();
  














  BasicLayerManager(nsIWidget* aWidget);
  virtual ~BasicLayerManager();

  










  enum BufferMode {
    BUFFER_NONE,
    BUFFER_BUFFERED
  };
  void SetDefaultTarget(gfxContext* aContext, BufferMode aDoubleBuffering);
  gfxContext* GetDefaultTarget() { return mDefaultTarget; }

  nsIWidget* GetRetainerWidget() { return mWidget; }
  void ClearRetainerWidget() { mWidget = nsnull; }

  virtual void BeginTransaction();
  virtual void BeginTransactionWithTarget(gfxContext* aTarget);
  virtual void EndTransaction(DrawThebesLayerCallback aCallback,
                              void* aCallbackData);

  virtual void SetRoot(Layer* aLayer);

  virtual already_AddRefed<ThebesLayer> CreateThebesLayer();
  virtual already_AddRefed<ContainerLayer> CreateContainerLayer();
  virtual already_AddRefed<ImageLayer> CreateImageLayer();
  virtual already_AddRefed<CanvasLayer> CreateCanvasLayer();
  virtual already_AddRefed<ImageContainer> CreateImageContainer();
  virtual already_AddRefed<ColorLayer> CreateColorLayer();
  virtual already_AddRefed<ShadowThebesLayer> CreateShadowThebesLayer()
  { return NULL; }
  virtual already_AddRefed<ShadowImageLayer> CreateShadowImageLayer()
  { return NULL; }
  virtual already_AddRefed<ShadowCanvasLayer> CreateShadowCanvasLayer()
  { return NULL; }

  virtual LayersBackend GetBackendType() { return LAYERS_BASIC; }

#ifdef DEBUG
  PRBool InConstruction() { return mPhase == PHASE_CONSTRUCTION; }
  PRBool InDrawing() { return mPhase == PHASE_DRAWING; }
  PRBool InForward() { return mPhase == PHASE_FORWARD; }
  PRBool InTransaction() { return mPhase != PHASE_NONE; }
#endif
  gfxContext* GetTarget() { return mTarget; }
  PRBool IsRetained() { return mWidget != nsnull; }

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() const { return "Basic"; }
#endif 

protected:
#ifdef DEBUG
  enum TransactionPhase {
    PHASE_NONE, PHASE_CONSTRUCTION, PHASE_DRAWING, PHASE_FORWARD
  };
  TransactionPhase mPhase;
#endif

private:
  
  void PaintLayer(Layer* aLayer,
                  DrawThebesLayerCallback aCallback,
                  void* aCallbackData,
                  float aOpacity);

  already_AddRefed<gfxContext> PushGroupWithCachedSurface(gfxContext *aTarget,
                                                          gfxASurface::gfxContentType aContent,
                                                          gfxPoint *aSavedOffset);
  void PopGroupWithCachedSurface(gfxContext *aTarget,
                                 const gfxPoint& aSavedOffset);

  
  
  nsIWidget* mWidget;
  
  nsRefPtr<gfxContext> mDefaultTarget;
  
  nsRefPtr<gfxContext> mTarget;

  
  gfxCachedTempSurface mCachedSurface;

  BufferMode   mDoubleBuffering;
  PRPackedBool mUsingDefaultTarget;
};
 

#ifdef MOZ_IPC
class BasicShadowLayerManager : public BasicLayerManager,
                                public ShadowLayerForwarder
{
  typedef nsTArray<nsRefPtr<Layer> > LayerRefArray;

public:
  BasicShadowLayerManager(nsIWidget* aWidget);
  virtual ~BasicShadowLayerManager();

  virtual void BeginTransactionWithTarget(gfxContext* aTarget);
  virtual void EndTransaction(DrawThebesLayerCallback aCallback,
                              void* aCallbackData);

  virtual void SetRoot(Layer* aLayer);

  virtual void Mutated(Layer* aLayer);

  virtual already_AddRefed<ThebesLayer> CreateThebesLayer();
  virtual already_AddRefed<ContainerLayer> CreateContainerLayer();
  virtual already_AddRefed<ImageLayer> CreateImageLayer();
  virtual already_AddRefed<CanvasLayer> CreateCanvasLayer();
  virtual already_AddRefed<ColorLayer> CreateColorLayer();
  virtual already_AddRefed<ShadowThebesLayer> CreateShadowThebesLayer();
  virtual already_AddRefed<ShadowImageLayer> CreateShadowImageLayer();
  virtual already_AddRefed<ShadowCanvasLayer> CreateShadowCanvasLayer();

  virtual const char* Name() const { return "BasicShadowLayerManager"; }

  ShadowableLayer* Hold(Layer* aLayer);

private:
  LayerRefArray mKeepAlive;
};
#endif  

}
}













class nsMainThreadSurfaceRef;

NS_SPECIALIZE_TEMPLATE
class nsAutoRefTraits<nsMainThreadSurfaceRef> {
public:
  typedef gfxASurface* RawRef;

  


  class SurfaceReleaser : public nsRunnable {
  public:
    SurfaceReleaser(RawRef aRef) : mRef(aRef) {}
    NS_IMETHOD Run() {
      mRef->Release();
      return NS_OK;
    }
    RawRef mRef;
  };

  static RawRef Void() { return nsnull; }
  static void Release(RawRef aRawRef)
  {
    if (NS_IsMainThread()) {
      aRawRef->Release();
      return;
    }
    nsCOMPtr<nsIRunnable> runnable = new SurfaceReleaser(aRawRef);
    NS_DispatchToMainThread(runnable);
  }
  static void AddRef(RawRef aRawRef)
  {
    NS_ASSERTION(NS_IsMainThread(),
                 "Can only add a reference on the main thread");
    aRawRef->AddRef();
  }
};

#endif 
