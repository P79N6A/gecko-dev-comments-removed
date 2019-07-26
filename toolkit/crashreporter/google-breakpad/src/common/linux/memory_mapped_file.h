































#ifndef COMMON_LINUX_MEMORY_MAPPED_FILE_H_
#define COMMON_LINUX_MEMORY_MAPPED_FILE_H_

#include "common/basictypes.h"
#include "common/memory_range.h"

namespace google_breakpad {




class MemoryMappedFile {
 public:
  MemoryMappedFile();

  
  
  explicit MemoryMappedFile(const char* path);

  ~MemoryMappedFile();

  
  
  
  
  
  bool Map(const char* path);

  
  
  void Unmap();

  
  
  const MemoryRange& content() const { return content_; }

  
  
  const void* data() const { return content_.data(); }

  
  
  size_t size() const { return content_.length(); }

 private:
  
  MemoryRange content_;

  DISALLOW_COPY_AND_ASSIGN(MemoryMappedFile);
};

}  

#endif  
