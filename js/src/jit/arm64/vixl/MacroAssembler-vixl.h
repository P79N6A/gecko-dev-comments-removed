

























#ifndef VIXL_A64_MACRO_ASSEMBLER_A64_H_
#define VIXL_A64_MACRO_ASSEMBLER_A64_H_

#include "jit/arm64/Assembler-arm64.h"

#include "jit/arm64/vixl/Debugger-vixl.h"
#include "jit/arm64/vixl/Globals-vixl.h"

#define LS_MACRO_LIST(V)                                      \
  V(Ldrb, Register&, rt, LDRB_w)                              \
  V(Strb, Register&, rt, STRB_w)                              \
  V(Ldrsb, Register&, rt, rt.Is64Bits() ? LDRSB_x : LDRSB_w)  \
  V(Ldrh, Register&, rt, LDRH_w)                              \
  V(Strh, Register&, rt, STRH_w)                              \
  V(Ldrsh, Register&, rt, rt.Is64Bits() ? LDRSH_x : LDRSH_w)  \
  V(Ldr, CPURegister&, rt, LoadOpFor(rt))                     \
  V(Str, CPURegister&, rt, StoreOpFor(rt))                    \
  V(Ldrsw, Register&, rt, LDRSW_x)

namespace vixl {

enum BranchType {
  
  
  
  integer_eq = eq,
  integer_ne = ne,
  integer_hs = hs,
  integer_lo = lo,
  integer_mi = mi,
  integer_pl = pl,
  integer_vs = vs,
  integer_vc = vc,
  integer_hi = hi,
  integer_ls = ls,
  integer_ge = ge,
  integer_lt = lt,
  integer_gt = gt,
  integer_le = le,
  integer_al = al,
  integer_nv = nv,

  
  
  
  
  always, never,
  
  reg_zero, reg_not_zero,
  
  reg_bit_clear, reg_bit_set,

  
  kBranchTypeFirstCondition = eq,
  kBranchTypeLastCondition = nv,
  kBranchTypeFirstUsingReg = reg_zero,
  kBranchTypeFirstUsingBit = reg_bit_clear
};

enum DiscardMoveMode { kDontDiscardForSameWReg, kDiscardForSameWReg };

class MacroAssembler : public js::jit::Assembler {
 public:
  MacroAssembler()
    : js::jit::Assembler(),
      sp_(x28),
      tmp_list_(ip0, ip1),
      fptmp_list_(d31) {
  }

  
  void And(const Register& rd, const Register& rn, const Operand& operand);
  void Ands(const Register& rd, const Register& rn, const Operand& operand);
  void Bic(const Register& rd, const Register& rn, const Operand& operand);
  void Bics(const Register& rd, const Register& rn, const Operand& operand);
  void Orr(const Register& rd, const Register& rn, const Operand& operand);
  void Orn(const Register& rd, const Register& rn, const Operand& operand);
  void Eor(const Register& rd, const Register& rn, const Operand& operand);
  void Eon(const Register& rd, const Register& rn, const Operand& operand);
  void Tst(const Register& rn, const Operand& operand);
  void LogicalMacro(const Register& rd, const Register& rn,
                    const Operand& operand, LogicalOp op);

  
  void Add(const Register& rd, const Register& rn, const Operand& operand);
  void Adds(const Register& rd, const Register& rn, const Operand& operand);
  void Sub(const Register& rd, const Register& rn, const Operand& operand);
  void Subs(const Register& rd, const Register& rn, const Operand& operand);
  void Cmn(const Register& rn, const Operand& operand);
  void Cmp(const Register& rn, const Operand& operand);
  void Neg(const Register& rd, const Operand& operand);
  void Negs(const Register& rd, const Operand& operand);
  void AddSubMacro(const Register& rd, const Register& rn,
                   const Operand& operand, FlagsUpdate S, AddSubOp op);

  
  void Adc(const Register& rd, const Register& rn, const Operand& operand);
  void Adcs(const Register& rd, const Register& rn, const Operand& operand);
  void Sbc(const Register& rd, const Register& rn, const Operand& operand);
  void Sbcs(const Register& rd, const Register& rn, const Operand& operand);
  void Ngc(const Register& rd, const Operand& operand);
  void Ngcs(const Register& rd, const Operand& operand);
  void AddSubWithCarryMacro(const Register& rd, const Register& rn,
                            const Operand& operand, FlagsUpdate S, AddSubWithCarryOp op);

  
  void Mov(const Register& rd, uint64_t imm);
  void Mov(const Register& rd, const Operand& operand,
           DiscardMoveMode discard_mode = kDontDiscardForSameWReg);
  void Mvn(const Register& rd, uint64_t imm) {
    Mov(rd, (rd.size() == kXRegSize) ? ~imm : (~imm & kWRegMask));
  }
  void Mvn(const Register& rd, const Operand& operand);
  bool IsImmMovz(uint64_t imm, unsigned reg_size);
  bool IsImmMovn(uint64_t imm, unsigned reg_size);
  unsigned CountClearHalfWords(uint64_t imm, unsigned reg_size);

  
  
  
  bool TryOneInstrMoveImmediate(const Register& dst, int64_t imm);

  
  
  
  
  
  Operand MoveImmediateForShiftedOp(const Register& dst, int64_t imm);

  
  void Ccmp(const Register& rn, const Operand& operand, StatusFlags nzcv, Condition cond);
  void Ccmn(const Register& rn, const Operand& operand, StatusFlags nzcv, Condition cond);
  void ConditionalCompareMacro(const Register& rn, const Operand& operand,
                               StatusFlags nzcv, Condition cond, ConditionalCompareOp op);
  void Csel(const Register& rd, const Register& rn, const Operand& operand, Condition cond);

  
#define DECLARE_FUNCTION(FN, REGTYPE, REG, OP) \
    void FN(const REGTYPE REG, const MemOperand& addr);
  LS_MACRO_LIST(DECLARE_FUNCTION)
#undef DECLARE_FUNCTION

