





#include "ImageContainerChild.h"
#include "gfxSharedImageSurface.h"
#include "ShadowLayers.h"
#include "mozilla/layers/PLayers.h"
#include "mozilla/layers/SharedImageUtils.h"
#include "ImageContainer.h"
#include "GonkIOSurfaceImage.h"
#include "GrallocImages.h"
#include "mozilla/ReentrantMonitor.h"
#include "nsThreadUtils.h"

namespace mozilla {
namespace layers {





















static const unsigned int POOL_MAX_SHARED_IMAGES = 5;
static const unsigned int MAX_ACTIVE_SHARED_IMAGES = 10;





class SharedPlanarYCbCrImage : public PlanarYCbCrImage
{
  friend class mozilla::layers::ImageContainerChild;
public:
  SharedPlanarYCbCrImage(ImageContainerChild* aProtocol)
  : PlanarYCbCrImage(nullptr), mImageDataAllocator(aProtocol), mSent(false)
  {
    MOZ_COUNT_CTOR(SharedPlanarYCbCrImage);
  }

  ~SharedPlanarYCbCrImage()
  {
    if (mYSurface) {
      SharedImage* sharedImg = new SharedImage();
      ToSharedImage(sharedImg);
      mImageDataAllocator->RecycleSharedImage(sharedImg);
    }
    MOZ_COUNT_DTOR(SharedPlanarYCbCrImage);
  }

  




  bool IsSent() const
  {
    return mSent;
  }

  SharedPlanarYCbCrImage* AsSharedPlanarYCbCrImage() MOZ_OVERRIDE
  {
    return this;
  }

  void ToSharedImage(SharedImage* aImage)
  {
    NS_ABORT_IF_FALSE(aImage, "aImage must be allocated");
    *aImage = YUVImage(mYSurface->GetShmem(),
                      mCbSurface->GetShmem(),
                      mCrSurface->GetShmem(),
                      mData.GetPictureRect(),
                      reinterpret_cast<uintptr_t>(this));
  }

  



  already_AddRefed<gfxASurface> GetAsSurface()
  {
    if (!mYSurface) {
      return nullptr;
    }
    return PlanarYCbCrImage::GetAsSurface();
  }

  



  virtual void Allocate(Data& aData) MOZ_OVERRIDE
  {
    
    
    
    mData = aData;
    
    
    mImageDataAllocator->AllocateSharedBufferForImage(this);
    
    aData.mYChannel  = mYSurface->Data();
    aData.mCbChannel = mCbSurface->Data();
    aData.mCrChannel = mCrSurface->Data();
  }

  


  virtual void SetData(const Data& aData)
  {
    
    
    
    mData = aData;
    mBufferSize = mData.mCbCrStride * mData.mCbCrSize.height * 2 +
                   mData.mYStride * mData.mYSize.height;
    mSize = mData.mPicSize;

    
    
    
    if (!mYSurface) {
      Data data = aData;
      Allocate(data);
    }
    if (mYSurface->Data() != aData.mYChannel) {
      ImageContainer::CopyPlane(mYSurface->Data(), aData.mYChannel,
                                aData.mYSize,
                                aData.mYStride, aData.mYSize.width,
                                0,0);
      ImageContainer::CopyPlane(mCbSurface->Data(), aData.mCbChannel,
                                aData.mCbCrSize,
                                aData.mCbCrStride, aData.mCbCrSize.width,
                                0,0);
      ImageContainer::CopyPlane(mCrSurface->Data(), aData.mCrChannel,
                                aData.mCbCrSize,
                                aData.mCbCrStride, aData.mCbCrSize.width,
                                0,0);
    }
  }

