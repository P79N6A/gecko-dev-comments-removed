

























#include "processor/disassembler_x86.h"

#include <string.h>
#include <unistd.h>

namespace google_breakpad {

DisassemblerX86::DisassemblerX86(const uint8_t *bytecode,
                                 uint32_t size,
                                 uint32_t virtual_address) :
                                     bytecode_(bytecode),
                                     size_(size),
                                     virtual_address_(virtual_address),
                                     current_byte_offset_(0),
                                     current_inst_offset_(0),
                                     instr_valid_(false),
                                     register_valid_(false),
                                     pushed_bad_value_(false),
                                     end_of_block_(false),
                                     flags_(0) {
  libdis::x86_init(libdis::opt_none, NULL, NULL);
}

DisassemblerX86::~DisassemblerX86() {
  if (instr_valid_)
    libdis::x86_oplist_free(&current_instr_);

  libdis::x86_cleanup();
}

uint32_t DisassemblerX86::NextInstruction() {
  if (instr_valid_)
    libdis::x86_oplist_free(&current_instr_);

  if (current_byte_offset_ >= size_) {
    instr_valid_ = false;
    return 0;
  }
  uint32_t instr_size = 0;
  instr_size = libdis::x86_disasm((unsigned char *)bytecode_, size_,
                          virtual_address_, current_byte_offset_,
                          &current_instr_);
  if (instr_size == 0) {
    instr_valid_ = false;
    return 0;
  }

  current_byte_offset_ += instr_size;
  current_inst_offset_++;
  instr_valid_ = libdis::x86_insn_is_valid(&current_instr_);
  if (!instr_valid_)
    return 0;

  if (current_instr_.type == libdis::insn_return)
    end_of_block_ = true;
  libdis::x86_op_t *src = libdis::x86_get_src_operand(&current_instr_);
  libdis::x86_op_t *dest = libdis::x86_get_dest_operand(&current_instr_);

  if (register_valid_) {
    switch (current_instr_.group) {
      
      
      case libdis::insn_controlflow:
        switch (current_instr_.type) {
          case libdis::insn_jmp:
          case libdis::insn_jcc:
          case libdis::insn_call:
          case libdis::insn_callcc:
            if (dest) {
              switch (dest->type) {
                case libdis::op_expression:
                  if (dest->data.expression.base.id == bad_register_.id)
                    flags_ |= DISX86_BAD_BRANCH_TARGET;
                  break;
                case libdis::op_register:
                  if (dest->data.reg.id == bad_register_.id)
                    flags_ |= DISX86_BAD_BRANCH_TARGET;
                  break;
                default:
                  if (pushed_bad_value_ &&
                      (current_instr_.type == libdis::insn_call ||
                      current_instr_.type == libdis::insn_callcc))
                    flags_ |= DISX86_BAD_ARGUMENT_PASSED;
                  break;
              }
            }
            break;
          default:
            break;
        }
        break;

      
      case libdis::insn_string:
        if (dest && dest->type == libdis::op_expression &&
            dest->data.expression.base.id == bad_register_.id)
          flags_ |= DISX86_BAD_BLOCK_WRITE;
        if (src && src->type == libdis::op_expression &&
            src->data.expression.base.id == bad_register_.id)
          flags_ |= DISX86_BAD_BLOCK_READ;
        break;

      
      case libdis::insn_comparison:
        if ((dest && dest->type == libdis::op_expression &&
            dest->data.expression.base.id == bad_register_.id) ||
            (src && src->type == libdis::op_expression &&
            src->data.expression.base.id == bad_register_.id) ||
            (dest && dest->type == libdis::op_register &&
            dest->data.reg.id == bad_register_.id) ||
            (src && src->type == libdis::op_register &&
            src->data.reg.id == bad_register_.id))
          flags_ |= DISX86_BAD_COMPARISON;
        break;

      
      
      default:
        if (dest && dest->type == libdis::op_expression &&
            dest->data.expression.base.id == bad_register_.id)
          flags_ |= DISX86_BAD_WRITE;
        if (src && src->type == libdis::op_expression &&
            src->data.expression.base.id == bad_register_.id)
          flags_ |= DISX86_BAD_READ;
        break;
    }
  }

  
  
  if (register_valid_ && dest && current_instr_.type == libdis::insn_push) {
    switch (dest->type) {
      case libdis::op_expression:
        if (dest->data.expression.base.id == bad_register_.id ||
            dest->data.expression.index.id == bad_register_.id)
          pushed_bad_value_ = true;
        break;
      case libdis::op_register:
        if (dest->data.reg.id == bad_register_.id)
          pushed_bad_value_ = true;
        break;
      default:
        break;
    }
  }

  
  
  
  if (register_valid_) {
    switch (current_instr_.type) {
      case libdis::insn_xor:
        if (src && src->type == libdis::op_register &&
            dest && dest->type == libdis::op_register &&
            src->data.reg.id == bad_register_.id &&
            src->data.reg.id == dest->data.reg.id)
          register_valid_ = false;
        break;
      case libdis::insn_pop:
      case libdis::insn_mov:
      case libdis::insn_movcc:
        if (dest && dest->type == libdis::op_register &&
            dest->data.reg.id == bad_register_.id)
          register_valid_ = false;
        break;
      case libdis::insn_popregs:
        register_valid_ = false;
        break;
      case libdis::insn_xchg:
      case libdis::insn_xchgcc:
        if (dest && dest->type == libdis::op_register &&
            src && src->type == libdis::op_register) {
          if (dest->data.reg.id == bad_register_.id)
            memcpy(&bad_register_, &src->data.reg, sizeof(libdis::x86_reg_t));
          else if (src->data.reg.id == bad_register_.id)
            memcpy(&bad_register_, &dest->data.reg, sizeof(libdis::x86_reg_t));
        }
        break;
      default:
        break;
    }
  }

  return instr_size;
}

bool DisassemblerX86::setBadRead() {
  if (!instr_valid_)
    return false;

  libdis::x86_op_t *operand = libdis::x86_get_src_operand(&current_instr_);
  if (!operand || operand->type != libdis::op_expression)
    return false;

  memcpy(&bad_register_, &operand->data.expression.base,
         sizeof(libdis::x86_reg_t));
  register_valid_ = true;
  return true;
}

bool DisassemblerX86::setBadWrite() {
  if (!instr_valid_)
    return false;

  libdis::x86_op_t *operand = libdis::x86_get_dest_operand(&current_instr_);
  if (!operand || operand->type != libdis::op_expression)
    return false;

  memcpy(&bad_register_, &operand->data.expression.base,
         sizeof(libdis::x86_reg_t));
  register_valid_ = true;
  return true;
}

}  
