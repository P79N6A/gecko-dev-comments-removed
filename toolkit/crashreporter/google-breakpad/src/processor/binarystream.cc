




























#include <arpa/inet.h>
#include <limits.h>

#include <vector>

#include "processor/binarystream.h"

namespace google_breakpad {
using std::string;
using std::vector;

binarystream &binarystream::operator>>(std::string &str) {
  u_int16_t length;
  *this >> length;
  if (eof())
    return *this;
  if (length == 0) {
    str.clear();
    return *this;
  }
  vector<char> buffer(length);
  stream_.read(&buffer[0], length);
  if (!eof())
    str.assign(&buffer[0], length);
  return *this;
}

binarystream &binarystream::operator>>(u_int8_t &u8) {
  stream_.read((char *)&u8, 1);
  return *this;
}

binarystream &binarystream::operator>>(u_int16_t &u16) {
  u_int16_t temp;
  stream_.read((char *)&temp, 2);
  if (!eof())
    u16 = ntohs(temp);
  return *this;
}

binarystream &binarystream::operator>>(u_int32_t &u32) {
  u_int32_t temp;
  stream_.read((char *)&temp, 4);
  if (!eof())
    u32 = ntohl(temp);
  return *this;
}

binarystream &binarystream::operator>>(u_int64_t &u64) {
  u_int32_t lower, upper;
  *this >> lower >> upper;
  if (!eof())
    u64 = static_cast<u_int64_t>(lower) | (static_cast<u_int64_t>(upper) << 32);
  return *this;
}

binarystream &binarystream::operator<<(const std::string &str) {
  if (str.length() > USHRT_MAX) {
    
    *this << static_cast<u_int16_t>(USHRT_MAX);
    stream_.write(str.c_str(), USHRT_MAX);
  } else {
    *this << (u_int16_t)(str.length() & 0xFFFF);
    stream_.write(str.c_str(), str.length());
  }
  return *this;
}

binarystream &binarystream::operator<<(u_int8_t u8) {
  stream_.write((const char*)&u8, 1);
  return *this;
}

binarystream &binarystream::operator<<(u_int16_t u16) {
  u16 = htons(u16);
  stream_.write((const char*)&u16, 2);
  return *this;
}

binarystream &binarystream::operator<<(u_int32_t u32) {
  u32 = htonl(u32);
  stream_.write((const char*)&u32, 4);
  return *this;
}

binarystream &binarystream::operator<<(u_int64_t u64) {
  
  u_int32_t lower = static_cast<u_int32_t>(u64 & 0xFFFFFFFF);
  u_int32_t upper = static_cast<u_int32_t>(u64 >> 32);
  *this << lower << upper;
  return *this;
}

}  
