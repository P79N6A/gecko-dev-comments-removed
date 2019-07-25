




































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

#if defined(DEBUG) || defined(PR_LOGGING)
#  include <stdio.h>            
#  include "prlog.h"
#  define MOZ_LAYERS_HAVE_LOG
#  define MOZ_LAYERS_LOG(_args)                             \
  PR_LOG(LayerManager::GetLog(), PR_LOG_DEBUG, _args)
#else
struct PRLogModuleInfo;
#  define MOZ_LAYERS_LOG(_args)
#endif  

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
class SpecificLayerAttributes;







struct FrameMetrics {
  FrameMetrics()
    : mViewportSize(0, 0)
    , mViewportScrollOffset(0, 0)
  {}

  

  PRBool operator==(const FrameMetrics& aOther) const
  {
    return (mViewportSize == aOther.mViewportSize &&
            mViewportScrollOffset == aOther.mViewportScrollOffset &&
            mDisplayPort == aOther.mDisplayPort);
  }

  PRBool IsDefault() const
  {
    return (FrameMetrics() == *this);
  }

  nsIntSize mViewportSize;
  nsIntPoint mViewportScrollOffset;
  nsIntRect mDisplayPort;
};

#define MOZ_LAYER_DECL_NAME(n, e)                           \
  virtual const char* Name() const { return n; }            \
  virtual LayerType GetType() const { return e; }




class THEBES_API LayerUserData {
public:
  virtual ~LayerUserData() {}
};






























class THEBES_API LayerUserDataSet {
public:
  LayerUserDataSet() : mKey(nsnull) {}

  void Set(void* aKey, LayerUserData* aValue)
  {
    NS_ASSERTION(!mKey || mKey == aKey,
                 "Multiple LayerUserData objects not supported");
    mKey = aKey;
    mValue = aValue;
  }
  


  LayerUserData* Remove(void* aKey)
  {
    if (mKey == aKey) {
      mKey = nsnull;
      LayerUserData* d = mValue.forget();
      return d;
    }
    return nsnull;
  }
  


  PRBool Has(void* aKey)
  {
    return mKey == aKey;
  }
  


  LayerUserData* Get(void* aKey)
  {
    return mKey == aKey ? mValue.get() : nsnull;
  }

private:
  void* mKey;
  nsAutoPtr<LayerUserData> mValue;
};
























class THEBES_API LayerManager {
  NS_INLINE_DECL_REFCOUNTING(LayerManager)

public:
  enum LayersBackend {
    LAYERS_BASIC = 0,
    LAYERS_OPENGL,
    LAYERS_D3D9
  };

  LayerManager() : mDestroyed(PR_FALSE)
  {
    InitLog();
  }
  virtual ~LayerManager() {}

  





  virtual void Destroy() { mDestroyed = PR_TRUE; }
  PRBool IsDestroyed() { return mDestroyed; }

  





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

  



  virtual void Mutated(Layer* aLayer) { }

  



  virtual already_AddRefed<ThebesLayer> CreateThebesLayer() = 0;
  



  virtual already_AddRefed<ContainerLayer> CreateContainerLayer() = 0;
  



  virtual already_AddRefed<ImageLayer> CreateImageLayer() = 0;
  



  virtual already_AddRefed<ColorLayer> CreateColorLayer() = 0;
  



  virtual already_AddRefed<CanvasLayer> CreateCanvasLayer() = 0;

  


  virtual already_AddRefed<ImageContainer> CreateImageContainer() = 0;

  




  virtual LayersBackend GetBackendType() = 0;
 
  



  virtual already_AddRefed<gfxASurface>
    CreateOptimalSurface(const gfxIntSize &aSize,
                         gfxASurface::gfxImageFormat imageFormat);

  


  virtual void GetBackendName(nsAString& aName) = 0;

  



  void SetUserData(void* aKey, LayerUserData* aData)
  { mUserData.Set(aKey, aData); }
  


  nsAutoPtr<LayerUserData> RemoveUserData(void* aKey)
  { nsAutoPtr<LayerUserData> d(mUserData.Remove(aKey)); return d; }
  


  PRBool HasUserData(void* aKey)
  { return mUserData.Has(aKey); }
  



