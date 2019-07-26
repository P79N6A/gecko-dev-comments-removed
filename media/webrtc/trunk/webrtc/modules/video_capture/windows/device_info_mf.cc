









#include "modules/video_capture/windows/device_info_mf.h"

namespace webrtc {
namespace videocapturemodule {

DeviceInfoMF::DeviceInfoMF(const WebRtc_Word32 id) : DeviceInfoImpl(id) {
}

DeviceInfoMF::~DeviceInfoMF() {
}

WebRtc_Word32 DeviceInfoMF::Init() {
  return -1;
}

WebRtc_UWord32 DeviceInfoMF::NumberOfDevices() {
  return 0;
}

WebRtc_Word32 DeviceInfoMF::GetDeviceName(
    WebRtc_UWord32 deviceNumber,
    char* deviceNameUTF8,
    WebRtc_UWord32 deviceNameLength,
    char* deviceUniqueIdUTF8,
    WebRtc_UWord32 deviceUniqueIdUTF8Length,
    char* productUniqueIdUTF8,
    WebRtc_UWord32 productUniqueIdUTF8Length) {
  return -1;
}

WebRtc_Word32 DeviceInfoMF::DisplayCaptureSettingsDialogBox(
    const char* deviceUniqueIdUTF8,
    const char* dialogTitleUTF8,
    void* parentWindow,
    WebRtc_UWord32 positionX,
    WebRtc_UWord32 positionY) {
  return -1;
}

}  
}  
