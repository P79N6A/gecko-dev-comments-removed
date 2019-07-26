




























#include <stdio.h>

#include "client/linux/handler/minidump_descriptor.h"

#include "common/linux/guid_creator.h"

namespace google_breakpad {

MinidumpDescriptor::MinidumpDescriptor(const MinidumpDescriptor& descriptor)
    : fd_(descriptor.fd_),
      directory_(descriptor.directory_),
      c_path_(NULL) {
  
  
  
  assert(descriptor.path_.empty());
}

MinidumpDescriptor& MinidumpDescriptor::operator=(
    const MinidumpDescriptor& descriptor) {
  assert(descriptor.path_.empty());

  fd_ = descriptor.fd_;
  directory_ = descriptor.directory_;
  path_.clear();
  if (c_path_) {
    
    c_path_ = NULL;
    UpdatePath();
  }
  return *this;
}

void MinidumpDescriptor::UpdatePath() {
  assert(fd_ == -1 && !directory_.empty());

  GUID guid;
  char guid_str[kGUIDStringLength + 1];
  bool r = CreateGUID(&guid) && GUIDToString(&guid, guid_str, sizeof(guid_str));
  assert(r);

  path_.clear();
  path_ = directory_ + "/" + guid_str + ".dmp";  
  c_path_ = path_.c_str();
}

}  
