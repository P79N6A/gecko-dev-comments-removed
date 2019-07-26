









#include "webrtc/modules/video_capture/windows/device_info_mf.h"

namespace webrtc {
namespace videocapturemodule {

DeviceInfoMF::DeviceInfoMF(const int32_t id) : DeviceInfoImpl(id) {
}

DeviceInfoMF::~DeviceInfoMF() {
}

int32_t DeviceInfoMF::Init() {
  return -1;
}

uint32_t DeviceInfoMF::NumberOfDevices() {
  return 0;
}

int32_t DeviceInfoMF::GetDeviceName(
    uint32_t deviceNumber,
    char* deviceNameUTF8,
    uint32_t deviceNameLength,
    char* deviceUniqueIdUTF8,
    uint32_t deviceUniqueIdUTF8Length,
    char* productUniqueIdUTF8,
    uint32_t productUniqueIdUTF8Length) {
  return -1;
}

int32_t DeviceInfoMF::DisplayCaptureSettingsDialogBox(
    const char* deviceUniqueIdUTF8,
    const char* dialogTitleUTF8,
    void* parentWindow,
    uint32_t positionX,
    uint32_t positionY) {
  return -1;
}

}  
}  
