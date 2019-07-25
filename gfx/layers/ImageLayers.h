




































#ifndef GFX_IMAGELAYER_H
#define GFX_IMAGELAYER_H

#include "Layers.h"

#include "nsISupportsImpl.h"
#include "gfxPattern.h"
#include "nsThreadUtils.h"
#include "mozilla/ReentrantMonitor.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/mozalloc.h"
#include "mozilla/Mutex.h"
#include "gfxPlatform.h"

#ifdef XP_MACOSX
#include "nsIOSurface.h"
#endif

namespace mozilla {
namespace layers {

enum StereoMode {
  STEREO_MODE_MONO,
  STEREO_MODE_LEFT_RIGHT,
  STEREO_MODE_RIGHT_LEFT,
  STEREO_MODE_BOTTOM_TOP,
  STEREO_MODE_TOP_BOTTOM
};

struct ImageBackendData
{
  virtual ~ImageBackendData() {}

protected:
  ImageBackendData() {}
};















class THEBES_API Image {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(Image)

public:
  virtual ~Image() {}

  enum Format {
    




    PLANAR_YCBCR,

    












    CAIRO_SURFACE,

    




    MAC_IO_SURFACE
  };

  Format GetFormat() { return mFormat; }
  void* GetImplData() { return mImplData; }

  virtual already_AddRefed<gfxASurface> GetAsSurface() = 0;
  virtual gfxIntSize GetSize() = 0;

  ImageBackendData* GetBackendData(LayerManager::LayersBackend aBackend)
  { return mBackendData[aBackend]; }
  void SetBackendData(LayerManager::LayersBackend aBackend, ImageBackendData* aData)
  { mBackendData[aBackend] = aData; }

protected:
  Image(void* aImplData, Format aFormat) :
    mImplData(aImplData),
    mFormat(aFormat)
  {}

  nsAutoPtr<ImageBackendData> mBackendData[LayerManager::LAYERS_LAST];

  void* mImplData;
  Format mFormat;
};








class BufferRecycleBin {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(RecycleBin)

  typedef mozilla::gl::GLContext GLContext;

public:
  BufferRecycleBin();

  void RecycleBuffer(PRUint8* aBuffer, PRUint32 aSize);
  
  PRUint8* GetBuffer(PRUint32 aSize);

private:
  typedef mozilla::Mutex Mutex;

  
  
  Mutex mLock;

  
  
  nsTArray<nsAutoArrayPtr<PRUint8> > mRecycledBuffers;
  
  PRUint32 mRecycledBufferSize;
};




static inline bool
FormatInList(const Image::Format* aFormats, PRUint32 aNumFormats,
             Image::Format aFormat)
{
  for (PRUint32 i = 0; i < aNumFormats; ++i) {
    if (aFormats[i] == aFormat) {
      return true;
    }
  }
  return false;
}

















class THEBES_API ImageFactory
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(ImageFactory)
protected:
  friend class ImageContainer;

  ImageFactory() {}
  virtual ~ImageFactory() {}

  virtual already_AddRefed<Image> CreateImage(const Image::Format* aFormats,
                                              PRUint32 aNumFormats,
                                              const gfxIntSize &aScaleHint,
                                              BufferRecycleBin *aRecycleBin);

};








class THEBES_API ImageContainer {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(ImageContainer)

public:
  ImageContainer() :
    mReentrantMonitor("ImageContainer.mReentrantMonitor"),
    mPaintCount(0),
    mPreviousImagePainted(false),
    mImageFactory(new ImageFactory()),
    mRecycleBin(new BufferRecycleBin())
  {}

  ~ImageContainer();

  







  already_AddRefed<Image> CreateImage(const Image::Format* aFormats,
                                      PRUint32 aNumFormats);

  







  void SetCurrentImage(Image* aImage);

  









  already_AddRefed<Image> GetCurrentImage()
  {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);

    nsRefPtr<Image> retval = mActiveImage;
    return retval.forget();
  }

  














  already_AddRefed<gfxASurface> GetCurrentAsSurface(gfxIntSize* aSizeResult);

  




  gfxIntSize GetCurrentSize();

  






  void SetScaleHint(const gfxIntSize& aScaleHint)
  { mScaleHint = aScaleHint; }

  void SetImageFactory(ImageFactory *aFactory)
  {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    mImageFactory = aFactory ? aFactory : new ImageFactory();
  }

  





  TimeStamp GetPaintTime() {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    return mPaintTime;
  }

  



  PRUint32 GetPaintCount() {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    return mPaintCount;
  }

  




  void NotifyPaintedImage(Image* aPainted) {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    nsRefPtr<Image> current = GetCurrentImage();
    if (aPainted == current) {
      if (mPaintTime.IsNull()) {
        mPaintTime = TimeStamp::Now();
        mPaintCount++;
      }
    } else if (!mPreviousImagePainted) {
      
      
      
      mPaintCount++;
      mPreviousImagePainted = true;
    }
  }

protected:
  typedef mozilla::ReentrantMonitor ReentrantMonitor;

  
  
  ReentrantMonitor mReentrantMonitor;

  
  
  
  void CurrentImageChanged() {
    mReentrantMonitor.AssertCurrentThreadIn();
    mPreviousImagePainted = !mPaintTime.IsNull();
    mPaintTime = TimeStamp();
  }

  nsRefPtr<Image> mActiveImage;

  
  
  
  PRUint32 mPaintCount;

  
  
  TimeStamp mPaintTime;

  
  bool mPreviousImagePainted;

  
  
  
  nsRefPtr<ImageFactory> mImageFactory;

  gfxIntSize mScaleHint;

  nsRefPtr<BufferRecycleBin> mRecycleBin;
};




class THEBES_API ImageLayer : public Layer {
public:
  