  void MarkAsSent() {
    mSent = true;
  }

protected:
  nsRefPtr<gfxSharedImageSurface> mYSurface;
  nsRefPtr<gfxSharedImageSurface> mCbSurface;
  nsRefPtr<gfxSharedImageSurface> mCrSurface;
  nsRefPtr<ImageContainerChild>   mImageDataAllocator;
  bool mSent;
};


ImageContainerChild::ImageContainerChild()
: mImageContainerID(0), mActiveImageCount(0),
  mStop(false), mDispatchedDestroy(false) 
{
  MOZ_COUNT_CTOR(ImageContainerChild);
  
  
  AddRef();
}

ImageContainerChild::~ImageContainerChild()
{
  MOZ_COUNT_DTOR(ImageContainerChild);
}

void ImageContainerChild::DispatchStop()
{
  GetMessageLoop()->PostTask(FROM_HERE,
                  NewRunnableMethod(this, &ImageContainerChild::StopChildAndParent));
}

void ImageContainerChild::SetIdleNow() 
{
  if (mStop) return;

  SendFlush();
  ClearSharedImagePool();
}

void ImageContainerChild::DispatchSetIdle()
{
  if (mStop) return;

  GetMessageLoop()->PostTask(FROM_HERE, 
                    NewRunnableMethod(this, &ImageContainerChild::SetIdleNow));
}

void ImageContainerChild::StopChildAndParent()
{
  if (mStop) {
    return;
  }
  mStop = true;    

  SendStop(); 
  DispatchDestroy();
}

void ImageContainerChild::StopChild()
{
  if (mStop) {
    return;
  }
  mStop = true;

  DispatchDestroy();
}

bool ImageContainerChild::RecvReturnImage(const SharedImage& aImage)
{
  
  if (mImageQueue.Length() > 0) {
    mImageQueue.RemoveElementAt(0);
  }

  if (aImage.type() == SharedImage::TYUVImage &&
      aImage.get_YUVImage().imageID() != 0) {
    
    
    
    
    return true;
  }

  SharedImage* img = new SharedImage(aImage);
  RecycleSharedImageNow(img);

  return true;
}

void ImageContainerChild::DestroySharedImage(const SharedImage& aImage)
{
  NS_ABORT_IF_FALSE(InImageBridgeChildThread(),
                    "Should be in ImageBridgeChild thread.");

  --mActiveImageCount;
  DeallocSharedImageData(this, aImage);
}

bool ImageContainerChild::CopyDataIntoSharedImage(Image* src, SharedImage* dest)
{
  if ((src->GetFormat() == PLANAR_YCBCR) && 
      (dest->type() == SharedImage::TYUVImage)) {
    PlanarYCbCrImage *YCbCrImage = static_cast<PlanarYCbCrImage*>(src);
    const PlanarYCbCrImage::Data *data = YCbCrImage->GetData();
    NS_ASSERTION(data, "Must be able to retrieve yuv data from image!");
    YUVImage& yuv = dest->get_YUVImage();

    nsRefPtr<gfxSharedImageSurface> surfY =
      gfxSharedImageSurface::Open(yuv.Ydata());
    nsRefPtr<gfxSharedImageSurface> surfU =
      gfxSharedImageSurface::Open(yuv.Udata());
    nsRefPtr<gfxSharedImageSurface> surfV =
      gfxSharedImageSurface::Open(yuv.Vdata());

    for (int i = 0; i < data->mYSize.height; i++) {
      memcpy(surfY->Data() + i * surfY->Stride(),
             data->mYChannel + i * data->mYStride,
             data->mYSize.width);
    }
    for (int i = 0; i < data->mCbCrSize.height; i++) {
      memcpy(surfU->Data() + i * surfU->Stride(),
             data->mCbChannel + i * data->mCbCrStride,
             data->mCbCrSize.width);
      memcpy(surfV->Data() + i * surfV->Stride(),
             data->mCrChannel + i * data->mCbCrStride,
             data->mCbCrSize.width);
    }

    return true;
  }
  return false; 
}

SharedImage* ImageContainerChild::CreateSharedImageFromData(Image* image)
{
  NS_ABORT_IF_FALSE(InImageBridgeChildThread(),
                  "Should be in ImageBridgeChild thread.");
  
  ++mActiveImageCount;

  if (image->GetFormat() == PLANAR_YCBCR ) {
    PlanarYCbCrImage *YCbCrImage = static_cast<PlanarYCbCrImage*>(image);
    const PlanarYCbCrImage::Data *data = YCbCrImage->GetData();
    NS_ASSERTION(data, "Must be able to retrieve yuv data from image!");
    
    nsRefPtr<gfxSharedImageSurface> tempBufferY;
    nsRefPtr<gfxSharedImageSurface> tempBufferU;
    nsRefPtr<gfxSharedImageSurface> tempBufferV;
    
    if (!AllocateSharedBuffer(this, data->mYSize, gfxASurface::CONTENT_ALPHA,
                              getter_AddRefs(tempBufferY)) ||
        !AllocateSharedBuffer(this, data->mCbCrSize, gfxASurface::CONTENT_ALPHA,
                              getter_AddRefs(tempBufferU)) ||
        !AllocateSharedBuffer(this, data->mCbCrSize, gfxASurface::CONTENT_ALPHA,
                              getter_AddRefs(tempBufferV))) {
      NS_RUNTIMEABORT("creating SharedImage failed!");
    }

    for (int i = 0; i < data->mYSize.height; i++) {
      memcpy(tempBufferY->Data() + i * tempBufferY->Stride(),
             data->mYChannel + i * data->mYStride,
             data->mYSize.width);
    }
    for (int i = 0; i < data->mCbCrSize.height; i++) {
      memcpy(tempBufferU->Data() + i * tempBufferU->Stride(),
             data->mCbChannel + i * data->mCbCrStride,
             data->mCbCrSize.width);
      memcpy(tempBufferV->Data() + i * tempBufferV->Stride(),
             data->mCrChannel + i * data->mCbCrStride,
             data->mCbCrSize.width);
    }

    SharedImage* result = new SharedImage( 
              YUVImage(tempBufferY->GetShmem(),
                       tempBufferU->GetShmem(),
                       tempBufferV->GetShmem(),
                       data->GetPictureRect(),
                       0));
    NS_ABORT_IF_FALSE(result->type() == SharedImage::TYUVImage,
                      "SharedImage type not set correctly");
    return result;
#ifdef MOZ_WIDGET_GONK
  } else if (image->GetFormat() == GONK_IO_SURFACE) {
    GonkIOSurfaceImage* gonkImage = static_cast<GonkIOSurfaceImage*>(image);
    SharedImage* result = new SharedImage(gonkImage->GetSurfaceDescriptor());
    return result;
  } else if (image->GetFormat() == GRALLOC_PLANAR_YCBCR) {
    GrallocPlanarYCbCrImage* GrallocImage = static_cast<GrallocPlanarYCbCrImage*>(image);
    SharedImage* result = new SharedImage(GrallocImage->GetSurfaceDescriptor());
    return result;
#endif
  } else {
    NS_RUNTIMEABORT("TODO: Only YUVImage is supported here right now.");
  }
  return nullptr;
}

bool ImageContainerChild::AddSharedImageToPool(SharedImage* img)
{
  NS_ABORT_IF_FALSE(InImageBridgeChildThread(), 
                    "AddSharedImageToPool must be called in the ImageBridgeChild thread");
  if (mStop) {
    return false;
  }

  if (mSharedImagePool.Length() >= POOL_MAX_SHARED_IMAGES) {
    return false;
  }
  if (img->type() == SharedImage::TYUVImage) {
    mSharedImagePool.AppendElement(img);
    return true;
  }
  return false; 
}

static bool
SharedImageCompatibleWith(SharedImage* aSharedImage, Image* aImage)
{
  
  switch (aImage->GetFormat()) {
  case PLANAR_YCBCR: {
    if (aSharedImage->type() != SharedImage::TYUVImage) {
      return false;
    }
    const PlanarYCbCrImage::Data* data =
      static_cast<PlanarYCbCrImage*>(aImage)->GetData();
    const YUVImage& yuv = aSharedImage->get_YUVImage();

    nsRefPtr<gfxSharedImageSurface> surfY =
      gfxSharedImageSurface::Open(yuv.Ydata());
    if (surfY->GetSize() != data->mYSize) {
      return false;
    }

    nsRefPtr<gfxSharedImageSurface> surfU =
      gfxSharedImageSurface::Open(yuv.Udata());
    if (surfU->GetSize() != data->mCbCrSize) {
      return false;
    }
    return true;
  }
  default:
    return false;
  }
}

SharedImage*
ImageContainerChild::GetSharedImageFor(Image* aImage)
{
  while (mSharedImagePool.Length() > 0) {
    
    nsAutoPtr<SharedImage> img(mSharedImagePool.LastElement());
    mSharedImagePool.RemoveElementAt(mSharedImagePool.Length() - 1);

    if (SharedImageCompatibleWith(img, aImage)) {
      return img.forget();
    }
    
    DeallocSharedImageData(this, *img);
  }
  
  return nullptr;
}

void ImageContainerChild::ClearSharedImagePool()
{
  NS_ABORT_IF_FALSE(InImageBridgeChildThread(),
                    "Should be in ImageBridgeChild thread.");
  for(unsigned int i = 0; i < mSharedImagePool.Length(); ++i) {
    DeallocSharedImageData(this, *mSharedImagePool[i]);
  }
  mSharedImagePool.Clear();
}

class ImageBridgeCopyAndSendTask : public Task
{
public:
  ImageBridgeCopyAndSendTask(ImageContainerChild * child, 
                             ImageContainer * aContainer, 
                             Image * aImage)
  : mChild(child), mImageContainer(aContainer), mImage(aImage) {}

