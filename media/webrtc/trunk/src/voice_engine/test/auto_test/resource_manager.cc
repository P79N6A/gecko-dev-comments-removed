









#include "resource_manager.h"

#include "testsupport/fileutils.h"

ResourceManager::ResourceManager() {
  std::string filename = "audio_long16.pcm";
#if defined(WEBRTC_ANDROID)
  long_audio_file_path_ = "/sdcard/" + filename;
#else
  std::string resource_path = webrtc::test::ProjectRootPath();
  if (resource_path == webrtc::test::kCannotFindProjectRootDir) {
    long_audio_file_path_ = "";
  } else {
    long_audio_file_path_ =
        resource_path + "data/voice_engine/" + filename;
  }
#endif
}

