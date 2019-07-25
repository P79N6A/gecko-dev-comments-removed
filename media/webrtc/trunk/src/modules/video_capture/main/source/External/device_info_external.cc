









#include "../device_info_impl.h"
#include "../video_capture_impl.h"

namespace webrtc {

namespace videocapturemodule {

class ExternalDeviceInfo : public DeviceInfoImpl {
 public:
  ExternalDeviceInfo(const WebRtc_Word32 id)
      : DeviceInfoImpl(id) {
  }
  virtual ~ExternalDeviceInfo() {}
  virtual WebRtc_UWord32 NumberOfDevices() { return 0; }
  virtual WebRtc_Word32 DisplayCaptureSettingsDialogBox(
      const char* ,
      const char* ,
      void* ,
      WebRtc_UWord32 ,
      WebRtc_UWord32 ) { return -1; }
  virtual WebRtc_Word32 GetDeviceName(
      WebRtc_UWord32 deviceNumber,
      char* deviceNameUTF8,
      WebRtc_UWord32 deviceNameLength,
      char* deviceUniqueIdUTF8,
      WebRtc_UWord32 deviceUniqueIdUTF8Length,
      char* productUniqueIdUTF8=0,
      WebRtc_UWord32 productUniqueIdUTF8Length=0) {
    return -1;
  }
  virtual WebRtc_Word32 CreateCapabilityMap(
      const char* deviceUniqueIdUTF8) { return 0; }
  virtual WebRtc_Word32 Init() { return 0; }
};

VideoCaptureModule::DeviceInfo* VideoCaptureImpl::CreateDeviceInfo(
    const WebRtc_Word32 id) {
  return new ExternalDeviceInfo(id);
}

}  

}  
