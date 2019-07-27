

































#include <google/protobuf/io/zero_copy_stream.h>

#include <google/protobuf/stubs/common.h>

namespace google {
namespace protobuf {
namespace io {

ZeroCopyInputStream::~ZeroCopyInputStream() {}
ZeroCopyOutputStream::~ZeroCopyOutputStream() {}


bool ZeroCopyOutputStream::WriteAliasedRaw(const void* ,
                                           int ) {
  GOOGLE_LOG(FATAL) << "This ZeroCopyOutputStream doesn't support aliasing. "
                "Reaching here usually means a ZeroCopyOutputStream "
                "implementation bug.";
  return false;
}

}  
}  
}  
