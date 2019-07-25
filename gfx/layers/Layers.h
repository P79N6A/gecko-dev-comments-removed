




































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

#include "mozilla/gfx/2D.h"

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
class ShadowLayer;
class ReadbackLayer;
class ReadbackProcessor;
class SpecificLayerAttributes;







struct THEBES_API FrameMetrics {
public:
  
  typedef PRUint64 ViewID;
  static const ViewID NULL_SCROLL_ID;   
  static const ViewID ROOT_SCROLL_ID;   
  static const ViewID START_SCROLL_ID;  
                                        

  FrameMetrics()
    : mViewport(0, 0, 0, 0)
    , mContentSize(0, 0)
    , mViewportScrollOffset(0, 0)
    , mScrollId(NULL_SCROLL_ID)
  {}

  

  PRBool operator==(const FrameMetrics& aOther) const
  {
    return (mViewport.IsEqualEdges(aOther.mViewport) &&
            mViewportScrollOffset == aOther.mViewportScrollOffset &&
            mDisplayPort.IsEqualEdges(aOther.mDisplayPort) &&
            mScrollId == aOther.mScrollId);
  }

  PRBool IsDefault() const
  {
    return (FrameMetrics() == *this);
  }

  PRBool IsRootScrollable() const
  {
    return mScrollId == ROOT_SCROLL_ID;
  }

  PRBool IsScrollable() const
  {
    return mScrollId != NULL_SCROLL_ID;
  }

  
  nsIntRect mViewport;
  nsIntSize mContentSize;
  nsIntPoint mViewportScrollOffset;
  nsIntRect mDisplayPort;
  ViewID mScrollId;
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

  


  void Clear()
  {
    mKey = nsnull;
    mValue = nsnull;
  }

private:
  void* mKey;
  nsAutoPtr<LayerUserData> mValue;
};
























class THEBES_API LayerManager {
  NS_INLINE_DECL_REFCOUNTING(LayerManager)

public:
  enum LayersBackend {
    LAYERS_NONE = 0,
    LAYERS_BASIC,
    LAYERS_OPENGL,
    LAYERS_D3D9,
    LAYERS_D3D10,
    LAYERS_LAST
  };

  LayerManager() : mDestroyed(PR_FALSE), mSnapEffectiveTransforms(PR_TRUE)
  {
    InitLog();
  }
  virtual ~LayerManager() {}

  





  virtual void Destroy() { mDestroyed = PR_TRUE; mUserData.Clear(); }
  PRBool IsDestroyed() { return mDestroyed; }

  





  virtual void BeginTransaction() = 0;
  






  virtual void BeginTransactionWithTarget(gfxContext* aTarget) = 0;
  







  virtual bool EndEmptyTransaction() = 0;

  

























  typedef void (* DrawThebesLayerCallback)(ThebesLayer* aLayer,
                                           gfxContext* aContext,
                                           const nsIntRegion& aRegionToDraw,
                                           const nsIntRegion& aRegionToInvalidate,
                                           void* aCallbackData);
  






  virtual void EndTransaction(DrawThebesLayerCallback aCallback,
                              void* aCallbackData) = 0;

  PRBool IsSnappingEffectiveTransforms() { return mSnapEffectiveTransforms; } 

  




  virtual void SetRoot(Layer* aLayer) = 0;
  


  Layer* GetRoot() { return mRoot; }

  





#ifdef DEBUG
  
  virtual void Mutated(Layer* aLayer);
#else
  virtual void Mutated(Layer* aLayer) { }
#endif

  



  virtual already_AddRefed<ThebesLayer> CreateThebesLayer() = 0;
  



  virtual already_AddRefed<ContainerLayer> CreateContainerLayer() = 0;
  



  virtual already_AddRefed<ImageLayer> CreateImageLayer() = 0;
  



  virtual already_AddRefed<ColorLayer> CreateColorLayer() = 0;
  



  virtual already_AddRefed<CanvasLayer> CreateCanvasLayer() = 0;
  



  virtual already_AddRefed<ReadbackLayer> CreateReadbackLayer() { return nsnull; }

  


  virtual already_AddRefed<ImageContainer> CreateImageContainer() = 0;

  




  virtual LayersBackend GetBackendType() = 0;
 
  



