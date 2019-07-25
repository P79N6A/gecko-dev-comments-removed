




































#ifndef GFX_BASICLAYERS_H
#define GFX_BASICLAYERS_H

#include "Layers.h"

#include "gfxContext.h"
#include "gfxCachedTempSurface.h"
#include "nsAutoRef.h"
#include "nsThreadUtils.h"

#include "mozilla/layers/ShadowLayers.h"

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









class THEBES_API BasicLayerManager :
    public ShadowLayerManager
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

  





  void SetResolution(float aXResolution, float aYResolution)
  {
    NS_ASSERTION(InConstruction(), "resolution must be set before drawing");
    mXResolution = aXResolution;
    mYResolution = aYResolution;
  }
  float XResolution() const { return mXResolution; }
  float YResolution() const { return mYResolution; }

  nsIWidget* GetRetainerWidget() { return mWidget; }
  void ClearRetainerWidget() { mWidget = nsnull; }

  virtual void BeginTransaction();
  virtual void BeginTransactionWithTarget(gfxContext* aTarget);
  virtual bool EndEmptyTransaction();
  virtual void EndTransaction(DrawThebesLayerCallback aCallback,
                              void* aCallbackData);

  virtual void SetRoot(Layer* aLayer);

  virtual already_AddRefed<ThebesLayer> CreateThebesLayer();
  virtual already_AddRefed<ContainerLayer> CreateContainerLayer();
  virtual already_AddRefed<ImageLayer> CreateImageLayer();
  virtual already_AddRefed<CanvasLayer> CreateCanvasLayer();
  virtual already_AddRefed<ImageContainer> CreateImageContainer();
  virtual already_AddRefed<ColorLayer> CreateColorLayer();
  virtual already_AddRefed<ReadbackLayer> CreateReadbackLayer();
  virtual already_AddRefed<ShadowThebesLayer> CreateShadowThebesLayer()
  { return nsnull; }
  virtual already_AddRefed<ShadowContainerLayer> CreateShadowContainerLayer()
  { return nsnull; }
  virtual already_AddRefed<ShadowImageLayer> CreateShadowImageLayer()
  { return nsnull; }
  virtual already_AddRefed<ShadowColorLayer> CreateShadowColorLayer()
  { return nsnull; }
  virtual already_AddRefed<ShadowCanvasLayer> CreateShadowCanvasLayer()
  { return nsnull; }

  virtual LayersBackend GetBackendType() { return LAYERS_BASIC; }
  virtual void GetBackendName(nsAString& name) { name.AssignLiteral("Basic"); }

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

  
  void ClearCachedResources();

  void SetTransactionIncomplete() { mTransactionIncomplete = true; }

  virtual PRBool IsCompositingCheap() { return PR_FALSE; }

protected:
#ifdef DEBUG
  enum TransactionPhase {
    PHASE_NONE, PHASE_CONSTRUCTION, PHASE_DRAWING, PHASE_FORWARD
  };
  TransactionPhase mPhase;
#endif

  
  void PaintLayer(Layer* aLayer,
                  DrawThebesLayerCallback aCallback,
                  void* aCallbackData,
                  ReadbackProcessor* aReadback);

  
  void ClearLayer(Layer* aLayer);

  already_AddRefed<gfxContext> PushGroupWithCachedSurface(gfxContext *aTarget,
                                                          gfxASurface::gfxContentType aContent,
                                                          gfxPoint *aSavedOffset);
  void PopGroupWithCachedSurface(gfxContext *aTarget,
                                 const gfxPoint& aSavedOffset);

  bool EndTransactionInternal(DrawThebesLayerCallback aCallback,
                              void* aCallbackData);

  
  float mXResolution;
  float mYResolution;

  
  
  nsIWidget* mWidget;
  
  nsRefPtr<gfxContext> mDefaultTarget;
  
  nsRefPtr<gfxContext> mTarget;

  
  gfxCachedTempSurface mCachedSurface;

  BufferMode   mDoubleBuffering;
  PRPackedBool mUsingDefaultTarget;
  bool         mTransactionIncomplete;
};
 

class BasicShadowLayerManager : public BasicLayerManager,
                                public ShadowLayerForwarder
{
  typedef nsTArray<nsRefPtr<Layer> > LayerRefArray;

public:
  BasicShadowLayerManager(nsIWidget* aWidget);
  virtual ~BasicShadowLayerManager();

  virtual void BeginTransactionWithTarget(gfxContext* aTarget);
  virtual bool EndEmptyTransaction();
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
  virtual already_AddRefed<ShadowContainerLayer> CreateShadowContainerLayer();
  virtual already_AddRefed<ShadowImageLayer> CreateShadowImageLayer();
  virtual already_AddRefed<ShadowColorLayer> CreateShadowColorLayer();
  virtual already_AddRefed<ShadowCanvasLayer> CreateShadowCanvasLayer();

  ShadowableLayer* Hold(Layer* aLayer);

  PLayersChild* GetShadowManager() const { return mShadowManager; }

  void SetShadowManager(PLayersChild* aShadowManager)
  {
    mShadowManager = aShadowManager;
  }

  virtual PRBool IsCompositingCheap();

private:
  


  void ForwardTransaction();

  LayerRefArray mKeepAlive;
};

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
