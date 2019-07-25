















#ifndef DOM_CAMERA_GONKCAMERAPREVIEW_H
#define DOM_CAMERA_GONKCAMERAPREVIEW_H

#include "CameraPreview.h"

#define DOM_CAMERA_LOG_LEVEL  3
#include "CameraCommon.h"

namespace mozilla {
namespace layers {
class GraphicBufferLocked;
} 
} 

namespace mozilla {

class GonkCameraPreview : public CameraPreview
{
public:
  GonkCameraPreview(nsIThread* aCameraThread, uint32_t aHwHandle, uint32_t aWidth, uint32_t aHeight)
    : CameraPreview(aCameraThread, aWidth, aHeight)
    , mHwHandle(aHwHandle)
    , mDiscardedFrameCount(0)
    , mFormat(GonkCameraHardware::PREVIEW_FORMAT_UNKNOWN)
  { }

  void ReceiveFrame(layers::GraphicBufferLocked* aBuffer);

  nsresult StartImpl();
  nsresult StopImpl();

protected:
  ~GonkCameraPreview()
  {
    Stop();
  }

  uint32_t mHwHandle;
  uint32_t mDiscardedFrameCount;
  uint32_t mFormat;

private:
  GonkCameraPreview(const GonkCameraPreview&) MOZ_DELETE;
  GonkCameraPreview& operator=(const GonkCameraPreview&) MOZ_DELETE;
};

} 

#endif 
