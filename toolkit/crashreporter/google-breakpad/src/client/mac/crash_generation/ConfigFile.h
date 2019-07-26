






























#import <Foundation/Foundation.h>

#import "common/mac/SimpleStringDictionary.h"

namespace google_breakpad {

BOOL EnsureDirectoryPathExists(NSString *dirPath);


class ConfigFile {
 public:
  ConfigFile() {
    config_file_ = -1;
    config_file_path_[0] = 0;
    has_created_file_ = false;
  };

  ~ConfigFile() {
  };

  void WriteFile(const char* directory,
                 const SimpleStringDictionary *configurationParameters,
                 const char *dump_dir,
                 const char *minidump_id);

  const char *GetFilePath() { return config_file_path_; }

  void Unlink() {
    if (config_file_ != -1)
      unlink(config_file_path_);

    config_file_ = -1;
  }

 private:
  BOOL WriteData(const void *data, size_t length);

  BOOL AppendConfigData(const char *key,
                        const void *data,
                        size_t length);

  BOOL AppendConfigString(const char *key,
                          const char *value);

  BOOL AppendCrashTimeParameters(const char *processStartTimeString);

  int   config_file_;                    
  char  config_file_path_[PATH_MAX];     
  bool  has_created_file_;
};

} 
