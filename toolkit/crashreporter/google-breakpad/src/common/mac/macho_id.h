
































#ifndef COMMON_MAC_MACHO_ID_H__
#define COMMON_MAC_MACHO_ID_H__

#include <limits.h>
#include <mach-o/loader.h>

#include "common/mac/macho_walker.h"
#include "common/md5.h"

namespace MacFileUtilities {

class MachoID {
 public:
  MachoID(const char *path);
  MachoID(const char *path, void *memory, size_t size);
  ~MachoID();

  
  
  bool UUIDCommand(int cpu_type, unsigned char identifier[16]);

  
  
  bool IDCommand(int cpu_type, unsigned char identifier[16]);

  
  
  
  uint32_t Adler32(int cpu_type);

  
  
  bool MD5(int cpu_type, unsigned char identifier[16]);

 private:
  
  typedef void (MachoID::*UpdateFunction)(unsigned char *bytes, size_t size);

  
  
  void UpdateCRC(unsigned char *bytes, size_t size);

  
  
  void UpdateMD5(unsigned char *bytes, size_t size);

  
  void Update(MachoWalker *walker, off_t offset, size_t size);

  
  bool WalkHeader(int cpu_type, MachoWalker::LoadCommandCallback callback,
                  void *context);

  
  static bool WalkerCB(MachoWalker *walker, load_command *cmd, off_t offset,
                       bool swap, void *context);

  
  static bool UUIDWalkerCB(MachoWalker *walker, load_command *cmd, off_t offset,
                           bool swap, void *context);

  
  static bool IDWalkerCB(MachoWalker *walker, load_command *cmd, off_t offset,
                         bool swap, void *context);

  
  char path_[PATH_MAX];

  
  void *memory_;

  
  size_t memory_size_;

  
  uint32_t crc_;

  
  google_breakpad::MD5Context md5_context_;

  
  UpdateFunction update_function_;
};

}  

#endif  
