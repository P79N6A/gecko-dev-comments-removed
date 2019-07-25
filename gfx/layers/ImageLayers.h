




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
#ifdef XP_WIN
struct ID3D10Texture2D;
struct ID3D10Device;
struct ID3D10ShaderResourceView;

typedef void* HANDLE;
#endif
#ifdef MOZ_WIDGET_GONK
# include <ui/GraphicBuffer.h>
#endif

namespace mozilla {

class CrossProcessMutex;
namespace ipc {
class Shmem;
}

namespace layers {

class ImageContainerChild;
class ImageBridgeChild;

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

    




    MAC_IO_SURFACE,

    




    GONK_IO_SURFACE,

    


    REMOTE_IMAGE_BITMAP,

    


    SHARED_TEXTURE,

    


    REMOTE_IMAGE_DXGI_TEXTURE
  };

  Format GetFormat() { return mFormat; }
  void* GetImplData() { return mImplData; }

  virtual already_AddRefed<gfxASurface> GetAsSurface() = 0;
  virtual gfxIntSize GetSize() = 0;

  ImageBackendData* GetBackendData(LayersBackend aBackend)
  { return mBackendData[aBackend]; }
  void SetBackendData(LayersBackend aBackend, ImageBackendData* aData)
  { mBackendData[aBackend] = aData; }

protected:
  Image(void* aImplData, Format aFormat) :
    mImplData(aImplData),
    mFormat(aFormat)
  {}

  nsAutoPtr<ImageBackendData> mBackendData[mozilla::layers::LAYERS_LAST];

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

class CompositionNotifySink
{
public:
  virtual void DidComposite() = 0;
  virtual ~CompositionNotifySink() {}
};

















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
 







struct RemoteImageData {
  enum Type {
    


    RAW_BITMAP,

    







    DXGI_TEXTURE_HANDLE
  };
  
  enum Format {
    
    BGRA32,
    
    BGRX32
  };

  
  
  bool mWasUpdated;
  Type mType;
  Format mFormat;
  gfxIntSize mSize;
  union {
    struct {
      



      unsigned char *mData;
      int mStride;
    } mBitmap;
#ifdef XP_WIN
    HANDLE mTextureHandle;
#endif
  };
};








class THEBES_API ImageContainer {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(ImageContainer)
public:

  enum { DISABLE_ASYNC = 0x0, ENABLE_ASYNC = 0x01 };

  ImageContainer(int flag = 0);

  ~ImageContainer();

  







  already_AddRefed<Image> CreateImage(const Image::Format* aFormats,
                                      PRUint32 aNumFormats);

  
















  void SetCurrentImage(Image* aImage);

  














  void SetCurrentImageInTransaction(Image* aImage);

  




  bool IsAsync() const;

  







  PRUint64 GetAsyncContainerID() const;

  




  bool HasCurrentImage();

  










  already_AddRefed<Image> LockCurrentImage();

  



  void UnlockCurrentImage();

  



















  already_AddRefed<gfxASurface> GetCurrentAsSurface(gfxIntSize* aSizeResult);

  







  already_AddRefed<gfxASurface> LockCurrentAsSurface(gfxIntSize* aSizeResult,
                                                     Image** aCurrentImage = nullptr);

  




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

    nsRefPtr<Image> current = mActiveImage;
    if (aPainted == current) {
      if (mPaintTime.IsNull()) {
        mPaintTime = TimeStamp::Now();
        mPaintCount++;
      }
    } else if (!mPreviousImagePainted) {
      
      
      
      mPaintCount++;
      mPreviousImagePainted = true;
    }

    if (mCompositionNotifySink) {
      mCompositionNotifySink->DidComposite();
    }
  }

  void SetCompositionNotifySink(CompositionNotifySink *aSink) {
    mCompositionNotifySink = aSink;
  }

  







  void SetRemoteImageData(RemoteImageData *aRemoteData,
                          CrossProcessMutex *aRemoteDataMutex);
  


  RemoteImageData *GetRemoteImageData() { return mRemoteData; }

protected:
  typedef mozilla::ReentrantMonitor ReentrantMonitor;

  void SetCurrentImageInternal(Image* aImage);

  
  
  
  
  void EnsureActiveImage();

  
  
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

  
  
  
  RemoteImageData *mRemoteData;

  
  
  
  CrossProcessMutex *mRemoteDataMutex;

  CompositionNotifySink *mCompositionNotifySink;

  
  
  
  
  
  
  
  nsRefPtr<ImageContainerChild> mImageContainerChild;
};
 
