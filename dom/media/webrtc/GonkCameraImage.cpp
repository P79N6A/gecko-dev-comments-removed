





#include "GonkCameraImage.h"
#include "stagefright/MediaBuffer.h"

namespace mozilla {

GonkCameraImage::GonkCameraImage()
  : GrallocImage()
  , mMonitor("GonkCameraImage.Monitor")
  , mMediaBuffer(nullptr)
  , mThread(nullptr)
{
  mFormat = ImageFormat::GONK_CAMERA_IMAGE;
}

GonkCameraImage::~GonkCameraImage()
{
  ReentrantMonitorAutoEnter mon(mMonitor);
  
  MOZ_ASSERT(mMediaBuffer == nullptr);
}

nsresult
GonkCameraImage::GetBuffer(android::MediaBuffer** aBuffer)
{
  ReentrantMonitorAutoEnter mon(mMonitor);

  if (!mMediaBuffer) {
    return NS_ERROR_FAILURE;
  }

  MOZ_ASSERT(NS_GetCurrentThread() == mThread);

  *aBuffer = mMediaBuffer;
  mMediaBuffer->add_ref();

  return NS_OK;
}

bool
GonkCameraImage::HasMediaBuffer()
{
  ReentrantMonitorAutoEnter mon(mMonitor);
  return mMediaBuffer != nullptr;
}

nsresult
GonkCameraImage::SetBuffer(android::MediaBuffer* aBuffer)
{
  ReentrantMonitorAutoEnter mon(mMonitor);
  MOZ_ASSERT(!mMediaBuffer);

  mMediaBuffer = aBuffer;
  mMediaBuffer->add_ref();
  mThread = NS_GetCurrentThread();

  return NS_OK;
}

nsresult
GonkCameraImage::ClearBuffer()
{
  ReentrantMonitorAutoEnter mon(mMonitor);

  if (mMediaBuffer) {
    MOZ_ASSERT(NS_GetCurrentThread() == mThread);
    mMediaBuffer->release();
    mMediaBuffer = nullptr;
    mThread = nullptr;
  }
  return NS_OK;
}

} 


