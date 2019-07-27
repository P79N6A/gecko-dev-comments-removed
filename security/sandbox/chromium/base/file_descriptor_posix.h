



#ifndef BASE_FILE_DESCRIPTOR_POSIX_H_
#define BASE_FILE_DESCRIPTOR_POSIX_H_

#include "base/files/file.h"
#include "base/files/scoped_file.h"

namespace base {









struct FileDescriptor {
  FileDescriptor() : fd(-1), auto_close(false) {}

  FileDescriptor(int ifd, bool iauto_close) : fd(ifd), auto_close(iauto_close) {
  }

  FileDescriptor(File file) : fd(file.TakePlatformFile()), auto_close(true) {}
  explicit FileDescriptor(ScopedFD fd) : fd(fd.release()), auto_close(true) {}

  bool operator==(const FileDescriptor& other) const {
    return (fd == other.fd && auto_close == other.auto_close);
  }

  bool operator!=(const FileDescriptor& other) const {
    return !operator==(other);
  }

  
  bool operator<(const FileDescriptor& other) const {
    return other.fd < fd;
  }

  int fd;
  
  
  
  bool auto_close;
};

}  

#endif  
