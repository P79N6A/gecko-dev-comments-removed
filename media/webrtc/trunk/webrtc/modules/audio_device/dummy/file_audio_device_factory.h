









#ifndef WEBRTC_AUDIO_DEVICE_FILE_AUDIO_DEVICE_FACTORY_H
#define WEBRTC_AUDIO_DEVICE_FILE_AUDIO_DEVICE_FACTORY_H

#include "webrtc/common_types.h"

namespace webrtc {

class FileAudioDevice;





class FileAudioDeviceFactory {
 public:
  static FileAudioDevice* CreateFileAudioDevice(const int32_t id);

  
  
  static void SetFilenamesToUse(const char* inputAudioFilename,
                                const char* outputAudioFilename);

 private:
  static const uint32_t MAX_FILENAME_LEN = 256;
  static char _inputAudioFilename[MAX_FILENAME_LEN];
  static char _outputAudioFilename[MAX_FILENAME_LEN];
};

}  

#endif  
