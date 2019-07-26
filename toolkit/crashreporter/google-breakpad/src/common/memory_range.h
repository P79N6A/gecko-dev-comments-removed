
































#ifndef COMMON_MEMORY_RANGE_H_
#define COMMON_MEMORY_RANGE_H_

#include <stddef.h>

#include "google_breakpad/common/breakpad_types.h"

namespace google_breakpad {






class MemoryRange {
 public:
  MemoryRange() : data_(NULL), length_(0) {}

  MemoryRange(const void* data, size_t length) {
    Set(data, length);
  }

  
  bool IsEmpty() const {
    
    return length_ == 0;
  }

  
  void Reset() {
    data_ = NULL;
    length_ = 0;
  }

  
  void Set(const void* data, size_t length) {
    data_ = reinterpret_cast<const uint8_t*>(data);
    
    length_ = data ? length : 0;
  }

  
  
  bool Covers(size_t sub_offset, size_t sub_length) const {
    
    
    
    
    return sub_offset < length_ &&
           sub_offset + sub_length >= sub_offset &&
           sub_offset + sub_length <= length_;
  }

  
  
  
  const void* GetData(size_t sub_offset, size_t sub_length) const {
    return Covers(sub_offset, sub_length) ? (data_ + sub_offset) : NULL;
  }

  
  
  template <typename DataType>
  const DataType* GetData(size_t sub_offset) const {
    return reinterpret_cast<const DataType*>(
        GetData(sub_offset, sizeof(DataType)));
  }

  
  
  
  const void* GetArrayElement(size_t element_offset,
                              size_t element_size,
                              unsigned element_index) const {
    size_t sub_offset = element_offset + element_index * element_size;
    return GetData(sub_offset, element_size);
  }

  
  
  
  template <typename ElementType>
  const ElementType* GetArrayElement(size_t element_offset,
                                     unsigned element_index) const {
    return reinterpret_cast<const ElementType*>(
        GetArrayElement(element_offset, sizeof(ElementType), element_index));
  }

  
  
  MemoryRange Subrange(size_t sub_offset, size_t sub_length) const {
    return Covers(sub_offset, sub_length) ?
        MemoryRange(data_ + sub_offset, sub_length) : MemoryRange();
  }

  
  const uint8_t* data() const { return data_; }

  
  size_t length() const { return length_; }

 private:
  
  const uint8_t* data_;

  
  size_t length_;
};

}  

#endif  
