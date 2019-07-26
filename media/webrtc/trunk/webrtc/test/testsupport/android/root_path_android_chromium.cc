









#include "base/android/path_utils.h"
#include "base/files/file_path.h"

namespace webrtc {
namespace test {

std::string OutputPathImpl();





std::string ProjectRootPathAndroid() {
  base::FilePath root_path;
  base::android::GetExternalStorageDirectory(&root_path);
  return root_path.value() + "/";
}

std::string OutputPathAndroid() {
  return OutputPathImpl();
}

}  
}  
