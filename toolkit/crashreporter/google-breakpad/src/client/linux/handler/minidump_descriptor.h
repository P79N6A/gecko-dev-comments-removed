




























#ifndef CLIENT_LINUX_HANDLER_MINIDUMP_DESCRIPTOR_H_
#define CLIENT_LINUX_HANDLER_MINIDUMP_DESCRIPTOR_H_

#include <assert.h>
#include <string>

#include "common/using_std_string.h"





namespace google_breakpad {

class MinidumpDescriptor {
 public:
  MinidumpDescriptor() : fd_(-1) {}

  explicit MinidumpDescriptor(const string& directory)
      : fd_(-1),
        directory_(directory),
        c_path_(NULL) {
    assert(!directory.empty());
  }

  explicit MinidumpDescriptor(int fd) : fd_(fd), c_path_(NULL) {
    assert(fd != -1);
  }

  explicit MinidumpDescriptor(const MinidumpDescriptor& descriptor);
  MinidumpDescriptor& operator=(const MinidumpDescriptor& descriptor);

  bool IsFD() const { return fd_ != -1; }

  int fd() const { return fd_; }

  string directory() const { return directory_; }

  const char* path() const { return c_path_; }

  
  
  void UpdatePath();

 private:
  
  int fd_;

  
  string directory_;
  
  string path_;
  
  
  const char* c_path_;
};

}  

#endif  