  virtual already_AddRefed<gfxASurface>
    CreateOptimalSurface(const gfxIntSize &aSize,
                         gfxASurface::gfxImageFormat imageFormat);

  



  virtual TemporaryRef<mozilla::gfx::DrawTarget>
    CreateDrawTarget(const mozilla::gfx::IntSize &aSize,
                     mozilla::gfx::SurfaceFormat aFormat);


  


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

  PRBool IsCompositingCheap(LayerManager::LayersBackend aBackend)
  { return LAYERS_BASIC != aBackend; }

  virtual PRBool IsCompositingCheap() { return PR_TRUE; }

protected:
  nsRefPtr<Layer> mRoot;
  LayerUserDataSet mUserData;
  PRPackedBool mDestroyed;
  PRPackedBool mSnapEffectiveTransforms;

  
  
  virtual nsACString& PrintInfo(nsACString& aTo, const char* aPrefix);

  static void InitLog();
  static PRLogModuleInfo* sLog;
};

class ThebesLayer;





class THEBES_API Layer {
  NS_INLINE_DECL_REFCOUNTING(Layer)  

public:
  
  enum LayerType {
    TYPE_CANVAS,
    TYPE_COLOR,
    TYPE_CONTAINER,
    TYPE_IMAGE,
    TYPE_READBACK,
    TYPE_SHADOW,
    TYPE_THEBES
  };

  virtual ~Layer() {}

  




  LayerManager* Manager() { return mManager; }

  enum {
    




    CONTENT_OPAQUE = 0x01,
    






    CONTENT_COMPONENT_ALPHA = 0x02
  };
  





  void SetContentFlags(PRUint32 aFlags)
  {
    NS_ASSERTION((aFlags & (CONTENT_OPAQUE | CONTENT_COMPONENT_ALPHA)) !=
                 (CONTENT_OPAQUE | CONTENT_COMPONENT_ALPHA),
                 "Can't be opaque and require component alpha");
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

  























  void SetTileSourceRect(const nsIntRect* aRect)
  {
    mUseTileSourceRect = aRect != nsnull;
    if (aRect) {
      mTileSourceRect = *aRect;
    }
    Mutated();
  }

  void SetIsFixedPosition(PRBool aFixedPosition) { mIsFixedPosition = aFixedPosition; }

  
  float GetOpacity() { return mOpacity; }
  const nsIntRect* GetClipRect() { return mUseClipRect ? &mClipRect : nsnull; }
  PRUint32 GetContentFlags() { return mContentFlags; }
  const nsIntRegion& GetVisibleRegion() { return mVisibleRegion; }
  ContainerLayer* GetParent() { return mParent; }
  Layer* GetNextSibling() { return mNextSibling; }
  Layer* GetPrevSibling() { return mPrevSibling; }
  virtual Layer* GetFirstChild() { return nsnull; }
  virtual Layer* GetLastChild() { return nsnull; }
  const gfx3DMatrix& GetTransform() { return mTransform; }
  const nsIntRect* GetTileSourceRect() { return mUseTileSourceRect ? &mTileSourceRect : nsnull; }
  bool GetIsFixedPosition() { return mIsFixedPosition; }

  





  virtual void FillSpecificAttributes(SpecificLayerAttributes& aAttrs) { }

  
  
  
  
  
  PRBool CanUseOpaqueSurface();

  enum SurfaceMode {
    SURFACE_OPAQUE,
    SURFACE_SINGLE_CHANNEL_ALPHA,
    SURFACE_COMPONENT_ALPHA
  };
  SurfaceMode GetSurfaceMode()
  {
    if (CanUseOpaqueSurface())
      return SURFACE_OPAQUE;
    if (mContentFlags & CONTENT_COMPONENT_ALPHA)
      return SURFACE_COMPONENT_ALPHA;
    return SURFACE_SINGLE_CHANNEL_ALPHA;
  }

  



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

  



  virtual ContainerLayer* AsContainerLayer() { return nsnull; }

  



  virtual ShadowLayer* AsShadowLayer() { return nsnull; }

  
  
  
  const nsIntRect* GetEffectiveClipRect();
  const nsIntRegion& GetEffectiveVisibleRegion();
  



  float GetEffectiveOpacity();
  







  const gfx3DMatrix& GetEffectiveTransform() const { return mEffectiveTransform; }

  










  virtual void ComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface) = 0;
  
  









