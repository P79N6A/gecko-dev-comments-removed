






























#ifndef GOOGLE_BREAKPAD_CLIENT_MAC_TESTS_AUTO_TEMPDIR
#define GOOGLE_BREAKPAD_CLIENT_MAC_TESTS_AUTO_TEMPDIR

#include <dirent.h>
#include <sys/types.h>

#include <string>

namespace google_breakpad {

class AutoTempDir {
 public:
  AutoTempDir() {
    char tempDir[16] = "/tmp/XXXXXXXXXX";
    mkdtemp(tempDir);
    path = tempDir;
  }

  ~AutoTempDir() {
    
    DIR* dir = opendir(path.c_str());
    if (!dir)
      return;

    dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
	continue;
      std::string entryPath = path + "/" + entry->d_name;
      unlink(entryPath.c_str());
    }
    closedir(dir);
    rmdir(path.c_str());
  }

  std::string path;
};

}  

#endif  
