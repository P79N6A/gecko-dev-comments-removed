









#include <string>

namespace webrtc {
namespace test {

static const char* kRootDirName = "/sdcard/";
std::string ProjectRootPathAndroid() {
  return kRootDirName;
}

std::string OutputPathAndroid() {
  return kRootDirName;
}

}  
}  
