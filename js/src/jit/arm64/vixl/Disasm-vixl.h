

























#ifndef VIXL_A64_DISASM_A64_H
#define VIXL_A64_DISASM_A64_H

#include "jit/arm64/vixl/Assembler-vixl.h"
#include "jit/arm64/vixl/Decoder-vixl.h"
#include "jit/arm64/vixl/Globals-vixl.h"
#include "jit/arm64/vixl/Instructions-vixl.h"
#include "jit/arm64/vixl/Utils-vixl.h"

namespace vixl {

class Disassembler: public DecoderVisitor {
 public:
  Disassembler();
  Disassembler(char* text_buffer, int buffer_size);
  virtual ~Disassembler();
  char* GetOutput();

  
  #define DECLARE(A) virtual void Visit##A(const Instruction* instr);
  VISITOR_LIST(DECLARE)
  #undef DECLARE

 protected:
  virtual void ProcessOutput(const Instruction* instr);

  
  
  

  
  virtual void AppendRegisterNameToOutput(const Instruction* instr,
                                          const CPURegister& reg);

  
  
  virtual void AppendPCRelativeOffsetToOutput(const Instruction* instr,
                                              int64_t offset);

  
  
  virtual void AppendCodeRelativeAddressToOutput(const Instruction* instr,
                                                 const void* addr);

  
  
  
  
  
  virtual void AppendCodeRelativeCodeAddressToOutput(const Instruction* instr,
                                                     const void* addr);

  
  
  
  virtual void AppendCodeRelativeDataAddressToOutput(const Instruction* instr,
                                                     const void* addr);

  
  
  virtual void AppendAddressToOutput(const Instruction* instr,
                                     const void* addr);
  virtual void AppendCodeAddressToOutput(const Instruction* instr,
                                         const void* addr);
  virtual void AppendDataAddressToOutput(const Instruction* instr,
                                         const void* addr);

 public:
  
  
  
  
  
  
  
  
  
  void MapCodeAddress(int64_t base_address, const Instruction* instr_address);
  int64_t CodeRelativeAddress(const void* instr);

 private:
  void Format(
      const Instruction* instr, const char* mnemonic, const char* format);
  void Substitute(const Instruction* instr, const char* string);
  int SubstituteField(const Instruction* instr, const char* format);
  int SubstituteRegisterField(const Instruction* instr, const char* format);
  int SubstituteImmediateField(const Instruction* instr, const char* format);
  int SubstituteLiteralField(const Instruction* instr, const char* format);
  int SubstituteBitfieldImmediateField(
      const Instruction* instr, const char* format);
  int SubstituteShiftField(const Instruction* instr, const char* format);
  int SubstituteExtendField(const Instruction* instr, const char* format);
  int SubstituteConditionField(const Instruction* instr, const char* format);
  int SubstitutePCRelAddressField(const Instruction* instr, const char* format);
  int SubstituteBranchTargetField(const Instruction* instr, const char* format);
  int SubstituteLSRegOffsetField(const Instruction* instr, const char* format);
  int SubstitutePrefetchField(const Instruction* instr, const char* format);
  int SubstituteBarrierField(const Instruction* instr, const char* format);

  bool RdIsZROrSP(const Instruction* instr) const {
    return (instr->Rd() == kZeroRegCode);
  }

  bool RnIsZROrSP(const Instruction* instr) const {
    return (instr->Rn() == kZeroRegCode);
  }

  bool RmIsZROrSP(const Instruction* instr) const {
    return (instr->Rm() == kZeroRegCode);
  }

  bool RaIsZROrSP(const Instruction* instr) const {
    return (instr->Ra() == kZeroRegCode);
  }

  bool IsMovzMovnImm(unsigned reg_size, uint64_t value);

  int64_t code_address_offset() const { return code_address_offset_; }

 protected:
  void ResetOutput();
  void AppendToOutput(const char* string, ...) PRINTF_CHECK(2, 3);

  void set_code_address_offset(int64_t code_address_offset) {
    code_address_offset_ = code_address_offset;
  }

  char* buffer_;
  uint32_t buffer_pos_;
  uint32_t buffer_size_;
  bool own_buffer_;

  int64_t code_address_offset_;
};


class PrintDisassembler: public Disassembler {
 public:
  explicit PrintDisassembler(FILE* stream) : stream_(stream) { }
  virtual ~PrintDisassembler() { }

 protected:
  virtual void ProcessOutput(const Instruction* instr);

 private:
  FILE *stream_;
};
}  

#endif  
