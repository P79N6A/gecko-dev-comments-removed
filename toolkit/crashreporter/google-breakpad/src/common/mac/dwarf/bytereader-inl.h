



























#ifndef UTIL_DEBUGINFO_BYTEREADER_INL_H__
#define UTIL_DEBUGINFO_BYTEREADER_INL_H__

#include "common/mac/dwarf/bytereader.h"

namespace dwarf2reader {

inline uint8 ByteReader::ReadOneByte(const char* buffer) const {
  return buffer[0];
}

inline uint16 ByteReader::ReadTwoBytes(const char* signed_buffer) const {
  const unsigned char *buffer
    = reinterpret_cast<const unsigned char *>(signed_buffer);
  const uint16 buffer0 = buffer[0];
  const uint16 buffer1 = buffer[1];
  if (endian_ == ENDIANNESS_LITTLE) {
    return buffer0 | buffer1 << 8;
  } else {
    return buffer1 | buffer0 << 8;
  }
}

inline uint64 ByteReader::ReadFourBytes(const char* signed_buffer) const {
  const unsigned char *buffer
    = reinterpret_cast<const unsigned char *>(signed_buffer);
  const uint32 buffer0 = buffer[0];
  const uint32 buffer1 = buffer[1];
  const uint32 buffer2 = buffer[2];
  const uint32 buffer3 = buffer[3];
  if (endian_ == ENDIANNESS_LITTLE) {
    return buffer0 | buffer1 << 8 | buffer2 << 16 | buffer3 << 24;
  } else {
    return buffer3 | buffer2 << 8 | buffer1 << 16 | buffer0 << 24;
  }
}

inline uint64 ByteReader::ReadEightBytes(const char* signed_buffer) const {
  const unsigned char *buffer
    = reinterpret_cast<const unsigned char *>(signed_buffer);
  const uint64 buffer0 = buffer[0];
  const uint64 buffer1 = buffer[1];
  const uint64 buffer2 = buffer[2];
  const uint64 buffer3 = buffer[3];
  const uint64 buffer4 = buffer[4];
  const uint64 buffer5 = buffer[5];
  const uint64 buffer6 = buffer[6];
  const uint64 buffer7 = buffer[7];
  if (endian_ == ENDIANNESS_LITTLE) {
    return buffer0 | buffer1 << 8 | buffer2 << 16 | buffer3 << 24 |
      buffer4 << 32 | buffer5 << 40 | buffer6 << 48 | buffer7 << 56;
  } else {
    return buffer7 | buffer6 << 8 | buffer5 << 16 | buffer4 << 24 |
      buffer3 << 32 | buffer2 << 40 | buffer1 << 48 | buffer0 << 56;
  }
}





inline uint64 ByteReader::ReadUnsignedLEB128(const char* buffer,
                                             size_t* len) const {
  uint64 result = 0;
  size_t num_read = 0;
  unsigned int shift = 0;
  unsigned char byte;

  do {
    byte = *buffer++;
    num_read++;

    result |= (static_cast<uint64>(byte & 0x7f)) << shift;

    shift += 7;

  } while (byte & 0x80);

  *len = num_read;

  return result;
}




inline int64 ByteReader::ReadSignedLEB128(const char* buffer,
                                          size_t* len) const {
  int64 result = 0;
  unsigned int shift = 0;
  size_t num_read = 0;
  unsigned char byte;

  do {
      byte = *buffer++;
      num_read++;
      result |= (static_cast<uint64>(byte & 0x7f) << shift);
      shift += 7;
  } while (byte & 0x80);

  if ((shift < 8 * sizeof (result)) && (byte & 0x40))
    result |= -((static_cast<int64>(1)) << shift);
  *len = num_read;
  return result;
}

inline uint64 ByteReader::ReadOffset(const char* buffer) const {
  assert(this->offset_reader_);
  return (this->*offset_reader_)(buffer);
}

inline uint64 ByteReader::ReadAddress(const char* buffer) const {
  assert(this->address_reader_);
  return (this->*address_reader_)(buffer);
}

}  

#endif  