  BufferOffset LoadStoreMacro(const CPURegister& rt, const MemOperand& addr, LoadStoreOp op);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  void Push(const CPURegister& src0, const CPURegister& src1 = NoReg,
            const CPURegister& src2 = NoReg, const CPURegister& src3 = NoReg);
  void Pop(const CPURegister& dst0, const CPURegister& dst1 = NoReg,
           const CPURegister& dst2 = NoReg, const CPURegister& dst3 = NoReg);
  void PushStackPointer();

  
  
  
  
  
  
  
  
  
  
  void PushCPURegList(CPURegList registers);
  void PopCPURegList(CPURegList registers);

  void PushSizeRegList(RegList registers, unsigned reg_size,
                       CPURegister::RegisterType type = CPURegister::kRegister) {
    PushCPURegList(CPURegList(type, reg_size, registers));
  }
  void PopSizeRegList(RegList registers, unsigned reg_size,
                      CPURegister::RegisterType type = CPURegister::kRegister) {
    PopCPURegList(CPURegList(type, reg_size, registers));
  }
  void PushXRegList(RegList regs) {
    PushSizeRegList(regs, kXRegSize);
  }
  void PopXRegList(RegList regs) {
    PopSizeRegList(regs, kXRegSize);
  }
  void PushWRegList(RegList regs) {
    PushSizeRegList(regs, kWRegSize);
  }
  void PopWRegList(RegList regs) {
    PopSizeRegList(regs, kWRegSize);
  }
  inline void PushDRegList(RegList regs) {
    PushSizeRegList(regs, kDRegSize, CPURegister::kFPRegister);
  }
  inline void PopDRegList(RegList regs) {
    PopSizeRegList(regs, kDRegSize, CPURegister::kFPRegister);
  }
  inline void PushSRegList(RegList regs) {
    PushSizeRegList(regs, kSRegSize, CPURegister::kFPRegister);
  }
  inline void PopSRegList(RegList regs) {
    PopSizeRegList(regs, kSRegSize, CPURegister::kFPRegister);
  }

  
  void PushMultipleTimes(int count, Register src);

  
  
  
  
  void Poke(const Register& src, const Operand& offset);

  
  
  
  
