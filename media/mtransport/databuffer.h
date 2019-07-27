







#ifndef databuffer_h__
#define databuffer_h__
#include <algorithm>
#include <mozilla/UniquePtr.h>
#include <m_cpp_utils.h>
#include <nsISupportsImpl.h>

namespace mozilla {

class DataBuffer {
 public:
  DataBuffer() : data_(nullptr), len_(0) {}
  DataBuffer(const uint8_t *data, size_t len) {
    Assign(data, len);
  }

  void Assign(const uint8_t *data, size_t len) {
    Allocate(len);
    memcpy(static_cast<void *>(data_.get()),
           static_cast<const void *>(data), len);
  }

  void Allocate(size_t len) {
    data_.reset(new uint8_t[len ? len : 1]);  
    len_ = len;
  }

  const uint8_t *data() const { return data_.get(); }
  uint8_t *data() { return data_.get(); }
  size_t len() const { return len_; }
  const bool empty() const { return len_ != 0; }

private:
  UniquePtr<uint8_t[]> data_;
  size_t len_;

  DISALLOW_COPY_ASSIGN(DataBuffer);
};

}

#endif
