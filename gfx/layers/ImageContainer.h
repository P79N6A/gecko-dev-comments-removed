




#ifndef GFX_IMAGECONTAINER_H
#define GFX_IMAGECONTAINER_H

#include <stdint.h>                     
#include <sys/types.h>                  
#include "ImageTypes.h"                 
#include "gfxASurface.h"                
#include "gfxPoint.h"                   
#include "mozilla/Assertions.h"         
#include "mozilla/Mutex.h"              
#include "mozilla/ReentrantMonitor.h"   
#include "mozilla/TimeStamp.h"          
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

class nsMainThreadSurfaceRef;

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
class DeprecatedSharedPlanarYCbCrImage;
class TextureClient;
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

    



    virtual TextureClient* GetTextureClient() = 0;
};















class Image {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(Image)

public:
  virtual ~Image() {}

  virtual ISharedImage* AsSharedImage() { return nullptr; }

  ImageFormat GetFormat() { return mFormat; }
  void* GetImplData() { return mImplData; }

  virtual already_AddRefed<gfxASurface> GetAsSurface() = 0;
  virtual gfxIntSize GetSize() = 0;
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

protected:
  Image(void* aImplData, ImageFormat aFormat) :
    mImplData(aImplData),
    mSerial(++sSerialCounter),
    mFormat(aFormat),
    mSent(false)
  {}

  nsAutoPtr<ImageBackendData> mBackendData[mozilla::layers::LAYERS_LAST];

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




static inline bool
FormatInList(const ImageFormat* aFormats, uint32_t aNumFormats,
             ImageFormat aFormat)
{
  for (uint32_t i = 0; i < aNumFormats; ++i) {
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

















class ImageFactory
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(ImageFactory)
protected:
  friend class ImageContainer;

  ImageFactory() {}
  virtual ~ImageFactory() {}

  virtual already_AddRefed<Image> CreateImage(const ImageFormat* aFormats,
                                              uint32_t aNumFormats,
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

























class ImageContainer {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(ImageContainer)
public:

  enum { DISABLE_ASYNC = 0x0, ENABLE_ASYNC = 0x01 };

  ImageContainer(int flag = 0);

  ~ImageContainer();

  







  already_AddRefed<Image> CreateImage(const ImageFormat* aFormats,
                                      uint32_t aNumFormats);

  
















  void SetCurrentImage(Image* aImage);

  





  void ClearCurrentImage();

  














  void SetCurrentImageInTransaction(Image* aImage);

  




  bool IsAsync() const;

  







  uint64_t GetAsyncContainerID() const;

  




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

  gfxIntSize mScaleHint;

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





































class PlanarYCbCrImage : public Image {
public:
  struct Data {
    
    uint8_t* mYChannel;
    int32_t mYStride;
    gfxIntSize mYSize;
    int32_t mYSkip;
    
    uint8_t* mCbChannel;
    uint8_t* mCrChannel;
    int32_t mCbCrStride;
    gfxIntSize mCbCrSize;
    int32_t mCbSkip;
    int32_t mCrSkip;
    
    uint32_t mPicX;
    uint32_t mPicY;
    gfxIntSize mPicSize;
    StereoMode mStereoMode;

    nsIntRect GetPictureRect() const {
      return nsIntRect(mPicX, mPicY,
                       mPicSize.width,
                       mPicSize.height);
    }

    Data()
      : mYChannel(nullptr), mYStride(0), mYSize(0, 0), mYSkip(0)
      , mCbChannel(nullptr), mCrChannel(nullptr)
      , mCbCrStride(0), mCbCrSize(0, 0) , mCbSkip(0), mCrSkip(0)
      , mPicX(0), mPicY(0), mPicSize(0, 0), mStereoMode(STEREO_MODE_MONO)
    {}
  };

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

  virtual gfxIntSize GetSize() { return mSize; }

  PlanarYCbCrImage(BufferRecycleBin *aRecycleBin);

  virtual SharedPlanarYCbCrImage *AsSharedPlanarYCbCrImage() { return nullptr; }
  virtual DeprecatedSharedPlanarYCbCrImage *AsDeprecatedSharedPlanarYCbCrImage() { return nullptr; }

protected:
  




  void CopyData(const Data& aData);

  




  virtual uint8_t* AllocateBuffer(uint32_t aSize);

  already_AddRefed<gfxASurface> GetAsSurface();

  void SetOffscreenFormat(gfxASurface::gfxImageFormat aFormat) { mOffscreenFormat = aFormat; }
  gfxASurface::gfxImageFormat GetOffscreenFormat();

  nsAutoArrayPtr<uint8_t> mBuffer;
  uint32_t mBufferSize;
  Data mData;
  gfxIntSize mSize;
  gfxASurface::gfxImageFormat mOffscreenFormat;
  nsCountedRef<nsMainThreadSurfaceRef> mSurface;
  nsRefPtr<BufferRecycleBin> mRecycleBin;
};






class CairoImage : public Image {
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
    nsRefPtr<gfxASurface> surface = mSurface.get();
    return surface.forget();
  }

  gfxIntSize GetSize() { return mSize; }

  CairoImage() : Image(nullptr, CAIRO_SURFACE) {}

  nsCountedRef<nsMainThreadSurfaceRef> mSurface;
  gfxIntSize mSize;
};

class RemoteBitmapImage : public Image {
public:
  RemoteBitmapImage() : Image(nullptr, REMOTE_IMAGE_BITMAP) {}

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