  nsIntRect CalculateScissorRect(const nsIntRect& aCurrentScissorRect,
                                 const gfxMatrix* aWorldTransform);

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
    mUseClipRect(PR_FALSE),
    mUseTileSourceRect(PR_FALSE),
    mIsFixedPosition(PR_FALSE)
    {}

  void Mutated() { mManager->Mutated(this); }

  
  
  
  
  
  virtual nsACString& PrintInfo(nsACString& aTo, const char* aPrefix);

  



  const gfx3DMatrix& GetLocalTransform();

  










  gfx3DMatrix SnapTransform(const gfx3DMatrix& aTransform,
                            const gfxRect& aSnapRect,
                            gfxMatrix* aResidualTransform);

  LayerManager* mManager;
  ContainerLayer* mParent;
  Layer* mNextSibling;
  Layer* mPrevSibling;
  void* mImplData;
  LayerUserDataSet mUserData;
  nsIntRegion mVisibleRegion;
  gfx3DMatrix mTransform;
  gfx3DMatrix mEffectiveTransform;
  float mOpacity;
  nsIntRect mClipRect;
  nsIntRect mTileSourceRect;
  PRUint32 mContentFlags;
  PRPackedBool mUseClipRect;
  PRPackedBool mUseTileSourceRect;
  PRPackedBool mIsFixedPosition;
};












class THEBES_API ThebesLayer : public Layer {
public:
  





  virtual void InvalidateRegion(const nsIntRegion& aRegion) = 0;
  











  void SetAllowResidualTranslation(bool aAllow) { mAllowResidualTranslation = aAllow; }

  


  const nsIntRegion& GetValidRegion() const { return mValidRegion; }

  virtual ThebesLayer* AsThebesLayer() { return this; }

  MOZ_LAYER_DECL_NAME("ThebesLayer", TYPE_THEBES)

  virtual void ComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface)
  {
    
    gfx3DMatrix idealTransform = GetLocalTransform()*aTransformToSurface;
    gfxMatrix residual;
    mEffectiveTransform = SnapTransform(idealTransform, gfxRect(0, 0, 0, 0),
        mAllowResidualTranslation ? &residual : nsnull);
    
    
    
    NS_ASSERTION(!residual.HasNonTranslation(),
                 "Residual transform can only be a translation");
    if (residual.GetTranslation() != mResidualTranslation) {
      mResidualTranslation = residual.GetTranslation();
      NS_ASSERTION(-0.5 <= mResidualTranslation.x && mResidualTranslation.x < 0.5 &&
                   -0.5 <= mResidualTranslation.y && mResidualTranslation.y < 0.5,
                   "Residual translation out of range");
      mValidRegion.SetEmpty();
    }
  }

  bool UsedForReadback() { return mUsedForReadback; }
  void SetUsedForReadback(bool aUsed) { mUsedForReadback = aUsed; }
  







  gfxPoint GetResidualTranslation() const { return mResidualTranslation; }

protected:
  ThebesLayer(LayerManager* aManager, void* aImplData)
    : Layer(aManager, aImplData)
    , mValidRegion()
    , mUsedForReadback(false)
    , mAllowResidualTranslation(false)
  {
    mContentFlags = 0; 
  }

  virtual nsACString& PrintInfo(nsACString& aTo, const char* aPrefix);

  




  gfxPoint mResidualTranslation;
  nsIntRegion mValidRegion;
  



  bool mUsedForReadback;
  


  bool mAllowResidualTranslation;
};





class THEBES_API ContainerLayer : public Layer {
public:
  






  virtual void InsertAfter(Layer* aChild, Layer* aAfter) = 0;
  




  virtual void RemoveChild(Layer* aChild) = 0;

  




  void SetFrameMetrics(const FrameMetrics& aFrameMetrics)
  {
    mFrameMetrics = aFrameMetrics;
  }

  

  virtual ContainerLayer* AsContainerLayer() { return this; }

  virtual Layer* GetFirstChild() { return mFirstChild; }
  virtual Layer* GetLastChild() { return mLastChild; }
  const FrameMetrics& GetFrameMetrics() { return mFrameMetrics; }

  MOZ_LAYER_DECL_NAME("ContainerLayer", TYPE_CONTAINER)

  





