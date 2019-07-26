
































#ifndef GOOGLE_BREAKPAD_PROCESSOR_BINARYSTREAM_H_
#define GOOGLE_BREAKPAD_PROCESSOR_BINARYSTREAM_H_

#include <sstream>
#include <string>

#include "common/using_std_string.h"
#include "google_breakpad/common/breakpad_types.h"

namespace google_breakpad {
using std::ios_base;
using std::ios;

class binarystream {
 public:
  explicit binarystream(ios_base::openmode which = ios_base::out|ios_base::in)
    : stream_(which) {}
  explicit binarystream(const string &str,
                        ios_base::openmode which = ios_base::out|ios_base::in)
    : stream_(str, which) {}
  explicit binarystream(const char *str, size_t size,
                        ios_base::openmode which = ios_base::out|ios_base::in)
    : stream_(string(str, size), which) {}

  binarystream &operator>>(string &str);
  binarystream &operator>>(uint8_t &u8);
  binarystream &operator>>(uint16_t &u16);
  binarystream &operator>>(uint32_t &u32);
  binarystream &operator>>(uint64_t &u64);

  
  binarystream &operator<<(const string &str);
  binarystream &operator<<(uint8_t u8);
  binarystream &operator<<(uint16_t u16);
  binarystream &operator<<(uint32_t u32);
  binarystream &operator<<(uint64_t u64);

  
  bool eof() const { return stream_.eof(); }
  void clear() { stream_.clear(); }
  string str() const { return stream_.str(); }
  void str(const string &s) { stream_.str(s); }
    
  
  void rewind() {
    stream_.seekg (0, ios::beg);
    stream_.seekp (0, ios::beg);
    
    
    stream_.clear();
  }

 private:
  std::stringstream stream_;
};

}  

#endif  
