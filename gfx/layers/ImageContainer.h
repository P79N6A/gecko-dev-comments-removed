




#ifndef GFX_IMAGECONTAINER_H
#define GFX_IMAGECONTAINER_H

#include <stdint.h>                     
#include <sys/types.h>                  
#include "ImageTypes.h"                 
#include "gfxASurface.h"                
#include "mozilla/Assertions.h"         
#include "mozilla/Mutex.h"              
#include "mozilla/ReentrantMonitor.h"   
#include "mozilla/TimeStamp.h"          
#include "mozilla/gfx/Point.h"          
#include "mozilla/layers/LayersTypes.h"  
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsAutoRef.h"                  
#include "nsCOMPtr.h"                   
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsRect.h"                     
#include "nsSize.h"                     
#include "nsTArray.h"                   
#include "mozilla/Atomics.h"
#include "mozilla/WeakPtr.h"
#include "nsThreadUtils.h"
#include "mozilla/gfx/2D.h"
#include "nsDataHashtable.h"

#ifndef XPCOM_GLUE_AVOID_NSPR















class nsMainThreadSurfaceRef;

template <>
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

  static RawRef Void() { return nullptr; }
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






class nsMainThreadSourceSurfaceRef;

template <>
class nsAutoRefTraits<nsMainThreadSourceSurfaceRef> {
public:
  typedef mozilla::gfx::SourceSurface* RawRef;

  


  class SurfaceReleaser : public nsRunnable {
  public:
    SurfaceReleaser(RawRef aRef) : mRef(aRef) {}
    NS_IMETHOD Run() {
      mRef->Release();
      return NS_OK;
    }
    RawRef mRef;
  };

  static RawRef Void() { return nullptr; }
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

#ifdef XP_WIN
struct ID3D10Texture2D;
struct ID3D10Device;
struct ID3D10ShaderResourceView;
#endif

typedef void* HANDLE;

namespace mozilla {

class CrossProcessMutex;

namespace layers {

class ImageClient;
class SharedPlanarYCbCrImage;
class TextureClient;
class CompositableClient;
class CompositableForwarder;
class SurfaceDescriptor;

struct ImageBackendData
{
  virtual ~ImageBackendData() {}

protected:
  ImageBackendData() {}
};


class ISharedImage {
public:
    virtual uint8_t* GetBuffer() = 0;

    



    virtual TextureClient* GetTextureClient(CompositableClient* aClient) = 0;
};















class Image {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(Image)

public:
  virtual ~Image() {}

  virtual ISharedImage* AsSharedImage() { return nullptr; }

  ImageFormat GetFormat() { return mFormat; }
  void* GetImplData() { return mImplData; }

  virtual already_AddRefed<gfxASurface> DeprecatedGetAsSurface() = 0;
  virtual gfx::IntSize GetSize() = 0;
  virtual nsIntRect GetPictureRect()
  {
    return nsIntRect(0, 0, GetSize().width, GetSize().height);
  }

  ImageBackendData* GetBackendData(LayersBackend aBackend)
  { return mBackendData[size_t(aBackend)]; }
  void SetBackendData(LayersBackend aBackend, ImageBackendData* aData)
  { mBackendData[size_t(aBackend)] = aData; }

  int32_t GetSerial() { return mSerial; }

  void MarkSent() { mSent = true; }
  bool IsSentToCompositor() { return mSent; }

  virtual TemporaryRef<gfx::SourceSurface> GetAsSourceSurface() = 0;

protected:
  Image(void* aImplData, ImageFormat aFormat) :
    mImplData(aImplData),
    mSerial(++sSerialCounter),
    mFormat(aFormat),
    mSent(false)
  {}

  nsAutoPtr<ImageBackendData> mBackendData[size_t(mozilla::layers::LayersBackend::LAYERS_LAST)];

  void* mImplData;
  int32_t mSerial;
  ImageFormat mFormat;
  static mozilla::Atomic<int32_t> sSerialCounter;
  bool mSent;
};








class BufferRecycleBin {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(RecycleBin)

  

public:
  BufferRecycleBin();

  void RecycleBuffer(uint8_t* aBuffer, uint32_t aSize);
  
  uint8_t* GetBuffer(uint32_t aSize);

private:
  typedef mozilla::Mutex Mutex;

  
  
