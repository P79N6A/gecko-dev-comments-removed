




























#ifndef CLIENT_LINUX_MINIDUMP_WRITER_DIRECTORY_READER_H_
#define CLIENT_LINUX_MINIDUMP_WRITER_DIRECTORY_READER_H_

#include <stdint.h>
#include <unistd.h>
#include <limits.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

#include "common/linux/linux_syscall_support.h"

namespace google_breakpad {



class DirectoryReader {
 public:
  DirectoryReader(int fd)
      : fd_(fd),
        buf_used_(0) {
  }

  
  
  
  
  
  
  
  bool GetNextEntry(const char** name) {
    struct kernel_dirent* const dent =
      reinterpret_cast<kernel_dirent*>(buf_);

    if (buf_used_ == 0) {
      
      const int n = sys_getdents(fd_, dent, sizeof(buf_));
      if (n < 0) {
        return false;
      } else if (n == 0) {
        hit_eof_ = true;
      } else {
        buf_used_ += n;
      }
    }

    if (buf_used_ == 0 && hit_eof_)
      return false;

    assert(buf_used_ > 0);

    *name = dent->d_name;
    return true;
  }

  void PopEntry() {
    if (!buf_used_)
      return;

    const struct kernel_dirent* const dent =
      reinterpret_cast<kernel_dirent*>(buf_);

    buf_used_ -= dent->d_reclen;
    memmove(buf_, buf_ + dent->d_reclen, buf_used_);
  }

 private:
  const int fd_;
  bool hit_eof_;
  unsigned buf_used_;
  uint8_t buf_[sizeof(struct kernel_dirent) + NAME_MAX + 1];
};

}  

#endif  
