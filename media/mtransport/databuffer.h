







#ifndef databuffer_h__
#define databuffer_h__
#include <algorithm>
#include <mozilla/Scoped.h>
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
    data_ = new unsigned char[ len ? len : 1];  
    memcpy(static_cast<void *>(data_.get()),
           static_cast<const void *>(data), len);
    len_ = len;
  }

  const uint8_t *data() const { return data_; }
  size_t len() const { return len_; }
  const bool empty() const { return len_ != 0; }

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(DataBuffer)

private:
  ScopedDeleteArray<uint8_t> data_;
  size_t len_;

  DISALLOW_COPY_ASSIGN(DataBuffer);
};

}

#endif
