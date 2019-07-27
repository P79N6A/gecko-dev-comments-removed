









#include "webrtc/modules/audio_device/dummy/file_audio_device_factory.h"

#include <cstring>

#include "webrtc/modules/audio_device/dummy/file_audio_device.h"

namespace webrtc {

char FileAudioDeviceFactory::_inputAudioFilename[MAX_FILENAME_LEN] = "";
char FileAudioDeviceFactory::_outputAudioFilename[MAX_FILENAME_LEN] = "";

FileAudioDevice* FileAudioDeviceFactory::CreateFileAudioDevice(
    const int32_t id) {
  
  if (strlen(_inputAudioFilename) == 0 || strlen(_outputAudioFilename) == 0) {
    printf("Was compiled with WEBRTC_DUMMY_AUDIO_PLAY_STATIC_FILE "
           "but did not set input/output files to use. Bailing out.\n");
    exit(1);
  }
  return new FileAudioDevice(id, _inputAudioFilename, _outputAudioFilename);
}

void FileAudioDeviceFactory::SetFilenamesToUse(
    const char* inputAudioFilename, const char* outputAudioFilename) {
#ifdef WEBRTC_DUMMY_FILE_DEVICES
  assert(strlen(inputAudioFilename) < MAX_FILENAME_LEN &&
         strlen(outputAudioFilename) < MAX_FILENAME_LEN);

  
  strncpy(_inputAudioFilename, inputAudioFilename, MAX_FILENAME_LEN);
  strncpy(_outputAudioFilename, outputAudioFilename, MAX_FILENAME_LEN);
#else
  
  printf("Trying to use dummy file devices, but is not compiled "
         "with WEBRTC_DUMMY_FILE_DEVICES. Bailing out.\n");
  exit(1);
#endif
}

}  
