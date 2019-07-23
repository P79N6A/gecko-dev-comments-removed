































#ifndef COMMON_LINUX_FILE_ID_H__
#define COMMON_LINUX_FILE_ID_H__

#include <limits.h>

namespace google_breakpad {

class FileID {
 public:
  FileID(const char *path);
  ~FileID() {};

  
  
  
  
  bool ElfFileIdentifier(unsigned char identifier[16]);

  
  
  
  
  static void ConvertIdentifierToString(const unsigned char identifier[16],
                                        char *buffer, int buffer_length);

 private:
  
  char path_[PATH_MAX];
};

}  

#endif  

