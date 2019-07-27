

























#include "jit/arm64/vixl/Assembler-vixl.h"

#include <cmath>

#include "jsutil.h"

#include "jit/Label.h"

namespace vixl {



CPURegister CPURegList::PopLowestIndex() {
  if (IsEmpty())
    return NoCPUReg;

  int index = CountTrailingZeros(list_, kRegListSizeInBits);
  VIXL_ASSERT((1 << index) & list_);
  Remove(index);
  return CPURegister(index, size_, type_);
}


CPURegister CPURegList::PopHighestIndex() {
  VIXL_ASSERT(IsValid());
  if (IsEmpty())
    return NoCPUReg;

  int index = CountLeadingZeros(list_, kRegListSizeInBits);
  index = kRegListSizeInBits - 1 - index;
  VIXL_ASSERT((1 << index) & list_);
  Remove(index);
  return CPURegister(index, size_, type_);
}


bool CPURegList::IsValid() const {
  if ((type_ == CPURegister::kRegister) || (type_ == CPURegister::kFPRegister)) {
    bool is_valid = true;
    
    for (int i = 0; i < kRegListSizeInBits; i++) {
      if (((list_ >> i) & 1) != 0)
        is_valid &= CPURegister(i, size_, type_).IsValid();
    }
    return is_valid;
  }

  if (type_ == CPURegister::kNoRegister) {
    
    return list_ == 0;
  }

  return false;
}


void CPURegList::RemoveCalleeSaved() {
  if (type() == CPURegister::kRegister) {
    Remove(GetCalleeSaved(RegisterSizeInBits()));
  } else if (type() == CPURegister::kFPRegister) {
    Remove(GetCalleeSavedFP(RegisterSizeInBits()));
  } else {
    VIXL_ASSERT(type() == CPURegister::kNoRegister);
    VIXL_ASSERT(IsEmpty());
    
  }
}


CPURegList CPURegList::GetCalleeSaved(unsigned size) {
  return CPURegList(CPURegister::kRegister, size, 19, 29);
}


CPURegList CPURegList::GetCalleeSavedFP(unsigned size) {
  return CPURegList(CPURegister::kFPRegister, size, 8, 15);
}


CPURegList CPURegList::GetCallerSaved(unsigned size) {
  
  CPURegList list = CPURegList(CPURegister::kRegister, size, 0, 18);
  list.Combine(vixl::lr);
  return list;
}


CPURegList CPURegList::GetCallerSavedFP(unsigned size) {
  
  CPURegList list = CPURegList(CPURegister::kFPRegister, size, 0, 7);
  list.Combine(CPURegList(CPURegister::kFPRegister, size, 16, 31));
  return list;
}


const CPURegList kCalleeSaved = CPURegList::GetCalleeSaved();
const CPURegList kCalleeSavedFP = CPURegList::GetCalleeSavedFP();
const CPURegList kCallerSaved = CPURegList::GetCallerSaved();
const CPURegList kCallerSavedFP = CPURegList::GetCallerSavedFP();



#define WREG(n) w##n,
const Register Register::wregisters[] = {
  REGISTER_CODE_LIST(WREG)
};
#undef WREG


#define XREG(n) x##n,
const Register Register::xregisters[] = {
  REGISTER_CODE_LIST(XREG)
};
#undef XREG


#define SREG(n) s##n,
const FPRegister FPRegister::sregisters[] = {
  REGISTER_CODE_LIST(SREG)
};
#undef SREG


#define DREG(n) d##n,
const FPRegister FPRegister::dregisters[] = {
  REGISTER_CODE_LIST(DREG)
};
#undef DREG


const Register& Register::WRegFromCode(unsigned code) {
  if (code == kSPRegInternalCode)
    return wsp;
  VIXL_ASSERT(code < kNumberOfRegisters);
  return wregisters[code];
}


const Register& Register::XRegFromCode(unsigned code) {
  if (code == kSPRegInternalCode)
    return sp;
  VIXL_ASSERT(code < kNumberOfRegisters);
  return xregisters[code];
}


const FPRegister& FPRegister::SRegFromCode(unsigned code) {
  VIXL_ASSERT(code < kNumberOfFPRegisters);
  return sregisters[code];
}


const FPRegister& FPRegister::DRegFromCode(unsigned code) {
  VIXL_ASSERT(code < kNumberOfFPRegisters);
  return dregisters[code];
}


const Register& CPURegister::W() const {
  VIXL_ASSERT(IsValidRegister());
  return Register::WRegFromCode(code_);
}


const Register& CPURegister::X() const {
  VIXL_ASSERT(IsValidRegister());
  return Register::XRegFromCode(code_);
}


const FPRegister& CPURegister::S() const {
  VIXL_ASSERT(IsValidFPRegister());
  return FPRegister::SRegFromCode(code_);
}


const FPRegister& CPURegister::D() const {
  VIXL_ASSERT(IsValidFPRegister());
  return FPRegister::DRegFromCode(code_);

}



Operand::Operand(int64_t immediate)
  : immediate_(immediate),
    reg_(NoReg),
    shift_(NO_SHIFT),
    extend_(NO_EXTEND),
    shift_amount_(0) {
}


Operand::Operand(Register reg, Shift shift, unsigned shift_amount)
  : reg_(reg),
    shift_(shift),
    extend_(NO_EXTEND),
    shift_amount_(shift_amount) {
  VIXL_ASSERT(reg.Is64Bits() || (shift_amount < kWRegSize));
  VIXL_ASSERT(reg.Is32Bits() || (shift_amount < kXRegSize));
  VIXL_ASSERT(!reg.IsSP());
}


Operand::Operand(Register reg, Extend extend, unsigned shift_amount)
  : reg_(reg),
    shift_(NO_SHIFT),
    extend_(extend),
    shift_amount_(shift_amount) {
  VIXL_ASSERT(reg.IsValid());
  VIXL_ASSERT(shift_amount <= 4);
  VIXL_ASSERT(!reg.IsSP());

  
  VIXL_ASSERT(reg.Is64Bits() || ((extend != SXTX) && (extend != UXTX)));
}


bool Operand::IsImmediate() const {
  return reg_.Is(NoReg);
}


bool Operand::IsShiftedRegister() const {
  return reg_.IsValid() && (shift_ != NO_SHIFT);
}


bool Operand::IsExtendedRegister() const {
  return reg_.IsValid() && (extend_ != NO_EXTEND);
}


bool Operand::IsZero() const {
  if (IsImmediate())
    return immediate() == 0;
  return reg().IsZero();
}


Operand Operand::ToExtendedRegister() const {
  VIXL_ASSERT(IsShiftedRegister());
  VIXL_ASSERT((shift_ == LSL) && (shift_amount_ <= 4));
  return Operand(reg_, reg_.Is64Bits() ? UXTX : UXTW, shift_amount_);
}



MemOperand::MemOperand(Register base, ptrdiff_t offset, AddrMode addrmode)
  : base_(base), regoffset_(NoReg), offset_(offset), addrmode_(addrmode) {
  VIXL_ASSERT(base.Is64Bits() && !base.IsZero());
}


MemOperand::MemOperand(Register base, Register regoffset,
                       Extend extend, unsigned shift_amount)
  : base_(base), regoffset_(regoffset), offset_(0), addrmode_(Offset),
    shift_(NO_SHIFT), extend_(extend), shift_amount_(shift_amount) {
  VIXL_ASSERT(base.Is64Bits() && !base.IsZero());
  VIXL_ASSERT(!regoffset.IsSP());
  VIXL_ASSERT((extend == UXTW) || (extend == SXTW) || (extend == SXTX));

  
  VIXL_ASSERT(regoffset.Is64Bits() || (extend != SXTX));
}


MemOperand::MemOperand(Register base, Register regoffset,
                       Shift shift, unsigned shift_amount)
  : base_(base), regoffset_(regoffset), offset_(0), addrmode_(Offset),
    shift_(shift), extend_(NO_EXTEND), shift_amount_(shift_amount) {
  VIXL_ASSERT(base.Is64Bits() && !base.IsZero());
  VIXL_ASSERT(regoffset.Is64Bits() && !regoffset.IsSP());
  VIXL_ASSERT(shift == LSL);
}


MemOperand::MemOperand(Register base, const Operand& offset, AddrMode addrmode)
  : base_(base), regoffset_(NoReg), addrmode_(addrmode) {
  VIXL_ASSERT(base.Is64Bits() && !base.IsZero());

  if (offset.IsImmediate()) {
    offset_ = offset.immediate();
  } else if (offset.IsShiftedRegister()) {
    VIXL_ASSERT(addrmode == Offset);

    regoffset_ = offset.reg();
    shift_= offset.shift();
    shift_amount_ = offset.shift_amount();

    extend_ = NO_EXTEND;
    offset_ = 0;

    
    VIXL_ASSERT(regoffset_.Is64Bits() && !regoffset_.IsSP());
    VIXL_ASSERT(shift_ == LSL);
  } else {
    VIXL_ASSERT(offset.IsExtendedRegister());
    VIXL_ASSERT(addrmode == Offset);

    regoffset_ = offset.reg();
    extend_ = offset.extend();
    shift_amount_ = offset.shift_amount();

    shift_= NO_SHIFT;
    offset_ = 0;

    
    VIXL_ASSERT(!regoffset_.IsSP());
    VIXL_ASSERT((extend_ == UXTW) || (extend_ == SXTW) || (extend_ == SXTX));
    VIXL_ASSERT((regoffset_.Is64Bits() || (extend_ != SXTX)));
  }
}


bool MemOperand::IsImmediateOffset() const {
  return (addrmode_ == Offset) && regoffset_.Is(NoReg);
}


bool MemOperand::IsRegisterOffset() const {
  return (addrmode_ == Offset) && !regoffset_.Is(NoReg);
}


bool MemOperand::IsPreIndex() const {
  return addrmode_ == PreIndex;
}


bool MemOperand::IsPostIndex() const {
  return addrmode_ == PostIndex;
}








void Assembler::br(const Register& xn) {
  VIXL_ASSERT(xn.Is64Bits());
  
  Emit(BR | Rn(xn));
}


void Assembler::br(Instruction * at, const Register& xn) {
  VIXL_ASSERT(xn.Is64Bits());
  
  Emit(at, BR | Rn(xn));
}


void Assembler::blr(const Register& xn) {
  VIXL_ASSERT(xn.Is64Bits());
  
  Emit(BLR | Rn(xn));
}


void Assembler::blr(Instruction* at, const Register& xn) {
  VIXL_ASSERT(xn.Is64Bits());
  
  Emit(at, BLR | Rn(xn));
}


void Assembler::ret(const Register& xn) {
  VIXL_ASSERT(xn.Is64Bits());
  
  Emit(RET | Rn(xn));
}










void Assembler::add(const Register& rd, const Register& rn, const Operand& operand) {
  AddSub(rd, rn, operand, LeaveFlags, ADD);
}


void Assembler::adds(const Register& rd, const Register& rn, const Operand& operand) {
  AddSub(rd, rn, operand, SetFlags, ADD);
}


void Assembler::cmn(const Register& rn, const Operand& operand) {
  Register zr = AppropriateZeroRegFor(rn);
  adds(zr, rn, operand);
}


void Assembler::sub(const Register& rd, const Register& rn, const Operand& operand) {
  AddSub(rd, rn, operand, LeaveFlags, SUB);
}


void Assembler::subs(const Register& rd, const Register& rn, const Operand& operand) {
  AddSub(rd, rn, operand, SetFlags, SUB);
}


void Assembler::cmp(const Register& rn, const Operand& operand) {
  Register zr = AppropriateZeroRegFor(rn);
  subs(zr, rn, operand);
}


void Assembler::neg(const Register& rd, const Operand& operand) {
  Register zr = AppropriateZeroRegFor(rd);
  sub(rd, zr, operand);
}


void Assembler::negs(const Register& rd, const Operand& operand) {
  Register zr = AppropriateZeroRegFor(rd);
  subs(rd, zr, operand);
}


void Assembler::adc(const Register& rd, const Register& rn, const Operand& operand) {
  AddSubWithCarry(rd, rn, operand, LeaveFlags, ADC);
}


void Assembler::adcs(const Register& rd, const Register& rn, const Operand& operand) {
  AddSubWithCarry(rd, rn, operand, SetFlags, ADC);
}


void Assembler::sbc(const Register& rd, const Register& rn, const Operand& operand) {
  AddSubWithCarry(rd, rn, operand, LeaveFlags, SBC);
}


void Assembler::sbcs(const Register& rd, const Register& rn, const Operand& operand) {
  AddSubWithCarry(rd, rn, operand, SetFlags, SBC);
}


void Assembler::ngc(const Register& rd, const Operand& operand) {
  Register zr = AppropriateZeroRegFor(rd);
  sbc(rd, zr, operand);
}


void Assembler::ngcs(const Register& rd, const Operand& operand) {
  Register zr = AppropriateZeroRegFor(rd);
  sbcs(rd, zr, operand);
}



void Assembler::and_(const Register& rd, const Register& rn, const Operand& operand) {
  Logical(rd, rn, operand, AND);
}


void Assembler::bic(const Register& rd, const Register& rn, const Operand& operand) {
  Logical(rd, rn, operand, BIC);
}


void Assembler::bics(const Register& rd, const Register& rn, const Operand& operand) {
  Logical(rd, rn, operand, BICS);
}


void Assembler::orr(const Register& rd, const Register& rn, const Operand& operand) {
  Logical(rd, rn, operand, ORR);
}


void Assembler::orn(const Register& rd, const Register& rn, const Operand& operand) {
  Logical(rd, rn, operand, ORN);
}


void Assembler::eor(const Register& rd, const Register& rn, const Operand& operand) {
  Logical(rd, rn, operand, EOR);
}


void Assembler::eon(const Register& rd, const Register& rn, const Operand& operand) {
  Logical(rd, rn, operand, EON);
}


void Assembler::lslv(const Register& rd, const Register& rn, const Register& rm) {
  VIXL_ASSERT(rd.size() == rn.size());
  VIXL_ASSERT(rd.size() == rm.size());
  Emit(SF(rd) | LSLV | Rm(rm) | Rn(rn) | Rd(rd));
}


void Assembler::lsrv(const Register& rd, const Register& rn, const Register& rm) {
  VIXL_ASSERT(rd.size() == rn.size());
  VIXL_ASSERT(rd.size() == rm.size());
  Emit(SF(rd) | LSRV | Rm(rm) | Rn(rn) | Rd(rd));
}


void Assembler::asrv(const Register& rd, const Register& rn, const Register& rm) {
  VIXL_ASSERT(rd.size() == rn.size());
  VIXL_ASSERT(rd.size() == rm.size());
  Emit(SF(rd) | ASRV | Rm(rm) | Rn(rn) | Rd(rd));
}


void Assembler::rorv(const Register& rd, const Register& rn, const Register& rm) {
  VIXL_ASSERT(rd.size() == rn.size());
  VIXL_ASSERT(rd.size() == rm.size());
  Emit(SF(rd) | RORV | Rm(rm) | Rn(rn) | Rd(rd));
}



void Assembler::bfm(const Register& rd, const Register& rn, unsigned immr, unsigned imms) {
  VIXL_ASSERT(rd.size() == rn.size());
  Instr N = SF(rd) >> (kSFOffset - kBitfieldNOffset);
  Emit(SF(rd) | BFM | N |
       ImmR(immr, rd.size()) | ImmS(imms, rn.size()) | Rn(rn) | Rd(rd));
}


void Assembler::sbfm(const Register& rd, const Register& rn, unsigned immr, unsigned imms) {
  VIXL_ASSERT(rd.Is64Bits() || rn.Is32Bits());
  Instr N = SF(rd) >> (kSFOffset - kBitfieldNOffset);
  Emit(SF(rd) | SBFM | N |
       ImmR(immr, rd.size()) | ImmS(imms, rn.size()) | Rn(rn) | Rd(rd));
}


void Assembler::ubfm(const Register& rd, const Register& rn, unsigned immr, unsigned imms) {
  VIXL_ASSERT(rd.size() == rn.size());
  Instr N = SF(rd) >> (kSFOffset - kBitfieldNOffset);
  Emit(SF(rd) | UBFM | N |
       ImmR(immr, rd.size()) | ImmS(imms, rn.size()) | Rn(rn) | Rd(rd));
}


void Assembler::extr(const Register& rd, const Register& rn,
                     const Register& rm, unsigned lsb) {
  VIXL_ASSERT(rd.size() == rn.size());
  VIXL_ASSERT(rd.size() == rm.size());
  Instr N = SF(rd) >> (kSFOffset - kBitfieldNOffset);
  Emit(SF(rd) | EXTR | N | Rm(rm) | ImmS(lsb, rn.size()) | Rn(rn) | Rd(rd));
}


void Assembler::csel(const Register& rd, const Register& rn,
                     const Register& rm, Condition cond) {
  ConditionalSelect(rd, rn, rm, cond, CSEL);
}


void Assembler::csinc(const Register& rd, const Register& rn,
                      const Register& rm, Condition cond) {
  ConditionalSelect(rd, rn, rm, cond, CSINC);
}


void Assembler::csinv(const Register& rd, const Register& rn,
                      const Register& rm, Condition cond) {
  ConditionalSelect(rd, rn, rm, cond, CSINV);
}


void Assembler::csneg(const Register& rd, const Register& rn,
                      const Register& rm, Condition cond) {
  ConditionalSelect(rd, rn, rm, cond, CSNEG);
}


void Assembler::cset(const Register& rd, Condition cond) {
  VIXL_ASSERT((cond != al) && (cond != nv));
  Register zr = AppropriateZeroRegFor(rd);
  csinc(rd, zr, zr, InvertCondition(cond));
}


void Assembler::csetm(const Register& rd, Condition cond) {
  VIXL_ASSERT((cond != al) && (cond != nv));
  Register zr = AppropriateZeroRegFor(rd);
  csinv(rd, zr, zr, InvertCondition(cond));
}


void Assembler::cinc(const Register& rd, const Register& rn, Condition cond) {
  VIXL_ASSERT((cond != al) && (cond != nv));
  csinc(rd, rn, rn, InvertCondition(cond));
}


void Assembler::cinv(const Register& rd, const Register& rn, Condition cond) {
  VIXL_ASSERT((cond != al) && (cond != nv));
  csinv(rd, rn, rn, InvertCondition(cond));
}


void Assembler::cneg(const Register& rd, const Register& rn, Condition cond) {
  VIXL_ASSERT((cond != al) && (cond != nv));
  csneg(rd, rn, rn, InvertCondition(cond));
}


void Assembler::ConditionalSelect(const Register& rd, const Register& rn,
                                  const Register& rm, Condition cond,
                                  ConditionalSelectOp op) {
  VIXL_ASSERT(rd.size() == rn.size());
  VIXL_ASSERT(rd.size() == rm.size());
  Emit(SF(rd) | op | Rm(rm) | Cond(cond) | Rn(rn) | Rd(rd));
}


void Assembler::ccmn(const Register& rn, const Operand& operand, StatusFlags nzcv, Condition cond) {
  ConditionalCompare(rn, operand, nzcv, cond, CCMN);
}


void Assembler::ccmp(const Register& rn, const Operand& operand, StatusFlags nzcv, Condition cond) {
  ConditionalCompare(rn, operand, nzcv, cond, CCMP);
}


void Assembler::DataProcessing3Source(const Register& rd, const Register& rn,
                                      const Register& rm, const Register& ra,
                                      DataProcessing3SourceOp op) {
  Emit(SF(rd) | op | Rm(rm) | Ra(ra) | Rn(rn) | Rd(rd));
}


void Assembler::mul(const Register& rd, const Register& rn, const Register& rm) {
  VIXL_ASSERT(AreSameSizeAndType(rd, rn, rm));
  DataProcessing3Source(rd, rn, rm, AppropriateZeroRegFor(rd), MADD);
}


void Assembler::madd(const Register& rd, const Register& rn,
                     const Register& rm, const Register& ra) {
  DataProcessing3Source(rd, rn, rm, ra, MADD);
}


void Assembler::mneg(const Register& rd, const Register& rn, const Register& rm) {
  VIXL_ASSERT(AreSameSizeAndType(rd, rn, rm));
  DataProcessing3Source(rd, rn, rm, AppropriateZeroRegFor(rd), MSUB);
}


void Assembler::msub(const Register& rd, const Register& rn,
                     const Register& rm, const Register& ra) {
  DataProcessing3Source(rd, rn, rm, ra, MSUB);
}


void Assembler::umaddl(const Register& rd, const Register& rn,
                       const Register& rm, const Register& ra) {
  VIXL_ASSERT(rd.Is64Bits() && ra.Is64Bits());
  VIXL_ASSERT(rn.Is32Bits() && rm.Is32Bits());
  DataProcessing3Source(rd, rn, rm, ra, UMADDL_x);
}


void Assembler::smaddl(const Register& rd, const Register& rn,
                       const Register& rm, const Register& ra) {
  VIXL_ASSERT(rd.Is64Bits() && ra.Is64Bits());
  VIXL_ASSERT(rn.Is32Bits() && rm.Is32Bits());
  DataProcessing3Source(rd, rn, rm, ra, SMADDL_x);
}


void Assembler::umsubl(const Register& rd, const Register& rn,
                       const Register& rm, const Register& ra) {
  VIXL_ASSERT(rd.Is64Bits() && ra.Is64Bits());
  VIXL_ASSERT(rn.Is32Bits() && rm.Is32Bits());
  DataProcessing3Source(rd, rn, rm, ra, UMSUBL_x);
}


void Assembler::smsubl(const Register& rd, const Register& rn,
                       const Register& rm, const Register& ra) {
  VIXL_ASSERT(rd.Is64Bits() && ra.Is64Bits());
  VIXL_ASSERT(rn.Is32Bits() && rm.Is32Bits());
  DataProcessing3Source(rd, rn, rm, ra, SMSUBL_x);
}


void Assembler::smull(const Register& rd, const Register& rn, const Register& rm) {
  VIXL_ASSERT(rd.Is64Bits());
  VIXL_ASSERT(rn.Is32Bits() && rm.Is32Bits());
  DataProcessing3Source(rd, rn, rm, xzr, SMADDL_x);
}


void Assembler::sdiv(const Register& rd, const Register& rn, const Register& rm) {
  VIXL_ASSERT(rd.size() == rn.size());
  VIXL_ASSERT(rd.size() == rm.size());
  Emit(SF(rd) | SDIV | Rm(rm) | Rn(rn) | Rd(rd));
}


void Assembler::smulh(const Register& xd, const Register& xn, const Register& xm) {
  VIXL_ASSERT(xd.Is64Bits() && xn.Is64Bits() && xm.Is64Bits());
  DataProcessing3Source(xd, xn, xm, xzr, SMULH_x);
}


void Assembler::udiv(const Register& rd, const Register& rn, const Register& rm) {
  VIXL_ASSERT(rd.size() == rn.size());
  VIXL_ASSERT(rd.size() == rm.size());
  Emit(SF(rd) | UDIV | Rm(rm) | Rn(rn) | Rd(rd));
}


void Assembler::rbit(const Register& rd, const Register& rn) {
  DataProcessing1Source(rd, rn, RBIT);
}


void Assembler::rev16(const Register& rd, const Register& rn) {
  DataProcessing1Source(rd, rn, REV16);
}


void Assembler::rev32(const Register& rd, const Register& rn) {
  VIXL_ASSERT(rd.Is64Bits());
  DataProcessing1Source(rd, rn, REV);
}


void Assembler::rev(const Register& rd, const Register& rn) {
  DataProcessing1Source(rd, rn, rd.Is64Bits() ? REV_x : REV_w);
}


void Assembler::clz(const Register& rd, const Register& rn) {
  DataProcessing1Source(rd, rn, CLZ);
}


void Assembler::cls(const Register& rd, const Register& rn) {
  DataProcessing1Source(rd, rn, CLS);
}


void Assembler::ldp(const CPURegister& rt, const CPURegister& rt2, const MemOperand& src) {
  LoadStorePair(rt, rt2, src, LoadPairOpFor(rt, rt2));
}


void Assembler::stp(const CPURegister& rt, const CPURegister& rt2, const MemOperand& dst) {
  LoadStorePair(rt, rt2, dst, StorePairOpFor(rt, rt2));
}


void Assembler::ldpsw(const Register& rt, const Register& rt2, const MemOperand& src) {
  VIXL_ASSERT(rt.Is64Bits());
  LoadStorePair(rt, rt2, src, LDPSW_x);
}


void Assembler::LoadStorePair(const CPURegister& rt, const CPURegister& rt2,
                              const MemOperand& addr, LoadStorePairOp op) {
  
  VIXL_ASSERT(((op & LoadStorePairLBit) == 0) || !rt.Is(rt2));
  VIXL_ASSERT(AreSameSizeAndType(rt, rt2));

  Instr memop = op | Rt(rt) | Rt2(rt2) | RnSP(addr.base()) |
                ImmLSPair(addr.offset(), CalcLSPairDataSize(op));

  Instr addrmodeop;
  if (addr.IsImmediateOffset()) {
    addrmodeop = LoadStorePairOffsetFixed;
  } else {
    VIXL_ASSERT(addr.offset() != 0);
    if (addr.IsPreIndex()) {
      addrmodeop = LoadStorePairPreIndexFixed;
    } else {
      VIXL_ASSERT(addr.IsPostIndex());
      addrmodeop = LoadStorePairPostIndexFixed;
    }
  }
  Emit(addrmodeop | memop);
}


void Assembler::ldnp(const CPURegister& rt, const CPURegister& rt2, const MemOperand& src) {
  LoadStorePairNonTemporal(rt, rt2, src, LoadPairNonTemporalOpFor(rt, rt2));
}


void Assembler::stnp(const CPURegister& rt, const CPURegister& rt2, const MemOperand& dst) {
  LoadStorePairNonTemporal(rt, rt2, dst, StorePairNonTemporalOpFor(rt, rt2));
}


void Assembler::LoadStorePairNonTemporal(const CPURegister& rt, const CPURegister& rt2,
                                         const MemOperand& addr, LoadStorePairNonTemporalOp op) {
  VIXL_ASSERT(!rt.Is(rt2));
  VIXL_ASSERT(AreSameSizeAndType(rt, rt2));
  VIXL_ASSERT(addr.IsImmediateOffset());

  LSDataSize size = CalcLSPairDataSize(static_cast<LoadStorePairOp>(op & LoadStorePairMask));
  Emit(op | Rt(rt) | Rt2(rt2) | RnSP(addr.base()) | ImmLSPair(addr.offset(), size));
}



void Assembler::ldrb(const Register& rt, const MemOperand& src, LoadStoreScalingOption option) {
  VIXL_ASSERT(option != RequireUnscaledOffset);
  VIXL_ASSERT(option != PreferUnscaledOffset);
  LoadStore(rt, src, LDRB_w, option);
}


void Assembler::strb(const Register& rt, const MemOperand& dst, LoadStoreScalingOption option) {
  VIXL_ASSERT(option != RequireUnscaledOffset);
  VIXL_ASSERT(option != PreferUnscaledOffset);
  LoadStore(rt, dst, STRB_w, option);
}


void Assembler::ldrsb(const Register& rt, const MemOperand& src, LoadStoreScalingOption option) {
  VIXL_ASSERT(option != RequireUnscaledOffset);
  VIXL_ASSERT(option != PreferUnscaledOffset);
  LoadStore(rt, src, rt.Is64Bits() ? LDRSB_x : LDRSB_w, option);
}


void Assembler::ldrh(const Register& rt, const MemOperand& src, LoadStoreScalingOption option) {
  VIXL_ASSERT(option != RequireUnscaledOffset);
  VIXL_ASSERT(option != PreferUnscaledOffset);
  LoadStore(rt, src, LDRH_w, option);
}


void Assembler::strh(const Register& rt, const MemOperand& dst, LoadStoreScalingOption option) {
  VIXL_ASSERT(option != RequireUnscaledOffset);
  VIXL_ASSERT(option != PreferUnscaledOffset);
  LoadStore(rt, dst, STRH_w, option);
}


void Assembler::ldrsh(const Register& rt, const MemOperand& src, LoadStoreScalingOption option) {
  VIXL_ASSERT(option != RequireUnscaledOffset);
  VIXL_ASSERT(option != PreferUnscaledOffset);
  LoadStore(rt, src, rt.Is64Bits() ? LDRSH_x : LDRSH_w, option);
}


void Assembler::ldr(const CPURegister& rt, const MemOperand& src, LoadStoreScalingOption option) {
  VIXL_ASSERT(option != RequireUnscaledOffset);
  VIXL_ASSERT(option != PreferUnscaledOffset);
  LoadStore(rt, src, LoadOpFor(rt), option);
}


void Assembler::str(const CPURegister& rt, const MemOperand& src, LoadStoreScalingOption option) {
  VIXL_ASSERT(option != RequireUnscaledOffset);
  VIXL_ASSERT(option != PreferUnscaledOffset);
  LoadStore(rt, src, StoreOpFor(rt), option);
}


void Assembler::ldrsw(const Register& rt, const MemOperand& src, LoadStoreScalingOption option) {
  VIXL_ASSERT(option != RequireUnscaledOffset);
  VIXL_ASSERT(option != PreferUnscaledOffset);
  VIXL_ASSERT(rt.Is64Bits());
  LoadStore(rt, src, LDRSW_x, option);
}


void Assembler::ldurb(const Register& rt, const MemOperand& src, LoadStoreScalingOption option) {
  VIXL_ASSERT(option != RequireScaledOffset);
  VIXL_ASSERT(option != PreferScaledOffset);
  LoadStore(rt, src, LDRB_w, option);
}


void Assembler::sturb(const Register& rt, const MemOperand& dst, LoadStoreScalingOption option) {
  VIXL_ASSERT(option != RequireScaledOffset);
  VIXL_ASSERT(option != PreferScaledOffset);
  LoadStore(rt, dst, STRB_w, option);
}


void Assembler::ldursb(const Register& rt, const MemOperand& src, LoadStoreScalingOption option) {
  VIXL_ASSERT(option != RequireScaledOffset);
  VIXL_ASSERT(option != PreferScaledOffset);
  LoadStore(rt, src, rt.Is64Bits() ? LDRSB_x : LDRSB_w, option);
}


void Assembler::ldurh(const Register& rt, const MemOperand& src, LoadStoreScalingOption option) {
  VIXL_ASSERT(option != RequireScaledOffset);
  VIXL_ASSERT(option != PreferScaledOffset);
  LoadStore(rt, src, LDRH_w, option);
}


void Assembler::sturh(const Register& rt, const MemOperand& dst, LoadStoreScalingOption option) {
  VIXL_ASSERT(option != RequireScaledOffset);
  VIXL_ASSERT(option != PreferScaledOffset);
  LoadStore(rt, dst, STRH_w, option);
}


void Assembler::ldursh(const Register& rt, const MemOperand& src, LoadStoreScalingOption option) {
  VIXL_ASSERT(option != RequireScaledOffset);
  VIXL_ASSERT(option != PreferScaledOffset);
  LoadStore(rt, src, rt.Is64Bits() ? LDRSH_x : LDRSH_w, option);
}


void Assembler::ldur(const CPURegister& rt, const MemOperand& src, LoadStoreScalingOption option) {
  VIXL_ASSERT(option != RequireScaledOffset);
  VIXL_ASSERT(option != PreferScaledOffset);
  LoadStore(rt, src, LoadOpFor(rt), option);
}


void Assembler::stur(const CPURegister& rt, const MemOperand& dst, LoadStoreScalingOption option) {
  VIXL_ASSERT(option != RequireScaledOffset);
  VIXL_ASSERT(option != PreferScaledOffset);
  LoadStore(rt, dst, StoreOpFor(rt), option);
}


void Assembler::ldursw(const Register& rt, const MemOperand& src, LoadStoreScalingOption option) {
  VIXL_ASSERT(rt.Is64Bits());
  VIXL_ASSERT(option != RequireScaledOffset);
  VIXL_ASSERT(option != PreferScaledOffset);
  LoadStore(rt, src, LDRSW_x, option);
}


void Assembler::ldr(const CPURegister& rt, int imm19) {
  LoadLiteralOp op = LoadLiteralOpFor(rt);
  Emit(op | ImmLLiteral(imm19) | Rt(rt));
}


void Assembler::ldrsw(const Register& rt, int imm19) {
  Emit(LDRSW_x_lit | ImmLLiteral(imm19) | Rt(rt));
}



void Assembler::stxrb(const Register& rs, const Register& rt, const MemOperand& dst) {
  VIXL_ASSERT(dst.IsImmediateOffset() && (dst.offset() == 0));
  Emit(STXRB_w | Rs(rs) | Rt(rt) | Rt2_mask | RnSP(dst.base()));
}


void Assembler::stxrh(const Register& rs, const Register& rt, const MemOperand& dst) {
  VIXL_ASSERT(dst.IsImmediateOffset() && (dst.offset() == 0));
  Emit(STXRH_w | Rs(rs) | Rt(rt) | Rt2_mask | RnSP(dst.base()));
}


void Assembler::stxr(const Register& rs, const Register& rt, const MemOperand& dst) {
  VIXL_ASSERT(dst.IsImmediateOffset() && (dst.offset() == 0));
  LoadStoreExclusive op = rt.Is64Bits() ? STXR_x : STXR_w;
  Emit(op | Rs(rs) | Rt(rt) | Rt2_mask | RnSP(dst.base()));
}


void Assembler::ldxrb(const Register& rt, const MemOperand& src) {
  VIXL_ASSERT(src.IsImmediateOffset() && (src.offset() == 0));
  Emit(LDXRB_w | Rs_mask | Rt(rt) | Rt2_mask | RnSP(src.base()));
}


void Assembler::ldxrh(const Register& rt, const MemOperand& src) {
  VIXL_ASSERT(src.IsImmediateOffset() && (src.offset() == 0));
  Emit(LDXRH_w | Rs_mask | Rt(rt) | Rt2_mask | RnSP(src.base()));
}


void Assembler::ldxr(const Register& rt, const MemOperand& src) {
  VIXL_ASSERT(src.IsImmediateOffset() && (src.offset() == 0));
  LoadStoreExclusive op = rt.Is64Bits() ? LDXR_x : LDXR_w;
  Emit(op | Rs_mask | Rt(rt) | Rt2_mask | RnSP(src.base()));
}


void Assembler::stxp(const Register& rs, const Register& rt, const Register& rt2, const MemOperand& dst) {
  VIXL_ASSERT(rt.size() == rt2.size());
  VIXL_ASSERT(dst.IsImmediateOffset() && (dst.offset() == 0));
  LoadStoreExclusive op = rt.Is64Bits() ? STXP_x : STXP_w;
  Emit(op | Rs(rs) | Rt(rt) | Rt2(rt2) | RnSP(dst.base()));
}


void Assembler::ldxp(const Register& rt, const Register& rt2, const MemOperand& src) {
  VIXL_ASSERT(rt.size() == rt2.size());
  VIXL_ASSERT(src.IsImmediateOffset() && (src.offset() == 0));
  LoadStoreExclusive op = rt.Is64Bits() ? LDXP_x : LDXP_w;
  Emit(op | Rs_mask | Rt(rt) | Rt2(rt2) | RnSP(src.base()));
}


void Assembler::stlxrb(const Register& rs, const Register& rt, const MemOperand& dst) {
  VIXL_ASSERT(dst.IsImmediateOffset() && (dst.offset() == 0));
  Emit(STLXRB_w | Rs(rs) | Rt(rt) | Rt2_mask | RnSP(dst.base()));
}


void Assembler::stlxrh(const Register& rs, const Register& rt, const MemOperand& dst) {
  VIXL_ASSERT(dst.IsImmediateOffset() && (dst.offset() == 0));
  Emit(STLXRH_w | Rs(rs) | Rt(rt) | Rt2_mask | RnSP(dst.base()));
}


void Assembler::stlxr(const Register& rs, const Register& rt, const MemOperand& dst) {
  VIXL_ASSERT(dst.IsImmediateOffset() && (dst.offset() == 0));
  LoadStoreExclusive op = rt.Is64Bits() ? STLXR_x : STLXR_w;
  Emit(op | Rs(rs) | Rt(rt) | Rt2_mask | RnSP(dst.base()));
}


void Assembler::ldaxrb(const Register& rt, const MemOperand& src) {
  VIXL_ASSERT(src.IsImmediateOffset() && (src.offset() == 0));
  Emit(LDAXRB_w | Rs_mask | Rt(rt) | Rt2_mask | RnSP(src.base()));
}


void Assembler::ldaxrh(const Register& rt, const MemOperand& src) {
  VIXL_ASSERT(src.IsImmediateOffset() && (src.offset() == 0));
  Emit(LDAXRH_w | Rs_mask | Rt(rt) | Rt2_mask | RnSP(src.base()));
}


void Assembler::ldaxr(const Register& rt, const MemOperand& src) {
  VIXL_ASSERT(src.IsImmediateOffset() && (src.offset() == 0));
  LoadStoreExclusive op = rt.Is64Bits() ? LDAXR_x : LDAXR_w;
  Emit(op | Rs_mask | Rt(rt) | Rt2_mask | RnSP(src.base()));
}


void Assembler::stlxp(const Register& rs, const Register& rt,
                      const Register& rt2, const MemOperand& dst) {
  VIXL_ASSERT(rt.size() == rt2.size());
  VIXL_ASSERT(dst.IsImmediateOffset() && (dst.offset() == 0));
  LoadStoreExclusive op = rt.Is64Bits() ? STLXP_x : STLXP_w;
  Emit(op | Rs(rs) | Rt(rt) | Rt2(rt2) | RnSP(dst.base()));
}


void Assembler::ldaxp(const Register& rt, const Register& rt2, const MemOperand& src) {
  VIXL_ASSERT(rt.size() == rt2.size());
  VIXL_ASSERT(src.IsImmediateOffset() && (src.offset() == 0));
  LoadStoreExclusive op = rt.Is64Bits() ? LDAXP_x : LDAXP_w;
  Emit(op | Rs_mask | Rt(rt) | Rt2(rt2) | RnSP(src.base()));
}


void Assembler::stlrb(const Register& rt, const MemOperand& dst) {
  VIXL_ASSERT(dst.IsImmediateOffset() && (dst.offset() == 0));
  Emit(STLRB_w | Rs_mask | Rt(rt) | Rt2_mask | RnSP(dst.base()));
}


void Assembler::stlrh(const Register& rt, const MemOperand& dst) {
  VIXL_ASSERT(dst.IsImmediateOffset() && (dst.offset() == 0));
  Emit(STLRH_w | Rs_mask | Rt(rt) | Rt2_mask | RnSP(dst.base()));
}


void Assembler::stlr(const Register& rt, const MemOperand& dst) {
  VIXL_ASSERT(dst.IsImmediateOffset() && (dst.offset() == 0));
  LoadStoreExclusive op = rt.Is64Bits() ? STLR_x : STLR_w;
  Emit(op | Rs_mask | Rt(rt) | Rt2_mask | RnSP(dst.base()));
}


void Assembler::ldarb(const Register& rt, const MemOperand& src) {
  VIXL_ASSERT(src.IsImmediateOffset() && (src.offset() == 0));
  Emit(LDARB_w | Rs_mask | Rt(rt) | Rt2_mask | RnSP(src.base()));
}


void Assembler::ldarh(const Register& rt, const MemOperand& src) {
  VIXL_ASSERT(src.IsImmediateOffset() && (src.offset() == 0));
  Emit(LDARH_w | Rs_mask | Rt(rt) | Rt2_mask | RnSP(src.base()));
}


void Assembler::ldar(const Register& rt, const MemOperand& src) {
  VIXL_ASSERT(src.IsImmediateOffset() && (src.offset() == 0));
  LoadStoreExclusive op = rt.Is64Bits() ? LDAR_x : LDAR_w;
  Emit(op | Rs_mask | Rt(rt) | Rt2_mask | RnSP(src.base()));
}


void Assembler::mov(const Register& rd, const Register& rm) {
  
  
  
  if (rd.IsSP() || rm.IsSP()) {
    add(rd, rm, 0);
  } else {
    orr(rd, AppropriateZeroRegFor(rd), rm);
  }
}


void Assembler::mvn(const Register& rd, const Operand& operand) {
  orn(rd, AppropriateZeroRegFor(rd), operand);
}


void Assembler::mrs(const Register& rt, SystemRegister sysreg) {
  VIXL_ASSERT(rt.Is64Bits());
  Emit(MRS | ImmSystemRegister(sysreg) | Rt(rt));
}


void Assembler::msr(SystemRegister sysreg, const Register& rt) {
  VIXL_ASSERT(rt.Is64Bits());
  Emit(MSR | Rt(rt) | ImmSystemRegister(sysreg));
}




void Assembler::clrex(int imm4) {
  Emit(CLREX | CRm(imm4));
}


void Assembler::dmb(BarrierDomain domain, BarrierType type) {
  Emit(DMB | ImmBarrierDomain(domain) | ImmBarrierType(type));
}


void Assembler::dsb(BarrierDomain domain, BarrierType type) {
  Emit(DSB | ImmBarrierDomain(domain) | ImmBarrierType(type));
}


void Assembler::isb() {
  Emit(ISB | ImmBarrierDomain(FullSystem) | ImmBarrierType(BarrierAll));
}


void Assembler::fmov(const FPRegister& fd, double imm) {
  VIXL_ASSERT(fd.Is64Bits());
  VIXL_ASSERT(IsImmFP64(imm));
  Emit(FMOV_d_imm | Rd(fd) | ImmFP64(imm));
}


void Assembler::fmov(const FPRegister& fd, float imm) {
  VIXL_ASSERT(fd.Is32Bits());
  VIXL_ASSERT(IsImmFP32(imm));
  Emit(FMOV_s_imm | Rd(fd) | ImmFP32(imm));
}


void Assembler::fmov(const Register& rd, const FPRegister& fn) {
  VIXL_ASSERT(rd.size() == fn.size());
  FPIntegerConvertOp op = rd.Is32Bits() ? FMOV_ws : FMOV_xd;
  Emit(op | Rd(rd) | Rn(fn));
}


void Assembler::fmov(const FPRegister& fd, const Register& rn) {
  VIXL_ASSERT(fd.size() == rn.size());
  FPIntegerConvertOp op = fd.Is32Bits() ? FMOV_sw : FMOV_dx;
  Emit(op | Rd(fd) | Rn(rn));
}


void Assembler::fmov(const FPRegister& fd, const FPRegister& fn) {
  VIXL_ASSERT(fd.size() == fn.size());
  Emit(FPType(fd) | FMOV | Rd(fd) | Rn(fn));
}


void Assembler::fadd(const FPRegister& fd, const FPRegister& fn, const FPRegister& fm) {
  FPDataProcessing2Source(fd, fn, fm, FADD);
}


void Assembler::fsub(const FPRegister& fd, const FPRegister& fn, const FPRegister& fm) {
  FPDataProcessing2Source(fd, fn, fm, FSUB);
}


void Assembler::fmul(const FPRegister& fd, const FPRegister& fn, const FPRegister& fm) {
  FPDataProcessing2Source(fd, fn, fm, FMUL);
}


void Assembler::fmadd(const FPRegister& fd, const FPRegister& fn,
                      const FPRegister& fm, const FPRegister& fa) {
  FPDataProcessing3Source(fd, fn, fm, fa, fd.Is32Bits() ? FMADD_s : FMADD_d);
}


void Assembler::fmsub(const FPRegister& fd, const FPRegister& fn,
                      const FPRegister& fm, const FPRegister& fa) {
  FPDataProcessing3Source(fd, fn, fm, fa, fd.Is32Bits() ? FMSUB_s : FMSUB_d);
}


void Assembler::fnmadd(const FPRegister& fd, const FPRegister& fn,
                       const FPRegister& fm, const FPRegister& fa) {
  FPDataProcessing3Source(fd, fn, fm, fa, fd.Is32Bits() ? FNMADD_s : FNMADD_d);
}


void Assembler::fnmsub(const FPRegister& fd, const FPRegister& fn,
                       const FPRegister& fm, const FPRegister& fa) {
  FPDataProcessing3Source(fd, fn, fm, fa, fd.Is32Bits() ? FNMSUB_s : FNMSUB_d);
}


void Assembler::fdiv(const FPRegister& fd, const FPRegister& fn, const FPRegister& fm) {
  FPDataProcessing2Source(fd, fn, fm, FDIV);
}


void Assembler::fmax(const FPRegister& fd, const FPRegister& fn, const FPRegister& fm) {
  FPDataProcessing2Source(fd, fn, fm, FMAX);
}


void Assembler::fmaxnm(const FPRegister& fd, const FPRegister& fn, const FPRegister& fm) {
  FPDataProcessing2Source(fd, fn, fm, FMAXNM);
}


void Assembler::fmin(const FPRegister& fd, const FPRegister& fn, const FPRegister& fm) {
  FPDataProcessing2Source(fd, fn, fm, FMIN);
}


void Assembler::fminnm(const FPRegister& fd, const FPRegister& fn, const FPRegister& fm) {
  FPDataProcessing2Source(fd, fn, fm, FMINNM);
}


void Assembler::fabs(const FPRegister& fd, const FPRegister& fn) {
  VIXL_ASSERT(fd.size() == fn.size());
  FPDataProcessing1Source(fd, fn, FABS);
}


void Assembler::fneg(const FPRegister& fd, const FPRegister& fn) {
  VIXL_ASSERT(fd.size() == fn.size());
  FPDataProcessing1Source(fd, fn, FNEG);
}


void Assembler::fsqrt(const FPRegister& fd, const FPRegister& fn) {
  VIXL_ASSERT(fd.size() == fn.size());
  FPDataProcessing1Source(fd, fn, FSQRT);
}


void Assembler::frinta(const FPRegister& fd, const FPRegister& fn) {
  VIXL_ASSERT(fd.size() == fn.size());
  FPDataProcessing1Source(fd, fn, FRINTA);
}


void Assembler::frintm(const FPRegister& fd, const FPRegister& fn) {
  VIXL_ASSERT(fd.size() == fn.size());
  FPDataProcessing1Source(fd, fn, FRINTM);
}


void Assembler::frintn(const FPRegister& fd, const FPRegister& fn) {
  VIXL_ASSERT(fd.size() == fn.size());
  FPDataProcessing1Source(fd, fn, FRINTN);
}


void Assembler::frintz(const FPRegister& fd, const FPRegister& fn) {
  VIXL_ASSERT(fd.size() == fn.size());
  FPDataProcessing1Source(fd, fn, FRINTZ);
}


void Assembler::fcmp(const FPRegister& fn, const FPRegister& fm) {
  VIXL_ASSERT(fn.size() == fm.size());
  Emit(FPType(fn) | FCMP | Rm(fm) | Rn(fn));
}


void Assembler::fcmp(const FPRegister& fn, double value) {
  USE(value);
  
  
  
  VIXL_ASSERT(value == 0.0);
  Emit(FPType(fn) | FCMP_zero | Rn(fn));
}


void Assembler::fccmp(const FPRegister& fn, const FPRegister& fm,
                      StatusFlags nzcv, Condition cond) {
  VIXL_ASSERT(fn.size() == fm.size());
  Emit(FPType(fn) | FCCMP | Rm(fm) | Cond(cond) | Rn(fn) | Nzcv(nzcv));
}


void Assembler::fcsel(const FPRegister& fd, const FPRegister& fn,
                      const FPRegister& fm, Condition cond) {
  VIXL_ASSERT(fd.size() == fn.size());
  VIXL_ASSERT(fd.size() == fm.size());
  Emit(FPType(fd) | FCSEL | Rm(fm) | Cond(cond) | Rn(fn) | Rd(fd));
}


void Assembler::FPConvertToInt(const Register& rd, const FPRegister& fn, FPIntegerConvertOp op) {
  Emit(SF(rd) | FPType(fn) | op | Rn(fn) | Rd(rd));
}


void Assembler::fcvt(const FPRegister& fd, const FPRegister& fn) {
  if (fd.Is64Bits()) {
    
    VIXL_ASSERT(fn.Is32Bits());
    FPDataProcessing1Source(fd, fn, FCVT_ds);
  } else {
    
    VIXL_ASSERT(fn.Is64Bits());
    FPDataProcessing1Source(fd, fn, FCVT_sd);
  }
}


void Assembler::fcvtau(const Register& rd, const FPRegister& fn) {
  FPConvertToInt(rd, fn, FCVTAU);
}


void Assembler::fcvtas(const Register& rd, const FPRegister& fn) {
  FPConvertToInt(rd, fn, FCVTAS);
}


void Assembler::fcvtmu(const Register& rd, const FPRegister& fn) {
  FPConvertToInt(rd, fn, FCVTMU);
}


void Assembler::fcvtms(const Register& rd, const FPRegister& fn) {
  FPConvertToInt(rd, fn, FCVTMS);
}


void Assembler::fcvtpu(const Register& rd, const FPRegister& fn) {
  FPConvertToInt(rd, fn, FCVTPU);
}


void Assembler::fcvtps(const Register& rd, const FPRegister& fn) {
  FPConvertToInt(rd, fn, FCVTPS);
}


void Assembler::fcvtnu(const Register& rd, const FPRegister& fn) {
  FPConvertToInt(rd, fn, FCVTNU);
}


void Assembler::fcvtns(const Register& rd, const FPRegister& fn) {
  FPConvertToInt(rd, fn, FCVTNS);
}


void Assembler::fcvtzu(const Register& rd, const FPRegister& fn) {
  FPConvertToInt(rd, fn, FCVTZU);
}


void Assembler::fcvtzs(const Register& rd, const FPRegister& fn) {

  FPConvertToInt(rd, fn, FCVTZS);
}


void Assembler::scvtf(const FPRegister& fd, const Register& rn, unsigned fbits) {
  if (fbits == 0)
    Emit(SF(rn) | FPType(fd) | SCVTF | Rn(rn) | Rd(fd));
  else
    Emit(SF(rn) | FPType(fd) | SCVTF_fixed | FPScale(64 - fbits) | Rn(rn) | Rd(fd));
}


void Assembler::ucvtf(const FPRegister& fd, const Register& rn, unsigned fbits) {
  if (fbits == 0)
    Emit(SF(rn) | FPType(fd) | UCVTF | Rn(rn) | Rd(fd));
  else
    Emit(SF(rn) | FPType(fd) | UCVTF_fixed | FPScale(64 - fbits) | Rn(rn) | Rd(fd));
}






Instr Assembler::ImmFP32(float imm) {
  VIXL_ASSERT(IsImmFP32(imm));
  
  uint32_t bits = float_to_rawbits(imm);
  
  uint32_t bit7 = ((bits >> 31) & 0x1) << 7;
  
  uint32_t bit6 = ((bits >> 29) & 0x1) << 6;
  
  uint32_t bit5_to_0 = (bits >> 19) & 0x3f;

  return (bit7 | bit6 | bit5_to_0) << ImmFP_offset;
}


Instr Assembler::ImmFP64(double imm) {
  VIXL_ASSERT(IsImmFP64(imm));
  
  
  uint64_t bits = double_to_rawbits(imm);
  
  uint32_t bit7 = ((bits >> 63) & 0x1) << 7;
  
  uint32_t bit6 = ((bits >> 61) & 0x1) << 6;
  
  uint32_t bit5_to_0 = (bits >> 48) & 0x3f;

  return (bit7 | bit6 | bit5_to_0) << ImmFP_offset;
}



void Assembler::MoveWide(const Register& rd, uint64_t imm, int shift, MoveWideImmediateOp mov_op) {
  
  if (rd.Is32Bits()) {
    
    
    VIXL_ASSERT(((imm >> kWRegSize) == 0) ||
               ((imm >> (kWRegSize - 1)) == 0x1ffffffff));
    imm &= kWRegMask;
  }

  if (shift >= 0) {
    
    VIXL_ASSERT((shift == 0) || (shift == 16) ||
               (shift == 32) || (shift == 48));
    VIXL_ASSERT(rd.Is64Bits() || (shift == 0) || (shift == 16));
    shift /= 16;
  } else {
    
    
    shift = 0;
    if ((imm & 0xffffffffffff0000) == 0) {
      
    } else if ((imm & 0xffffffff0000ffff) == 0) {
      imm >>= 16;
      shift = 1;
    } else if ((imm & 0xffff0000ffffffff) == 0) {
      VIXL_ASSERT(rd.Is64Bits());
      imm >>= 32;
      shift = 2;
    } else if ((imm & 0x0000ffffffffffff) == 0) {
      VIXL_ASSERT(rd.Is64Bits());
      imm >>= 48;
      shift = 3;
    }
  }

  VIXL_ASSERT(is_uint16(imm));

  Emit(SF(rd) | MoveWideImmediateFixed | mov_op |
       Rd(rd) | ImmMoveWide(imm) | ShiftMoveWide(shift));
}


void Assembler::AddSub(const Register& rd, const Register& rn, const Operand& operand,
                       FlagsUpdate S, AddSubOp op) {
  VIXL_ASSERT(rd.size() == rn.size());

  if (operand.IsImmediate()) {
    int64_t immediate = operand.immediate();
    VIXL_ASSERT(IsImmAddSub(immediate));
    Instr dest_reg = (S == SetFlags) ? Rd(rd) : RdSP(rd);
    Emit(SF(rd) | AddSubImmediateFixed | op | Flags(S) |
         ImmAddSub(immediate) | dest_reg | RnSP(rn));
  } else if (operand.IsShiftedRegister()) {
    VIXL_ASSERT(operand.reg().size() == rd.size());
    VIXL_ASSERT(operand.shift() != ROR);

    
    
    
    
    
    
    
    if (rn.IsSP() || rd.IsSP()) {
      VIXL_ASSERT(!(rd.IsSP() && (S == SetFlags)));
      DataProcExtendedRegister(rd, rn, operand.ToExtendedRegister(), S,
                               AddSubExtendedFixed | op);
    } else {
      DataProcShiftedRegister(rd, rn, operand, S, AddSubShiftedFixed | op);
    }
  } else {
    VIXL_ASSERT(operand.IsExtendedRegister());
    DataProcExtendedRegister(rd, rn, operand, S, AddSubExtendedFixed | op);
  }
}


void Assembler::AddSubWithCarry(const Register& rd, const Register& rn,
                                const Operand& operand, FlagsUpdate S, AddSubWithCarryOp op) {
  VIXL_ASSERT(rd.size() == rn.size());
  VIXL_ASSERT(rd.size() == operand.reg().size());
  VIXL_ASSERT(operand.IsShiftedRegister() && (operand.shift_amount() == 0));
  Emit(SF(rd) | op | Flags(S) | Rm(operand.reg()) | Rn(rn) | Rd(rd));
}


void Assembler::hlt(int code) {
  VIXL_ASSERT(is_uint16(code));
  Emit(HLT | ImmException(code));
}


void Assembler::brk(int code) {
  VIXL_ASSERT(is_uint16(code));
  Emit(BRK | ImmException(code));
}


void Assembler::svc(int code) {
  VIXL_ASSERT(is_uint16(code));
  Emit(SVC | ImmException(code));
}



void Assembler::ConditionalCompare(const Register& rn, const Operand& operand,
                                   StatusFlags nzcv, Condition cond, ConditionalCompareOp op) {
  Instr ccmpop;
  if (operand.IsImmediate()) {
    int64_t immediate = operand.immediate();
    VIXL_ASSERT(IsImmConditionalCompare(immediate));
    ccmpop = ConditionalCompareImmediateFixed | op | ImmCondCmp(immediate);
  } else {
    VIXL_ASSERT(operand.IsShiftedRegister() && (operand.shift_amount() == 0));
    ccmpop = ConditionalCompareRegisterFixed | op | Rm(operand.reg());
  }
  Emit(SF(rn) | ccmpop | Cond(cond) | Rn(rn) | Nzcv(nzcv));
}


void Assembler::DataProcessing1Source(const Register& rd, const Register& rn,
                                      DataProcessing1SourceOp op) {
  VIXL_ASSERT(rd.size() == rn.size());
  Emit(SF(rn) | op | Rn(rn) | Rd(rd));
}


void Assembler::FPDataProcessing1Source(const FPRegister& fd, const FPRegister& fn,
                                        FPDataProcessing1SourceOp op) {
  Emit(FPType(fn) | op | Rn(fn) | Rd(fd));
}


void Assembler::FPDataProcessing2Source(const FPRegister& fd, const FPRegister& fn,
                                        const FPRegister& fm, FPDataProcessing2SourceOp op) {
  VIXL_ASSERT(fd.size() == fn.size());
  VIXL_ASSERT(fd.size() == fm.size());
  Emit(FPType(fd) | op | Rm(fm) | Rn(fn) | Rd(fd));
}


void Assembler::FPDataProcessing3Source(const FPRegister& fd, const FPRegister& fn,
                                        const FPRegister& fm, const FPRegister& fa,
                                        FPDataProcessing3SourceOp op) {
  VIXL_ASSERT(AreSameSizeAndType(fd, fn, fm, fa));
  Emit(FPType(fd) | op | Rm(fm) | Rn(fn) | Rd(fd) | Ra(fa));
}


void Assembler::EmitShift(const Register& rd, const Register& rn,
                          Shift shift, unsigned shift_amount) {
  switch (shift) {
  case LSL:
    lsl(rd, rn, shift_amount);
    break;
  case LSR:
    lsr(rd, rn, shift_amount);
    break;
  case ASR:
    asr(rd, rn, shift_amount);
    break;
  case ROR:
    ror(rd, rn, shift_amount);
    break;
  default:
    VIXL_UNREACHABLE();
  }
}


void Assembler::EmitExtendShift(const Register& rd, const Register& rn,
                                Extend extend, unsigned left_shift) {
  VIXL_ASSERT(rd.size() >= rn.size());
  unsigned reg_size = rd.size();
  
  Register rn_ = Register(rn.code(), rd.size());
  
  unsigned high_bit = (8 << (extend & 0x3)) - 1;
  
  unsigned non_shift_bits = (reg_size - left_shift) & (reg_size - 1);

  if ((non_shift_bits > high_bit) || (non_shift_bits == 0)) {
    switch (extend) {
    case UXTB:
    case UXTH:
    case UXTW:
      ubfm(rd, rn_, non_shift_bits, high_bit);
      break;
    case SXTB:
    case SXTH:
    case SXTW:
      sbfm(rd, rn_, non_shift_bits, high_bit);
      break;
    case UXTX:
    case SXTX: {
        VIXL_ASSERT(rn.size() == kXRegSize);
        
        lsl(rd, rn_, left_shift);
        break;
      }
    default:
      VIXL_UNREACHABLE();
    }
  } else {
    
    lsl(rd, rn_, left_shift);
  }
}


void Assembler::DataProcExtendedRegister(const Register& rd, const Register& rn,
                                         const Operand& operand, FlagsUpdate S, Instr op) {
  Instr dest_reg = (S == SetFlags) ? Rd(rd) : RdSP(rd);
  Emit(SF(rd) | op | Flags(S) | Rm(operand.reg()) |
       ExtendMode(operand.extend()) | ImmExtendShift(operand.shift_amount()) |
       dest_reg | RnSP(rn));
}


bool Assembler::IsImmAddSub(int64_t immediate) {
  return is_uint12(immediate) || (is_uint12(immediate >> 12) && ((immediate & 0xfff) == 0));
}


BufferOffset Assembler::LoadStore(const CPURegister& rt, const MemOperand& addr,
                                  LoadStoreOp op, LoadStoreScalingOption option) {
  Instr memop = op | Rt(rt) | RnSP(addr.base());
  ptrdiff_t offset = addr.offset();
  LSDataSize size = CalcLSDataSize(op);

  if (addr.IsImmediateOffset()) {
    bool prefer_unscaled = (option == PreferUnscaledOffset) ||
                           (option == RequireUnscaledOffset);
    if (prefer_unscaled && IsImmLSUnscaled(offset)) {
      
      return Emit(LoadStoreUnscaledOffsetFixed | memop | ImmLS(offset));
    }

    if ((option != RequireUnscaledOffset) && IsImmLSScaled(offset, size)) {
      
      return Emit(LoadStoreUnsignedOffsetFixed | memop |
                  ImmLSUnsigned(offset >> size));
    }

    if ((option != RequireScaledOffset) && IsImmLSUnscaled(offset)) {
      
      return Emit(LoadStoreUnscaledOffsetFixed | memop | ImmLS(offset));
    }
  }

  
  
  VIXL_ASSERT((option != RequireUnscaledOffset) &&
             (option != RequireScaledOffset));

  if (addr.IsRegisterOffset()) {
    Extend ext = addr.extend();
    Shift shift = addr.shift();
    unsigned shift_amount = addr.shift_amount();

    
    if (shift == LSL) {
      ext = UXTX;
    }

    
    
    VIXL_ASSERT((shift_amount == 0) ||
               (shift_amount == static_cast<unsigned>(CalcLSDataSize(op))));
    return Emit(LoadStoreRegisterOffsetFixed | memop | Rm(addr.regoffset()) |
                ExtendMode(ext) | ImmShiftLS((shift_amount > 0) ? 1 : 0));
  }

  if (addr.IsPreIndex() && IsImmLSUnscaled(offset)) {
    return Emit(LoadStorePreIndexFixed | memop | ImmLS(offset));
  }

  if (addr.IsPostIndex() && IsImmLSUnscaled(offset)) {
    return Emit(LoadStorePostIndexFixed | memop | ImmLS(offset));
  }

  
  VIXL_UNREACHABLE();
}


bool Assembler::IsImmLSUnscaled(ptrdiff_t offset) {
  return is_int9(offset);
}


bool Assembler::IsImmLSScaled(ptrdiff_t offset, LSDataSize size) {
  bool offset_is_size_multiple = (((offset >> size) << size) == offset);
  return offset_is_size_multiple && is_uint12(offset >> size);
}


void Assembler::LoadLiteral(const CPURegister& rt, uint64_t imm, LoadLiteralOp op) {
  VIXL_ASSERT(is_int32(imm) || is_uint32(imm) || (rt.Is64Bits()));

  
  MOZ_CRASH("LoadLiteral");
  RecordLiteral(imm, rt.SizeInBytes());
  Emit(op | ImmLLiteral(0) | Rt(rt));
}


void Assembler::LoadPCLiteral(const CPURegister& rt, ptrdiff_t pcInsOffset, LoadLiteralOp op) {
  VIXL_ASSERT(is_int19(pcInsOffset));

  
  Emit(op | ImmLLiteral(pcInsOffset) | Rt(rt));
}









bool Assembler::IsImmLogical(uint64_t value, unsigned width, unsigned* n,
                             unsigned* imm_s, unsigned* imm_r) {
  VIXL_ASSERT((width == kWRegSize) || (width == kXRegSize));

  bool negate = false;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  if (value & 1) {
    
    
    negate = true;
    value = ~value;
  }

  if (width == kWRegSize) {
    
    
    
    

    
    
    value <<= kWRegSize;
    value |= value >> kWRegSize;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  uint64_t a = LowestSetBit(value);
  uint64_t value_plus_a = value + a;
  uint64_t b = LowestSetBit(value_plus_a);
  uint64_t value_plus_a_minus_b = value_plus_a - b;
  uint64_t c = LowestSetBit(value_plus_a_minus_b);

  int d, clz_a, out_n;
  uint64_t mask;

  if (c != 0) {
    
    
    
    
    clz_a = CountLeadingZeros(a, kXRegSize);
    int clz_c = CountLeadingZeros(c, kXRegSize);
    d = clz_a - clz_c;
    mask = ((UINT64_C(1) << d) - 1);
    out_n = 0;
  } else {
    
    
    
    
    
    
    if (a == 0) {
      
      
      
      return false;
    } else {
      
      
      
      
      clz_a = CountLeadingZeros(a, kXRegSize);
      d = 64;
      mask = ~UINT64_C(0);
      out_n = 1;
    }
  }

  
  if (!js::IsPowerOfTwo(d)) {
    return false;
  }

  if (((b - a) & ~mask) != 0) {
    
    
    return false;
  }

  
  
  
  
  
  
  
  static const uint64_t multipliers[] = {
    0x0000000000000001UL,
    0x0000000100000001UL,
    0x0001000100010001UL,
    0x0101010101010101UL,
    0x1111111111111111UL,
    0x5555555555555555UL,
  };
  uint64_t multiplier = multipliers[CountLeadingZeros(d, kXRegSize) - 57];
  uint64_t candidate = (b - a) * multiplier;

  if (value != candidate) {
    
    return false;
  }

  
  
  

  
  
  
  int clz_b = (b == 0) ? -1 : CountLeadingZeros(b, kXRegSize);
  int s = clz_a - clz_b;

  
  
  int r;
  if (negate) {
    
    
    
    
    s = d - s;
    r = (clz_b + 1) & (d - 1);
  } else {
    r = (clz_a + 1) & (d - 1);
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  if ((n != NULL) || (imm_s != NULL) || (imm_r != NULL)) {
    *n = out_n;
    *imm_s = ((-d << 1) | (s - 1)) & 0x3f;
    *imm_r = r;
  }

  return true;
}


bool Assembler::IsImmConditionalCompare(int64_t immediate) {
  return is_uint5(immediate);
}


bool Assembler::IsImmFP32(float imm) {
  
  
  uint32_t bits = float_to_rawbits(imm);
  
  if ((bits & 0x7ffff) != 0)
    return false;

  
  uint32_t b_pattern = (bits >> 16) & 0x3e00;
  if (b_pattern != 0 && b_pattern != 0x3e00)
    return false;

  
  if (((bits ^ (bits << 1)) & 0x40000000) == 0)
    return false;

  return true;
}


bool Assembler::IsImmFP64(double imm) {
  
  
  
  uint64_t bits = double_to_rawbits(imm);
  
  if ((bits & 0x0000ffffffffffff) != 0)
    return false;

  
  uint32_t b_pattern = (bits >> 48) & 0x3fc0;
  if ((b_pattern != 0) && (b_pattern != 0x3fc0))
    return false;

  
  if (((bits ^ (bits << 1)) & (UINT64_C(1) << 62)) == 0)
    return false;

  return true;
}


LoadStoreOp Assembler::LoadOpFor(const CPURegister& rt) {
  VIXL_ASSERT(rt.IsValid());
  if (rt.IsRegister())
    return rt.Is64Bits() ? LDR_x : LDR_w;

  VIXL_ASSERT(rt.IsFPRegister());
  return rt.Is64Bits() ? LDR_d : LDR_s;
}


LoadStorePairOp Assembler::LoadPairOpFor(const CPURegister& rt, const CPURegister& rt2) {
  VIXL_ASSERT(AreSameSizeAndType(rt, rt2));
  USE(rt2);
  if (rt.IsRegister())
    return rt.Is64Bits() ? LDP_x : LDP_w;

  VIXL_ASSERT(rt.IsFPRegister());
  return rt.Is64Bits() ? LDP_d : LDP_s;
}


LoadStoreOp Assembler::StoreOpFor(const CPURegister& rt) {
  VIXL_ASSERT(rt.IsValid());
  if (rt.IsRegister())
    return rt.Is64Bits() ? STR_x : STR_w;

  VIXL_ASSERT(rt.IsFPRegister());
  return rt.Is64Bits() ? STR_d : STR_s;
}


LoadStorePairOp Assembler::StorePairOpFor(const CPURegister& rt, const CPURegister& rt2) {
  VIXL_ASSERT(AreSameSizeAndType(rt, rt2));
  USE(rt2);
  if (rt.IsRegister())
    return rt.Is64Bits() ? STP_x : STP_w;

  VIXL_ASSERT(rt.IsFPRegister());
  return rt.Is64Bits() ? STP_d : STP_s;
}


LoadStorePairNonTemporalOp Assembler::LoadPairNonTemporalOpFor(const CPURegister& rt, const CPURegister& rt2) {
  VIXL_ASSERT(AreSameSizeAndType(rt, rt2));
  USE(rt2);
  if (rt.IsRegister())
    return rt.Is64Bits() ? LDNP_x : LDNP_w;

  VIXL_ASSERT(rt.IsFPRegister());
  return rt.Is64Bits() ? LDNP_d : LDNP_s;
}


LoadStorePairNonTemporalOp Assembler::StorePairNonTemporalOpFor(const CPURegister& rt, const CPURegister& rt2) {
  VIXL_ASSERT(AreSameSizeAndType(rt, rt2));
  USE(rt2);
  if (rt.IsRegister())
    return rt.Is64Bits() ? STNP_x : STNP_w;

  VIXL_ASSERT(rt.IsFPRegister());
  return rt.Is64Bits() ? STNP_d : STNP_s;
}


LoadLiteralOp Assembler::LoadLiteralOpFor(const CPURegister& rt) {
  if (rt.IsRegister())
    return rt.Is64Bits() ? LDR_x_lit : LDR_w_lit;

  VIXL_ASSERT(rt.IsFPRegister());
  return rt.Is64Bits() ? LDR_d_lit : LDR_s_lit;
}


void Assembler::RecordLiteral(int64_t imm, unsigned size) {
  MOZ_CRASH("RecordLiteral");
}



bool AreAliased(const CPURegister& reg1, const CPURegister& reg2,
                const CPURegister& reg3, const CPURegister& reg4,
                const CPURegister& reg5, const CPURegister& reg6,
                const CPURegister& reg7, const CPURegister& reg8) {
  int number_of_valid_regs = 0;
  int number_of_valid_fpregs = 0;

  RegList unique_regs = 0;
  RegList unique_fpregs = 0;

  const CPURegister regs[] = {reg1, reg2, reg3, reg4, reg5, reg6, reg7, reg8};

  for (unsigned i = 0; i < sizeof(regs) / sizeof(regs[0]); i++) {
    if (regs[i].IsRegister()) {
      number_of_valid_regs++;
      unique_regs |= regs[i].Bit();
    } else if (regs[i].IsFPRegister()) {
      number_of_valid_fpregs++;
      unique_fpregs |= regs[i].Bit();
    } else {
      VIXL_ASSERT(!regs[i].IsValid());
    }
  }

  int number_of_unique_regs = CountSetBits(unique_regs, sizeof(unique_regs) * 8);
  int number_of_unique_fpregs = CountSetBits(unique_fpregs, sizeof(unique_fpregs) * 8);

  VIXL_ASSERT(number_of_valid_regs >= number_of_unique_regs);
  VIXL_ASSERT(number_of_valid_fpregs >= number_of_unique_fpregs);

  return (number_of_valid_regs != number_of_unique_regs) ||
         (number_of_valid_fpregs != number_of_unique_fpregs);
}


bool AreSameSizeAndType(const CPURegister& reg1, const CPURegister& reg2,
                        const CPURegister& reg3, const CPURegister& reg4,
                        const CPURegister& reg5, const CPURegister& reg6,
                        const CPURegister& reg7, const CPURegister& reg8) {
  VIXL_ASSERT(reg1.IsValid());
  bool match = true;
  match &= !reg2.IsValid() || reg2.IsSameSizeAndType(reg1);
  match &= !reg3.IsValid() || reg3.IsSameSizeAndType(reg1);
  match &= !reg4.IsValid() || reg4.IsSameSizeAndType(reg1);
  match &= !reg5.IsValid() || reg5.IsSameSizeAndType(reg1);
  match &= !reg6.IsValid() || reg6.IsSameSizeAndType(reg1);
  match &= !reg7.IsValid() || reg7.IsSameSizeAndType(reg1);
  match &= !reg8.IsValid() || reg8.IsSameSizeAndType(reg1);
  return match;
}

} 
