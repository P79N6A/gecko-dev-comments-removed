





























#ifndef COMMON_DWARF_BYTEREADER_H__
#define COMMON_DWARF_BYTEREADER_H__

#include <string>
#include "common/dwarf/types.h"
#include "common/dwarf/dwarf2enums.h"

namespace dwarf2reader {



enum Endianness {
  ENDIANNESS_BIG,
  ENDIANNESS_LITTLE
};




class ByteReader {
 public:
  
  
  
  
  
  explicit ByteReader(enum Endianness endianness);
  virtual ~ByteReader();

  
  
  uint8 ReadOneByte(const char* buffer) const;

  
  
  uint16 ReadTwoBytes(const char* buffer) const;

  
  
  
  
  
  uint64 ReadFourBytes(const char* buffer) const;

  
  
  uint64 ReadEightBytes(const char* buffer) const;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  uint64 ReadUnsignedLEB128(const char* buffer, size_t* len) const;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  int64 ReadSignedLEB128(const char* buffer, size_t* len) const;

  
  
  
  
  
  
  
  
  
  
  
  
  void SetAddressSize(uint8 size);

  
  
  uint8 AddressSize() const { return address_size_; }

  
  
  
  uint64 ReadAddress(const char* buffer) const;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  uint64 ReadInitialLength(const char* start, size_t* len);

  
  
  
  
  
  uint64 ReadOffset(const char* buffer) const;

  
  
  
  uint8 OffsetSize() const { return offset_size_; }

  
  
  
  
  
  void SetOffsetSize(uint8 size);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  
  
  
  void SetCFIDataBase(uint64 section_base, const char *buffer_base);

  
  
  void SetTextBase(uint64 text_base);

  
  
  
  
  
  void SetDataBase(uint64 data_base);

  
  
  
  
  void SetFunctionBase(uint64 function_base);

  
  
  void ClearFunctionBase();

  
  bool ValidEncoding(DwarfPointerEncoding encoding) const;

  
  
  
  bool UsableEncoding(DwarfPointerEncoding encoding) const;

  
  
  
  
  
  
  
  
  uint64 ReadEncodedPointer(const char *buffer, DwarfPointerEncoding encoding,
                            size_t *len) const;

 private:

  
  typedef uint64 (ByteReader::*AddressReader)(const char*) const;

  
  
  
  
  AddressReader offset_reader_;

  
  
  
  
  
  AddressReader address_reader_;

  Endianness endian_;
  uint8 address_size_;
  uint8 offset_size_;

  
  bool have_section_base_, have_text_base_, have_data_base_;
  bool have_function_base_;
  size_t section_base_, text_base_, data_base_, function_base_;
  const char *buffer_base_;
};

}  

#endif  
