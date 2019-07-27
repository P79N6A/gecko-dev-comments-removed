








































#ifndef LulDwarfExt_h
#define LulDwarfExt_h

#include <stdint.h>

#include "mozilla/Assertions.h"

#include "LulDwarfSummariser.h"

typedef signed char         int8;
typedef short               int16;
typedef int                 int32;
typedef long long           int64;

typedef unsigned char      uint8;
typedef unsigned short     uint16;
typedef unsigned int       uint32;
typedef unsigned long long uint64;

#ifdef __PTRDIFF_TYPE__
typedef          __PTRDIFF_TYPE__ intptr;
typedef unsigned __PTRDIFF_TYPE__ uintptr;
#else
#error "Can't find pointer-sized integral types."
#endif


namespace lul {




enum DwarfPointerEncoding
  {
    DW_EH_PE_absptr	= 0x00,
    DW_EH_PE_omit	= 0xff,
    DW_EH_PE_uleb128    = 0x01,
    DW_EH_PE_udata2	= 0x02,
    DW_EH_PE_udata4	= 0x03,
    DW_EH_PE_udata8	= 0x04,
    DW_EH_PE_sleb128    = 0x09,
    DW_EH_PE_sdata2	= 0x0A,
    DW_EH_PE_sdata4	= 0x0B,
    DW_EH_PE_sdata8	= 0x0C,
    DW_EH_PE_pcrel	= 0x10,
    DW_EH_PE_textrel	= 0x20,
    DW_EH_PE_datarel	= 0x30,
    DW_EH_PE_funcrel	= 0x40,
    DW_EH_PE_aligned	= 0x50,

    
    
    
    DW_EH_PE_signed	= 0x08,

    
    
    
    
    
    DW_EH_PE_indirect	= 0x80
  };




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
  uint64 section_base_;
  uint64 text_base_, data_base_, function_base_;
  const char *buffer_base_;
};


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
  MOZ_ASSERT(this->offset_reader_);
  return (this->*offset_reader_)(buffer);
}

inline uint64 ByteReader::ReadAddress(const char* buffer) const {
  MOZ_ASSERT(this->address_reader_);
  return (this->*address_reader_)(buffer);
}

inline void ByteReader::SetCFIDataBase(uint64 section_base,
                                       const char *buffer_base) {
  section_base_ = section_base;
  buffer_base_ = buffer_base;
  have_section_base_ = true;
}

inline void ByteReader::SetTextBase(uint64 text_base) {
  text_base_ = text_base;
  have_text_base_ = true;
}

inline void ByteReader::SetDataBase(uint64 data_base) {
  data_base_ = data_base;
  have_data_base_ = true;
}

inline void ByteReader::SetFunctionBase(uint64 function_base) {
  function_base_ = function_base;
  have_function_base_ = true;
}

inline void ByteReader::ClearFunctionBase() {
  have_function_base_ = false;
}




























































































































































class CallFrameInfo {
 public:
  
  
  enum EntryKind { kUnknown, kCIE, kFDE, kTerminator };

  
  
  class Handler;

  
  
  class Reporter;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  CallFrameInfo(const char *buffer, size_t buffer_length,
                ByteReader *reader, Handler *handler, Reporter *reporter,
                bool eh_frame = false)
      : buffer_(buffer), buffer_length_(buffer_length),
        reader_(reader), handler_(handler), reporter_(reporter),
        eh_frame_(eh_frame) { }

  ~CallFrameInfo() { }

  
  
  
  bool Start();

  
  static const char *KindName(EntryKind kind);

 private:

  struct CIE;

  
  struct Entry {
    
    
    size_t offset;

    
    const char *start;

    
    
    
    
    
    EntryKind kind;

    
    
    const char *fields;

    
    const char *instructions;

    
    
    
    
    const char *end;

    
    
    uint64 id;

    
    
    CIE *cie;
  };

  
  struct CIE: public Entry {
    uint8 version;                      
    std::string augmentation;           
    uint64 code_alignment_factor;       
    int data_alignment_factor;          
    unsigned return_address_register;   

    
    bool has_z_augmentation;

    
    
    bool has_z_lsda;                    
    bool has_z_personality;             
    bool has_z_signal_frame;            

    
    
    DwarfPointerEncoding lsda_encoding;

    
    
    DwarfPointerEncoding personality_encoding;

    
    
    
    uint64 personality_address;

    
    
    
    
    DwarfPointerEncoding pointer_encoding;
  };

  
  struct FDE: public Entry {
    uint64 address;                     
    uint64 size;                        

    
    
    
    uint64 lsda_address;
  };

  
  class Rule;
  class UndefinedRule;
  class SameValueRule;
  class OffsetRule;
  class ValOffsetRule;
  class RegisterRule;
  class ExpressionRule;
  class ValExpressionRule;
  class RuleMap;
  class State;

  
  
  
  
  
  
  bool ReadEntryPrologue(const char *cursor, Entry *entry);

  
  
  
  
  
  bool ReadCIEFields(CIE *cie);

  
  
  
  
  
  
  bool ReadFDEFields(FDE *fde);

  
  
  
  bool ReportIncomplete(Entry *entry);

  
  static bool IsIndirectEncoding(DwarfPointerEncoding encoding) {
    return encoding & DW_EH_PE_indirect;
  }

  
  const char *buffer_;
  size_t buffer_length_;

  
  ByteReader *reader_;

  
  Handler *handler_;

  
  Reporter *reporter_;

  
  bool eh_frame_;
};




class CallFrameInfo::Handler {
 public:
  
