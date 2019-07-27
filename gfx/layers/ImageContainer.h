




#ifndef GFX_IMAGECONTAINER_H
#define GFX_IMAGECONTAINER_H

#include <stdint.h>                     
#include <sys/types.h>                  
#include "gfxTypes.h"
#include "ImageTypes.h"                 
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
#include "mozilla/EnumeratedArray.h"

#ifndef XPCOM_GLUE_AVOID_NSPR












class nsMainThreadSourceSurfaceRef;

template <>
class nsAutoRefTraits<nsMainThreadSourceSurfaceRef> {
public:
  typedef mozilla::gfx::SourceSurface* RawRef;

  


  class SurfaceReleaser : public nsRunnable {
  public:
    explicit SurfaceReleaser(RawRef aRef) : mRef(aRef) {}
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


namespace layers {

class ImageClient;
class SharedPlanarYCbCrImage;
class TextureClient;
class CompositableClient;
class GrallocImage;

struct ImageBackendData
{
  virtual ~ImageBackendData() {}

protected:
  ImageBackendData() {}
};















class Image {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(Image)

public:
  ImageFormat GetFormat() { return mFormat; }
  void* GetImplData() { return mImplData; }

  virtual gfx::IntSize GetSize() = 0;
  virtual nsIntRect GetPictureRect()
  {
    return nsIntRect(0, 0, GetSize().width, GetSize().height);
  }

  ImageBackendData* GetBackendData(LayersBackend aBackend)
  { return mBackendData[aBackend]; }
  void SetBackendData(LayersBackend aBackend, ImageBackendData* aData)
  { mBackendData[aBackend] = aData; }

  int32_t GetSerial() { return mSerial; }

  void MarkSent() { mSent = true; }
  bool IsSentToCompositor() { return mSent; }

  virtual TemporaryRef<gfx::SourceSurface> GetAsSourceSurface() = 0;

  virtual GrallocImage* AsGrallocImage()
  {
    return nullptr;
  }

  virtual bool IsValid() { return true; }

  virtual uint8_t* GetBuffer() { return nullptr; }

  



  virtual TextureClient* GetTextureClient(CompositableClient* aClient) { return nullptr; }

protected:
  Image(void* aImplData, ImageFormat aFormat) :
    mImplData(aImplData),
    mSerial(++sSerialCounter),
    mFormat(aFormat),
    mSent(false)
  {}

  
  virtual ~Image() {}

  mozilla::EnumeratedArray<mozilla::layers::LayersBackend,
                           mozilla::layers::LayersBackend::LAYERS_LAST,
                           nsAutoPtr<ImageBackendData>>
    mBackendData;

  void* mImplData;
  int32_t mSerial;
  ImageFormat mFormat;
  static mozilla::Atomic<int32_t> sSerialCounter;
  bool mSent;
};








class BufferRecycleBin final {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(BufferRecycleBin)

  

public:
  BufferRecycleBin();

  void RecycleBuffer(uint8_t* aBuffer, uint32_t aSize);
  
  uint8_t* GetBuffer(uint32_t aSize);

private:
  typedef mozilla::Mutex Mutex;

  
  ~BufferRecycleBin()
  {
  }

  
  
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
 






















class ImageContainer final : public SupportsWeakPtr<ImageContainer> {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(ImageContainer)
public:
  MOZ_DECLARE_REFCOUNTED_TYPENAME(ImageContainer)

  enum { DISABLE_ASYNC = 0x0, ENABLE_ASYNC = 0x01 };

  explicit ImageContainer(int flag = 0);

  







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

  ImageFactory* GetImageFactory() const
  {
    return mImageFactory;
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

private:
  typedef mozilla::ReentrantMonitor ReentrantMonitor;

  
  ~ImageContainer();

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

  CompositionNotifySink *mCompositionNotifySink;

  
  
  
  
  
  
  
  ImageClient* mImageClient;
};

class AutoLockImage
{
public:
  explicit AutoLockImage(ImageContainer *aContainer) : mContainer(aContainer) { mImage = mContainer->LockCurrentImage(); }
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

  explicit PlanarYCbCrImage(BufferRecycleBin *aRecycleBin);

  virtual SharedPlanarYCbCrImage *AsSharedPlanarYCbCrImage() { return nullptr; }

  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

  virtual size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const;

protected:
  




  void CopyData(const Data& aData);

  




  virtual uint8_t* AllocateBuffer(uint32_t aSize);

  TemporaryRef<gfx::SourceSurface> GetAsSourceSurface();

  void SetOffscreenFormat(gfxImageFormat aFormat) { mOffscreenFormat = aFormat; }
  gfxImageFormat GetOffscreenFormat();

  nsAutoArrayPtr<uint8_t> mBuffer;
  uint32_t mBufferSize;
  Data mData;
  gfx::IntSize mSize;
  gfxImageFormat mOffscreenFormat;
  nsCountedRef<nsMainThreadSourceSurfaceRef> mSourceSurface;
  nsRefPtr<BufferRecycleBin> mRecycleBin;
};






class CairoImage final : public Image {
public:
  struct Data {
    gfx::IntSize mSize;
    RefPtr<gfx::SourceSurface> mSourceSurface;
  };

  




  void SetData(const Data& aData)
  {
    mSize = aData.mSize;
    mSourceSurface = aData.mSourceSurface;
  }

  virtual TemporaryRef<gfx::SourceSurface> GetAsSourceSurface() override
  {
    return mSourceSurface.get();
  }

  virtual TextureClient* GetTextureClient(CompositableClient* aClient) override;

  virtual gfx::IntSize GetSize() override { return mSize; }

  CairoImage();
  ~CairoImage();

  gfx::IntSize mSize;

  nsCountedRef<nsMainThreadSourceSurfaceRef> mSourceSurface;
  nsDataHashtable<nsUint32HashKey, RefPtr<TextureClient> >  mTextureClients;
};

#ifdef MOZ_WIDGET_GONK
class OverlayImage : public Image {
  





public:
  struct Data {
    int32_t mOverlayId;
    gfx::IntSize mSize;
  };

  OverlayImage() : Image(nullptr, ImageFormat::OVERLAY_IMAGE) { mOverlayId = INVALID_OVERLAY; }

  void SetData(const Data& aData)
  {
    mOverlayId = aData.mOverlayId;
    mSize = aData.mSize;
  }

  TemporaryRef<gfx::SourceSurface> GetAsSourceSurface() { return nullptr; } ;
  int32_t GetOverlayId() { return mOverlayId; }

  gfx::IntSize GetSize() { return mSize; }

private:
  int32_t mOverlayId;
  gfx::IntSize mSize;
};
#endif

} 
} 

#endif