class AutoLockImage
{
public:
  AutoLockImage(ImageContainer *aContainer) : mContainer(aContainer) { mImage = mContainer->LockCurrentImage(); }
  AutoLockImage(ImageContainer *aContainer, gfxASurface **aSurface) : mContainer(aContainer) {
    *aSurface = mContainer->LockCurrentAsSurface(&mSize, getter_AddRefs(mImage)).get();
  }
  ~AutoLockImage() { if (mContainer) { mContainer->UnlockCurrentImage(); } }

  Image* GetImage() { return mImage; }
  const gfxIntSize &GetSize() { return mSize; }

  void Unlock() { 
    if (mContainer) {
      mImage = nullptr;
      mContainer->UnlockCurrentImage();
      mContainer = nullptr;
    }
  }

  




  void Refresh() {
    if (mContainer) {
      mContainer->UnlockCurrentImage();
      mImage = mContainer->LockCurrentImage();
    }
  }

private:
  ImageContainer *mContainer;
  nsRefPtr<Image> mImage;
  gfxIntSize mSize;
};




class THEBES_API ImageLayer : public Layer {
public:
  enum ScaleMode {
    SCALE_NONE,
    SCALE_STRETCH 
  
  };

  




  void SetContainer(ImageContainer* aContainer) 
  {
    mContainer = aContainer;  
  }
  



  void SetFilter(gfxPattern::GraphicsFilter aFilter) { mFilter = aFilter; }

  



  void SetScaleToSize(const gfxIntSize &aSize, ScaleMode aMode)
  {
    mScaleToSize = aSize;
    mScaleMode = aMode;
  }


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
        SnapTransform(GetLocalTransform(), snap, nullptr)*
        SnapTransform(aTransformToSurface, gfxRect(0, 0, 0, 0), nullptr);
    ComputeEffectiveTransformForMaskLayer(aTransformToSurface);
  }

  


  void SetForceSingleTile(bool aForceSingleTile)
  {
    mForceSingleTile = aForceSingleTile;
    Mutated();
  }

protected:
  ImageLayer(LayerManager* aManager, void* aImplData)
    : Layer(aManager, aImplData), mFilter(gfxPattern::FILTER_GOOD)
    , mScaleMode(SCALE_NONE), mForceSingleTile(false) {}

  virtual nsACString& PrintInfo(nsACString& aTo, const char* aPrefix);


  nsRefPtr<ImageContainer> mContainer;
  gfxPattern::GraphicsFilter mFilter;
  gfxIntSize mScaleToSize;
  ScaleMode mScaleMode;
  bool mForceSingleTile;
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

  










  void CopyData(const Data& aData,
                PRInt32 aYOffset = 0, PRInt32 aYSkip = 0,
                PRInt32 aCbOffset = 0, PRInt32 aCbSkip = 0,
                PRInt32 aCrOffset = 0, PRInt32 aCrSkip = 0);

  




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

#ifdef MOZ_WIDGET_GONK









class GraphicBufferLocked {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(GraphicBufferLocked)

public:
  GraphicBufferLocked(android::GraphicBuffer* aGraphicBuffer)
    : mGraphicBuffer(aGraphicBuffer)
  {}

  virtual ~GraphicBufferLocked() {}

  virtual void Unlock() {}

  virtual void* GetNativeBuffer()
  {
    return mGraphicBuffer->getNativeBuffer();
  }   

protected:
  android::GraphicBuffer* mGraphicBuffer;
};

class THEBES_API GonkIOSurfaceImage : public Image {
public:
  struct Data {
    nsRefPtr<GraphicBufferLocked> mGraphicBuffer;
    gfxIntSize mPicSize;
  };

  GonkIOSurfaceImage()
    : Image(NULL, GONK_IO_SURFACE)
    , mSize(0, 0)
    {}

  virtual ~GonkIOSurfaceImage()
  {
    mGraphicBuffer->Unlock();
  }

  virtual void SetData(const Data& aData)
  {
    mGraphicBuffer = aData.mGraphicBuffer;
    mSize = aData.mPicSize;
  }

  virtual gfxIntSize GetSize()
  {
    return mSize;
  }

  virtual already_AddRefed<gfxASurface> GetAsSurface()
  {
    
    return nullptr;
  }

  void* GetNativeBuffer()
  {
    return mGraphicBuffer->GetNativeBuffer();
  }

private:
  nsRefPtr<GraphicBufferLocked> mGraphicBuffer;
  gfxIntSize mSize;
};
#endif

class RemoteBitmapImage : public Image {
public:
  RemoteBitmapImage() : Image(NULL, REMOTE_IMAGE_BITMAP) {}

  already_AddRefed<gfxASurface> GetAsSurface();

  gfxIntSize GetSize() { return mSize; }

  unsigned char *mData;
  int mStride;
  gfxIntSize mSize;
  RemoteImageData::Format mFormat;
};

}
}

#endif
