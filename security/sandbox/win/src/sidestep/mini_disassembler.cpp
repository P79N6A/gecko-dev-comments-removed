





#ifdef _WIN64
#error The code in this file should not be used on 64-bit Windows.
#endif

#include "sandbox/win/src/sidestep/mini_disassembler.h"

namespace sidestep {

MiniDisassembler::MiniDisassembler(bool operand_default_is_32_bits,
                                   bool address_default_is_32_bits)
    : operand_default_is_32_bits_(operand_default_is_32_bits),
      address_default_is_32_bits_(address_default_is_32_bits) {
  Initialize();
}

MiniDisassembler::MiniDisassembler()
    : operand_default_is_32_bits_(true),
      address_default_is_32_bits_(true) {
  Initialize();
}

InstructionType MiniDisassembler::Disassemble(
    unsigned char* start_byte,
    unsigned int* instruction_bytes) {
  
  Initialize();

  
  unsigned char* current_byte = start_byte;
  unsigned int size = 0;
  InstructionType instruction_type = ProcessPrefixes(current_byte, &size);

  if (IT_UNKNOWN == instruction_type)
    return instruction_type;

  current_byte += size;
  size = 0;

  
  

  instruction_type = ProcessOpcode(current_byte, 0, &size);

  
  if ((IT_UNKNOWN == instruction_type_) || (IT_UNUSED == instruction_type_)) {
    return IT_UNKNOWN;
  }

  current_byte += size;

  
  
  
  
  

  
  
  
  
  
  
  *instruction_bytes += operand_bytes_ + (current_byte - start_byte);

  
  return instruction_type_;
}

void MiniDisassembler::Initialize() {
  operand_is_32_bits_ = operand_default_is_32_bits_;
  address_is_32_bits_ = address_default_is_32_bits_;
  operand_bytes_ = 0;
  have_modrm_ = false;
  should_decode_modrm_ = false;
  instruction_type_ = IT_UNKNOWN;
  got_f2_prefix_ = false;
  got_f3_prefix_ = false;
  got_66_prefix_ = false;
}

InstructionType MiniDisassembler::ProcessPrefixes(unsigned char* start_byte,
                                                  unsigned int* size) {
  InstructionType instruction_type = IT_GENERIC;
  const Opcode& opcode = s_ia32_opcode_map_[0].table_[*start_byte];

  switch (opcode.type_) {
    case IT_PREFIX_ADDRESS:
      address_is_32_bits_ = !address_default_is_32_bits_;
      goto nochangeoperand;
    case IT_PREFIX_OPERAND:
      operand_is_32_bits_ = !operand_default_is_32_bits_;
      nochangeoperand:
    case IT_PREFIX:

      if (0xF2 == (*start_byte))
        got_f2_prefix_ = true;
      else if (0xF3 == (*start_byte))
        got_f3_prefix_ = true;
      else if (0x66 == (*start_byte))
        got_66_prefix_ = true;

      instruction_type = opcode.type_;
      (*size)++;
      
      ProcessPrefixes(start_byte + 1, size);
    default:
      break;   
  }

  return instruction_type;
}

InstructionType MiniDisassembler::ProcessOpcode(unsigned char* start_byte,
                                                unsigned int table_index,
                                                unsigned int* size) {
  const OpcodeTable& table = s_ia32_opcode_map_[table_index];   
  unsigned char current_byte = (*start_byte) >> table.shift_;
  current_byte = current_byte & table.mask_;  

  
  if (current_byte < table.min_lim_ || current_byte > table.max_lim_) {
    instruction_type_ = IT_UNKNOWN;
    return instruction_type_;
  }

  const Opcode& opcode = table.table_[current_byte];
  if (IT_UNUSED == opcode.type_) {
    
    
    
    instruction_type_ = IT_UNUSED;
    return instruction_type_;
  } else if (IT_REFERENCE == opcode.type_) {
    
    
    
    (*size)++;
    ProcessOpcode(start_byte + 1, opcode.table_index_, size);
    return instruction_type_;
  }

  const SpecificOpcode* specific_opcode = reinterpret_cast<
                                              const SpecificOpcode*>(&opcode);
  if (opcode.is_prefix_dependent_) {
    if (got_f2_prefix_ && opcode.opcode_if_f2_prefix_.mnemonic_ != 0) {
      specific_opcode = &opcode.opcode_if_f2_prefix_;
    } else if (got_f3_prefix_ && opcode.opcode_if_f3_prefix_.mnemonic_ != 0) {
      specific_opcode = &opcode.opcode_if_f3_prefix_;
    } else if (got_66_prefix_ && opcode.opcode_if_66_prefix_.mnemonic_ != 0) {
      specific_opcode = &opcode.opcode_if_66_prefix_;
    }
  }

  
  instruction_type_ = specific_opcode->type_;

  
  

  ProcessOperand(specific_opcode->flag_dest_);
  ProcessOperand(specific_opcode->flag_source_);
  ProcessOperand(specific_opcode->flag_aux_);

  
  
  
  
  

  if (table.mask_ != 0xff) {
    if (have_modrm_) {
      
      
      ProcessModrm(start_byte, size);
      return IT_GENERIC;
    } else {
      
      
      (*size)++;
      return IT_GENERIC;
    }
  } else {
    if (have_modrm_) {
      
      (*size)++;
      ProcessModrm(start_byte + 1, size);
      return IT_GENERIC;
    } else {
      (*size)++;
      return IT_GENERIC;
    }
  }
}

bool MiniDisassembler::ProcessOperand(int flag_operand) {
  bool succeeded = true;
  if (AM_NOT_USED == flag_operand)
    return succeeded;

  
  switch (flag_operand & AM_MASK) {
    
    
    case AM_A:  
    case AM_F:  
    case AM_X:  
    case AM_Y:  
    case AM_IMPLICIT:  
                       
      break;

    
    
    case AM_C:  
    case AM_D:  
    case AM_G:  
    case AM_P:  
    case AM_R:  
    case AM_S:  
    case AM_T:  
    case AM_V:  
      have_modrm_ = true;
      break;

    
    
    case AM_E:  
                
    case AM_M:  
    case AM_Q:  
                
    case AM_W:  
                
      have_modrm_ = true;
      should_decode_modrm_ = true;
      break;

    
    
    
    case AM_I:  
    case AM_J:  
    case AM_O:  
      switch (flag_operand & OT_MASK) {
        case OT_B:  
          operand_bytes_ += OS_BYTE;
          break;
        case OT_C:  
          if (operand_is_32_bits_)
            operand_bytes_ += OS_WORD;
          else
            operand_bytes_ += OS_BYTE;
          break;
        case OT_D:  
          operand_bytes_ += OS_DOUBLE_WORD;
          break;
        case OT_DQ:  
          operand_bytes_ += OS_DOUBLE_QUAD_WORD;
          break;
        case OT_P:  
                    
          if (operand_is_32_bits_)
            operand_bytes_ += OS_48_BIT_POINTER;
          else
            operand_bytes_ += OS_32_BIT_POINTER;
          break;
        case OT_PS:  
          operand_bytes_ += OS_128_BIT_PACKED_SINGLE_PRECISION_FLOATING;
          break;
        case OT_Q:  
          operand_bytes_ += OS_QUAD_WORD;
          break;
        case OT_S:  
          operand_bytes_ += OS_PSEUDO_DESCRIPTOR;
          break;
        case OT_SD:  
        case OT_PD:  
          operand_bytes_ += OS_DOUBLE_PRECISION_FLOATING;
          break;
        case OT_SS:
          
          
          
          
          succeeded = false;
          break;
        case OT_V:  
          if (operand_is_32_bits_)
            operand_bytes_ += OS_DOUBLE_WORD;
          else
            operand_bytes_ += OS_WORD;
          break;
        case OT_W:  
          operand_bytes_ += OS_WORD;
          break;

        
        case OT_A:  
                    
        case OT_PI:  
        case OT_SI:  
          break;

        default:
          break;
      }
      break;

    default:
      break;
  }

  return succeeded;
}

bool MiniDisassembler::ProcessModrm(unsigned char* start_byte,
                                    unsigned int* size) {
  
  
  if (!should_decode_modrm_) {
    (*size)++;
    return true;
  }

  
  
  
  unsigned char modrm = (*start_byte);
  unsigned char mod = modrm & 0xC0;  
  modrm = modrm & 0x07;  
  mod = mod >> 3;  
  modrm = mod | modrm;  
  mod = mod >> 3;  

  
  

  const ModrmEntry* modrm_entry = 0;
  if (address_is_32_bits_)
    modrm_entry = &s_ia32_modrm_map_[modrm];
  else
    modrm_entry = &s_ia16_modrm_map_[modrm];

  
  

  
  
  if (modrm_entry->is_encoded_in_instruction_)
    operand_bytes_ += modrm_entry->operand_size_;

  
  
  if (modrm_entry->use_sib_byte_) {
    (*size)++;
    return ProcessSib(start_byte + 1, mod, size);
  } else {
    (*size)++;
    return true;
  }
}

bool MiniDisassembler::ProcessSib(unsigned char* start_byte,
                                  unsigned char mod,
                                  unsigned int* size) {
  
  unsigned char sib_base = (*start_byte) & 0x07;
  if (0x05 == sib_base) {
    switch (mod) {
      case 0x00:  
      case 0x02:  
        operand_bytes_ += OS_DOUBLE_WORD;
        break;
      case 0x01:  
        operand_bytes_ += OS_BYTE;
        break;
      case 0x03:  
        
        
      default:
        break;
    }
  }

  (*size)++;
  return true;
}

};  