  void Run()
  { 
    SharedImage* img = mChild->ImageToSharedImage(mImage.get());
    if (img) {
      mChild->SendPublishImage(*img);
    }
  }

  ImageContainerChild *mChild;
  nsRefPtr<ImageContainer> mImageContainer;
  nsRefPtr<Image> mImage;
};

SharedImage* ImageContainerChild::ImageToSharedImage(Image* aImage)
{
  if (mStop) {
    return nullptr;
  }

  NS_ABORT_IF_FALSE(InImageBridgeChildThread(),
                    "Should be in ImageBridgeChild thread.");
  if (aImage->GetFormat() == ImageFormat::PLANAR_YCBCR) {
    SharedPlanarYCbCrImage* sharedYCbCr =
      static_cast<PlanarYCbCrImage*>(aImage)->AsSharedPlanarYCbCrImage();
    if (sharedYCbCr) {
      if (sharedYCbCr->IsSent()) {
        
        return nullptr;
      }
      
      
      SharedImage* result = new SharedImage();
      sharedYCbCr->ToSharedImage(result);
      sharedYCbCr->MarkAsSent();
      return result;
    }
  }

  if (mActiveImageCount > (int)MAX_ACTIVE_SHARED_IMAGES) {
    
    
    return nullptr;
  }

  SharedImage *img = GetSharedImageFor(aImage);
  if (img) {
    CopyDataIntoSharedImage(aImage, img);  
  } else {
    img = CreateSharedImageFromData(aImage);
  }
  
  
  mImageQueue.AppendElement(aImage);
  return img;
}

void ImageContainerChild::SendImageAsync(ImageContainer* aContainer,
                                         Image* aImage)
{
  if(!aContainer || !aImage) {
      return;
  }

  if (mStop) {
    return;
  }

  if (InImageBridgeChildThread()) {
    SharedImage *img = ImageToSharedImage(aImage);
    if (img) {
      SendPublishImage(*img);
    }
    delete img;
    return;
  }

  
  
  Task *t = new ImageBridgeCopyAndSendTask(this, aContainer, aImage);   
  GetMessageLoop()->PostTask(FROM_HERE, t);
}

void ImageContainerChild::DestroyNow()
{
  NS_ABORT_IF_FALSE(InImageBridgeChildThread(),
                    "Should be in ImageBridgeChild thread.");
  NS_ABORT_IF_FALSE(mDispatchedDestroy,
                    "Incorrect state in the destruction sequence.");

  ClearSharedImagePool();

  
  Send__delete__(this);
  Release(); 
}

void ImageContainerChild::DispatchDestroy()
{
  NS_ABORT_IF_FALSE(mStop, "The state should be 'stopped' when destroying");

  if (mDispatchedDestroy) {
    return;
  }
  mDispatchedDestroy = true;
  AddRef(); 
  GetMessageLoop()->PostTask(FROM_HERE, 
                    NewRunnableMethod(this, &ImageContainerChild::DestroyNow));
}


already_AddRefed<Image> ImageContainerChild::CreateImage(const ImageFormat *aFormats,
                                                         uint32_t aNumFormats)
{
  
  nsRefPtr<Image> img = new SharedPlanarYCbCrImage(this);
  return img.forget();
}

void ImageContainerChild::AllocateSharedBufferForImageNow(Image* aImage)
{
  NS_ABORT_IF_FALSE(InImageBridgeChildThread(),
                    "Should be in ImageBridgeChild thread.");
  NS_ABORT_IF_FALSE(aImage && aImage->GetFormat() == ImageFormat::PLANAR_YCBCR,
                    "Only YUV images supported now");

  SharedPlanarYCbCrImage* sharedYCbCr =
    static_cast<PlanarYCbCrImage*>(aImage)->AsSharedPlanarYCbCrImage();

  
  SharedImage* fromPool = GetSharedImageFor(aImage);
  if (fromPool) {
    YUVImage yuv = fromPool->get_YUVImage();
    nsRefPtr<gfxSharedImageSurface> surfY =
      gfxSharedImageSurface::Open(yuv.Ydata());
    nsRefPtr<gfxSharedImageSurface> surfU =
      gfxSharedImageSurface::Open(yuv.Udata());
    nsRefPtr<gfxSharedImageSurface> surfV =
      gfxSharedImageSurface::Open(yuv.Vdata());
    sharedYCbCr->mYSurface  = surfY;
    sharedYCbCr->mCbSurface = surfU;
    sharedYCbCr->mCrSurface = surfV;
  } else {
    
    nsRefPtr<gfxSharedImageSurface> surfY;
    nsRefPtr<gfxSharedImageSurface> surfU;
    nsRefPtr<gfxSharedImageSurface> surfV;
    const PlanarYCbCrImage::Data* data = sharedYCbCr->GetData();
    if (!AllocateSharedBuffer(this, data->mYSize, gfxASurface::CONTENT_ALPHA,
                              getter_AddRefs(surfY)) ||
        !AllocateSharedBuffer(this, data->mCbCrSize, gfxASurface::CONTENT_ALPHA,
                              getter_AddRefs(surfU)) ||
        !AllocateSharedBuffer(this, data->mCbCrSize, gfxASurface::CONTENT_ALPHA,
                              getter_AddRefs(surfV))) {
      NS_RUNTIMEABORT("creating SharedImage failed!");
    }
    sharedYCbCr->mYSurface  = surfY;
    sharedYCbCr->mCbSurface = surfU;
    sharedYCbCr->mCrSurface = surfV;
  }

}

void ImageContainerChild::AllocateSharedBufferForImageSync(ReentrantMonitor* aBarrier,
                                                           bool* aDone,
                                                           Image* aImage)
{
  AllocateSharedBufferForImageNow(aImage);
  ReentrantMonitorAutoEnter autoMon(*aBarrier);
  *aDone = true;
  aBarrier->NotifyAll();
}


void ImageContainerChild::AllocateSharedBufferForImage(Image* aImage)
{
  
  if (InImageBridgeChildThread()) {
    AllocateSharedBufferForImageNow(aImage);
    return;
  }

  bool done = false;
  ReentrantMonitor barrier("ImageBridgeChild::AllocateSharedBufferForImage");
  ReentrantMonitorAutoEnter autoMon(barrier);
  GetMessageLoop()->PostTask(FROM_HERE, 
                             NewRunnableMethod(this,
                                               &ImageContainerChild::AllocateSharedBufferForImageSync,
                                               &barrier,
                                               &done,
                                               aImage));
  while (!done) {
    barrier.Wait();
  }
}

void ImageContainerChild::RecycleSharedImageNow(SharedImage* aImage)
{
  NS_ABORT_IF_FALSE(InImageBridgeChildThread(),"Must be in the ImageBridgeChild Thread.");
  
  if (mStop || !AddSharedImageToPool(aImage)) {
    DestroySharedImage(*aImage);
    delete aImage;
  }
}

void ImageContainerChild::RecycleSharedImage(SharedImage* aImage)
{
  if (InImageBridgeChildThread()) {
    RecycleSharedImageNow(aImage);
    return;
  }
  GetMessageLoop()->PostTask(FROM_HERE, 
                             NewRunnableMethod(this,
                                               &ImageContainerChild::RecycleSharedImageNow,
                                               aImage));
}

} 
} 
