































#include "common/linux/memory_mapped_file.h"

#include <fcntl.h>
#include <sys/mman.h>
#if defined(__ANDROID__)
#include <sys/stat.h>
#endif
#include <unistd.h>

#include "common/memory_range.h"
#include "third_party/lss/linux_syscall_support.h"

namespace google_breakpad {

MemoryMappedFile::MemoryMappedFile() {}

MemoryMappedFile::MemoryMappedFile(const char* path) {
  Map(path);
}

MemoryMappedFile::~MemoryMappedFile() {
  Unmap();
}

bool MemoryMappedFile::Map(const char* path) {
  Unmap();

  int fd = sys_open(path, O_RDONLY, 0);
  if (fd == -1) {
    return false;
  }

#if defined(__x86_64__)
  struct kernel_stat st;
  if (sys_fstat(fd, &st) == -1 || st.st_size < 0) {
#else
  struct kernel_stat64 st;
  if (sys_fstat64(fd, &st) == -1 || st.st_size < 0) {
#endif
    sys_close(fd);
    return false;
  }

  
  
  
  if (st.st_size == 0) {
    sys_close(fd);
    return true;
  }

#if defined(__x86_64__)
  void* data = sys_mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
#else
  void* data = sys_mmap2(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
#endif
  sys_close(fd);
  if (data == MAP_FAILED) {
    return false;
  }

  content_.Set(data, st.st_size);
  return true;
}

void MemoryMappedFile::Unmap() {
  if (content_.data()) {
    sys_munmap(const_cast<uint8_t*>(content_.data()), content_.length());
    content_.Set(NULL, 0);
  }
}

}  
