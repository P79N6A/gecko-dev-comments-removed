
































#ifndef GOOGLE_BREAKPAD_PROCESSOR_BINARYSTREAM_H_
#define GOOGLE_BREAKPAD_PROCESSOR_BINARYSTREAM_H_

#include <sstream>
#include <string>

#include "google_breakpad/common/breakpad_types.h"

namespace google_breakpad {
using std::ios_base;
using std::ios;

class binarystream {
 public:
  explicit binarystream(ios_base::openmode which = ios_base::out|ios_base::in)
    : stream_(which) {}
  explicit binarystream(const std::string &str,
                        ios_base::openmode which = ios_base::out|ios_base::in)
    : stream_(str, which) {}
  explicit binarystream(const char *str, size_t size,
                        ios_base::openmode which = ios_base::out|ios_base::in)
    : stream_(std::string(str, size), which) {}

  binarystream &operator>>(std::string &str);
  binarystream &operator>>(u_int8_t &u8);
  binarystream &operator>>(u_int16_t &u16);
  binarystream &operator>>(u_int32_t &u32);
  binarystream &operator>>(u_int64_t &u64);

  
  binarystream &operator<<(const std::string &str);
  binarystream &operator<<(u_int8_t u8);
  binarystream &operator<<(u_int16_t u16);
  binarystream &operator<<(u_int32_t u32);
  binarystream &operator<<(u_int64_t u64);

  
  bool eof() const { return stream_.eof(); }
  void clear() { stream_.clear(); }
  std::string str() const { return stream_.str(); }
  void str(const std::string &s) { stream_.str(s); }
    
  
  void rewind() {
    stream_.seekg (0, ios::beg);
    stream_.seekp (0, ios::beg);
  }

 private:
  std::stringstream stream_;
};

}  

#endif  
