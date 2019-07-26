






#ifndef SANDBOX_SRC_SIDESTEP_MINI_DISASSEMBLER_TYPES_H__
#define SANDBOX_SRC_SIDESTEP_MINI_DISASSEMBLER_TYPES_H__

namespace sidestep {


enum InstructionType {
  
  IT_UNUSED,
  
  IT_UNKNOWN,
  
  IT_REFERENCE,
  
  IT_PREFIX,
  
  IT_PREFIX_ADDRESS,
  
  IT_PREFIX_OPERAND,
  
  IT_JUMP,
  
  IT_RETURN,
  
  IT_GENERIC,
};


enum OperandSize {
  OS_ZERO = 0,
  OS_BYTE = 1,
  OS_WORD = 2,
  OS_DOUBLE_WORD = 4,
  OS_QUAD_WORD = 8,
  OS_DOUBLE_QUAD_WORD = 16,
  OS_32_BIT_POINTER = 32/8,
  OS_48_BIT_POINTER = 48/8,
  OS_SINGLE_PRECISION_FLOATING = 32/8,
  OS_DOUBLE_PRECISION_FLOATING = 64/8,
  OS_DOUBLE_EXTENDED_PRECISION_FLOATING = 80/8,
  OS_128_BIT_PACKED_SINGLE_PRECISION_FLOATING = 128/8,
  OS_PSEUDO_DESCRIPTOR = 6
};









enum AddressingMethod {
  AM_NOT_USED = 0,        
  AM_MASK = 0x00FF0000,  
  AM_A = 0x00010000,    
  AM_C = 0x00020000,    
  AM_D = 0x00030000,    
  AM_E = 0x00040000,    
  AM_F = 0x00050000,    
  AM_G = 0x00060000,    
  AM_I = 0x00070000,    
  AM_J = 0x00080000,    
  AM_M = 0x00090000,    
  AM_O = 0x000A0000,    
  AM_P = 0x000B0000,    
  AM_Q = 0x000C0000,    
  AM_R = 0x000D0000,    
  AM_S = 0x000E0000,    
  AM_T = 0x000F0000,    
  AM_V = 0x00100000,    
  AM_W = 0x00110000,    
  AM_X = 0x00120000,    
  AM_Y = 0x00130000,    
  AM_REGISTER = 0x00140000,  
  AM_IMPLICIT = 0x00150000,  
};









enum OperandType {
  OT_MASK = 0xFF000000,
  OT_A = 0x01000000,
  OT_B = 0x02000000,
  OT_C = 0x03000000,
  OT_D = 0x04000000,
  OT_DQ = 0x05000000,
  OT_P = 0x06000000,
  OT_PI = 0x07000000,
  OT_PS = 0x08000000,  
  OT_Q = 0x09000000,
  OT_S = 0x0A000000,
  OT_SS = 0x0B000000,
  OT_SI = 0x0C000000,
  OT_V = 0x0D000000,
  OT_W = 0x0E000000,
  OT_SD = 0x0F000000,  
  OT_PD = 0x10000000,  
  
  
  OT_ADDRESS_MODE_M = 0x80000000
};



struct SpecificOpcode {
  
  
  int table_index_;

  
  InstructionType type_;

  
  
  
  int flag_dest_;
  int flag_source_;
  int flag_aux_;

  
  const char* mnemonic_;
};



struct Opcode {
  
  
  int table_index_;

  
  InstructionType type_;

  
  
  
  int flag_dest_;
  int flag_source_;
  int flag_aux_;

  
  const char* mnemonic_;

  
  
  
  bool is_prefix_dependent_;
  SpecificOpcode opcode_if_f2_prefix_;
  SpecificOpcode opcode_if_f3_prefix_;
  SpecificOpcode opcode_if_66_prefix_;
};


struct OpcodeTable {
  
  const Opcode* table_;
  
  unsigned char shift_;
  
  unsigned char mask_;
  
  unsigned char min_lim_;
  unsigned char max_lim_;
};


struct ModrmEntry {
  
  
  
  bool is_encoded_in_instruction_;

  
  bool use_sib_byte_;

  
  
  OperandSize operand_size_;
};

};  

#endif  