  void SetContainer(ImageContainer* aContainer) 
  {
    mContainer = aContainer;  
  }
  



  void SetFilter(gfxPattern::GraphicsFilter aFilter) { mFilter = aFilter; }

  ImageContainer* GetContainer() { return mContainer; }
  gfxPattern::GraphicsFilter GetFilter() { return mFilter; }

  MOZ_LAYER_DECL_NAME("ImageLayer", TYPE_IMAGE)

  virtual void ComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface)
  {
    
    gfxRect snap(0, 0, 0, 0);
    if (mContainer) {
      gfxIntSize size = mContainer->GetCurrentSize();
      snap.SizeTo(gfxSize(size.width, size.height));
    }
    
    
    
    
    mEffectiveTransform =
        SnapTransform(GetLocalTransform(), snap, nsnull)*
        SnapTransform(aTransformToSurface, gfxRect(0, 0, 0, 0), nsnull);
  }

protected:
  ImageLayer(LayerManager* aManager, void* aImplData)
    : Layer(aManager, aImplData), mFilter(gfxPattern::FILTER_GOOD) {}

  virtual nsACString& PrintInfo(nsACString& aTo, const char* aPrefix);

  nsRefPtr<ImageContainer> mContainer;
  gfxPattern::GraphicsFilter mFilter;
};




















class THEBES_API PlanarYCbCrImage : public Image {
public:
  struct Data {
    
    PRUint8* mYChannel;
    PRInt32 mYStride;
    gfxIntSize mYSize;
    
    PRUint8* mCbChannel;
    PRUint8* mCrChannel;
    PRInt32 mCbCrStride;
    gfxIntSize mCbCrSize;
    
    PRUint32 mPicX;
    PRUint32 mPicY;
    gfxIntSize mPicSize;
    StereoMode mStereoMode;

    nsIntRect GetPictureRect() const {
      return nsIntRect(mPicX, mPicY,
                       mPicSize.width,
                       mPicSize.height);
    }
  };

  enum {
    MAX_DIMENSION = 16384
  };

  ~PlanarYCbCrImage();

  



  virtual void SetData(const Data& aData);

  




  virtual void SetDelayedConversion(bool aDelayed) { }

  


  virtual const Data* GetData() { return &mData; }

  




  void CopyData(const Data& aData);

  




  virtual PRUint8* AllocateBuffer(PRUint32 aSize);

  


  virtual PRUint32 GetDataSize() { return mBufferSize; }

  already_AddRefed<gfxASurface> GetAsSurface();

  virtual gfxIntSize GetSize() { return mSize; }

  void SetOffscreenFormat(gfxImageFormat aFormat) { mOffscreenFormat = aFormat; }
  gfxImageFormat GetOffscreenFormat() { return mOffscreenFormat; }

  
  nsAutoArrayPtr<PRUint8> mBuffer;
  PRUint32 mBufferSize;
  Data mData;
  gfxIntSize mSize;
  gfxImageFormat mOffscreenFormat;
  nsCountedRef<nsMainThreadSurfaceRef> mSurface;
  nsRefPtr<BufferRecycleBin> mRecycleBin;

  PlanarYCbCrImage(BufferRecycleBin *aRecycleBin);
};






class THEBES_API CairoImage : public Image {
public:
  struct Data {
    gfxASurface* mSurface;
    gfxIntSize mSize;
  };

  




  void SetData(const Data& aData)
  {
    mSurface = aData.mSurface;
    mSize = aData.mSize;
  }


  virtual already_AddRefed<gfxASurface> GetAsSurface()
  {
    NS_ASSERTION(NS_IsMainThread(), "Must be main thread");
    nsRefPtr<gfxASurface> surface = mSurface.get();
    return surface.forget();
  }

  gfxIntSize GetSize() { return mSize; }

  CairoImage() : Image(NULL, CAIRO_SURFACE) {}

  nsCountedRef<nsMainThreadSurfaceRef> mSurface;
  gfxIntSize mSize;
};

#ifdef XP_MACOSX
class THEBES_API MacIOSurfaceImage : public Image {
public:
  struct Data {
    nsIOSurface* mIOSurface;
  };

  MacIOSurfaceImage()
    : Image(NULL, MAC_IO_SURFACE)
    , mSize(0, 0)
    , mPluginInstanceOwner(NULL)
    , mUpdateCallback(NULL)
    , mDestroyCallback(NULL)
    {}

  virtual ~MacIOSurfaceImage()
  {
    if (mDestroyCallback) {
      mDestroyCallback(mPluginInstanceOwner);
    }
  }

 




  virtual void SetData(const Data& aData);

  




  typedef void (*UpdateSurfaceCallback)(ImageContainer* aContainer, void* aInstanceOwner);
  virtual void SetUpdateCallback(UpdateSurfaceCallback aCallback, void* aInstanceOwner)
  {
    mUpdateCallback = aCallback;
    mPluginInstanceOwner = aInstanceOwner;
  }

  typedef void (*DestroyCallback)(void* aInstanceOwner);
  virtual void SetDestroyCallback(DestroyCallback aCallback)
  {
    mDestroyCallback = aCallback;
  }

  virtual gfxIntSize GetSize()
  {
    return mSize;
  }

  nsIOSurface* GetIOSurface()
  {
    return mIOSurface;
  }

  void Update(ImageContainer* aContainer);

  virtual already_AddRefed<gfxASurface> GetAsSurface();

private:
  gfxIntSize mSize;
  nsRefPtr<nsIOSurface> mIOSurface;
  void* mPluginInstanceOwner;
  UpdateSurfaceCallback mUpdateCallback;
  DestroyCallback mDestroyCallback;
};
#endif

}
}

#endif 
