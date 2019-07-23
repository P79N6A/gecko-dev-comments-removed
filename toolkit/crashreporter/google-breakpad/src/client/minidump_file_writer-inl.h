
































#ifndef CLIENT_MINIDUMP_FILE_WRITER_INL_H__
#define CLIENT_MINIDUMP_FILE_WRITER_INL_H__

#include <assert.h>

#include "client/minidump_file_writer.h"

namespace google_airbag {

template<typename MDType>
inline bool TypedMDRVA<MDType>::Allocate() {
  allocation_state_ = SINGLE_OBJECT;
  return UntypedMDRVA::Allocate(sizeof(MDType));
}

template<typename MDType>
inline bool TypedMDRVA<MDType>::Allocate(size_t additional) {
  allocation_state_ = SINGLE_OBJECT;
  return UntypedMDRVA::Allocate(sizeof(MDType) + additional);
}

template<typename MDType>
inline bool TypedMDRVA<MDType>::AllocateArray(size_t count) {
  assert(count);
  allocation_state_ = ARRAY;
  return UntypedMDRVA::Allocate(sizeof(MDType) * count);
}

template<typename MDType>
inline bool TypedMDRVA<MDType>::AllocateObjectAndArray(unsigned int count,
                                                       size_t size) {
  assert(count && size);
  allocation_state_ = SINGLE_OBJECT_WITH_ARRAY;
  return UntypedMDRVA::Allocate(sizeof(MDType) + count * size);
}

template<typename MDType>
inline bool TypedMDRVA<MDType>::CopyIndex(unsigned int index, MDType *item) {
  assert(allocation_state_ == ARRAY);
  return writer_->Copy(position_ + index * sizeof(MDType), item,
                       sizeof(MDType));
}

template<typename MDType>
inline bool TypedMDRVA<MDType>::CopyIndexAfterObject(unsigned int index,
                                                     void *src, size_t size) {
  assert(allocation_state_ == SINGLE_OBJECT_WITH_ARRAY);
  return writer_->Copy(position_ + sizeof(MDType) + index * size, src, size);
}

template<typename MDType>
inline bool TypedMDRVA<MDType>::Flush() {
  return writer_->Copy(position_, &data_, sizeof(MDType));
}

}  

#endif  
