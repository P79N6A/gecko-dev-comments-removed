















#include "base/basictypes.h"
#include "VideoUtils.h"
#include "GonkCameraHwMgr.h"
#include "GonkCameraPreview.h"

#define DOM_CAMERA_LOG_LEVEL  2
#include "CameraCommon.h"

#include "GonkIOSurfaceImage.h"

using namespace mozilla;

void
GonkCameraPreview::ReceiveFrame(mozilla::layers::GraphicBufferLocked* aBuffer)
{
  DOM_CAMERA_LOGI("%s:%d : this=%p\n", __func__, __LINE__, this);
  if (!aBuffer)
    return;

  Image::Format format = Image::GONK_IO_SURFACE;
  nsRefPtr<Image> image = mImageContainer->CreateImage(&format, 1);
  GonkIOSurfaceImage* videoImage = static_cast<GonkIOSurfaceImage*>(image.get());
  GonkIOSurfaceImage::Data data;
  data.mGraphicBuffer = aBuffer;
  data.mPicSize = gfxIntSize(mWidth, mHeight);
  videoImage->SetData(data);

  
  mVideoSegment.AppendFrame(image.forget(), 1, gfxIntSize(mWidth, mHeight));
  mInput->AppendToTrack(TRACK_VIDEO, &mVideoSegment);

  mFrameCount += 1;

  if ((mFrameCount % 10) == 0) {
    DOM_CAMERA_LOGI("%s:%d : mFrameCount = %d\n", __func__, __LINE__, mFrameCount);
  }
}

nsresult
GonkCameraPreview::StartImpl()
{
  DOM_CAMERA_LOGI("%s:%d : this=%p\n", __func__, __LINE__, this);

  



  GonkCameraHardware::SetPreviewSize(mHwHandle, mWidth, mHeight);
  GonkCameraHardware::GetPreviewSize(mHwHandle, &mWidth, &mHeight);
  SetFrameRate(GonkCameraHardware::GetFps(mHwHandle));

  if (GonkCameraHardware::StartPreview(mHwHandle) != OK) {
    DOM_CAMERA_LOGE("%s: failed to start preview\n", __func__);
    return NS_ERROR_FAILURE;
  }

  
  mFormat = GonkCameraHardware::GetPreviewFormat(mHwHandle);
  DOM_CAMERA_LOGI("preview stream is (actually!) %d x %d (w x h), %d frames per second, format %d\n", mWidth, mHeight, mFramesPerSecond, mFormat);
  return NS_OK;
}

nsresult
GonkCameraPreview::StopImpl()
{
  DOM_CAMERA_LOGI("%s:%d : this=%p\n", __func__, __LINE__, this);

  GonkCameraHardware::StopPreview(mHwHandle);
  mInput->EndTrack(TRACK_VIDEO);
  mInput->Finish();

  return NS_OK;
}
