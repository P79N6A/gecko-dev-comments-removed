





#ifndef databuffer_h__
#define databuffer_h__

class DataBuffer {
 public:
  DataBuffer() : data_(nullptr), len_(0) {}
  DataBuffer(const uint8_t *data, size_t len) : data_(nullptr), len_(0) {
    Assign(data, len);
  }
  ~DataBuffer() { delete[] data_; }

  void Assign(const uint8_t *data, size_t len) {
    Allocate(len);
    memcpy(static_cast<void *>(data_), static_cast<const void *>(data), len);
  }

  void Allocate(size_t len) {
    delete[] data_;
    data_ = new unsigned char[len ? len : 1];  
    len_ = len;
  }

  const uint8_t *data() const { return data_; }
  uint8_t *data() { return data_; }
  size_t len() const { return len_; }
  const bool empty() const { return len_ != 0; }

 private:
  uint8_t *data_;
  size_t len_;
};

#endif
