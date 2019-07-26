





































#ifndef COMMON_BYTE_CURSOR_H_
#define COMMON_BYTE_CURSOR_H_

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <string>

namespace google_breakpad {


struct ByteBuffer {
  ByteBuffer() : start(0), end(0) { }
  ByteBuffer(const uint8_t *set_start, size_t set_size)
      : start(set_start), end(set_start + set_size) { }
  ~ByteBuffer() { };

  
  
  bool operator==(const ByteBuffer &that) const {
    return start == that.start && end == that.end;
  }
  bool operator!=(const ByteBuffer &that) const {
    return start != that.start || end != that.end;
  }

  
  size_t Size() const {
    assert(start <= end);
    return end - start;
  }

  const uint8_t *start, *end;
};





class ByteCursor {
 public:
  
  
  ByteCursor(const ByteBuffer *buffer, bool big_endian = false)
      : buffer_(buffer), here_(buffer->start), 
        big_endian_(big_endian), complete_(true) { }

  
  bool big_endian() const { return big_endian_; }
  void set_big_endian(bool big_endian) { big_endian_ = big_endian; }

  
  
  const uint8_t *here() const { return here_; }
  ByteCursor &set_here(const uint8_t *here) {
    assert(buffer_->start <= here && here <= buffer_->end);
    here_ = here;
    return *this;
  }

  
  size_t Available() const { return size_t(buffer_->end - here_); }

  
  bool AtEnd() const { return Available() == 0; }

  
  
  
  operator bool() const { return complete_; }

  
  
  
  
  
  template<typename T>
  ByteCursor &Read(size_t size, bool is_signed, T *result) {
    if (CheckAvailable(size)) {
      T v = 0;
      if (big_endian_) {
        for (size_t i = 0; i < size; i++)
          v = (v << 8) + here_[i];
      } else {
        
        
        for (size_t i = size - 1; i < size; i--)
          v = (v << 8) + here_[i];
      }
      if (is_signed && size < sizeof(T)) {
        size_t sign_bit = (T)1 << (size * 8 - 1);
        v = (v ^ sign_bit) - sign_bit;
      }
      here_ += size;
      *result = v;
    } else {
      *result = (T) 0xdeadbeef;
    }
    return *this;
  }

  
  
  
  
  template<typename T>
  ByteCursor &operator>>(T &result) {
    bool T_is_signed = (T)-1 < 0;
    return Read(sizeof(T), T_is_signed, &result); 
  }

  
  
  
  
  ByteCursor &Read(uint8_t *buffer, size_t size) {
    if (CheckAvailable(size)) {
      memcpy(buffer, here_, size);
      here_ += size;
    }
    return *this;
  }

  
  
  
  
  ByteCursor &CString(std::string *str) {
    const uint8_t *end
      = static_cast<const uint8_t *>(memchr(here_, '\0', Available()));
    if (end) {
      str->assign(reinterpret_cast<const char *>(here_), end - here_);
      here_ = end + 1;
    } else {
      str->clear();
      here_ = buffer_->end;
      complete_ = false;
    }
    return *this;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  ByteCursor &CString(std::string *str, size_t limit) {
    if (CheckAvailable(limit)) {
      const uint8_t *end
        = static_cast<const uint8_t *>(memchr(here_, '\0', limit));
      if (end)
        str->assign(reinterpret_cast<const char *>(here_), end - here_);
      else
        str->assign(reinterpret_cast<const char *>(here_), limit);
      here_ += limit;
    } else {
      str->clear();
    }
    return *this;
  }

  
  
  
  
  
  ByteCursor &PointTo(const uint8_t **pointer, size_t size = 0) {
    if (CheckAvailable(size)) {
      *pointer = here_;
      here_ += size;
    } else {
      *pointer = NULL;
    }
    return *this;
  }

  
  
  
  ByteCursor &Skip(size_t size) {
    if (CheckAvailable(size))
      here_ += size;
    return *this;
  }

 private:
  
  
  
  bool CheckAvailable(size_t size) {
    if (Available() >= size) {
      return true;
    } else {
      here_ = buffer_->end;
      complete_ = false;
      return false;
    }
  }

  
  const ByteBuffer *buffer_;

  
  const uint8_t *here_;

  
  
  bool big_endian_;

  
  bool complete_;
};

}  

#endif  
