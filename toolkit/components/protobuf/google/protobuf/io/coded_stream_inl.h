


































#ifndef GOOGLE_PROTOBUF_IO_CODED_STREAM_INL_H__
#define GOOGLE_PROTOBUF_IO_CODED_STREAM_INL_H__

#include <google/protobuf/io/coded_stream.h>
#include <string>
#include <google/protobuf/stubs/stl_util-inl.h>

namespace google {
namespace protobuf {
namespace io {

inline bool CodedInputStream::InternalReadStringInline(string* buffer,
                                                       int size) {
  if (size < 0) return false;  

  if (BufferSize() >= size) {
    STLStringResizeUninitialized(buffer, size);
    memcpy(string_as_array(buffer), buffer_, size);
    Advance(size);
    return true;
  }

  return ReadStringFallback(buffer, size);
}

}  
}  
}  
#endif  
