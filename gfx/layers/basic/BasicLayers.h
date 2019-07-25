




































#ifndef GFX_BASICLAYERS_H
#define GFX_BASICLAYERS_H

#include "Layers.h"

#include "gfxContext.h"
#include "nsAutoRef.h"
#include "nsThreadUtils.h"

namespace mozilla {
namespace layers {

class BasicThebesLayer;









class THEBES_API BasicLayerManager : public LayerManager {
public:
  




  BasicLayerManager(gfxContext* aContext);
  virtual ~BasicLayerManager();

  




  void SetRetain(PRBool aRetain);

  










  enum BufferMode {
    BUFFER_NONE,
    BUFFER_BUFFERED
  };
  void SetDefaultTarget(gfxContext* aContext, BufferMode aDoubleBuffering);

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
  virtual LayersBackend GetBackendType() { return LAYERS_BASIC; }

#ifdef DEBUG
  PRBool InConstruction() { return mPhase == PHASE_CONSTRUCTION; }
  PRBool InDrawing() { return mPhase == PHASE_DRAWING; }
  PRBool InTransaction() { return mPhase != PHASE_NONE; }
#endif
  gfxContext* GetTarget() { return mTarget; }
  PRBool IsRetained() { return mRetain; }

private:
  
  void PaintLayer(Layer* aLayer,
                  DrawThebesLayerCallback aCallback,
                  void* aCallbackData);

  
  nsRefPtr<gfxContext> mDefaultTarget;
  
  nsRefPtr<gfxContext> mTarget;

#ifdef DEBUG
  enum TransactionPhase { PHASE_NONE, PHASE_CONSTRUCTION, PHASE_DRAWING };
  TransactionPhase mPhase;
#endif

  BufferMode   mDoubleBuffering;
  PRPackedBool mUsingDefaultTarget;
  PRPackedBool mRetain;
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
