




































#ifndef GFX_LAYERS_H
#define GFX_LAYERS_H

#include "gfxTypes.h"
#include "gfxASurface.h"
#include "nsRegion.h"
#include "nsPoint.h"
#include "nsRect.h"
#include "nsISupportsImpl.h"
#include "nsAutoPtr.h"
#include "gfx3DMatrix.h"
#include "gfxColor.h"
#include "gfxPattern.h"

class gfxContext;
class nsPaintEvent;

namespace mozilla {
namespace gl {
class GLContext;
}

namespace layers {

class Layer;
class ThebesLayer;
class ContainerLayer;
class ImageLayer;
class ColorLayer;
class ImageContainer;
class CanvasLayer;

#if defined(DEBUG) || defined(PR_LOGGING)
#define NS_LAYER_DECL_NAME(n, e) \
  virtual const char* Name() { return n; } \
  virtual LayerType GetType() { return e; }
#else
#define NS_LAYER_DECL_NAME(n, e) \
  virtual LayerType GetType() { return e; }
#endif


















































class THEBES_API LayerManager {
  NS_INLINE_DECL_REFCOUNTING(LayerManager)

public:
  enum LayersBackend {
    LAYERS_BASIC = 0,
    LAYERS_OPENGL,
    LAYERS_D3D9
  };

  LayerManager() : mUserData(nsnull) {}
  virtual ~LayerManager() {}

  





  virtual void BeginTransaction() = 0;
  






  virtual void BeginTransactionWithTarget(gfxContext* aTarget) = 0;
  

















  typedef void (* DrawThebesLayerCallback)(ThebesLayer* aLayer,
                                           gfxContext* aContext,
                                           const nsIntRegion& aRegionToDraw,
                                           const nsIntRegion& aRegionToInvalidate,
                                           void* aCallbackData);
  






  virtual void EndTransaction(DrawThebesLayerCallback aCallback,
                              void* aCallbackData) = 0;

  



  virtual void SetRoot(Layer* aLayer) = 0;
  


  Layer* GetRoot() { return mRoot; }

  



  virtual already_AddRefed<ThebesLayer> CreateThebesLayer() = 0;
  



  virtual already_AddRefed<ContainerLayer> CreateContainerLayer() = 0;
  



  virtual already_AddRefed<ImageLayer> CreateImageLayer() = 0;
  



  virtual already_AddRefed<ColorLayer> CreateColorLayer() = 0;
  



  virtual already_AddRefed<CanvasLayer> CreateCanvasLayer() = 0;

  


  virtual already_AddRefed<ImageContainer> CreateImageContainer() = 0;

  




  virtual LayersBackend GetBackendType() = 0;

  
  
  void SetUserData(void* aData) { mUserData = aData; }
  void* GetUserData() { return mUserData; }

protected:
  nsRefPtr<Layer> mRoot;
  void* mUserData;
};

class ThebesLayer;





class THEBES_API Layer {
  NS_INLINE_DECL_REFCOUNTING(Layer)  

public:
  enum LayerType {
    TYPE_THEBES,
    TYPE_CONTAINER,
    TYPE_IMAGE,
    TYPE_COLOR,
    TYPE_CANVAS
  };

  virtual ~Layer() {}

  


  LayerManager* Manager() { return mManager; }

  







  void SetIsOpaqueContent(PRBool aOpaque) { mIsOpaqueContent = aOpaque; }
  






  virtual void SetVisibleRegion(const nsIntRegion& aRegion) { mVisibleRegion = aRegion; }

  




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
  const nsIntRegion& GetVisibleRegion() { return mVisibleRegion; }
  ContainerLayer* GetParent() { return mParent; }
  Layer* GetNextSibling() { return mNextSibling; }
  Layer* GetPrevSibling() { return mPrevSibling; }
  virtual Layer* GetFirstChild() { return nsnull; }
  const gfx3DMatrix& GetTransform() { return mTransform; }

  
  
  void SetUserData(void* aData) { mUserData = aData; }
  void* GetUserData() { return mUserData; }

  



  virtual ThebesLayer* AsThebesLayer() { return nsnull; }

#ifdef DEBUG
  virtual const char* Name() = 0;
#endif
  virtual LayerType GetType() = 0;

  




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
    mUserData(nsnull),
    mOpacity(1.0),
    mUseClipRect(PR_FALSE),
    mIsOpaqueContent(PR_FALSE)
    {}

  LayerManager* mManager;
  ContainerLayer* mParent;
  Layer* mNextSibling;
  Layer* mPrevSibling;
  void* mImplData;
  void* mUserData;
  nsIntRegion mVisibleRegion;
  gfx3DMatrix mTransform;
  float mOpacity;
  nsIntRect mClipRect;
  PRPackedBool mUseClipRect;
  PRPackedBool mIsOpaqueContent;
};












class THEBES_API ThebesLayer : public Layer {
public:
  





  virtual void InvalidateRegion(const nsIntRegion& aRegion) = 0;

  


  const nsIntRegion& GetValidRegion() { return mValidRegion; }

  virtual ThebesLayer* AsThebesLayer() { return this; }

  NS_LAYER_DECL_NAME("ThebesLayer", TYPE_THEBES)

protected:
  ThebesLayer(LayerManager* aManager, void* aImplData)
    : Layer(aManager, aImplData) {}

  nsIntRegion mValidRegion;
};





class THEBES_API ContainerLayer : public Layer {
public:
  






  virtual void InsertAfter(Layer* aChild, Layer* aAfter) = 0;
  




  virtual void RemoveChild(Layer* aChild) = 0;

  
  virtual Layer* GetFirstChild() { return mFirstChild; }

  NS_LAYER_DECL_NAME("ContainerLayer", TYPE_CONTAINER)

protected:
  ContainerLayer(LayerManager* aManager, void* aImplData)
    : Layer(aManager, aImplData),
      mFirstChild(nsnull)
  {}

  Layer* mFirstChild;
};






class THEBES_API ColorLayer : public Layer {
public:
  



  virtual void SetColor(const gfxRGBA& aColor)
  {
    mColor = aColor;
  }

  
  virtual const gfxRGBA& GetColor() { return mColor; }

  NS_LAYER_DECL_NAME("ColorLayer", TYPE_COLOR)

protected:
  ColorLayer(LayerManager* aManager, void* aImplData)
    : Layer(aManager, aImplData),
      mColor(0.0, 0.0, 0.0, 0.0)
  {}

  gfxRGBA mColor;
};











class THEBES_API CanvasLayer : public Layer {
public:
  struct Data {
    Data()
      : mSurface(nsnull), mGLContext(nsnull),
        mGLBufferIsPremultiplied(PR_FALSE)
    { }

    
    gfxASurface* mSurface;  
    mozilla::gl::GLContext* mGLContext; 

    
    nsIntSize mSize;

    


    PRPackedBool mGLBufferIsPremultiplied;
  };

  







  virtual void Initialize(const Data& aData) = 0;

  






  virtual void Updated(const nsIntRect& aRect) = 0;

  



  void SetFilter(gfxPattern::GraphicsFilter aFilter) { mFilter = aFilter; }
  gfxPattern::GraphicsFilter GetFilter() const { return mFilter; }

  NS_LAYER_DECL_NAME("CanvasLayer", TYPE_CANVAS)

protected:
  CanvasLayer(LayerManager* aManager, void* aImplData)
    : Layer(aManager, aImplData), mFilter(gfxPattern::FILTER_GOOD) {}

  gfxPattern::GraphicsFilter mFilter;
};

}
}

#endif
