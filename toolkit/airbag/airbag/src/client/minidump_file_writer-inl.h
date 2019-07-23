
































#ifndef CLIENT_MINIDUMP_FILE_WRITER_INL_H__
#define CLIENT_MINIDUMP_FILE_WRITER_INL_H__

#include <assert.h>

#include "client/minidump_file_writer.h"
#include "google_breakpad/common/minidump_size.h"

namespace google_breakpad {

template<typename MDType>
inline bool TypedMDRVA<MDType>::Allocate() {
  allocation_state_ = SINGLE_OBJECT;
  return UntypedMDRVA::Allocate(minidump_size<MDType>::size());
}

template<typename MDType>
inline bool TypedMDRVA<MDType>::Allocate(size_t additional) {
  allocation_state_ = SINGLE_OBJECT;
  return UntypedMDRVA::Allocate(minidump_size<MDType>::size() + additional);
}

template<typename MDType>
inline bool TypedMDRVA<MDType>::AllocateArray(size_t count) {
  assert(count);
  allocation_state_ = ARRAY;
  return UntypedMDRVA::Allocate(minidump_size<MDType>::size() * count);
}

template<typename MDType>
inline bool TypedMDRVA<MDType>::AllocateObjectAndArray(unsigned int count,
                                                       size_t size) {
  assert(count && size);
  allocation_state_ = SINGLE_OBJECT_WITH_ARRAY;
  return UntypedMDRVA::Allocate(minidump_size<MDType>::size() + count * size);
}

template<typename MDType>
inline bool TypedMDRVA<MDType>::CopyIndex(unsigned int index, MDType *item) {
  assert(allocation_state_ == ARRAY);
  return writer_->Copy(position_ + index * minidump_size<MDType>::size(), item,
                       minidump_size<MDType>::size());
}

template<typename MDType>
inline bool TypedMDRVA<MDType>::CopyIndexAfterObject(unsigned int index,
                                                     const void *src, 
                                                     size_t size) {
  assert(allocation_state_ == SINGLE_OBJECT_WITH_ARRAY);
  return writer_->Copy(position_ + minidump_size<MDType>::size() + index * size,
                       src, size);
}

template<typename MDType>
inline bool TypedMDRVA<MDType>::Flush() {
  return writer_->Copy(position_, &data_, minidump_size<MDType>::size());
}

}  

#endif  
