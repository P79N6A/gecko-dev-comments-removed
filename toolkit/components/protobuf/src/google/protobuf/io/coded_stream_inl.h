


































#ifndef GOOGLE_PROTOBUF_IO_CODED_STREAM_INL_H__
#define GOOGLE_PROTOBUF_IO_CODED_STREAM_INL_H__

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <string>
#include <google/protobuf/stubs/stl_util.h>

namespace google {
namespace protobuf {
namespace io {

inline bool CodedInputStream::InternalReadStringInline(string* buffer,
                                                       int size) {
  if (size < 0) return false;  

  if (BufferSize() >= size) {
    STLStringResizeUninitialized(buffer, size);
    
    
    if (size > 0) {
      memcpy(mutable_string_data(buffer), buffer_, size);
      Advance(size);
    }
    return true;
  }

  return ReadStringFallback(buffer, size);
}

}  
}  
}  
#endif  
