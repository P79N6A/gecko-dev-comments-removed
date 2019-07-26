





#include "ImageContainerChild.h"
#include "gfxSharedImageSurface.h"
#include "ShadowLayers.h"
#include "mozilla/layers/PLayers.h"
#include "mozilla/layers/SharedImageUtils.h"
#include "ImageContainer.h"
#include "GonkIOSurfaceImage.h"
#include "GrallocImages.h"
#include "mozilla/layers/ShmemYCbCrImage.h"

using namespace mozilla::ipc;

namespace mozilla {
namespace layers {





















static const unsigned int POOL_MAX_SHARED_IMAGES = 5;
static const unsigned int MAX_ACTIVE_SHARED_IMAGES = 10;

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
  mImageQueue.Clear();
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
  SharedImage* img = new SharedImage(aImage);
  
  if (mImageQueue.Length() > 0) {
    mImageQueue.RemoveElementAt(0);
  }
  if (!AddSharedImageToPool(img) || mStop) {
    DestroySharedImage(*img);
    delete img;
  }
  return true;
}

void ImageContainerChild::DestroySharedImage(const SharedImage& aImage)
{
  NS_ABORT_IF_FALSE(InImageBridgeChildThread(),
                    "Should be in ImageBridgeChild thread.");

  --mActiveImageCount;
  DeallocSharedImageData(this, aImage);
}

SharedImage* ImageContainerChild::AsSharedImage(Image* aImage)
{
#ifdef MOZ_WIDGET_GONK
  if (aImage->GetFormat() == GONK_IO_SURFACE) {
    GonkIOSurfaceImage* gonkImage = static_cast<GonkIOSurfaceImage*>(aImage);
    SharedImage* result = new SharedImage(gonkImage->GetSurfaceDescriptor());
    return result;
  } else if (aImage->GetFormat() == GRALLOC_PLANAR_YCBCR) {
    GrallocPlanarYCbCrImage* GrallocImage = static_cast<GrallocPlanarYCbCrImage*>(aImage);
    SharedImage* result = new SharedImage(GrallocImage->GetSurfaceDescriptor());
    return result;
  }
#endif
  return nullptr; 
}

bool ImageContainerChild::CopyDataIntoSharedImage(Image* src, SharedImage* dest)
{
  if ((src->GetFormat() == PLANAR_YCBCR) && 
      (dest->type() == SharedImage::TYCbCrImage)) {
    PlanarYCbCrImage *planarYCbCrImage = static_cast<PlanarYCbCrImage*>(src);
    const PlanarYCbCrImage::Data *data = planarYCbCrImage->GetData();
    NS_ASSERTION(data, "Must be able to retrieve yuv data from image!");
    YCbCrImage& yuv = dest->get_YCbCrImage();

    ShmemYCbCrImage shmemImage(yuv.data(), yuv.offset());

    if (!shmemImage.CopyData(data->mYChannel, data->mCbChannel, data->mCrChannel,
                             data->mYSize, data->mYStride,
                             data->mCbCrSize, data->mCbCrStride)) {
      NS_WARNING("Failed to copy image data!");
      return false;
    }
    return true;
  }
  return false; 
}

SharedImage* ImageContainerChild::AllocateSharedImageFor(Image* image)
{
  NS_ABORT_IF_FALSE(InImageBridgeChildThread(),
                  "Should be in ImageBridgeChild thread.");
  if (!image) {
    return nullptr;
  }

  if (image->GetFormat() == PLANAR_YCBCR ) {
    PlanarYCbCrImage *planarYCbCrImage = static_cast<PlanarYCbCrImage*>(image);
    const PlanarYCbCrImage::Data *data = planarYCbCrImage->GetData();
    NS_ASSERTION(data, "Must be able to retrieve yuv data from image!");
    if (!data) {
      return nullptr;
    }

    SharedMemory::SharedMemoryType shmType = OptimalShmemType();
    size_t size = ShmemYCbCrImage::ComputeMinBufferSize(data->mYSize,
                                                        data->mCbCrSize);
    Shmem shmem;
    if (!AllocUnsafeShmem(size, shmType, &shmem)) {
      return nullptr;
    }

    ShmemYCbCrImage::InitializeBufferInfo(shmem.get<uint8_t>(),
                                          data->mYSize,
                                          data->mCbCrSize);
    ShmemYCbCrImage shmemImage(shmem);

    if (!shmemImage.IsValid()) {
      NS_WARNING("Failed to properly allocate image data!");
      DeallocShmem(shmem);
      return nullptr;
    }

    ++mActiveImageCount;
    return new SharedImage(YCbCrImage(shmem, 0, data->GetPictureRect()));
  } else {
    NS_RUNTIMEABORT("TODO: Only YCbCrImage is supported here right now.");
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
  if (img->type() == SharedImage::TYCbCrImage) {
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
    if (aSharedImage->type() != SharedImage::TYCbCrImage) {
      return false;
    }
    const PlanarYCbCrImage::Data* data =
      static_cast<PlanarYCbCrImage*>(aImage)->GetData();
    const YCbCrImage& yuv = aSharedImage->get_YCbCrImage();

    ShmemYCbCrImage shmImg(yuv.data(),yuv.offset());

    if (shmImg.GetYSize() != data->mYSize) {
      return false;
    }
    if (shmImg.GetCbCrSize() != data->mCbCrSize) {
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

void ImageContainerChild::SendImageNow(Image* aImage)
{
  NS_ABORT_IF_FALSE(InImageBridgeChildThread(),
                    "Should be in ImageBridgeChild thread.");

  if (mStop) {
    return;
  }

  bool needsCopy = false;
  
  SharedImage* img = AsSharedImage(aImage);
  if (!img) {
    needsCopy = true;
    
    img = GetSharedImageFor(aImage);
    if (!img && mActiveImageCount < (int)MAX_ACTIVE_SHARED_IMAGES) {
      
      img = AllocateSharedImageFor(aImage);
    }
  }

  if (img && (!needsCopy || CopyDataIntoSharedImage(aImage, img))) {
    
    
    mImageQueue.AppendElement(aImage);
    SendPublishImage(*img);
  } else {
    NS_WARNING("Failed to send an image to the compositor");
  }
  delete img;
  return;
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
    mChild->SendImageNow(mImage);
  }

  ImageContainerChild* mChild;
  nsRefPtr<ImageContainer> mImageContainer;
  nsRefPtr<Image> mImage;
};

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
    SendImageNow(aImage);
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
  mImageQueue.Clear();

  
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

} 
} 

