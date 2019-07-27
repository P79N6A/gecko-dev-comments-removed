





#ifndef SANDBOX_SRC_SIDESTEP_MINI_DISASSEMBLER_H__
#define SANDBOX_SRC_SIDESTEP_MINI_DISASSEMBLER_H__

#include "sandbox/win/src/sidestep/mini_disassembler_types.h"

namespace sidestep {





























class MiniDisassembler {
 public:

  
  
  
  
  
  
  MiniDisassembler(bool operand_default_32_bits,
                   bool address_default_32_bits);

  
  MiniDisassembler();

  
  
  
  
  
  
  
  
  
  
  
  
  
  InstructionType Disassemble(unsigned char* start,
                              unsigned int* instruction_bytes);

 private:

  
  void Initialize();

  
  
  InstructionType ProcessPrefixes(unsigned char* start, unsigned int* size);

  
  
  
  InstructionType ProcessOpcode(unsigned char* start,
                                unsigned int table,
                                unsigned int* size);

  
  
  
  
  bool ProcessOperand(int flag_operand);

  
  
  
  
  bool ProcessModrm(unsigned char* start, unsigned int* size);

  
  
  
  
  bool ProcessSib(unsigned char* start, unsigned char mod, unsigned int* size);

  
  InstructionType instruction_type_;

  
  
  
  unsigned int operand_bytes_;

  
  bool have_modrm_;

  
  
  bool should_decode_modrm_;

  
  bool operand_is_32_bits_;

  
  bool operand_default_is_32_bits_;

  
  bool address_is_32_bits_;

  
  bool address_default_is_32_bits_;

  
  
  static const OpcodeTable s_ia32_opcode_map_[];

  
  
  
  static const ModrmEntry s_ia16_modrm_map_[];

  
  
  
  static const ModrmEntry s_ia32_modrm_map_[];

  
  
  
  bool got_f2_prefix_, got_f3_prefix_, got_66_prefix_;
};

};  

#endif  