  enum { kCFARegister = DW_REG_CFA };

  Handler() { }
  virtual ~Handler() { }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual bool Entry(size_t offset, uint64 address, uint64 length,
                     uint8 version, const std::string &augmentation,
                     unsigned return_address) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  virtual bool UndefinedRule(uint64 address, int reg) = 0;

  
  
  virtual bool SameValueRule(uint64 address, int reg) = 0;

  
  
  virtual bool OffsetRule(uint64 address, int reg,
                          int base_register, long offset) = 0;

  
  
  
  virtual bool ValOffsetRule(uint64 address, int reg,
                             int base_register, long offset) = 0;

  
  
  
  
  
  virtual bool RegisterRule(uint64 address, int reg, int base_register) = 0;

  
  
  virtual bool ExpressionRule(uint64 address, int reg,
                              const std::string &expression) = 0;

  
  
  
  virtual bool ValExpressionRule(uint64 address, int reg,
                                 const std::string &expression) = 0;

  
  
  
  
  virtual bool End() = 0;

  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  
  
  
  virtual bool PersonalityRoutine(uint64 address, bool indirect) {
    return true;
  }

  
  
  
  
  
  virtual bool LanguageSpecificDataArea(uint64 address, bool indirect) {
    return true;
  }

  
  
  
  
  
  
  
  
  virtual bool SignalHandler() { return true; }
};






class CallFrameInfo::Reporter final {
 public:
  
  
  
  
  
  
  
  Reporter(void (*aLog)(const char*),
           const std::string &filename,
           const std::string &section = ".debug_frame")
      : log_(aLog), filename_(filename), section_(section) { }
  virtual ~Reporter() { }

  
  
  
  virtual void Incomplete(uint64 offset, CallFrameInfo::EntryKind kind);

  
  
  
  
  virtual void EarlyEHTerminator(uint64 offset);

  
  
  virtual void CIEPointerOutOfRange(uint64 offset, uint64 cie_offset);

  
  
  virtual void BadCIEId(uint64 offset, uint64 cie_offset);

  
  
  
  virtual void UnrecognizedVersion(uint64 offset, int version);

  
  
  
  virtual void UnrecognizedAugmentation(uint64 offset,
                                        const std::string &augmentation);

  
  
  virtual void InvalidPointerEncoding(uint64 offset, uint8 encoding);

  
  
  virtual void UnusablePointerEncoding(uint64 offset, uint8 encoding);

  
  
  virtual void RestoreInCIE(uint64 offset, uint64 insn_offset);

  
  
  virtual void BadInstruction(uint64 offset, CallFrameInfo::EntryKind kind,
                              uint64 insn_offset);

  
  
  
  virtual void NoCFARule(uint64 offset, CallFrameInfo::EntryKind kind,
                         uint64 insn_offset);

  
  
  
  virtual void EmptyStateStack(uint64 offset, CallFrameInfo::EntryKind kind,
                               uint64 insn_offset);

  
  
  
  
  
  virtual void ClearingCFARule(uint64 offset, CallFrameInfo::EntryKind kind,
                               uint64 insn_offset);

 private:
  
  void (*log_)(const char*);

 protected:
  
  std::string filename_;

  
  std::string section_;
};


using lul::CallFrameInfo;
using lul::Summariser;




class DwarfCFIToModule: public CallFrameInfo::Handler {
 public:

  
  
  class Reporter {
   public:
    
    
    
    
    Reporter(void (*aLog)(const char*),
             const std::string &file, const std::string &section)
      : log_(aLog), file_(file), section_(section) { }
    virtual ~Reporter() { }

    
    
    virtual void UndefinedNotSupported(size_t offset,
                                       const UniqueString* reg);

    
    
    
    
    virtual void ExpressionsNotSupported(size_t offset,
                                         const UniqueString* reg);

  private:
    
    void (*log_)(const char*);
  protected:
    std::string file_, section_;
  };

  
  
  
  class RegisterNames {
   public:
    
    static const unsigned int I386();

    
    static const unsigned int X86_64();

    
    static const unsigned int ARM();
  };

  
  
  
  
  
  
  
  
  
  DwarfCFIToModule(const unsigned int num_dw_regs,
                   Reporter *reporter,
                   UniqueStringUniverse* usu,
                   Summariser* summ)
      : summ_(summ), usu_(usu), num_dw_regs_(num_dw_regs),
        reporter_(reporter), return_address_(-1) {
  }
  virtual ~DwarfCFIToModule() {}

  virtual bool Entry(size_t offset, uint64 address, uint64 length,
                     uint8 version, const std::string &augmentation,
                     unsigned return_address);
  virtual bool UndefinedRule(uint64 address, int reg);
  virtual bool SameValueRule(uint64 address, int reg);
  virtual bool OffsetRule(uint64 address, int reg,
                          int base_register, long offset);
  virtual bool ValOffsetRule(uint64 address, int reg,
                             int base_register, long offset);
  virtual bool RegisterRule(uint64 address, int reg, int base_register);
  virtual bool ExpressionRule(uint64 address, int reg,
                              const std::string &expression);
  virtual bool ValExpressionRule(uint64 address, int reg,
                                 const std::string &expression);
  virtual bool End();

 private:
  
  const UniqueString* RegisterName(int i);

  
  Summariser* summ_;

  
  UniqueStringUniverse* usu_;

  
  const unsigned int num_dw_regs_;

  
  Reporter *reporter_;

  
  
  size_t entry_offset_;

  
  unsigned return_address_;
};

} 

#endif
