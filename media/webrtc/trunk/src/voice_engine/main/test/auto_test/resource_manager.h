









#ifndef SRC_VOICE_ENGINE_MAIN_TEST_AUTO_TEST_RESOURCE_MANAGER_H_
#define SRC_VOICE_ENGINE_MAIN_TEST_AUTO_TEST_RESOURCE_MANAGER_H_

#include <string>

class ResourceManager {
 public:
  ResourceManager();

  
  
  const std::string& long_audio_file_path() const {
    return long_audio_file_path_;
  }

 private:
  std::string long_audio_file_path_;
};

#endif 
