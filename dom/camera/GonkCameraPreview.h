















#ifndef DOM_CAMERA_GONKCAMERAPREVIEW_H
#define DOM_CAMERA_GONKCAMERAPREVIEW_H

#include "CameraPreview.h"

#define DOM_CAMERA_LOG_LEVEL  3
#include "CameraCommon.h"

namespace mozilla {

class GonkCameraPreview : public CameraPreview
{
public:
  GonkCameraPreview(PRUint32 aHwHandle, PRUint32 aWidth, PRUint32 aHeight)
    : CameraPreview(aWidth, aHeight)
    , mHwHandle(aHwHandle)
    , mDiscardedFrameCount(0)
    , mFormat(GonkCameraHardware::PREVIEW_FORMAT_UNKNOWN)
  { }

  void ReceiveFrame(PRUint8 *aData, PRUint32 aLength);

  void Start();
  void Stop();

protected:
  ~GonkCameraPreview()
  {
    Stop();
  }

  PRUint32 mHwHandle;
  PRUint32 mDiscardedFrameCount;
  PRUint32 mFormat;

private:
  GonkCameraPreview(const GonkCameraPreview&) MOZ_DELETE;
  GonkCameraPreview& operator=(const GonkCameraPreview&) MOZ_DELETE;
};

} 

#endif 
