
































#ifndef COMMON_MAC_FILE_ID_H__
#define COMMON_MAC_FILE_ID_H__

#include <limits.h>
#include <mach/machine.h>

namespace google_breakpad {

class FileID {
 public:
  FileID(const char *path);
  ~FileID() {};

  
  
  
  
  bool FileIdentifier(unsigned char identifier[16]);

  
  
  
  
  
  
  
  
  
  
  bool MachoIdentifier(cpu_type_t cpu_type,
                       cpu_subtype_t cpu_subtype,
                       unsigned char identifier[16]);

  
  
  
  
  static void ConvertIdentifierToString(const unsigned char identifier[16],
                                        char *buffer, int buffer_length);

 private:
  
  char path_[PATH_MAX];
};

}  

#endif  
