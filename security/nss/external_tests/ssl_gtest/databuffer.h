





#ifndef databuffer_h__
#define databuffer_h__

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iomanip>
#include <iostream>

namespace nss_test {

class DataBuffer {
 public:
  DataBuffer() : data_(nullptr), len_(0) {}
  DataBuffer(const uint8_t *data, size_t len) : data_(nullptr), len_(0) {
    Assign(data, len);
  }
  explicit DataBuffer(const DataBuffer& other) : data_(nullptr), len_(0) {
    Assign(other.data(), other.len());
  }
  ~DataBuffer() { delete[] data_; }

  DataBuffer& operator=(const DataBuffer& other) {
    if (&other != this) {
      Assign(other.data(), other.len());
    }
    return *this;
  }

  void Allocate(size_t len) {
    delete[] data_;
    data_ = new uint8_t[len ? len : 1];  
    len_ = len;
  }

  void Truncate(size_t len) {
    len_ = std::min(len_, len);
  }

  void Assign(const uint8_t* data, size_t len) {
    Allocate(len);
    memcpy(static_cast<void *>(data_), static_cast<const void *>(data), len);
  }

  
  void Write(size_t index, const uint8_t* val, size_t count) {
    if (index + count > len_) {
      size_t newlen = index + count;
      uint8_t* tmp = new uint8_t[newlen]; 
      memcpy(static_cast<void*>(tmp),
             static_cast<const void*>(data_), len_);
      if (index > len_) {
        memset(static_cast<void*>(tmp + len_), 0, index - len_);
      }
      delete[] data_;
      data_ = tmp;
      len_ = newlen;
    }
    memcpy(static_cast<void*>(data_ + index),
           static_cast<const void*>(val), count);
  }

  void Write(size_t index, const DataBuffer& buf) {
    Write(index, buf.data(), buf.len());
  }

  
  void Write(size_t index, uint32_t val, size_t count) {
    assert(count <= sizeof(uint32_t));
    uint32_t nvalue = htonl(val);
    auto* addr = reinterpret_cast<const uint8_t*>(&nvalue);
    Write(index, addr + sizeof(uint32_t) - count, count);
  }

  
  
  void Splice(const DataBuffer& buf, size_t index, size_t remove = 0) {
    Splice(buf.data(), buf.len(), index, remove);
  }

  void Splice(const uint8_t* ins, size_t ins_len, size_t index, size_t remove = 0) {
    uint8_t* old_value = data_;
    size_t old_len = len_;

    
    size_t tail_len = old_len - std::min(old_len, index + remove);
    
    len_ = index + ins_len + tail_len;
    data_ = new uint8_t[len_ ? len_ : 1];

    
    Write(0, old_value, std::min(old_len, index));
    
    if (index > old_len) {
      memset(old_value + index, 0, index - old_len);
    }
    
    Write(index, ins, ins_len);
    
    if (tail_len > 0) {
      Write(index + ins_len,
            old_value + index + remove, tail_len);
    }

    delete[] old_value;
  }

  void Append(const DataBuffer& buf) { Splice(buf, len_); }

  const uint8_t *data() const { return data_; }
  uint8_t* data() { return data_; }
  size_t len() const { return len_; }
  bool empty() const { return len_ == 0; }

 private:
  uint8_t* data_;
  size_t len_;
};

#ifdef DEBUG
static const size_t kMaxBufferPrint = 10000;
#else
static const size_t kMaxBufferPrint = 32;
#endif

inline std::ostream& operator<<(std::ostream& stream, const DataBuffer& buf) {
  stream << "[" << buf.len() << "] ";
  for (size_t i = 0; i < buf.len(); ++i) {
    if (i >= kMaxBufferPrint) {
      stream << "...";
      break;
    }
    stream << std::hex << std::setfill('0') << std::setw(2)
           << static_cast<unsigned>(buf.data()[i]);
  }
  stream << std::dec;
  return stream;
}

} 

#endif