  virtual void ComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface) = 0;

  





  PRBool UseIntermediateSurface() { return mUseIntermediateSurface; }

  



  nsIntRect GetIntermediateSurfaceRect()
  {
    NS_ASSERTION(mUseIntermediateSurface, "Must have intermediate surface");
    return mVisibleRegion.GetBounds();
  }

  


  PRBool HasMultipleChildren();

  



  PRBool SupportsComponentAlphaChildren() { return mSupportsComponentAlphaChildren; }

protected:
  friend class ReadbackProcessor;

  void DidInsertChild(Layer* aLayer);
  void DidRemoveChild(Layer* aLayer);

  ContainerLayer(LayerManager* aManager, void* aImplData)
    : Layer(aManager, aImplData),
      mFirstChild(nsnull),
      mLastChild(nsnull),
      mUseIntermediateSurface(PR_FALSE),
      mSupportsComponentAlphaChildren(PR_FALSE),
      mMayHaveReadbackChild(PR_FALSE)
  {
    mContentFlags = 0; 
  }

  



  void DefaultComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface);

  


  void ComputeEffectiveTransformsForChildren(const gfx3DMatrix& aTransformToSurface);

  virtual nsACString& PrintInfo(nsACString& aTo, const char* aPrefix);

  Layer* mFirstChild;
  Layer* mLastChild;
  FrameMetrics mFrameMetrics;
  PRPackedBool mUseIntermediateSurface;
  PRPackedBool mSupportsComponentAlphaChildren;
  PRPackedBool mMayHaveReadbackChild;
};






class THEBES_API ColorLayer : public Layer {
public:
  



  virtual void SetColor(const gfxRGBA& aColor)
  {
    mColor = aColor;
  }

  
  virtual const gfxRGBA& GetColor() { return mColor; }

  MOZ_LAYER_DECL_NAME("ColorLayer", TYPE_COLOR)

  virtual void ComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface)
  {
    
    gfx3DMatrix idealTransform = GetLocalTransform()*aTransformToSurface;
    mEffectiveTransform = SnapTransform(idealTransform, gfxRect(0, 0, 0, 0), nsnull);
  }

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
      : mSurface(nsnull), mGLContext(nsnull)
      , mDrawTarget(nsnull), mGLBufferIsPremultiplied(PR_FALSE)
    { }

    
    gfxASurface* mSurface;  
    mozilla::gl::GLContext* mGLContext; 
    mozilla::gfx::DrawTarget *mDrawTarget; 

    
    nsIntSize mSize;

    


    PRPackedBool mGLBufferIsPremultiplied;
  };

  







  virtual void Initialize(const Data& aData) = 0;

  



  void Updated() { mDirty = PR_TRUE; }

  


  typedef void (* DidTransactionCallback)(void* aClosureData);
  void SetDidTransactionCallback(DidTransactionCallback aCallback, void* aClosureData)
  {
    mCallback = aCallback;
    mCallbackData = aClosureData;
  }

  



  void SetFilter(gfxPattern::GraphicsFilter aFilter) { mFilter = aFilter; }
  gfxPattern::GraphicsFilter GetFilter() const { return mFilter; }

  MOZ_LAYER_DECL_NAME("CanvasLayer", TYPE_CANVAS)

  virtual void ComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface)
  {
    
    
    
    
    mEffectiveTransform =
        SnapTransform(GetLocalTransform(), gfxRect(0, 0, mBounds.width, mBounds.height),
                      nsnull)*
        SnapTransform(aTransformToSurface, gfxRect(0, 0, 0, 0), nsnull);
  }

protected:
  CanvasLayer(LayerManager* aManager, void* aImplData)
    : Layer(aManager, aImplData),
      mCallback(nsnull), mCallbackData(nsnull), mFilter(gfxPattern::FILTER_GOOD),
      mDirty(PR_FALSE) {}

  virtual nsACString& PrintInfo(nsACString& aTo, const char* aPrefix);

  void FireDidTransactionCallback()
  {
    if (mCallback) {
      mCallback(mCallbackData);
    }
  }

  


  nsIntRect mBounds;
  DidTransactionCallback mCallback;
  void* mCallbackData;
  gfxPattern::GraphicsFilter mFilter;
  


  PRPackedBool mDirty;
};

}
}

#endif 
