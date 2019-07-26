









#include "webrtc/modules/video_capture/device_info_impl.h"
#include "webrtc/modules/video_capture/video_capture_impl.h"

namespace webrtc {

namespace videocapturemodule {

class ExternalDeviceInfo : public DeviceInfoImpl {
 public:
  ExternalDeviceInfo(const int32_t id)
      : DeviceInfoImpl(id) {
  }
  virtual ~ExternalDeviceInfo() {}
  virtual uint32_t NumberOfDevices() { return 0; }
  virtual int32_t DisplayCaptureSettingsDialogBox(
      const char* ,
      const char* ,
      void* ,
      uint32_t ,
      uint32_t ) { return -1; }
  virtual int32_t GetDeviceName(
      uint32_t deviceNumber,
      char* deviceNameUTF8,
      uint32_t deviceNameLength,
      char* deviceUniqueIdUTF8,
      uint32_t deviceUniqueIdUTF8Length,
      char* productUniqueIdUTF8=0,
      uint32_t productUniqueIdUTF8Length=0) {
    return -1;
  }
  virtual int32_t CreateCapabilityMap(
      const char* deviceUniqueIdUTF8) { return 0; }
  virtual int32_t Init() { return 0; }
};

VideoCaptureModule::DeviceInfo* VideoCaptureImpl::CreateDeviceInfo(
    const int32_t id) {
  return new ExternalDeviceInfo(id);
}

}  

}  
