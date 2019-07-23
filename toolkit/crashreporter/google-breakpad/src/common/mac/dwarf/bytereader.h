



























#ifndef COMMON_MAC_DWARF_BYTEREADER_H__
#define COMMON_MAC_DWARF_BYTEREADER_H__

#include <string>
#include "common/mac/dwarf/types.h"

namespace dwarf2reader {



enum Endianness {
  ENDIANNESS_BIG,
  ENDIANNESS_LITTLE
};






class ByteReader {
 public:
  explicit ByteReader(enum Endianness endian);
  virtual ~ByteReader();

  
  
  void SetAddressSize(uint8 size);

  
  
  void SetOffsetSize(uint8 size);

  
  uint8 OffsetSize() const { return offset_size_; }

  
  uint8 AddressSize() const { return address_size_; }

  
  
  uint8 ReadOneByte(const char* buffer) const;

  
  
  uint16 ReadTwoBytes(const char* buffer) const;

  
  
  
  
  uint64 ReadFourBytes(const char* buffer) const;

  
  
  uint64 ReadEightBytes(const char* buffer) const;

  
  
  
  
  uint64 ReadUnsignedLEB128(const char* buffer, size_t* len) const;

  
  
  int64 ReadSignedLEB128(const char* buffer, size_t* len) const;

  
  
  
  uint64 ReadOffset(const char* buffer) const;

  
  
  
  
  uint64 ReadAddress(const char* buffer) const;

 private:

  
  typedef uint64 (ByteReader::*AddressReader)(const char*) const;

  
  
  
  
  AddressReader offset_reader_;

  
  
  
  
  
  AddressReader address_reader_;

  Endianness endian_;
  uint8 address_size_;
  uint8 offset_size_;
};

}  

#endif  
