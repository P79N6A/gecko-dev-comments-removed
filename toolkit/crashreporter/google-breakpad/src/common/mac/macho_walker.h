
































#ifndef COMMON_MAC_MACHO_WALKER_H__
#define COMMON_MAC_MACHO_WALKER_H__

#include <mach-o/loader.h>
#include <sys/types.h>

namespace MacFileUtilities {

class MachoWalker {
 public:
  
  
  
  
  
  
  
  typedef bool (*LoadCommandCallback)(MachoWalker *walker, load_command *cmd,
                                      off_t offset, bool swap, void *context);

  MachoWalker(const char *path, LoadCommandCallback callback, void *context);
  MachoWalker(int file_descriptor, LoadCommandCallback callback, void *context);
  ~MachoWalker();

  
  
  
  
  
  bool WalkHeader(int cpu_type);

  
  
  bool FindHeader(int cpu_type, off_t &offset);

  
  bool ReadBytes(void *buffer, size_t size, off_t offset);
  
  
  bool CurrentHeader(struct mach_header_64 *header, off_t *offset);

 private:
  
  int ValidateCPUType(int cpu_type);

  
  
  bool WalkHeaderAtOffset(off_t offset);
  bool WalkHeader64AtOffset(off_t offset);

  
  bool WalkHeaderCore(off_t offset, uint32_t number_of_commands, bool swap);

  
  int file_;

  
  LoadCommandCallback callback_;
  void *callback_context_;
  
  
  
  
  
  
  struct mach_header_64 *current_header_;
  unsigned long current_header_size_;
  off_t current_header_offset_;
};

}  

#endif  