  LayerUserData* GetUserData(void* aKey)
  { return mUserData.Get(aKey); }

  
  
  
  
  virtual const char* Name() const { return "???"; }

  



  void Dump(FILE* aFile=NULL, const char* aPrefix="");
  



  void DumpSelf(FILE* aFile=NULL, const char* aPrefix="");

  



  void Log(const char* aPrefix="");
  



  void LogSelf(const char* aPrefix="");

  static bool IsLogEnabled();
  static PRLogModuleInfo* GetLog() { return sLog; }

protected:
  nsRefPtr<Layer> mRoot;
  LayerUserDataSet mUserData;
  PRPackedBool mDestroyed;

  
  
  virtual nsACString& PrintInfo(nsACString& aTo, const char* aPrefix);

  static void InitLog();
  static PRLogModuleInfo* sLog;
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
    TYPE_CANVAS,
    TYPE_SHADOW
  };

  virtual ~Layer() {}

  




  LayerManager* Manager() { return mManager; }

  enum {
    




    CONTENT_OPAQUE = 0x01,
    





    CONTENT_NO_TEXT = 0x02,
    





    CONTENT_NO_TEXT_OVER_TRANSPARENT = 0x04
  };
  





  void SetContentFlags(PRUint32 aFlags)
  {
    mContentFlags = aFlags;
    Mutated();
  }
  






  virtual void SetVisibleRegion(const nsIntRegion& aRegion)
  {
    mVisibleRegion = aRegion;
    Mutated();
  }

  




  void SetOpacity(float aOpacity)
  {
    mOpacity = aOpacity;
    Mutated();
  }

  









  void SetClipRect(const nsIntRect* aRect)
  {
    mUseClipRect = aRect != nsnull;
    if (aRect) {
      mClipRect = *aRect;
    }
    Mutated();
  }

  









  void IntersectClipRect(const nsIntRect& aRect)
  {
    if (mUseClipRect) {
      mClipRect.IntersectRect(mClipRect, aRect);
    } else {
      mUseClipRect = PR_TRUE;
      mClipRect = aRect;
    }
    Mutated();
  }

  






  void SetTransform(const gfx3DMatrix& aMatrix)
  {
    mTransform = aMatrix;
    Mutated();
  }

  
  float GetOpacity() { return mOpacity; }
  const nsIntRect* GetClipRect() { return mUseClipRect ? &mClipRect : nsnull; }
  PRUint32 GetContentFlags() { return mContentFlags; }
  const nsIntRegion& GetVisibleRegion() { return mVisibleRegion; }
  ContainerLayer* GetParent() { return mParent; }
  Layer* GetNextSibling() { return mNextSibling; }
  Layer* GetPrevSibling() { return mPrevSibling; }
  virtual Layer* GetFirstChild() { return nsnull; }
  const gfx3DMatrix& GetTransform() { return mTransform; }

  





  virtual void FillSpecificAttributes(SpecificLayerAttributes& aAttrs) { }

  
  
  
  
  
  PRBool CanUseOpaqueSurface();

  



  void SetUserData(void* aKey, LayerUserData* aData)
  { mUserData.Set(aKey, aData); }
  


  nsAutoPtr<LayerUserData> RemoveUserData(void* aKey)
  { nsAutoPtr<LayerUserData> d(mUserData.Remove(aKey)); return d; }
  


  PRBool HasUserData(void* aKey)
  { return mUserData.Has(aKey); }
  



  LayerUserData* GetUserData(void* aKey)
  { return mUserData.Get(aKey); }

  








  virtual void Disconnect() {}

  



  virtual ThebesLayer* AsThebesLayer() { return nsnull; }

  virtual const char* Name() const =0;
  virtual LayerType GetType() const =0;

  




  void* ImplData() { return mImplData; }

  


  void SetParent(ContainerLayer* aParent) { mParent = aParent; }
  void SetNextSibling(Layer* aSibling) { mNextSibling = aSibling; }
  void SetPrevSibling(Layer* aSibling) { mPrevSibling = aSibling; }

  



  void Dump(FILE* aFile=NULL, const char* aPrefix="");
  



  void DumpSelf(FILE* aFile=NULL, const char* aPrefix="");

  



  void Log(const char* aPrefix="");
  



  void LogSelf(const char* aPrefix="");

  static bool IsLogEnabled() { return LayerManager::IsLogEnabled(); }

