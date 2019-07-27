

























#include "jit/arm64/vixl/MacroAssembler-vixl.h"

#include <ctype.h>

namespace vixl {


void MacroAssembler::B(Label* label, BranchType type, Register reg, int bit) {
  VIXL_ASSERT((reg.Is(NoReg) || (type >= kBranchTypeFirstUsingReg)));
  VIXL_ASSERT((bit == -1) || (type >= kBranchTypeFirstUsingBit));

  if (kBranchTypeFirstCondition <= type && type <= kBranchTypeLastCondition) {
    B(static_cast<Condition>(type), label);
  } else {
    switch (type) {
    case always:
      B(label);
      break;
    case never:
      break;
    case reg_zero:
      Cbz(reg, label);
      break;
    case reg_not_zero:
      Cbnz(reg, label);
      break;
    case reg_bit_clear:
      Tbz(reg, bit, label);
      break;
    case reg_bit_set:
      Tbnz(reg, bit, label);
      break;
    default:
      VIXL_UNREACHABLE();
    }
  }
}


void MacroAssembler::And(const Register& rd, const Register& rn, const Operand& operand) {
  LogicalMacro(rd, rn, operand, AND);
}


void MacroAssembler::Ands(const Register& rd, const Register& rn, const Operand& operand) {
  LogicalMacro(rd, rn, operand, ANDS);
}


void MacroAssembler::Tst(const Register& rn, const Operand& operand) {
  Ands(AppropriateZeroRegFor(rn), rn, operand);
}


void MacroAssembler::Bic(const Register& rd, const Register& rn, const Operand& operand) {
  LogicalMacro(rd, rn, operand, BIC);
}


void MacroAssembler::Bics(const Register& rd, const Register& rn, const Operand& operand) {
  LogicalMacro(rd, rn, operand, BICS);
}


void MacroAssembler::Orr(const Register& rd, const Register& rn, const Operand& operand) {
  LogicalMacro(rd, rn, operand, ORR);
}


void MacroAssembler::Orn(const Register& rd, const Register& rn, const Operand& operand) {
  LogicalMacro(rd, rn, operand, ORN);
}


void MacroAssembler::Eor(const Register& rd, const Register& rn, const Operand& operand) {
  LogicalMacro(rd, rn, operand, EOR);
}


void MacroAssembler::Eon(const Register& rd, const Register& rn, const Operand& operand) {
  LogicalMacro(rd, rn, operand, EON);
}


void MacroAssembler::LogicalMacro(const Register& rd, const Register& rn,
                                  const Operand& operand, LogicalOp op) {
  UseScratchRegisterScope temps(this);

  if (operand.IsImmediate()) {
    int64_t immediate = operand.immediate();
    unsigned reg_size = rd.size();

    
    if ((op & NOT) == NOT) {
      op = static_cast<LogicalOp>(op & ~NOT);
      immediate = ~immediate;
    }

    
    if (rd.Is32Bits()) {
      
      VIXL_ASSERT(((immediate >> kWRegSize) == 0) ||
                 ((immediate >> kWRegSize) == -1));
      immediate &= kWRegMask;
    }

    VIXL_ASSERT(rd.Is64Bits() || is_uint32(immediate));

    
    if (immediate == 0) {
      switch (op) {
      case AND:
        Mov(rd, 0);
        return;
      case ORR:  
      case EOR:
        Mov(rd, rn);
        return;
      case ANDS:  
      case BICS:
        break;
      default:
        VIXL_UNREACHABLE();
      }
    } else if ((rd.Is64Bits() && (immediate == -1)) ||
               (rd.Is32Bits() && (immediate == 0xffffffff))) {
      switch (op) {
      case AND:
        Mov(rd, rn);
        return;
      case ORR:
        Mov(rd, immediate);
        return;
      case EOR:
        Mvn(rd, rn);
        return;
      case ANDS:  
      case BICS:
        break;
      default:
        VIXL_UNREACHABLE();
      }
    }

    unsigned n, imm_s, imm_r;
    if (IsImmLogical(immediate, reg_size, &n, &imm_s, &imm_r)) {
      
      LogicalImmediate(rd, rn, n, imm_s, imm_r, op);
    } else {
      
      Register temp = temps.AcquireSameSizeAs(rn);
      Operand imm_operand = MoveImmediateForShiftedOp(temp, immediate);

      
      VIXL_ASSERT(!temp.Is(rd) && !temp.Is(rn));
      VIXL_ASSERT(!temp.Is(operand.maybeReg()));

      if (rd.Is(sp)) {
        
        
        Logical(temp, rn, imm_operand, op);
        Mov(sp, temp);
      } else {
        Logical(rd, rn, imm_operand, op);
      }
    }

  } else if (operand.IsExtendedRegister()) {
    VIXL_ASSERT(operand.reg().size() <= rd.size());
    
    
    VIXL_ASSERT(operand.shift_amount() <= 4);
    VIXL_ASSERT(operand.reg().Is64Bits() ||
               ((operand.extend() != UXTX) && (operand.extend() != SXTX)));

    temps.Exclude(operand.reg());
    Register temp = temps.AcquireSameSizeAs(rn);

    
    VIXL_ASSERT(!temp.Is(rd) && !temp.Is(rn));
    VIXL_ASSERT(!temp.Is(operand.maybeReg()));

    EmitExtendShift(temp, operand.reg(), operand.extend(),
                    operand.shift_amount());
    Logical(rd, rn, Operand(temp), op);
  } else {
    
    VIXL_ASSERT(operand.IsShiftedRegister());
    Logical(rd, rn, operand, op);
  }
}


void MacroAssembler::Mov(const Register& rd, const Operand& operand, DiscardMoveMode discard_mode) {
  if (operand.IsImmediate()) {
    
    Mov(rd, operand.immediate());
  } else if (operand.IsShiftedRegister() && (operand.shift_amount() != 0)) {
    
    
    
    EmitShift(rd, operand.reg(), operand.shift(), operand.shift_amount());
  } else if (operand.IsExtendedRegister()) {
    
    
    EmitExtendShift(rd, operand.reg(), operand.extend(), operand.shift_amount());
  } else {
    
    
    
    
    
    
    
    
    
    if (!rd.Is(operand.reg()) || (rd.Is32Bits() && (discard_mode == kDontDiscardForSameWReg)))
      mov(rd, operand.reg());
  }
}


void MacroAssembler::Mvn(const Register& rd, const Operand& operand) {
  if (operand.IsImmediate()) {
    
    Mvn(rd, operand.immediate());
  } else if (operand.IsExtendedRegister()) {
    UseScratchRegisterScope temps(this);
    temps.Exclude(operand.reg());

    
    
    Register temp = temps.AcquireSameSizeAs(rd);

    
    VIXL_ASSERT(!temp.Is(rd) && !temp.Is(operand.maybeReg()));

    EmitExtendShift(temp, operand.reg(), operand.extend(), operand.shift_amount());
    mvn(rd, Operand(temp));
  } else {
    
    
    mvn(rd, operand);
  }
}


void MacroAssembler::Mov(const Register& rd, uint64_t imm) {
  VIXL_ASSERT(is_uint32(imm) || is_int32(imm) || rd.Is64Bits());

  
  
  

  
  
  
  
  
  
  
  
  
  
  

  
  
  if (!TryOneInstrMoveImmediate(rd, imm)) {
    unsigned reg_size = rd.size();

    
    
    
    

    uint64_t ignored_halfword = 0;
    bool invert_move = false;
    
    
    if (CountClearHalfWords(~imm, reg_size) > CountClearHalfWords(imm, reg_size)) {
      ignored_halfword = 0xffff;
      invert_move = true;
    }

    
    
    UseScratchRegisterScope temps(this);
    Register temp = rd.IsSP() ? temps.AcquireSameSizeAs(rd) : rd;

    
    
    VIXL_ASSERT((reg_size % 16) == 0);
    bool first_mov_done = false;
    for (unsigned i = 0; i < (temp.size() / 16); i++) {
      uint64_t imm16 = (imm >> (16 * i)) & 0xffff;
      if (imm16 != ignored_halfword) {
        if (!first_mov_done) {
          if (invert_move)
            movn(temp, ~imm16 & 0xffff, 16 * i);
          else
            movz(temp, imm16, 16 * i);
          first_mov_done = true;
        } else {
          
          movk(temp, imm16, 16 * i);
        }
      }
    }

    VIXL_ASSERT(first_mov_done);

    
    
    if (rd.IsSP())
      mov(rd, temp);
  }
}


unsigned MacroAssembler::CountClearHalfWords(uint64_t imm, unsigned reg_size) {
  VIXL_ASSERT((reg_size % 8) == 0);
  int count = 0;
  for (unsigned i = 0; i < (reg_size / 16); i++) {
    if ((imm & 0xffff) == 0)
      count++;
    imm >>= 16;
  }
  return count;
}




bool MacroAssembler::IsImmMovz(uint64_t imm, unsigned reg_size) {
  VIXL_ASSERT((reg_size == kXRegSize) || (reg_size == kWRegSize));
  return CountClearHalfWords(imm, reg_size) >= ((reg_size / 16) - 1);
}




bool MacroAssembler::IsImmMovn(uint64_t imm, unsigned reg_size) {
  return IsImmMovz(~imm, reg_size);
}


void MacroAssembler::Ccmp(const Register& rn, const Operand& operand,
                          StatusFlags nzcv, Condition cond) {
  if (operand.IsImmediate() && (operand.immediate() < 0))
    ConditionalCompareMacro(rn, -operand.immediate(), nzcv, cond, CCMN);
  else
    ConditionalCompareMacro(rn, operand, nzcv, cond, CCMP);
}


void MacroAssembler::Ccmn(const Register& rn, const Operand& operand,
                          StatusFlags nzcv, Condition cond) {
  if (operand.IsImmediate() && (operand.immediate() < 0))
    ConditionalCompareMacro(rn, -operand.immediate(), nzcv, cond, CCMP);
  else
    ConditionalCompareMacro(rn, operand, nzcv, cond, CCMN);
}


void MacroAssembler::ConditionalCompareMacro(const Register& rn, const Operand& operand,
                                             StatusFlags nzcv, Condition cond,
                                             ConditionalCompareOp op) {
  VIXL_ASSERT((cond != al) && (cond != nv));
  if ((operand.IsShiftedRegister() && (operand.shift_amount() == 0)) ||
      (operand.IsImmediate() && IsImmConditionalCompare(operand.immediate()))) {
    
    
    ConditionalCompare(rn, operand, nzcv, cond, op);
  } else {
    UseScratchRegisterScope temps(this);
    
    
    Register temp = temps.AcquireSameSizeAs(rn);
    VIXL_ASSERT(!temp.Is(rn) && !temp.Is(operand.maybeReg()));

    Mov(temp, operand);
    ConditionalCompare(rn, temp, nzcv, cond, op);
  }
}


void MacroAssembler::Csel(const Register& rd, const Register& rn,
                          const Operand& operand, Condition cond) {
  VIXL_ASSERT(!rd.IsZero());
  VIXL_ASSERT(!rn.IsZero());
  VIXL_ASSERT((cond != al) && (cond != nv));
  if (operand.IsImmediate()) {
    
    
    int64_t imm = operand.immediate();
    Register zr = AppropriateZeroRegFor(rn);
    if (imm == 0) {
      csel(rd, rn, zr, cond);
    } else if (imm == 1) {
      csinc(rd, rn, zr, cond);
    } else if (imm == -1) {
      csinv(rd, rn, zr, cond);
    } else {
      UseScratchRegisterScope temps(this);
      Register temp = temps.AcquireSameSizeAs(rn);
      VIXL_ASSERT(!temp.Is(rd) && !temp.Is(rn));
      VIXL_ASSERT(!temp.Is(operand.maybeReg()));

      Mov(temp, operand.immediate());
      csel(rd, rn, temp, cond);
    }
  } else if (operand.IsShiftedRegister() && (operand.shift_amount() == 0)) {
    
    csel(rd, rn, operand.reg(), cond);
  } else {
    
    UseScratchRegisterScope temps(this);
    Register temp = temps.AcquireSameSizeAs(rn);
    VIXL_ASSERT(!temp.Is(rd) && !temp.Is(rn));
    VIXL_ASSERT(!temp.Is(operand.maybeReg()));

    Mov(temp, operand);
    csel(rd, rn, temp, cond);
  }
}


void MacroAssembler::Add(const Register& rd, const Register& rn, const Operand& operand) {
  if (operand.IsImmediate() && (operand.immediate() < 0) && IsImmAddSub(-operand.immediate()))
    AddSubMacro(rd, rn, -operand.immediate(), LeaveFlags, SUB);
  else
    AddSubMacro(rd, rn, operand, LeaveFlags, ADD);
}


void MacroAssembler::Adds(const Register& rd, const Register& rn, const Operand& operand) {
  if (operand.IsImmediate() && (operand.immediate() < 0) && IsImmAddSub(-operand.immediate()))
    AddSubMacro(rd, rn, -operand.immediate(), SetFlags, SUB);
  else
    AddSubMacro(rd, rn, operand, SetFlags, ADD);
}


void MacroAssembler::Sub(const Register& rd, const Register& rn, const Operand& operand) {
  if (operand.IsImmediate() && (operand.immediate() < 0) && IsImmAddSub(-operand.immediate()))
    AddSubMacro(rd, rn, -operand.immediate(), LeaveFlags, ADD);
  else
    AddSubMacro(rd, rn, operand, LeaveFlags, SUB);
}


void MacroAssembler::Subs(const Register& rd, const Register& rn, const Operand& operand) {
  if (operand.IsImmediate() && (operand.immediate() < 0) && IsImmAddSub(-operand.immediate()))
    AddSubMacro(rd, rn, -operand.immediate(), SetFlags, ADD);
  else
    AddSubMacro(rd, rn, operand, SetFlags, SUB);
}


void MacroAssembler::Cmn(const Register& rn, const Operand& operand) {
  Adds(AppropriateZeroRegFor(rn), rn, operand);
}


void MacroAssembler::Cmp(const Register& rn, const Operand& operand) {
  Subs(AppropriateZeroRegFor(rn), rn, operand);
}


void MacroAssembler::Fcmp(const FPRegister& fn, double value) {
  if (value != 0.0) {
    UseScratchRegisterScope temps(this);
    FPRegister temp = temps.AcquireSameSizeAs(fn);
    VIXL_ASSERT(!temp.Is(fn));

    Fmov(temp, value);
    fcmp(fn, temp);
  } else {
    fcmp(fn, value);
  }
}


void MacroAssembler::Fmov(FPRegister fd, double imm) {
  if (fd.Is32Bits()) {
    Fmov(fd, static_cast<float>(imm));
    return;
  }

  VIXL_ASSERT(fd.Is64Bits());
  if (IsImmFP64(imm))
    fmov(fd, imm);
  else if ((imm == 0.0) && (copysign(1.0, imm) == 1.0))
    fmov(fd, xzr);
  else
    Assembler::fImmPool64(fd, imm);
}


void MacroAssembler::Fmov(FPRegister fd, float imm) {
  if (fd.Is64Bits()) {
    Fmov(fd, static_cast<double>(imm));
    return;
  }

  VIXL_ASSERT(fd.Is32Bits());
  if (IsImmFP32(imm))
    fmov(fd, imm);
  else if ((imm == 0.0) && (copysign(1.0, imm) == 1.0))
    fmov(fd, wzr);
  else
    Assembler::fImmPool32(fd, imm);
}


void MacroAssembler::Neg(const Register& rd, const Operand& operand) {
  if (operand.IsImmediate())
    Mov(rd, -operand.immediate());
  else
    Sub(rd, AppropriateZeroRegFor(rd), operand);
}


void MacroAssembler::Negs(const Register& rd, const Operand& operand) {
  Subs(rd, AppropriateZeroRegFor(rd), operand);
}


bool MacroAssembler::TryOneInstrMoveImmediate(const Register& dst, int64_t imm) {
  unsigned n, imm_s, imm_r;
  int reg_size = dst.size();

  if (imm == 0 && !dst.IsSP()) {
    
    mov(dst, AppropriateZeroRegFor(dst));
    return true;
  }

  if (IsImmMovz(imm, reg_size) && !dst.IsSP()) {
    
    
    movz(dst, imm);
    return true;
  }

  if (IsImmMovn(imm, reg_size) && !dst.IsSP()) {
    
    
    movn(dst, dst.Is64Bits() ? ~imm : (~imm & kWRegMask));
    return true;
  }

  if (IsImmLogical(imm, reg_size, &n, &imm_s, &imm_r)) {
    
    VIXL_ASSERT(!dst.IsZero());
    LogicalImmediate(dst, AppropriateZeroRegFor(dst), n, imm_s, imm_r, ORR);
    return true;
  }

  return false;
}


Operand MacroAssembler::MoveImmediateForShiftedOp(const Register& dst, int64_t imm) {
  int reg_size = dst.size();

  
  if (TryOneInstrMoveImmediate(dst, imm)) {
    
  } else {
    
    int shift_low = CountTrailingZeros(imm, reg_size);
    int64_t imm_low = imm >> shift_low;

    
    
    int shift_high = CountLeadingZeros(imm, reg_size);
    int64_t imm_high = (imm << shift_high) | ((1 << shift_high) - 1);

    if (TryOneInstrMoveImmediate(dst, imm_low)) {
      
      
      return Operand(dst, LSL, shift_low);
    } else if (TryOneInstrMoveImmediate(dst, imm_high)) {
      
      
      return Operand(dst, LSR, shift_high);
    } else {
      Mov(dst, imm);
    }
  }
  return Operand(dst);
}


void MacroAssembler::AddSubMacro(const Register& rd, const Register& rn,
                                 const Operand& operand, FlagsUpdate S, AddSubOp op) {
  if (operand.IsZero() && rd.Is(rn) && rd.Is64Bits() && rn.Is64Bits() &&
      (S == LeaveFlags)) {
    
    return;
  }

  if ((operand.IsImmediate() && !IsImmAddSub(operand.immediate())) ||
      (rn.IsZero() && !operand.IsShiftedRegister())                ||
      (operand.IsShiftedRegister() && (operand.shift() == ROR))) {
    UseScratchRegisterScope temps(this);
    Register temp = temps.AcquireSameSizeAs(rn);

    
    VIXL_ASSERT(!temp.Is(rd) && !temp.Is(rn));
    VIXL_ASSERT(!temp.Is(operand.maybeReg()));

    if (operand.IsImmediate()) {
      Operand imm_operand = MoveImmediateForShiftedOp(temp, operand.immediate());
      AddSub(rd, rn, imm_operand, S, op);
    } else {
      Mov(temp, operand);
      AddSub(rd, rn, temp, S, op);
    }
  } else {
    AddSub(rd, rn, operand, S, op);
  }
}


void MacroAssembler::Adc(const Register& rd, const Register& rn, const Operand& operand) {
  AddSubWithCarryMacro(rd, rn, operand, LeaveFlags, ADC);
}


void MacroAssembler::Adcs(const Register& rd, const Register& rn, const Operand& operand) {
  AddSubWithCarryMacro(rd, rn, operand, SetFlags, ADC);
}


void MacroAssembler::Sbc(const Register& rd, const Register& rn, const Operand& operand) {
  AddSubWithCarryMacro(rd, rn, operand, LeaveFlags, SBC);
}


void MacroAssembler::Sbcs(const Register& rd, const Register& rn, const Operand& operand) {
  AddSubWithCarryMacro(rd, rn, operand, SetFlags, SBC);
}


void MacroAssembler::Ngc(const Register& rd, const Operand& operand) {
  Register zr = AppropriateZeroRegFor(rd);
  Sbc(rd, zr, operand);
}


void MacroAssembler::Ngcs(const Register& rd, const Operand& operand) {
  Register zr = AppropriateZeroRegFor(rd);
  Sbcs(rd, zr, operand);
}


void MacroAssembler::AddSubWithCarryMacro(const Register& rd, const Register& rn,
                                          const Operand& operand, FlagsUpdate S, AddSubWithCarryOp op) {
  VIXL_ASSERT(rd.size() == rn.size());
  UseScratchRegisterScope temps(this);

  if (operand.IsImmediate() || (operand.IsShiftedRegister() && (operand.shift() == ROR))) {
    
    Register temp = temps.AcquireSameSizeAs(rn);
    VIXL_ASSERT(!temp.Is(rd) && !temp.Is(rn) && !temp.Is(operand.maybeReg()));

    Mov(temp, operand);
    AddSubWithCarry(rd, rn, Operand(temp), S, op);
  } else if (operand.IsShiftedRegister() && (operand.shift_amount() != 0)) {
    
    VIXL_ASSERT(operand.reg().size() == rd.size());
    VIXL_ASSERT(operand.shift() != ROR);
    VIXL_ASSERT(is_uintn(rd.size() == kXRegSize ? kXRegSizeLog2 : kWRegSizeLog2,
                        operand.shift_amount()));

    temps.Exclude(operand.reg());
    Register temp = temps.AcquireSameSizeAs(rn);
    VIXL_ASSERT(!temp.Is(rd) && !temp.Is(rn) && !temp.Is(operand.maybeReg()));

    EmitShift(temp, operand.reg(), operand.shift(), operand.shift_amount());
    AddSubWithCarry(rd, rn, Operand(temp), S, op);
  } else if (operand.IsExtendedRegister()) {
    
    VIXL_ASSERT(operand.reg().size() <= rd.size());
    
    
    VIXL_ASSERT(operand.shift_amount() <= 4);
    VIXL_ASSERT(operand.reg().Is64Bits() ||
               ((operand.extend() != UXTX) && (operand.extend() != SXTX)));

    temps.Exclude(operand.reg());
    Register temp = temps.AcquireSameSizeAs(rn);
    VIXL_ASSERT(!temp.Is(rd) && !temp.Is(rn) && !temp.Is(operand.maybeReg()));

    EmitExtendShift(temp, operand.reg(), operand.extend(), operand.shift_amount());
    AddSubWithCarry(rd, rn, Operand(temp), S, op);
  } else {
    
    AddSubWithCarry(rd, rn, operand, S, op);
  }
}


#define DEFINE_FUNCTION(FN, REGTYPE, REG, OP)                             \
void MacroAssembler::FN(const REGTYPE REG, const MemOperand& addr) {  \
    LoadStoreMacro(REG, addr, OP);                                        \
}
LS_MACRO_LIST(DEFINE_FUNCTION)
#undef DEFINE_FUNCTION



BufferOffset MacroAssembler::LoadStoreMacro(const CPURegister& rt, const MemOperand& addr,
                                            LoadStoreOp op) {
  int64_t offset = addr.offset();
  LSDataSize size = CalcLSDataSize(op);

  
  
  
  if (addr.IsImmediateOffset() && !IsImmLSScaled(offset, size) && !IsImmLSUnscaled(offset)) {
    
    
    UseScratchRegisterScope temps(this);
    Register temp = temps.AcquireSameSizeAs(addr.base());
    VIXL_ASSERT(!temp.Is(rt));
    VIXL_ASSERT(!temp.Is(addr.base()) && !temp.Is(addr.regoffset()));

    Mov(temp, addr.offset());
    return LoadStore(rt, MemOperand(addr.base(), temp), op);
  } else if (addr.IsPostIndex() && !IsImmLSUnscaled(offset)) {
    
    BufferOffset ret = LoadStore(rt, MemOperand(addr.base()), op);
    Add(addr.base(), addr.base(), Operand(offset));
    return ret;
  } else if (addr.IsPreIndex() && !IsImmLSUnscaled(offset)) {
    
    Add(addr.base(), addr.base(), Operand(offset));
    return LoadStore(rt, MemOperand(addr.base()), op);
  } else {
    
    return LoadStore(rt, addr, op);
  }
}


void MacroAssembler::PushStackPointer() {
  PrepareForPush(1, 8);

  
  
  
  
  
  
  UseScratchRegisterScope temps(this);
  Register scratch = temps.AcquireX();

  Mov(scratch, GetStackPointer64());
  str(scratch, MemOperand(GetStackPointer64(), -8, PreIndex));
}


void MacroAssembler::Push(const CPURegister& src0, const CPURegister& src1,
                     const CPURegister& src2, const CPURegister& src3) {
  VIXL_ASSERT(AreSameSizeAndType(src0, src1, src2, src3));
  VIXL_ASSERT(src0.IsValid());

  int count = 1 + src1.IsValid() + src2.IsValid() + src3.IsValid();
  int size = src0.SizeInBytes();

  if (src0.Is(GetStackPointer64())) {
    VIXL_ASSERT(count == 1);
    VIXL_ASSERT(size == 8);
    PushStackPointer();
    return;
  }

  PrepareForPush(count, size);
  PushHelper(count, size, src0, src1, src2, src3);
}


void MacroAssembler::Pop(const CPURegister& dst0, const CPURegister& dst1,
                         const CPURegister& dst2, const CPURegister& dst3) {
  
  
  VIXL_ASSERT(!AreAliased(dst0, dst1, dst2, dst3));
  VIXL_ASSERT(AreSameSizeAndType(dst0, dst1, dst2, dst3));
  VIXL_ASSERT(dst0.IsValid());

  int count = 1 + dst1.IsValid() + dst2.IsValid() + dst3.IsValid();
  int size = dst0.SizeInBytes();

  PrepareForPop(count, size);
  PopHelper(count, size, dst0, dst1, dst2, dst3);
}


void MacroAssembler::PushCPURegList(CPURegList registers) {
  int size = registers.RegisterSizeInBytes();

  PrepareForPush(registers.Count(), size);
  
  
  
  while (!registers.IsEmpty()) {
    int count_before = registers.Count();
    const CPURegister& src0 = registers.PopHighestIndex();
    const CPURegister& src1 = registers.PopHighestIndex();
    const CPURegister& src2 = registers.PopHighestIndex();
    const CPURegister& src3 = registers.PopHighestIndex();
    int count = count_before - registers.Count();
    PushHelper(count, size, src0, src1, src2, src3);
  }
}


void MacroAssembler::PopCPURegList(CPURegList registers) {
  int size = registers.RegisterSizeInBytes();

  PrepareForPop(registers.Count(), size);
  
  
  
  while (!registers.IsEmpty()) {
    int count_before = registers.Count();
    const CPURegister& dst0 = registers.PopLowestIndex();
    const CPURegister& dst1 = registers.PopLowestIndex();
    const CPURegister& dst2 = registers.PopLowestIndex();
    const CPURegister& dst3 = registers.PopLowestIndex();
    int count = count_before - registers.Count();
    PopHelper(count, size, dst0, dst1, dst2, dst3);
  }
}


void MacroAssembler::PushMultipleTimes(int count, Register src) {
  int size = src.SizeInBytes();

  PrepareForPush(count, size);
  
  
  
  while (count >= 4) {
    PushHelper(4, size, src, src, src, src);
    count -= 4;
  }
  if (count >= 2) {
    PushHelper(2, size, src, src, NoReg, NoReg);
    count -= 2;
  }
  if (count == 1) {
    PushHelper(1, size, src, NoReg, NoReg, NoReg);
    count -= 1;
  }
  VIXL_ASSERT(count == 0);
}


void MacroAssembler::PushHelper(int count, int size, const CPURegister& src0,
                                const CPURegister& src1, const CPURegister& src2,
                                const CPURegister& src3) {
  
  InstructionAccurateScope scope(this);

  VIXL_ASSERT(AreSameSizeAndType(src0, src1, src2, src3));
  VIXL_ASSERT(size == src0.SizeInBytes());

  
  VIXL_ASSERT(!src0.Is(GetStackPointer64()) && !src0.Is(sp));
  VIXL_ASSERT(!src1.Is(GetStackPointer64()) && !src1.Is(sp));
  VIXL_ASSERT(!src2.Is(GetStackPointer64()) && !src2.Is(sp));
  VIXL_ASSERT(!src3.Is(GetStackPointer64()) && !src3.Is(sp));

  
  VIXL_ASSERT(size >= 8);

  
  
  switch (count) {
  case 1:
    VIXL_ASSERT(src1.IsNone() && src2.IsNone() && src3.IsNone());
    str(src0, MemOperand(GetStackPointer64(), -1 * size, PreIndex));
    break;
  case 2:
    VIXL_ASSERT(src2.IsNone() && src3.IsNone());
    stp(src1, src0, MemOperand(GetStackPointer64(), -2 * size, PreIndex));
    break;
  case 3:
    VIXL_ASSERT(src3.IsNone());
    stp(src2, src1, MemOperand(GetStackPointer64(), -3 * size, PreIndex));
    str(src0, MemOperand(GetStackPointer64(), 2 * size));
    break;
  case 4:
    
    
    
    stp(src3, src2, MemOperand(GetStackPointer64(), -4 * size, PreIndex));
    stp(src1, src0, MemOperand(GetStackPointer64(), 2 * size));
    break;
  default:
    VIXL_UNREACHABLE();
  }
}


void MacroAssembler::PopHelper(int count, int size, const CPURegister& dst0,
                               const CPURegister& dst1, const CPURegister& dst2,
                               const CPURegister& dst3) {
  
  InstructionAccurateScope scope(this);

  VIXL_ASSERT(AreSameSizeAndType(dst0, dst1, dst2, dst3));
  VIXL_ASSERT(size == dst0.SizeInBytes());

  
  
  switch (count) {
  case 1:
    VIXL_ASSERT(dst1.IsNone() && dst2.IsNone() && dst3.IsNone());
    ldr(dst0, MemOperand(GetStackPointer64(), 1 * size, PostIndex));
    break;
  case 2:
    VIXL_ASSERT(dst2.IsNone() && dst3.IsNone());
    ldp(dst0, dst1, MemOperand(GetStackPointer64(), 2 * size, PostIndex));
    break;
  case 3:
    VIXL_ASSERT(dst3.IsNone());
    ldr(dst2, MemOperand(GetStackPointer64(), 2 * size));
    ldp(dst0, dst1, MemOperand(GetStackPointer64(), 3 * size, PostIndex));
    break;
  case 4:
    
    
    
    
    ldp(dst2, dst3, MemOperand(GetStackPointer64(), 2 * size));
    ldp(dst0, dst1, MemOperand(GetStackPointer64(), 4 * size, PostIndex));
    break;
  default:
    VIXL_UNREACHABLE();
  }
}


void MacroAssembler::PrepareForPush(int count, int size) {
  if (sp.Is(GetStackPointer64())) {
    
    
    
    VIXL_ASSERT((count * size) % 16 == 0);
  } else {
    
    
    
    BumpSystemStackPointer(count * size);
  }
}


void MacroAssembler::PrepareForPop(int count, int size) {
  if (sp.Is(GetStackPointer64())) {
    
    
    
    VIXL_ASSERT((count * size) % 16 == 0);
  }
}


void MacroAssembler::Poke(const Register& src, const Operand& offset) {
  if (offset.IsImmediate())
    VIXL_ASSERT(offset.immediate() >= 0);

  Str(src, MemOperand(GetStackPointer64(), offset));
}


void MacroAssembler::Peek(const Register& dst, const Operand& offset) {
  if (offset.IsImmediate())
    VIXL_ASSERT(offset.immediate() >= 0);

  Ldr(dst, MemOperand(GetStackPointer64(), offset));
}


void MacroAssembler::Claim(const Operand& size) {
  if (size.IsZero())
    return;

  if (size.IsImmediate()) {
    VIXL_ASSERT(size.immediate() > 0);
    if (sp.Is(GetStackPointer64()))
      VIXL_ASSERT((size.immediate() % 16) == 0);
  }

  if (!sp.Is(GetStackPointer64()))
    BumpSystemStackPointer(size);

  Sub(GetStackPointer64(), GetStackPointer64(), size);
}


void MacroAssembler::Drop(const Operand& size) {
  if (size.IsZero())
    return;

  if (size.IsImmediate()) {
    VIXL_ASSERT(size.immediate() > 0);
    if (sp.Is(GetStackPointer64()))
      VIXL_ASSERT((size.immediate() % 16) == 0);
  }

  Add(GetStackPointer64(), GetStackPointer64(), size);
}


void MacroAssembler::PushCalleeSavedRegisters() {
  
  InstructionAccurateScope scope(this);

  
  VIXL_ASSERT(sp.Is(GetStackPointer64()));

  MemOperand tos(sp, -2 * kXRegSizeInBytes, PreIndex);

  stp(x29, x30, tos);
  stp(x27, x28, tos);
  stp(x25, x26, tos);
  stp(x23, x24, tos);
  stp(x21, x22, tos);
  stp(x19, x20, tos);

  stp(d14, d15, tos);
  stp(d12, d13, tos);
  stp(d10, d11, tos);
  stp(d8, d9, tos);
}


void MacroAssembler::PopCalleeSavedRegisters() {
  
  InstructionAccurateScope scope(this);

  
  VIXL_ASSERT(sp.Is(GetStackPointer64()));

  MemOperand tos(sp, 2 * kXRegSizeInBytes, PostIndex);

  ldp(d8, d9, tos);
  ldp(d10, d11, tos);
  ldp(d12, d13, tos);
  ldp(d14, d15, tos);

  ldp(x19, x20, tos);
  ldp(x21, x22, tos);
  ldp(x23, x24, tos);
  ldp(x25, x26, tos);
  ldp(x27, x28, tos);
  ldp(x29, x30, tos);
}


void MacroAssembler::BumpSystemStackPointer(const Operand& space) {
  VIXL_ASSERT(!sp.Is(GetStackPointer64()));
  
  
  
  InstructionAccurateScope scope(this);
  sub(sp, GetStackPointer64(), space);
}




void MacroAssembler::PrintfNoPreserve(const char * format, const CPURegister& arg0,
                                      const CPURegister& arg1, const CPURegister& arg2,
                                      const CPURegister& arg3) {
  
  
  VIXL_ASSERT(!kCallerSaved.IncludesAliasOf(GetStackPointer64()));

  
  CPURegister args[kPrintfMaxArgCount] = {arg0, arg1, arg2, arg3};
  CPURegister pcs[kPrintfMaxArgCount];

  int arg_count = kPrintfMaxArgCount;

  
  
  static const CPURegList kPCSVarargs =
    CPURegList(CPURegister::kRegister, kXRegSize, 1, arg_count);
  static const CPURegList kPCSVarargsFP =
    CPURegList(CPURegister::kFPRegister, kDRegSize, 0, arg_count - 1);

  
  
  UseScratchRegisterScope temps(this);
  temps.Include(kCallerSaved);
  temps.Include(kCallerSavedFP);
  temps.Exclude(kPCSVarargs);
  temps.Exclude(kPCSVarargsFP);
  temps.Exclude(arg0, arg1, arg2, arg3);

  
  CPURegList pcs_varargs = kPCSVarargs;
  CPURegList pcs_varargs_fp = kPCSVarargsFP;

  
  
  
  
  for (unsigned i = 0; i < kPrintfMaxArgCount; i++) {
    
    if (args[i].IsRegister()) {
      pcs[i] = pcs_varargs.PopLowestIndex().X();
      
      
      if (args[i].Is32Bits())
        pcs[i] = pcs[i].W();
    } else if (args[i].IsFPRegister()) {
      
      pcs[i] = pcs_varargs_fp.PopLowestIndex().D();
    } else {
      VIXL_ASSERT(args[i].IsNone());
      arg_count = i;
      break;
    }

    
    if (args[i].Aliases(pcs[i]))
      continue;

    
    
    if (kPCSVarargs.IncludesAliasOf(args[i]) || kPCSVarargsFP.IncludesAliasOf(args[i])) {
      if (args[i].IsRegister()) {
        Register old_arg = Register(args[i]);
        Register new_arg = temps.AcquireSameSizeAs(old_arg);
        Mov(new_arg, old_arg);
        args[i] = new_arg;
      } else {
        FPRegister old_arg = FPRegister(args[i]);
        FPRegister new_arg = temps.AcquireSameSizeAs(old_arg);
        Fmov(new_arg, old_arg);
        args[i] = new_arg;
      }
    }
  }

  
  
  for (int i = 0; i < arg_count; i++) {
    VIXL_ASSERT(pcs[i].type() == args[i].type());
    if (pcs[i].IsRegister()) {
      Mov(Register(pcs[i]), Register(args[i]), kDiscardForSameWReg);
    } else {
      VIXL_ASSERT(pcs[i].IsFPRegister());
      if (pcs[i].size() == args[i].size())
        Fmov(FPRegister(pcs[i]), FPRegister(args[i]));
      else
        Fcvt(FPRegister(pcs[i]), FPRegister(args[i]));
    }
  }

  
  
  
  
  
  
  temps.Exclude(x0);
  Label format_address;
  Adr(x0, &format_address);

  
  {
    flushBuffer();
    Label after_data;
    B(&after_data);
    Bind(&format_address);
    EmitStringData(format);
    Unreachable();
    Bind(&after_data);
  }

  
  
  if (!sp.Is(GetStackPointer64()))
    Bic(sp, GetStackPointer64(), 0xf);

  
  
  
#ifdef JS_SIMULATOR_ARM64
  {
    InstructionAccurateScope scope(this, kPrintfLength / kInstructionSize);
    hlt(kPrintfOpcode);
    dc32(arg_count);          

    
    uint32_t arg_pattern_list = 0;
    for (int i = 0; i < arg_count; i++) {
      uint32_t arg_pattern;
      if (pcs[i].IsRegister()) {
        arg_pattern = pcs[i].Is32Bits() ? kPrintfArgW : kPrintfArgX;
      } else {
        VIXL_ASSERT(pcs[i].Is64Bits());
        arg_pattern = kPrintfArgD;
      }
      VIXL_ASSERT(arg_pattern < (1 << kPrintfArgPatternBits));
      arg_pattern_list |= (arg_pattern << (kPrintfArgPatternBits * i));
    }
    dc32(arg_pattern_list);   
  }
#else
  Register tmp = temps.AcquireX();
  Mov(tmp, reinterpret_cast<uintptr_t>(printf));
  Blr(tmp);
#endif
}


void MacroAssembler::Printf(const char * format, CPURegister arg0, CPURegister arg1,
                            CPURegister arg2, CPURegister arg3) {
  
  if (!sp.Is(GetStackPointer64())) {
    VIXL_ASSERT(!sp.Aliases(arg0));
    VIXL_ASSERT(!sp.Aliases(arg1));
    VIXL_ASSERT(!sp.Aliases(arg2));
    VIXL_ASSERT(!sp.Aliases(arg3));
  }

  
  
  UseScratchRegisterScope exclude_all(this);
  exclude_all.ExcludeAll();

  
  
  
  PushCPURegList(kCallerSaved);
  PushCPURegList(kCallerSavedFP);

  {
    UseScratchRegisterScope temps(this);
    
    temps.Include(kCallerSaved);
    temps.Include(kCallerSavedFP);
    temps.Exclude(arg0, arg1, arg2, arg3);

    
    
    
    bool arg0_sp = GetStackPointer64().Aliases(arg0);
    bool arg1_sp = GetStackPointer64().Aliases(arg1);
    bool arg2_sp = GetStackPointer64().Aliases(arg2);
    bool arg3_sp = GetStackPointer64().Aliases(arg3);
    if (arg0_sp || arg1_sp || arg2_sp || arg3_sp) {
      
      
      Register arg_sp = temps.AcquireX();
      Add(arg_sp, GetStackPointer64(),
          kCallerSaved.TotalSizeInBytes() + kCallerSavedFP.TotalSizeInBytes());
      if (arg0_sp) arg0 = Register(arg_sp.code(), arg0.size());
      if (arg1_sp) arg1 = Register(arg_sp.code(), arg1.size());
      if (arg2_sp) arg2 = Register(arg_sp.code(), arg2.size());
      if (arg3_sp) arg3 = Register(arg_sp.code(), arg3.size());
    }

    
    Register tmp = temps.AcquireX();
    Mrs(tmp, NZCV);
    Push(tmp, xzr);
    temps.Release(tmp);

    PrintfNoPreserve(format, arg0, arg1, arg2, arg3);

    
    tmp = temps.AcquireX();
    Pop(xzr, tmp);
    Msr(NZCV, tmp);
    temps.Release(tmp);
  }

  PopCPURegList(kCallerSavedFP);
  PopCPURegList(kCallerSaved);
}


void MacroAssembler::Trace(TraceParameters parameters, TraceCommand command) {
#ifdef JS_SIMULATOR_ARM64
  
  
  InstructionAccurateScope scope(this, kTraceLength / kInstructionSize);

  Label start;
  bind(&start);

  
  
  hlt(kTraceOpcode);

  
  dc32(parameters);

  
  dc32(command);
#else
  
#endif
}


void MacroAssembler::Log(TraceParameters parameters) {
#ifdef JS_SIMULATOR_ARM64
  
  
  InstructionAccurateScope scope(this, kLogLength / kInstructionSize);

  Label start;
  bind(&start);

  
  
  hlt(kLogOpcode);

  
  dc32(parameters);
#else
  
#endif
}


void MacroAssembler::EnableInstrumentation() {
  VIXL_ASSERT(!isprint(InstrumentStateEnable));
  InstructionAccurateScope scope(this, 1);
  movn(xzr, InstrumentStateEnable);
}


void MacroAssembler::DisableInstrumentation() {
  VIXL_ASSERT(!isprint(InstrumentStateDisable));
  InstructionAccurateScope scope(this, 1);
  movn(xzr, InstrumentStateDisable);
}


void MacroAssembler::AnnotateInstrumentation(const char* marker_name) {
  VIXL_ASSERT(strlen(marker_name) == 2);

  
  
  VIXL_ASSERT(isprint(marker_name[0]) && isprint(marker_name[1]));

  InstructionAccurateScope scope(this, 1);
  movn(xzr, (marker_name[1] << 8) | marker_name[0]);
}


UseScratchRegisterScope::~UseScratchRegisterScope() {
  available_->set_list(old_available_);
  availablefp_->set_list(old_availablefp_);
}


bool UseScratchRegisterScope::IsAvailable(const CPURegister& reg) const {
  return available_->IncludesAliasOf(reg) || availablefp_->IncludesAliasOf(reg);
}


Register UseScratchRegisterScope::AcquireSameSizeAs(const Register& reg) {
  int code = AcquireNextAvailable(available_).code();
  return Register(code, reg.size());
}


FPRegister UseScratchRegisterScope::AcquireSameSizeAs(const FPRegister& reg) {
  int code = AcquireNextAvailable(availablefp_).code();
  return FPRegister(code, reg.size());
}


void UseScratchRegisterScope::Release(const CPURegister& reg) {
  if (reg.IsRegister())
    ReleaseByCode(available_, reg.code());
  else if (reg.IsFPRegister())
    ReleaseByCode(availablefp_, reg.code());
  else
    VIXL_ASSERT(reg.IsNone());
}


void UseScratchRegisterScope::Include(const CPURegList& list) {
  if (list.type() == CPURegister::kRegister) {
    
    IncludeByRegList(available_, list.list() & ~(xzr.Bit() | sp.Bit()));
  } else {
    VIXL_ASSERT(list.type() == CPURegister::kFPRegister);
    IncludeByRegList(availablefp_, list.list());
  }
}


void UseScratchRegisterScope::Include(const Register& reg1, const Register& reg2,
                                      const Register& reg3, const Register& reg4) {
  RegList include = reg1.Bit() | reg2.Bit() | reg3.Bit() | reg4.Bit();
  
  include &= ~(xzr.Bit() | sp.Bit());

  IncludeByRegList(available_, include);
}


void UseScratchRegisterScope::Include(const FPRegister& reg1, const FPRegister& reg2,
                                      const FPRegister& reg3, const FPRegister& reg4) {
  RegList include = reg1.Bit() | reg2.Bit() | reg3.Bit() | reg4.Bit();
  IncludeByRegList(availablefp_, include);
}


void UseScratchRegisterScope::Exclude(const CPURegList& list) {
  if (list.type() == CPURegister::kRegister) {
    ExcludeByRegList(available_, list.list());
  } else {
    VIXL_ASSERT(list.type() == CPURegister::kFPRegister);
    ExcludeByRegList(availablefp_, list.list());
  }
}


void UseScratchRegisterScope::Exclude(const Register& reg1, const Register& reg2,
                                      const Register& reg3, const Register& reg4) {
  RegList exclude = reg1.Bit() | reg2.Bit() | reg3.Bit() | reg4.Bit();
  ExcludeByRegList(available_, exclude);
}


void UseScratchRegisterScope::Exclude(const FPRegister& reg1, const FPRegister& reg2,
                                      const FPRegister& reg3, const FPRegister& reg4) {
  RegList excludefp = reg1.Bit() | reg2.Bit() | reg3.Bit() | reg4.Bit();
  ExcludeByRegList(availablefp_, excludefp);
}


void UseScratchRegisterScope::Exclude(const CPURegister& reg1, const CPURegister& reg2,
                                      const CPURegister& reg3, const CPURegister& reg4) {
  RegList exclude = 0;
  RegList excludefp = 0;

  const CPURegister regs[] = {reg1, reg2, reg3, reg4};

  for (unsigned i = 0; i < (sizeof(regs) / sizeof(regs[0])); i++) {
    if (regs[i].IsRegister())
      exclude |= regs[i].Bit();
    else if (regs[i].IsFPRegister())
      excludefp |= regs[i].Bit();
    else
      VIXL_ASSERT(regs[i].IsNone());
  }

  ExcludeByRegList(available_, exclude);
  ExcludeByRegList(availablefp_, excludefp);
}


void UseScratchRegisterScope::ExcludeAll() {
  ExcludeByRegList(available_, available_->list());
  ExcludeByRegList(availablefp_, availablefp_->list());
}


CPURegister UseScratchRegisterScope::AcquireNextAvailable(CPURegList* available) {
  VIXL_ASSERT(!available->IsEmpty());
  CPURegister result = available->PopLowestIndex();
  VIXL_ASSERT(!AreAliased(result, xzr, sp));
  return result;
}


void UseScratchRegisterScope::ReleaseByCode(CPURegList* available, int code) {
  ReleaseByRegList(available, static_cast<RegList>(1) << code);
}


void UseScratchRegisterScope::ReleaseByRegList(CPURegList* available, RegList regs) {
  available->set_list(available->list() | regs);
}


void UseScratchRegisterScope::IncludeByRegList(CPURegList* available, RegList regs) {
  available->set_list(available->list() | regs);
}


void UseScratchRegisterScope::ExcludeByRegList(CPURegList* available, RegList exclude) {
  available->set_list(available->list() & ~exclude);
}


} 
