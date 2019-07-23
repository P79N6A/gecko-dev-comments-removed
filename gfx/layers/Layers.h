




































#ifndef GFX_LAYERS_H
#define GFX_LAYERS_H

#include "gfxTypes.h"
#include "nsRegion.h"
#include "nsPoint.h"
#include "nsRect.h"
#include "gfx3DMatrix.h"

class gfxContext;
class nsPaintEvent;

namespace mozilla {
namespace layers {

class Layer;
class ThebesLayer;
class ContainerLayer;


















































class THEBES_API LayerManager {
  THEBES_INLINE_DECL_REFCOUNTING(LayerManager)  

public:
  virtual ~LayerManager() {}

  





  virtual void BeginTransaction() = 0;
  






  virtual void BeginTransactionWithTarget(gfxContext* aTarget) = 0;
  



  virtual void EndConstruction() = 0;
  


  virtual void EndTransaction() = 0;

  



  virtual void SetRoot(Layer* aLayer) = 0;

  



  virtual already_AddRefed<ThebesLayer> CreateThebesLayer() = 0;
  



  virtual already_AddRefed<ContainerLayer> CreateContainerLayer() = 0;
};





class THEBES_API Layer {
  THEBES_INLINE_DECL_REFCOUNTING(Layer)  

public:
  virtual ~Layer() {}

  


  LayerManager* Manager() { return mManager; }

  







  void SetIsOpaqueContent(PRBool aOpaque) { mIsOpaqueContent = aOpaque; }
  






  virtual void SetVisibleRegion(const nsIntRegion& aRegion) {}

  




  void SetOpacity(float aOpacity) { mOpacity = aOpacity; }

  









  void SetClipRect(const nsIntRect* aRect)
  {
    mUseClipRect = aRect != nsnull;
    if (aRect) {
      mClipRect = *aRect;
    }
  }
  









  void IntersectClipRect(const nsIntRect& aRect)
  {
    if (mUseClipRect) {
      mClipRect.IntersectRect(mClipRect, aRect);
    } else {
      mUseClipRect = PR_TRUE;
      mClipRect = aRect;
    }
  }

  






  void SetTransform(const gfx3DMatrix& aMatrix) { mTransform = aMatrix; }

  
  float GetOpacity() { return mOpacity; }
  const nsIntRect* GetClipRect() { return mUseClipRect ? &mClipRect : nsnull; }
  PRBool IsOpaqueContent() { return mIsOpaqueContent; }
  ContainerLayer* GetParent() { return mParent; }
  Layer* GetNextSibling() { return mNextSibling; }
  Layer* GetPrevSibling() { return mPrevSibling; }
  virtual Layer* GetFirstChild() { return nsnull; }
  const gfx3DMatrix& GetTransform() { return mTransform; }

  




  void* ImplData() { return mImplData; }

  


  void SetParent(ContainerLayer* aParent) { mParent = aParent; }
  void SetNextSibling(Layer* aSibling) { mNextSibling = aSibling; }
  void SetPrevSibling(Layer* aSibling) { mPrevSibling = aSibling; }

protected:
  Layer(LayerManager* aManager, void* aImplData) :
    mManager(aManager),
    mParent(nsnull),
    mNextSibling(nsnull),
    mPrevSibling(nsnull),
    mImplData(aImplData),
    mOpacity(1.0),
    mUseClipRect(PR_FALSE),
    mIsOpaqueContent(PR_FALSE)
    {}

  LayerManager* mManager;
  ContainerLayer* mParent;
  Layer* mNextSibling;
  Layer* mPrevSibling;
  void* mImplData;
  gfx3DMatrix mTransform;
  float mOpacity;
  nsIntRect mClipRect;
  PRPackedBool mUseClipRect;
  PRPackedBool mIsOpaqueContent;
};












class THEBES_API ThebesLayer : public Layer {
public:
  





  virtual void InvalidateRegion(const nsIntRegion& aRegion) = 0;

  




















  virtual gfxContext* BeginDrawing(nsIntRegion* aRegionToDraw) = 0;
  






  virtual void EndDrawing() = 0;

  









  virtual void CopyFrom(ThebesLayer* aSource,
                        const nsIntRegion& aRegion,
                        const nsIntPoint& aDelta) = 0;

protected:
  ThebesLayer(LayerManager* aManager, void* aImplData)
    : Layer(aManager, aImplData) {}
};





class THEBES_API ContainerLayer : public Layer {
public:
  






  virtual void InsertAfter(Layer* aChild, Layer* aAfter) = 0;
  




  virtual void RemoveChild(Layer* aChild) = 0;

  
  virtual Layer* GetFirstChild() { return mFirstChild; }

protected:
  ContainerLayer(LayerManager* aManager, void* aImplData)
    : Layer(aManager, aImplData),
      mFirstChild(nsnull)
  {}

  Layer* mFirstChild;
};

}
}

#endif 