  void Peek(const Register& dst, const Operand& offset);

  
  
  
  
  
  void Claim(const Operand& size);
  void Drop(const Operand& size);

  
  
  
  
  
  
  
  
  
  void PushCalleeSavedRegisters();

  
  
  
  
  
  
  
  
  
  void PopCalleeSavedRegisters();

  
  void Adr(const Register& rd, Label* label) {
    VIXL_ASSERT(!rd.IsZero());
    adr(rd, label);
  }
  void Adrp(const Register& rd, Label* label) {
    VIXL_ASSERT(!rd.IsZero());
    adrp(rd, label);
  }
  void Asr(const Register& rd, const Register& rn, unsigned shift) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    asr(rd, rn, shift);
  }
  void Asr(const Register& rd, const Register& rn, const Register& rm) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    VIXL_ASSERT(!rm.IsZero());
    asrv(rd, rn, rm);
  }

  
  JS_STATIC_ASSERT((reg_zero      == (reg_not_zero ^ 1)) &&
                   (reg_bit_clear == (reg_bit_set ^ 1)) &&
                   (always        == (never ^ 1)));

  BranchType InvertBranchType(BranchType type) {
    if (kBranchTypeFirstCondition <= type && type <= kBranchTypeLastCondition)
      return static_cast<BranchType>(InvertCondition(static_cast<Condition>(type)));
    return static_cast<BranchType>(type ^ 1);
  }

  void B(Label* label, BranchType type, Register reg = NoReg, int bit = -1);

  void B(Label* label) {
    b(label);
  }
  void B(Label* label, Condition cond) {
    VIXL_ASSERT((cond != al) && (cond != nv));
    b(label, cond);
  }
  void B(Condition cond, Label* label) {
    B(label, cond);
  }
  void Bfi(const Register& rd, const Register& rn, unsigned lsb, unsigned width) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    bfi(rd, rn, lsb, width);
  }
  void Bfxil(const Register& rd, const Register& rn, unsigned lsb, unsigned width) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    bfxil(rd, rn, lsb, width);
  }
  void Bind(Label* label) {
    bind(label);
  }
  void Bl(Label* label) {
    bl(label);
  }
  void Blr(const Register& xn) {
    VIXL_ASSERT(!xn.IsZero());
    blr(xn);
  }
  void Br(const Register& xn) {
    VIXL_ASSERT(!xn.IsZero());
    br(xn);
  }
  void Brk(int code = 0) {
    brk(code);
  }
  void Cbnz(const Register& rt, Label* label) {
    VIXL_ASSERT(!rt.IsZero());
    cbnz(rt, label);
  }
  void Cbz(const Register& rt, Label* label) {
    VIXL_ASSERT(!rt.IsZero());
    cbz(rt, label);
  }
  void Cinc(const Register& rd, const Register& rn, Condition cond) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    cinc(rd, rn, cond);
  }
  void Cinv(const Register& rd, const Register& rn, Condition cond) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    cinv(rd, rn, cond);
  }
  void Clrex() {
    clrex();
  }
  void Cls(const Register& rd, const Register& rn) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    cls(rd, rn);
  }
  void Clz(const Register& rd, const Register& rn) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    clz(rd, rn);
  }
  void Cneg(const Register& rd, const Register& rn, Condition cond) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    cneg(rd, rn, cond);
  }
  void Cset(const Register& rd, Condition cond) {
    VIXL_ASSERT(!rd.IsZero());
    cset(rd, cond);
  }
  void Csetm(const Register& rd, Condition cond) {
    VIXL_ASSERT(!rd.IsZero());
    csetm(rd, cond);
  }
  void Csinc(const Register& rd, const Register& rn, const Register& rm, Condition cond) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    VIXL_ASSERT((cond != al) && (cond != nv));
    csinc(rd, rn, rm, cond);
  }
  void Csinv(const Register& rd, const Register& rn, const Register& rm, Condition cond) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    VIXL_ASSERT(!rm.IsZero());
    VIXL_ASSERT((cond != al) && (cond != nv));
    csinv(rd, rn, rm, cond);
  }
  void Csneg(const Register& rd, const Register& rn, const Register& rm, Condition cond) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    VIXL_ASSERT((cond != al) && (cond != nv));
    csneg(rd, rn, rm, cond);
  }
  void Dmb(BarrierDomain domain, BarrierType type) {
    dmb(domain, type);
  }
  void Dsb(BarrierDomain domain, BarrierType type) {
    dsb(domain, type);
  }
  void Extr(const Register& rd, const Register& rn, const Register& rm, unsigned lsb) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    VIXL_ASSERT(!rm.IsZero());
    extr(rd, rn, rm, lsb);
  }
  void Fabs(const FPRegister& fd, const FPRegister& fn) {
    fabs(fd, fn);
  }
  void Fadd(const FPRegister& fd, const FPRegister& fn, const FPRegister& fm) {
    fadd(fd, fn, fm);
  }
  void Fccmp(const FPRegister& fn, const FPRegister& fm, StatusFlags nzcv, Condition cond) {
    VIXL_ASSERT((cond != al) && (cond != nv));
    fccmp(fn, fm, nzcv, cond);
  }
  void Fcmp(const FPRegister& fn, const FPRegister& fm) {
    fcmp(fn, fm);
  }
  void Fcmp(const FPRegister& fn, double value);
  void Fcsel(const FPRegister& fd, const FPRegister& fn,
             const FPRegister& fm, Condition cond) {
    VIXL_ASSERT((cond != al) && (cond != nv));
    fcsel(fd, fn, fm, cond);
  }
  void Fcvt(const FPRegister& fd, const FPRegister& fn) {
    fcvt(fd, fn);
  }
  void Fcvtas(const Register& rd, const FPRegister& fn) {
    VIXL_ASSERT(!rd.IsZero());
    fcvtas(rd, fn);
  }
  void Fcvtau(const Register& rd, const FPRegister& fn) {
    VIXL_ASSERT(!rd.IsZero());
    fcvtau(rd, fn);
  }
  void Fcvtms(const Register& rd, const FPRegister& fn) {
    VIXL_ASSERT(!rd.IsZero());
    fcvtms(rd, fn);
  }
  void Fcvtmu(const Register& rd, const FPRegister& fn) {
    VIXL_ASSERT(!rd.IsZero());
    fcvtmu(rd, fn);
  }
  void Fcvtps(const Register& rd, const FPRegister& fn) {
    VIXL_ASSERT(!rd.IsZero());
    fcvtps(rd, fn);
  }
  void Fcvtpu(const Register& rd, const FPRegister& fn) {
    VIXL_ASSERT(!rd.IsZero());
    fcvtpu(rd, fn);
  }
  void Fcvtns(const Register& rd, const FPRegister& fn) {
    VIXL_ASSERT(!rd.IsZero());
    fcvtns(rd, fn);
  }
  void Fcvtnu(const Register& rd, const FPRegister& fn) {
    VIXL_ASSERT(!rd.IsZero());
    fcvtnu(rd, fn);
  }
  void Fcvtzs(const Register& rd, const FPRegister& fn) {
    VIXL_ASSERT(!rd.IsZero());
    fcvtzs(rd, fn);
  }
  void Fcvtzu(const Register& rd, const FPRegister& fn) {
    VIXL_ASSERT(!rd.IsZero());
    fcvtzu(rd, fn);
  }
  void Fdiv(const FPRegister& fd, const FPRegister& fn, const FPRegister& fm) {
    fdiv(fd, fn, fm);
  }
  void Fmax(const FPRegister& fd, const FPRegister& fn, const FPRegister& fm) {
    fmax(fd, fn, fm);
  }
  void Fmaxnm(const FPRegister& fd, const FPRegister& fn, const FPRegister& fm) {
    fmaxnm(fd, fn, fm);
  }
  void Fmin(const FPRegister& fd, const FPRegister& fn, const FPRegister& fm) {
    fmin(fd, fn, fm);
  }
  void Fminnm(const FPRegister& fd, const FPRegister& fn, const FPRegister& fm) {
    fminnm(fd, fn, fm);
  }
  void Fmov(FPRegister fd, FPRegister fn) {
    
    
    
    
    if (!fd.Is(fn) || !fd.Is64Bits())
      fmov(fd, fn);
  }
  void Fmov(FPRegister fd, Register rn) {
    VIXL_ASSERT(!rn.IsZero());
    fmov(fd, rn);
  }
  
  
  
  
  void Fmov(FPRegister fd, double imm);
  void Fmov(FPRegister fd, float imm);
  
  template<typename T>
  void Fmov(FPRegister fd, T imm) {
    Fmov(fd, static_cast<double>(imm));
  }
  void Fmov(Register rd, FPRegister fn) {
    VIXL_ASSERT(!rd.IsZero());
    fmov(rd, fn);
  }
  void Fmul(const FPRegister& fd, const FPRegister& fn, const FPRegister& fm) {
    fmul(fd, fn, fm);
  }
  void Fmadd(const FPRegister& fd, const FPRegister& fn,
             const FPRegister& fm, const FPRegister& fa) {
    fmadd(fd, fn, fm, fa);
  }
  void Fmsub(const FPRegister& fd, const FPRegister& fn,
             const FPRegister& fm, const FPRegister& fa) {
    fmsub(fd, fn, fm, fa);
  }
  void Fnmadd(const FPRegister& fd, const FPRegister& fn,
              const FPRegister& fm, const FPRegister& fa) {
    fnmadd(fd, fn, fm, fa);
  }
  void Fnmsub(const FPRegister& fd, const FPRegister& fn,
              const FPRegister& fm, const FPRegister& fa) {
    fnmsub(fd, fn, fm, fa);
  }
  void Fneg(const FPRegister& fd, const FPRegister& fn) {
    fneg(fd, fn);
  }
  void Frinta(const FPRegister& fd, const FPRegister& fn) {
    frinta(fd, fn);
  }
  void Frintm(const FPRegister& fd, const FPRegister& fn) {
    frintm(fd, fn);
  }
  void Frintn(const FPRegister& fd, const FPRegister& fn) {
    frintn(fd, fn);
  }
  void Frintz(const FPRegister& fd, const FPRegister& fn) {
    frintz(fd, fn);
  }
  void Fsqrt(const FPRegister& fd, const FPRegister& fn) {
    fsqrt(fd, fn);
  }
  void Fsub(const FPRegister& fd, const FPRegister& fn, const FPRegister& fm) {
    fsub(fd, fn, fm);
  }
  void Hint(SystemHint code) {
    hint(code);
  }
  void Hlt(int code) {
    hlt(code);
  }
  void Isb() {
    isb();
  }
  void Ldar(const Register& rt, const MemOperand& src) {
    ldar(rt, src);
  }
  void Ldarb(const Register& rt, const MemOperand& src) {
    ldarb(rt, src);
  }
  void Ldarh(const Register& rt, const MemOperand& src) {
    ldarh(rt, src);
  }
  void Ldaxp(const Register& rt, const Register& rt2, const MemOperand& src) {
    VIXL_ASSERT(!rt.Aliases(rt2));
    ldaxp(rt, rt2, src);
  }
  void Ldaxr(const Register& rt, const MemOperand& src) {
    ldaxr(rt, src);
  }
  void Ldaxrb(const Register& rt, const MemOperand& src) {
    ldaxrb(rt, src);
  }
  void Ldaxrh(const Register& rt, const MemOperand& src) {
    ldaxrh(rt, src);
  }
  void Ldnp(const CPURegister& rt, const CPURegister& rt2, const MemOperand& src) {
    ldnp(rt, rt2, src);
  }
  void Ldp(const CPURegister& rt, const CPURegister& rt2, const MemOperand& src) {
    ldp(rt, rt2, src);
  }
  void Ldpsw(const Register& rt, const Register& rt2, const MemOperand& src) {
    ldpsw(rt, rt2, src);
  }
  
  
  
  
  void Ldr(const FPRegister& ft, double imm) {
    if (ft.Is64Bits()) {
      ldr(ft, imm);
    } else {
      ldr(ft, static_cast<float>(imm));
    }
  }
  void Ldr(const FPRegister& ft, float imm) {
    if (ft.Is32Bits()) {
      ldr(ft, imm);
    } else {
      ldr(ft, static_cast<double>(imm));
    }
  }
  void Ldr(const Register& rt, uint64_t imm) {
    VIXL_ASSERT(!rt.IsZero());
    ldr(rt, imm);
  }
  void Ldxp(const Register& rt, const Register& rt2, const MemOperand& src) {
    VIXL_ASSERT(!rt.Aliases(rt2));
    ldxp(rt, rt2, src);
  }
  void Ldxr(const Register& rt, const MemOperand& src) {
    ldxr(rt, src);
  }
  void Ldxrb(const Register& rt, const MemOperand& src) {
    ldxrb(rt, src);
  }
  void Ldxrh(const Register& rt, const MemOperand& src) {
    ldxrh(rt, src);
  }
  void Lsl(const Register& rd, const Register& rn, unsigned shift) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    lsl(rd, rn, shift);
  }
  void Lsl(const Register& rd, const Register& rn, const Register& rm) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    VIXL_ASSERT(!rm.IsZero());
    lslv(rd, rn, rm);
  }
  void Lsr(const Register& rd, const Register& rn, unsigned shift) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    lsr(rd, rn, shift);
  }
  void Lsr(const Register& rd, const Register& rn, const Register& rm) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    VIXL_ASSERT(!rm.IsZero());
    lsrv(rd, rn, rm);
  }
  void Madd(const Register& rd, const Register& rn,
            const Register& rm, const Register& ra) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    VIXL_ASSERT(!rm.IsZero());
    VIXL_ASSERT(!ra.IsZero());
    madd(rd, rn, rm, ra);
  }
  void Mneg(const Register& rd, const Register& rn, const Register& rm) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    VIXL_ASSERT(!rm.IsZero());
    mneg(rd, rn, rm);
  }
  void Mov(const Register& rd, const Register& rn) {
    mov(rd, rn);
  }
  void Movk(const Register& rd, uint64_t imm, int shift = -1) {
    VIXL_ASSERT(!rd.IsZero());
    movk(rd, imm, shift);
  }
  void Mrs(const Register& rt, SystemRegister sysreg) {
    VIXL_ASSERT(!rt.IsZero());
    mrs(rt, sysreg);
  }
  void Msr(SystemRegister sysreg, const Register& rt) {
    VIXL_ASSERT(!rt.IsZero());
    msr(sysreg, rt);
  }
  void Msub(const Register& rd, const Register& rn,
            const Register& rm, const Register& ra) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    VIXL_ASSERT(!rm.IsZero());
    VIXL_ASSERT(!ra.IsZero());
    msub(rd, rn, rm, ra);
  }
  void Mul(const Register& rd, const Register& rn, const Register& rm) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    VIXL_ASSERT(!rm.IsZero());
    mul(rd, rn, rm);
  }
  void Nop() {
    nop();
  }
  void Rbit(const Register& rd, const Register& rn) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    rbit(rd, rn);
  }
  void Ret(const Register& xn = lr) {
    VIXL_ASSERT(!xn.IsZero());
    ret(xn);
  }
  void Rev(const Register& rd, const Register& rn) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    rev(rd, rn);
  }
  void Rev16(const Register& rd, const Register& rn) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    rev16(rd, rn);
  }
  void Rev32(const Register& rd, const Register& rn) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    rev32(rd, rn);
  }
  void Ror(const Register& rd, const Register& rs, unsigned shift) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rs.IsZero());
    ror(rd, rs, shift);
  }
  void Ror(const Register& rd, const Register& rn, const Register& rm) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    VIXL_ASSERT(!rm.IsZero());
    rorv(rd, rn, rm);
  }
  void Sbfiz(const Register& rd, const Register& rn, unsigned lsb, unsigned width) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    sbfiz(rd, rn, lsb, width);
  }
  void Sbfx(const Register& rd, const Register& rn, unsigned lsb, unsigned width) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    sbfx(rd, rn, lsb, width);
  }
  void Scvtf(const FPRegister& fd, const Register& rn, unsigned fbits = 0) {
    VIXL_ASSERT(!rn.IsZero());
    scvtf(fd, rn, fbits);
  }
  void Sdiv(const Register& rd, const Register& rn, const Register& rm) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    VIXL_ASSERT(!rm.IsZero());
    sdiv(rd, rn, rm);
  }
  void Smaddl(const Register& rd, const Register& rn,
              const Register& rm, const Register& ra) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    VIXL_ASSERT(!rm.IsZero());
    VIXL_ASSERT(!ra.IsZero());
    smaddl(rd, rn, rm, ra);
  }
  void Smsubl(const Register& rd, const Register& rn,
              const Register& rm, const Register& ra) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    VIXL_ASSERT(!rm.IsZero());
    VIXL_ASSERT(!ra.IsZero());
    smsubl(rd, rn, rm, ra);
  }
  void Smull(const Register& rd, const Register& rn, const Register& rm) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    VIXL_ASSERT(!rm.IsZero());
    smull(rd, rn, rm);
  }
  void Smulh(const Register& xd, const Register& xn, const Register& xm) {
    VIXL_ASSERT(!xd.IsZero());
    VIXL_ASSERT(!xn.IsZero());
    VIXL_ASSERT(!xm.IsZero());
    smulh(xd, xn, xm);
  }
  void Stlr(const Register& rt, const MemOperand& dst) {
    stlr(rt, dst);
  }
  void Stlrb(const Register& rt, const MemOperand& dst) {
    stlrb(rt, dst);
  }
  void Stlrh(const Register& rt, const MemOperand& dst) {
    stlrh(rt, dst);
  }
  void Stlxp(const Register& rs, const Register& rt,
             const Register& rt2, const MemOperand& dst) {
    VIXL_ASSERT(!rs.Aliases(dst.base()));
    VIXL_ASSERT(!rs.Aliases(rt));
    VIXL_ASSERT(!rs.Aliases(rt2));
    stlxp(rs, rt, rt2, dst);
  }
  void Stlxr(const Register& rs, const Register& rt, const MemOperand& dst) {
    VIXL_ASSERT(!rs.Aliases(dst.base()));
    VIXL_ASSERT(!rs.Aliases(rt));
    stlxr(rs, rt, dst);
  }
  void Stlxrb(const Register& rs, const Register& rt, const MemOperand& dst) {
    VIXL_ASSERT(!rs.Aliases(dst.base()));
    VIXL_ASSERT(!rs.Aliases(rt));
    stlxrb(rs, rt, dst);
  }
  void Stlxrh(const Register& rs, const Register& rt, const MemOperand& dst) {
    VIXL_ASSERT(!rs.Aliases(dst.base()));
    VIXL_ASSERT(!rs.Aliases(rt));
    stlxrh(rs, rt, dst);
  }
  void Stnp(const CPURegister& rt, const CPURegister& rt2, const MemOperand& dst) {
    stnp(rt, rt2, dst);
  }
  void Stp(const CPURegister& rt, const CPURegister& rt2, const MemOperand& dst) {
    stp(rt, rt2, dst);
  }
  void Stxp(const Register& rs, const Register& rt,
            const Register& rt2, const MemOperand& dst) {
    VIXL_ASSERT(!rs.Aliases(dst.base()));
    VIXL_ASSERT(!rs.Aliases(rt));
    VIXL_ASSERT(!rs.Aliases(rt2));
    stxp(rs, rt, rt2, dst);
  }
  void Stxr(const Register& rs, const Register& rt, const MemOperand& dst) {
    VIXL_ASSERT(!rs.Aliases(dst.base()));
    VIXL_ASSERT(!rs.Aliases(rt));
    stxr(rs, rt, dst);
  }
  void Stxrb(const Register& rs, const Register& rt, const MemOperand& dst) {
    VIXL_ASSERT(!rs.Aliases(dst.base()));
    VIXL_ASSERT(!rs.Aliases(rt));
    stxrb(rs, rt, dst);
  }
  void Stxrh(const Register& rs, const Register& rt, const MemOperand& dst) {
    VIXL_ASSERT(!rs.Aliases(dst.base()));
    VIXL_ASSERT(!rs.Aliases(rt));
    stxrh(rs, rt, dst);
  }
  void Sxtb(const Register& rd, const Register& rn) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    sxtb(rd, rn);
  }
  void Sxth(const Register& rd, const Register& rn) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    sxth(rd, rn);
  }
  void Sxtw(const Register& rd, const Register& rn) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    sxtw(rd, rn);
  }
  void Tbnz(const Register& rt, unsigned bit_pos, Label* label) {
    VIXL_ASSERT(!rt.IsZero());
    tbnz(rt, bit_pos, label);
  }
  void Tbz(const Register& rt, unsigned bit_pos, Label* label) {
    VIXL_ASSERT(!rt.IsZero());
    tbz(rt, bit_pos, label);
  }
  void Ubfiz(const Register& rd, const Register& rn, unsigned lsb, unsigned width) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    ubfiz(rd, rn, lsb, width);
  }
  void Ubfx(const Register& rd, const Register& rn, unsigned lsb, unsigned width) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    ubfx(rd, rn, lsb, width);
  }
  void Ucvtf(const FPRegister& fd, const Register& rn, unsigned fbits = 0) {
    VIXL_ASSERT(!rn.IsZero());
    ucvtf(fd, rn, fbits);
  }
  void Udiv(const Register& rd, const Register& rn, const Register& rm) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    VIXL_ASSERT(!rm.IsZero());
    udiv(rd, rn, rm);
  }
  void Umaddl(const Register& rd, const Register& rn,
              const Register& rm, const Register& ra) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    VIXL_ASSERT(!rm.IsZero());
    VIXL_ASSERT(!ra.IsZero());
    umaddl(rd, rn, rm, ra);
  }
  void Umsubl(const Register& rd, const Register& rn,
              const Register& rm, const Register& ra) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    VIXL_ASSERT(!rm.IsZero());
    VIXL_ASSERT(!ra.IsZero());
    umsubl(rd, rn, rm, ra);
  }
  void Unreachable() {
#ifdef JS_SIMULATOR_ARM64
    hlt(kUnreachableOpcode);
#else
    
    
    blr(xzr);
#endif
  }
  void Uxtb(const Register& rd, const Register& rn) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    uxtb(rd, rn);
  }
  void Uxth(const Register& rd, const Register& rn) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    uxth(rd, rn);
  }
  void Uxtw(const Register& rd, const Register& rn) {
    VIXL_ASSERT(!rd.IsZero());
    VIXL_ASSERT(!rn.IsZero());
    uxtw(rd, rn);
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  void BumpSystemStackPointer(const Operand& space);

  
  void SetStackPointer64(const Register& stack_pointer) {
    VIXL_ASSERT(!TmpList()->IncludesAliasOf(stack_pointer));
    sp_ = stack_pointer;
  }

  
  const Register& GetStackPointer64() const {
    return sp_;
  }
  const js::jit::Register getStackPointer() const {
    int code = sp_.code();
    if (code == kSPRegInternalCode) {
      code = 31;
    }
    return js::jit::Register::FromCode(code);
  }

  CPURegList* TmpList() {
    return &tmp_list_;
  }
  CPURegList* FPTmpList() {
    return &fptmp_list_;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  void Printf(const char * format,
              CPURegister arg0 = NoCPUReg,
              CPURegister arg1 = NoCPUReg,
              CPURegister arg2 = NoCPUReg,
              CPURegister arg3 = NoCPUReg);

  
  
  
  void PrintfNoPreserve(const char * format,
                        const CPURegister& arg0 = NoCPUReg,
                        const CPURegister& arg1 = NoCPUReg,
                        const CPURegister& arg2 = NoCPUReg,
                        const CPURegister& arg3 = NoCPUReg);


  
  
  
  
  
  
  
  
  
  
  void Trace(TraceParameters parameters, TraceCommand command);

  
  
  
  
  
  
  void Log(TraceParameters parameters);

  
  
  void EnableInstrumentation();
  void DisableInstrumentation();

  
  
  
  void AnnotateInstrumentation(const char* marker_name);

 private:
  
  
  
  
  
  
  void PushHelper(int count, int size,
                  const CPURegister& src0, const CPURegister& src1,
                  const CPURegister& src2, const CPURegister& src3);
  void PopHelper(int count, int size,
                 const CPURegister& dst0, const CPURegister& dst1,
                 const CPURegister& dst2, const CPURegister& dst3);

  
  
  
  void PrepareForPush(int count, int size);
  void PrepareForPop(int count, int size);

#if DEBUG
  
  
  
#endif

  
  Register sp_;

  
  CPURegList tmp_list_;
  CPURegList fptmp_list_;
};







class InstructionAccurateScope {
 public:
#ifdef DEBUG
  explicit InstructionAccurateScope(MacroAssembler* masm, int count = 0)
    : masm_(masm), size_(count * kInstructionSize) {
    
    if (size_ != 0) {
      masm_->bind(&start_);
    }
  }
#else
  explicit InstructionAccurateScope(MacroAssembler* masm, int count = 0)
    : masm_(masm) {
    USE(count);
    
  }
#endif

  ~InstructionAccurateScope() {
    
#if 0 
#ifdef DEBUG
    if (start_.bound()) {
      VIXL_ASSERT(masm_->SizeOfCodeGeneratedSince(&start_) == size_);
    }
#endif
#endif
  }

 private:
  MacroAssembler* masm_;
#ifdef DEBUG
  uint64_t size_;
  Label start_;
#endif
};









class UseScratchRegisterScope {
 public:
  explicit UseScratchRegisterScope(MacroAssembler* masm)
    : available_(masm->TmpList()),
      availablefp_(masm->FPTmpList()),
      old_available_(available_->list()),
      old_availablefp_(availablefp_->list()) {
    VIXL_ASSERT(available_->type() == CPURegister::kRegister);
    VIXL_ASSERT(availablefp_->type() == CPURegister::kFPRegister);
  }

  ~UseScratchRegisterScope();

  bool IsAvailable(const CPURegister& reg) const;

  
  
  Register AcquireW() {
    return AcquireNextAvailable(available_).W();
  }
  Register AcquireX() {
    return AcquireNextAvailable(available_).X();
  }
  FPRegister AcquireS() {
    return AcquireNextAvailable(availablefp_).S();
  }
  FPRegister AcquireD() {
    return AcquireNextAvailable(availablefp_).D();
  }


  Register AcquireSameSizeAs(const Register& reg);
  FPRegister AcquireSameSizeAs(const FPRegister& reg);


  
  
  void Release(const CPURegister& reg);


  
  
  void Include(const CPURegList& list);
  void Include(const Register& reg1,
               const Register& reg2 = NoReg,
               const Register& reg3 = NoReg,
               const Register& reg4 = NoReg);
  void Include(const FPRegister& reg1,
               const FPRegister& reg2 = NoFPReg,
               const FPRegister& reg3 = NoFPReg,
               const FPRegister& reg4 = NoFPReg);


  
  
  
  void Exclude(const CPURegList& list);
  void Exclude(const Register& reg1,
               const Register& reg2 = NoReg,
               const Register& reg3 = NoReg,
               const Register& reg4 = NoReg);
  void Exclude(const FPRegister& reg1,
               const FPRegister& reg2 = NoFPReg,
               const FPRegister& reg3 = NoFPReg,
               const FPRegister& reg4 = NoFPReg);
  void Exclude(const CPURegister& reg1,
               const CPURegister& reg2 = NoCPUReg,
               const CPURegister& reg3 = NoCPUReg,
               const CPURegister& reg4 = NoCPUReg);


  
  void ExcludeAll();


 private:
  static CPURegister AcquireNextAvailable(CPURegList* available);
  static void ReleaseByCode(CPURegList* available, int code);
  static void ReleaseByRegList(CPURegList* available, RegList regs);
  static void IncludeByRegList(CPURegList* available, RegList exclude);
  static void ExcludeByRegList(CPURegList* available, RegList exclude);

  
  CPURegList* available_;     
  CPURegList* availablefp_;   

  
  RegList old_available_;     
  RegList old_availablefp_;   
};

} 

#endif  
