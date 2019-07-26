































#ifndef COMMON_LINUX_FILE_ID_H__
#define COMMON_LINUX_FILE_ID_H__

#include <limits.h>

#include "common/linux/guid_creator.h"

namespace google_breakpad {

static const size_t kMDGUIDSize = sizeof(MDGUID);

class FileID {
 public:
  explicit FileID(const char* path);
  ~FileID() {}

  
  
  
  
  
  
  bool ElfFileIdentifier(uint8_t identifier[kMDGUIDSize]);

  
  
  
  static bool ElfFileIdentifierFromMappedFile(const void* base,
                                              uint8_t identifier[kMDGUIDSize]);

  
  
  
  
  static void ConvertIdentifierToString(const uint8_t identifier[kMDGUIDSize],
                                        char* buffer, int buffer_length);

 private:
  
  char path_[PATH_MAX];
};

}  

#endif  
