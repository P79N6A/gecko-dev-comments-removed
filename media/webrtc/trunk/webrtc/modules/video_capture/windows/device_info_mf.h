









#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_WINDOWS_DEVICE_INFO_MF_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_WINDOWS_DEVICE_INFO_MF_H_

#include "modules/video_capture/device_info_impl.h"

namespace webrtc {
namespace videocapturemodule {


class DeviceInfoMF : public DeviceInfoImpl {
 public:
  explicit DeviceInfoMF(const WebRtc_Word32 id);
  virtual ~DeviceInfoMF();

  WebRtc_Word32 Init();
  virtual WebRtc_UWord32 NumberOfDevices();

  virtual WebRtc_Word32 GetDeviceName(WebRtc_UWord32 deviceNumber,
      char* deviceNameUTF8, WebRtc_UWord32 deviceNameLength,
      char* deviceUniqueIdUTF8, WebRtc_UWord32 deviceUniqueIdUTF8Length,
      char* productUniqueIdUTF8, WebRtc_UWord32 productUniqueIdUTF8Length);

  virtual WebRtc_Word32 DisplayCaptureSettingsDialogBox(
      const char* deviceUniqueIdUTF8, const char* dialogTitleUTF8,
      void* parentWindow, WebRtc_UWord32 positionX, WebRtc_UWord32 positionY);
};

}  
}  

#endif  
