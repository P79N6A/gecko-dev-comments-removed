






























#ifndef GOOGLE_BREAKPAD_COMMON_TESTS_AUTO_TEMPDIR
#define GOOGLE_BREAKPAD_COMMON_TESTS_AUTO_TEMPDIR

#include <dirent.h>
#include <sys/types.h>

#include <string>

#include "breakpad_googletest_includes.h"
#include "common/using_std_string.h"

#if !defined(__ANDROID__)
#define TEMPDIR "/tmp"
#else
#define TEMPDIR "/data/local/tmp"
#include "common/android/testing/mkdtemp.h"
#endif

namespace google_breakpad {

class AutoTempDir {
 public:
  AutoTempDir() {
    char temp_dir[] = TEMPDIR "/breakpad.XXXXXX";
    EXPECT_TRUE(mkdtemp(temp_dir) != NULL);
    path_.assign(temp_dir);
  }

  ~AutoTempDir() {
    DeleteRecursively(path_);
  }

  const string& path() const {
    return path_;
  }

 private:
  void DeleteRecursively(const string& path) {
    
    DIR* dir = opendir(path.c_str());
    if (!dir)
      return;

    dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        continue;
      string entry_path = path + "/" + entry->d_name;
      struct stat stats;
      EXPECT_TRUE(lstat(entry_path.c_str(), &stats) == 0);
      if (S_ISDIR(stats.st_mode))
        DeleteRecursively(entry_path);
      else
        EXPECT_TRUE(unlink(entry_path.c_str()) == 0);
    }
    EXPECT_TRUE(closedir(dir) == 0);
    EXPECT_TRUE(rmdir(path.c_str()) == 0);
  }

  
  AutoTempDir(const AutoTempDir&);
  AutoTempDir& operator=(const AutoTempDir&);

  string path_;
};

}  

#endif  