protected:
  Layer(LayerManager* aManager, void* aImplData) :
    mManager(aManager),
    mParent(nsnull),
    mNextSibling(nsnull),
    mPrevSibling(nsnull),
    mImplData(aImplData),
    mOpacity(1.0),
    mContentFlags(0),
    mUseClipRect(PR_FALSE)
    {}

  void Mutated() { mManager->Mutated(this); }

  
  
  
  
  
  virtual nsACString& PrintInfo(nsACString& aTo, const char* aPrefix);

  LayerManager* mManager;
  ContainerLayer* mParent;
  Layer* mNextSibling;
  Layer* mPrevSibling;
  void* mImplData;
  LayerUserDataSet mUserData;
  nsIntRegion mVisibleRegion;
  gfx3DMatrix mTransform;
  float mOpacity;
  nsIntRect mClipRect;
  PRUint32 mContentFlags;
  PRPackedBool mUseClipRect;
};












class THEBES_API ThebesLayer : public Layer {
public:
  





  virtual void InvalidateRegion(const nsIntRegion& aRegion) = 0;

  


  const nsIntRegion& GetValidRegion() const { return mValidRegion; }
  float GetXResolution() const { return mXResolution; }
  float GetYResolution() const { return mYResolution; }

  virtual ThebesLayer* AsThebesLayer() { return this; }

  MOZ_LAYER_DECL_NAME("ThebesLayer", TYPE_THEBES)

protected:
  ThebesLayer(LayerManager* aManager, void* aImplData)
    : Layer(aManager, aImplData)
    , mValidRegion()
    , mXResolution(1.0)
    , mYResolution(1.0)
  {}

  virtual nsACString& PrintInfo(nsACString& aTo, const char* aPrefix);

  nsIntRegion mValidRegion;
  
  
  
  
  
  
  
  
  
  
  
  
  
  float mXResolution;
  float mYResolution;
};





class THEBES_API ContainerLayer : public Layer {
public:
  






  virtual void InsertAfter(Layer* aChild, Layer* aAfter) = 0;
  




  virtual void RemoveChild(Layer* aChild) = 0;

  




  void SetFrameMetrics(const FrameMetrics& aFrameMetrics)
  {
    mFrameMetrics = aFrameMetrics;
  }

  

  virtual Layer* GetFirstChild() { return mFirstChild; }
  const FrameMetrics& GetFrameMetrics() { return mFrameMetrics; }

  MOZ_LAYER_DECL_NAME("ContainerLayer", TYPE_CONTAINER)

protected:
  ContainerLayer(LayerManager* aManager, void* aImplData)
    : Layer(aManager, aImplData),
      mFirstChild(nsnull)
  {}

  virtual nsACString& PrintInfo(nsACString& aTo, const char* aPrefix);

  Layer* mFirstChild;
  FrameMetrics mFrameMetrics;
};






class THEBES_API ColorLayer : public Layer {
public:
  



  virtual void SetColor(const gfxRGBA& aColor)
  {
    mColor = aColor;
  }

  
  virtual const gfxRGBA& GetColor() { return mColor; }

  MOZ_LAYER_DECL_NAME("ColorLayer", TYPE_COLOR)

protected:
  ColorLayer(LayerManager* aManager, void* aImplData)
    : Layer(aManager, aImplData),
      mColor(0.0, 0.0, 0.0, 0.0)
  {}

  virtual nsACString& PrintInfo(nsACString& aTo, const char* aPrefix);

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

  MOZ_LAYER_DECL_NAME("CanvasLayer", TYPE_CANVAS)

protected:
  CanvasLayer(LayerManager* aManager, void* aImplData)
    : Layer(aManager, aImplData), mFilter(gfxPattern::FILTER_GOOD) {}

  virtual nsACString& PrintInfo(nsACString& aTo, const char* aPrefix);

  gfxPattern::GraphicsFilter mFilter;
};

}
}

#endif 
