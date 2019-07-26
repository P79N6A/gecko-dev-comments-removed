









#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_IOS_DEVICE_INFO_IOS_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_IOS_DEVICE_INFO_IOS_H_

#include "webrtc/modules/video_capture/device_info_impl.h"

namespace webrtc {
namespace videocapturemodule {
class DeviceInfoIos : public DeviceInfoImpl {
 public:
  explicit DeviceInfoIos(const int32_t device_id);
  virtual ~DeviceInfoIos();

  
  virtual int32_t Init() OVERRIDE;
  virtual uint32_t NumberOfDevices() OVERRIDE;
  virtual int32_t GetDeviceName(
      uint32_t deviceNumber,
      char* deviceNameUTF8,
      uint32_t deviceNameLength,
      char* deviceUniqueIdUTF8,
      uint32_t deviceUniqueIdUTF8Length,
      char* productUniqueIdUTF8 = 0,
      uint32_t productUniqueIdUTF8Length = 0) OVERRIDE;

  virtual int32_t NumberOfCapabilities(const char* deviceUniqueIdUTF8) OVERRIDE;

  virtual int32_t GetCapability(const char* deviceUniqueIdUTF8,
                                const uint32_t deviceCapabilityNumber,
                                VideoCaptureCapability& capability) OVERRIDE;

  virtual int32_t GetBestMatchedCapability(
      const char* deviceUniqueIdUTF8,
      const VideoCaptureCapability& requested,
      VideoCaptureCapability& resulting) OVERRIDE;

  virtual int32_t DisplayCaptureSettingsDialogBox(
      const char* deviceUniqueIdUTF8,
      const char* dialogTitleUTF8,
      void* parentWindow,
      uint32_t positionX,
      uint32_t positionY) OVERRIDE;

  virtual int32_t GetOrientation(const char* deviceUniqueIdUTF8,
                                 VideoCaptureRotation& orientation) OVERRIDE;

  virtual int32_t CreateCapabilityMap(
      const char* device_unique_id_utf8) OVERRIDE;
};

}  
}  

#endif  
