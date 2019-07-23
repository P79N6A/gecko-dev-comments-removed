




































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

  



  void SetDefaultTarget(gfxContext* aContext);

  virtual void BeginTransaction();
  virtual void BeginTransactionWithTarget(gfxContext* aTarget);
  virtual void EndConstruction();
  virtual void EndTransaction();

  virtual void SetRoot(Layer* aLayer);

  virtual already_AddRefed<ThebesLayer> CreateThebesLayer();
  virtual already_AddRefed<ContainerLayer> CreateContainerLayer();
  virtual already_AddRefed<ImageLayer> CreateImageLayer();
  virtual already_AddRefed<ImageContainer> CreateImageContainer();
  virtual LayersBackend GetBackendType() { return LAYERS_BASIC; }

#ifdef DEBUG
  PRBool InConstruction() { return mPhase == PHASE_CONSTRUCTION; }
  PRBool InDrawing() { return mPhase == PHASE_DRAWING; }
  PRBool IsBeforeInTree(Layer* aBefore, Layer* aLayer);
#endif
  
  
  
  
  
  void AdvancePaintingTo(BasicThebesLayer* aLayer);
  Layer* GetLastPainted() { return mLastPainted; }
  gfxContext* GetTarget() { return mTarget; }

private:
  
  
  
  void BeginPaintingLayer(Layer* aLayer);
  
  
  void EndPaintingLayer();

  nsRefPtr<Layer> mRoot;
  
  nsRefPtr<gfxContext> mDefaultTarget;
  
  
  nsRefPtr<gfxContext> mTarget;
  
  
  
  Layer* mLastPainted;

#ifdef DEBUG
  enum TransactionPhase { PHASE_NONE, PHASE_CONSTRUCTION, PHASE_DRAWING };
  TransactionPhase mPhase;
#endif
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
