







#ifndef databuffer_h__
#define databuffer_h__
#include <algorithm>
#include <mozilla/UniquePtr.h>
#include <m_cpp_utils.h>
#include <nsISupportsImpl.h>

namespace mozilla {

class DataBuffer {
 public:
  DataBuffer() : data_(nullptr), len_(0), capacity_(0) {}
  DataBuffer(const uint8_t *data, size_t len) {
    Assign(data, len, len);
  }
  DataBuffer(const uint8_t *data, size_t len, size_t capacity) {
    Assign(data, len, capacity);
  }

  
  void Assign(const uint8_t *data, size_t len, size_t capacity) {
    MOZ_RELEASE_ASSERT(len <= capacity);
    Allocate(capacity); 
    memcpy(static_cast<void *>(data_.get()),
           static_cast<const void *>(data), len);
    len_ = len;
  }

  void Allocate(size_t capacity) {
    data_.reset(new uint8_t[capacity ? capacity : 1]);  
    len_ = capacity_ = capacity;
  }

  void EnsureCapacity(size_t capacity) {
    if (capacity_ < capacity) {
      uint8_t *new_data = new uint8_t[ capacity ? capacity : 1];
      memcpy(static_cast<void *>(new_data),
             static_cast<const void *>(data_.get()), len_);
      data_.reset(new_data); 
      capacity_ = capacity;
    }
  }

  
  
  void SetLength(size_t len) {
    MOZ_RELEASE_ASSERT(len <= capacity_);
    len_ = len;
  }

  const uint8_t *data() const { return data_.get(); }
  uint8_t *data() { return data_.get(); }
  size_t len() const { return len_; }
  size_t capacity() const { return capacity_; }

private:
  UniquePtr<uint8_t[]> data_;
  size_t len_;
  size_t capacity_;

  DISALLOW_COPY_ASSIGN(DataBuffer);
};

}

#endif
