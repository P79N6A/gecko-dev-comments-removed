




























#ifndef CLIENT_LINUX_MINIDUMP_WRITER_LINE_READER_H_
#define CLIENT_LINUX_MINIDUMP_WRITER_LINE_READER_H_

#include <stdint.h>
#include <assert.h>
#include <string.h>

#include "common/linux/linux_syscall_support.h"

namespace google_breakpad {



class LineReader {
 public:
  LineReader(int fd)
      : fd_(fd),
        hit_eof_(false),
        buf_used_(0) {
  }

  
  static const size_t kMaxLineLen = 512;

  
  
  
  
  
  
  
  
  
  bool GetNextLine(const char **line, unsigned *len) {
    for (;;) {
      if (buf_used_ == 0 && hit_eof_)
        return false;

      for (unsigned i = 0; i < buf_used_; ++i) {
        if (buf_[i] == '\n' || buf_[i] == 0) {
          buf_[i] = 0;
          *len = i;
          *line = buf_;
          return true;
        }
      }

      if (buf_used_ == sizeof(buf_)) {
        
        
        return false;
      }

      
      
      if (hit_eof_) {
        assert(buf_used_);
        
        
        buf_[buf_used_] = 0;
        *len = buf_used_;
        buf_used_ += 1;  
        *line = buf_;
        return true;
      }

      
      const ssize_t n = sys_read(fd_, buf_ + buf_used_,
                                 sizeof(buf_) - buf_used_);
      if (n < 0) {
        return false;
      } else if (n == 0) {
        hit_eof_ = true;
      } else {
        buf_used_ += n;
      }

      
      
    }
  }

  void PopLine(unsigned len) {
    

    assert(buf_used_ >= len + 1);
    buf_used_ -= len + 1;
    memmove(buf_, buf_ + len + 1, buf_used_);
  }

 private:
  const int fd_;

  bool hit_eof_;
  unsigned buf_used_;
  char buf_[kMaxLineLen];
};

}  

#endif  