  Mutex mLock;

  
  
  nsTArray<nsAutoArrayPtr<uint8_t> > mRecycledBuffers;
  
  uint32_t mRecycledBufferSize;
};

class CompositionNotifySink
{
public:
  virtual void DidComposite() = 0;
  virtual ~CompositionNotifySink() {}
};

















class ImageFactory
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(ImageFactory)
protected:
  friend class ImageContainer;

  ImageFactory() {}
  virtual ~ImageFactory() {}

  virtual already_AddRefed<Image> CreateImage(ImageFormat aFormat,
                                              const gfx::IntSize &aScaleHint,
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
  gfx::IntSize mSize;
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

























class ImageContainer : public SupportsWeakPtr<ImageContainer> {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(ImageContainer)
public:
  MOZ_DECLARE_REFCOUNTED_TYPENAME(ImageContainer)

  enum { DISABLE_ASYNC = 0x0, ENABLE_ASYNC = 0x01 };

  ImageContainer(int flag = 0);

  ~ImageContainer();

  







  already_AddRefed<Image> CreateImage(ImageFormat aFormat);

  
















  void SetCurrentImage(Image* aImage);

  


  void ClearAllImages();

  



  void ClearAllImagesExceptFront();

  





  void ClearCurrentImage();

  














  void SetCurrentImageInTransaction(Image* aImage);

  




  bool IsAsync() const;

  







  uint64_t GetAsyncContainerID() const;

  




  bool HasCurrentImage();

  










  already_AddRefed<Image> LockCurrentImage();

  



  void UnlockCurrentImage();

  



















  already_AddRefed<gfxASurface> DeprecatedGetCurrentAsSurface(gfx::IntSize* aSizeResult);

  


  TemporaryRef<gfx::SourceSurface> GetCurrentAsSourceSurface(gfx::IntSize* aSizeResult);

  


  TemporaryRef<gfx::SourceSurface> LockCurrentAsSourceSurface(gfx::IntSize* aSizeResult,
                                                              Image** aCurrentImage = nullptr);

  




  gfx::IntSize GetCurrentSize();

  






  void SetScaleHint(const gfx::IntSize& aScaleHint)
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

  



  uint32_t GetPaintCount() {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    return mPaintCount;
  }

  



  void ResetPaintCount() {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    mPaintCount = 0;
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

  
  
  
  uint32_t mPaintCount;

  
  
  TimeStamp mPaintTime;

  
  bool mPreviousImagePainted;

  
  
  
  nsRefPtr<ImageFactory> mImageFactory;

  gfx::IntSize mScaleHint;

  nsRefPtr<BufferRecycleBin> mRecycleBin;

  
  
  
  RemoteImageData *mRemoteData;

  
  
  
  CrossProcessMutex *mRemoteDataMutex;

  CompositionNotifySink *mCompositionNotifySink;

  
  
  
  
  
  
  
  ImageClient* mImageClient;
};

class AutoLockImage
{
public:
  AutoLockImage(ImageContainer *aContainer) : mContainer(aContainer) { mImage = mContainer->LockCurrentImage(); }
  AutoLockImage(ImageContainer *aContainer, RefPtr<gfx::SourceSurface> *aSurface) : mContainer(aContainer) {
    *aSurface = mContainer->LockCurrentAsSourceSurface(&mSize, getter_AddRefs(mImage));
  }
  ~AutoLockImage() { if (mContainer) { mContainer->UnlockCurrentImage(); } }

  Image* GetImage() { return mImage; }
  const gfx::IntSize &GetSize() { return mSize; }

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
  gfx::IntSize mSize;
};

struct PlanarYCbCrData {
  
  uint8_t* mYChannel;
  int32_t mYStride;
  gfx::IntSize mYSize;
  int32_t mYSkip;
  
  uint8_t* mCbChannel;
  uint8_t* mCrChannel;
  int32_t mCbCrStride;
  gfx::IntSize mCbCrSize;
  int32_t mCbSkip;
  int32_t mCrSkip;
  
  uint32_t mPicX;
  uint32_t mPicY;
  gfx::IntSize mPicSize;
  StereoMode mStereoMode;

  nsIntRect GetPictureRect() const {
    return nsIntRect(mPicX, mPicY,
                     mPicSize.width,
                     mPicSize.height);
  }

  PlanarYCbCrData()
    : mYChannel(nullptr), mYStride(0), mYSize(0, 0), mYSkip(0)
    , mCbChannel(nullptr), mCrChannel(nullptr)
    , mCbCrStride(0), mCbCrSize(0, 0) , mCbSkip(0), mCrSkip(0)
    , mPicX(0), mPicY(0), mPicSize(0, 0), mStereoMode(StereoMode::MONO)
  {}
};





































class PlanarYCbCrImage : public Image {
public:
  typedef PlanarYCbCrData Data;

  enum {
    MAX_DIMENSION = 16384
  };

  virtual ~PlanarYCbCrImage();

  



  virtual void SetData(const Data& aData);

  






  virtual void SetDataNoCopy(const Data &aData);

  


  virtual uint8_t* AllocateAndGetNewBuffer(uint32_t aSize);

  




  virtual void SetDelayedConversion(bool aDelayed) { }

  


  virtual const Data* GetData() { return &mData; }

  


  virtual uint32_t GetDataSize() { return mBufferSize; }

  virtual bool IsValid() { return !!mBufferSize; }

  virtual gfx::IntSize GetSize() { return mSize; }

  PlanarYCbCrImage(BufferRecycleBin *aRecycleBin);

  virtual SharedPlanarYCbCrImage *AsSharedPlanarYCbCrImage() { return nullptr; }

  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

  virtual size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const;

protected:
  




  void CopyData(const Data& aData);

  




  virtual uint8_t* AllocateBuffer(uint32_t aSize);

  already_AddRefed<gfxASurface> DeprecatedGetAsSurface();
  TemporaryRef<gfx::SourceSurface> GetAsSourceSurface();

  void SetOffscreenFormat(gfxImageFormat aFormat) { mOffscreenFormat = aFormat; }
  gfxImageFormat GetOffscreenFormat();

  nsAutoArrayPtr<uint8_t> mBuffer;
  uint32_t mBufferSize;
  Data mData;
  gfx::IntSize mSize;
  gfxImageFormat mOffscreenFormat;
  nsCountedRef<nsMainThreadSurfaceRef> mDeprecatedSurface;
  nsCountedRef<nsMainThreadSourceSurfaceRef> mSourceSurface;
  nsRefPtr<BufferRecycleBin> mRecycleBin;
};






class CairoImage : public Image,
                   public ISharedImage {
public:
  struct Data {
    gfxASurface* mDeprecatedSurface;
    gfx::IntSize mSize;

    
    
    RefPtr<gfx::SourceSurface> mSourceSurface;
  };

  




  void SetData(const Data& aData)
  {
    mDeprecatedSurface = aData.mDeprecatedSurface;
    mSize = aData.mSize;
    mSourceSurface = aData.mSourceSurface;
  }

  virtual TemporaryRef<gfx::SourceSurface> GetAsSourceSurface()
  {
    return mSourceSurface.get();
  }

  virtual already_AddRefed<gfxASurface> DeprecatedGetAsSurface()
  {
    nsRefPtr<gfxASurface> surface = mDeprecatedSurface.get();
    return surface.forget();
  }

  virtual ISharedImage* AsSharedImage() { return this; }
  virtual uint8_t* GetBuffer() { return nullptr; }
  virtual TextureClient* GetTextureClient(CompositableClient* aClient);

  gfx::IntSize GetSize() { return mSize; }

  CairoImage();
  ~CairoImage();

  nsCountedRef<nsMainThreadSurfaceRef> mDeprecatedSurface;
  gfx::IntSize mSize;

  
  
  nsCountedRef<nsMainThreadSourceSurfaceRef> mSourceSurface;
  nsDataHashtable<nsUint32HashKey, RefPtr<TextureClient> >  mTextureClients;
};

class RemoteBitmapImage : public Image {
public:
  RemoteBitmapImage() : Image(nullptr, ImageFormat::REMOTE_IMAGE_BITMAP) {}

  already_AddRefed<gfxASurface> DeprecatedGetAsSurface();
  TemporaryRef<gfx::SourceSurface> GetAsSourceSurface();

  gfx::IntSize GetSize() { return mSize; }

  unsigned char *mData;
  int mStride;
  gfx::IntSize mSize;
  RemoteImageData::Format mFormat;
};

} 
} 

#endif
