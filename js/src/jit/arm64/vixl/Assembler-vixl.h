

























#ifndef VIXL_A64_ASSEMBLER_A64_H_
#define VIXL_A64_ASSEMBLER_A64_H_

#include "jit/arm64/vixl/Globals-vixl.h"
#include "jit/arm64/vixl/Instructions-vixl.h"
#include "jit/arm64/vixl/MozBaseAssembler-vixl.h"
#include "jit/arm64/vixl/Utils-vixl.h"

#include "jit/JitSpewer.h"

#include "jit/shared/Assembler-shared.h"
#include "jit/shared/IonAssemblerBufferWithConstantPools.h"

namespace vixl {

using js::jit::BufferOffset;
using js::jit::Label;
using js::jit::Address;
using js::jit::BaseIndex;




typedef uint64_t RegList;
static const int kRegListSizeInBits = sizeof(RegList) * 8;



class Register;
class FPRegister;

class CPURegister {
 public:
  enum RegisterType {
    
    
    kInvalid = 0,
    kRegister,
    kFPRegister,
    kNoRegister
  };

  constexpr CPURegister()
    : code_(0), size_(0), type_(kNoRegister) {
  }

  constexpr CPURegister(unsigned code, unsigned size, RegisterType type)
    : code_(code), size_(size), type_(type) {
  }

  unsigned code() const {
    VIXL_ASSERT(IsValid());
    return code_;
  }

  RegisterType type() const {
    VIXL_ASSERT(IsValidOrNone());
    return type_;
  }

  RegList Bit() const {
    VIXL_ASSERT(code_ < (sizeof(RegList) * 8));
    return IsValid() ? (static_cast<RegList>(1) << code_) : 0;
  }

  unsigned size() const {
    VIXL_ASSERT(IsValid());
    return size_;
  }

  int SizeInBytes() const {
    VIXL_ASSERT(IsValid());
    VIXL_ASSERT(size() % 8 == 0);
    return size_ / 8;
  }

  int SizeInBits() const {
    VIXL_ASSERT(IsValid());
    return size_;
  }

  bool Is32Bits() const {
    VIXL_ASSERT(IsValid());
    return size_ == 32;
  }

  bool Is64Bits() const {
    VIXL_ASSERT(IsValid());
    return size_ == 64;
  }

  bool IsValid() const {
    if (IsValidRegister() || IsValidFPRegister()) {
      VIXL_ASSERT(!IsNone());
      return true;
    }

    VIXL_ASSERT(IsNone());
    return false;
  }

  bool IsValidRegister() const {
    return IsRegister() &&
           ((size_ == kWRegSize) || (size_ == kXRegSize)) &&
           ((code_ < kNumberOfRegisters) || (code_ == kSPRegInternalCode));
  }

  bool IsValidFPRegister() const {
    return IsFPRegister() &&
           ((size_ == kSRegSize) || (size_ == kDRegSize)) &&
           (code_ < kNumberOfFPRegisters);
  }

  bool IsNone() const {
    
    VIXL_ASSERT((type_ != kNoRegister) || (code_ == 0));
    VIXL_ASSERT((type_ != kNoRegister) || (size_ == 0));

    return type_ == kNoRegister;
  }

  bool Aliases(const CPURegister& other) const {
    VIXL_ASSERT(IsValidOrNone() && other.IsValidOrNone());
    return (code_ == other.code_) && (type_ == other.type_);
  }

  bool Is(const CPURegister& other) const {
    VIXL_ASSERT(IsValidOrNone() && other.IsValidOrNone());
    return Aliases(other) && (size_ == other.size_);
  }

  inline bool IsZero() const {
    VIXL_ASSERT(IsValid());
    return IsRegister() && (code_ == kZeroRegCode);
  }

  inline bool IsSP() const {
    VIXL_ASSERT(IsValid());
    return IsRegister() && (code_ == kSPRegInternalCode);
  }

  inline bool IsRegister() const {
    return type_ == kRegister;
  }

  inline bool IsFPRegister() const {
    return type_ == kFPRegister;
  }

  const Register& W() const;
  const Register& X() const;
  const FPRegister& S() const;
  const FPRegister& D() const;

  inline bool IsSameSizeAndType(const CPURegister& other) const {
    return (size_ == other.size_) && (type_ == other.type_);
  }

 protected:
  unsigned code_;
  unsigned size_;
  RegisterType type_;

 private:
  bool IsValidOrNone() const {
    return IsValid() || IsNone();
  }
};

class Register : public CPURegister {
 public:
  explicit Register() : CPURegister() {}

  inline explicit Register(const CPURegister& other)
    : CPURegister(other.code(), other.size(), other.type()) {
    VIXL_ASSERT(IsValidRegister());
  }

  constexpr Register(unsigned code, unsigned size)
    : CPURegister(code, size, kRegister) {
  }

  constexpr Register(js::jit::Register r, unsigned size)
    : CPURegister(r.code(), size, kRegister) {
  }

  bool IsValid() const {
    VIXL_ASSERT(IsRegister() || IsNone());
    return IsValidRegister();
  }

  static const Register& WRegFromCode(unsigned code);
  static const Register& XRegFromCode(unsigned code);

  
  static const int kNumRegisters = kNumberOfRegisters;
  static const int kNumAllocatableRegisters = kNumberOfRegisters - 1;

 private:
  static const Register wregisters[];
  static const Register xregisters[];
};

class FPRegister : public CPURegister {
 public:
  inline FPRegister() : CPURegister() {}

  inline explicit FPRegister(const CPURegister& other)
    : CPURegister(other.code(), other.size(), other.type()) {
    VIXL_ASSERT(IsValidFPRegister());
  }
  constexpr inline FPRegister(js::jit::FloatRegister r, unsigned size)
    : CPURegister(r.code_, size, kFPRegister) {
  }
  constexpr inline FPRegister(js::jit::FloatRegister r)
    : CPURegister(r.code_, r.size() * 8, kFPRegister) {
  }
  constexpr inline FPRegister(unsigned code, unsigned size)
    : CPURegister(code, size, kFPRegister) {
  }

  bool IsValid() const {
    VIXL_ASSERT(IsFPRegister() || IsNone());
    return IsValidFPRegister();
  }

  static const FPRegister& SRegFromCode(unsigned code);
  static const FPRegister& DRegFromCode(unsigned code);

  
  static const int kNumRegisters = kNumberOfFPRegisters;
  static const int kNumAllocatableRegisters = kNumberOfFPRegisters - 1;

 private:
  static const FPRegister sregisters[];
  static const FPRegister dregisters[];
};




const Register NoReg;
const FPRegister NoFPReg;
const CPURegister NoCPUReg;

#define DEFINE_REGISTERS(N)  \
constexpr Register w##N(N, kWRegSize);  \
constexpr Register x##N(N, kXRegSize);
REGISTER_CODE_LIST(DEFINE_REGISTERS)
#undef DEFINE_REGISTERS
constexpr Register wsp(kSPRegInternalCode, kWRegSize);
constexpr Register sp(kSPRegInternalCode, kXRegSize);

#define DEFINE_FPREGISTERS(N)  \
constexpr FPRegister s##N(N, kSRegSize);  \
constexpr FPRegister d##N(N, kDRegSize);
REGISTER_CODE_LIST(DEFINE_FPREGISTERS)
#undef DEFINE_FPREGISTERS


constexpr Register ip0 = x16;
constexpr Register ip1 = x17;
constexpr Register lr = x30;
constexpr Register xzr = x31;
constexpr Register wzr = w31;



bool AreAliased(const CPURegister& reg1,
                const CPURegister& reg2,
                const CPURegister& reg3 = NoReg,
                const CPURegister& reg4 = NoReg,
                const CPURegister& reg5 = NoReg,
                const CPURegister& reg6 = NoReg,
                const CPURegister& reg7 = NoReg,
                const CPURegister& reg8 = NoReg);





bool AreSameSizeAndType(const CPURegister& reg1,
                        const CPURegister& reg2,
                        const CPURegister& reg3 = NoCPUReg,
                        const CPURegister& reg4 = NoCPUReg,
                        const CPURegister& reg5 = NoCPUReg,
                        const CPURegister& reg6 = NoCPUReg,
                        const CPURegister& reg7 = NoCPUReg,
                        const CPURegister& reg8 = NoCPUReg);


class CPURegList {
 public:
  inline explicit CPURegList(CPURegister reg1,
                             CPURegister reg2 = NoCPUReg,
                             CPURegister reg3 = NoCPUReg,
                             CPURegister reg4 = NoCPUReg)
    : list_(reg1.Bit() | reg2.Bit() | reg3.Bit() | reg4.Bit()),
      size_(reg1.size()), type_(reg1.type()) {
    VIXL_ASSERT(AreSameSizeAndType(reg1, reg2, reg3, reg4));
    VIXL_ASSERT(IsValid());
  }

  inline CPURegList(CPURegister::RegisterType type, unsigned size, RegList list)
    : list_(list), size_(size), type_(type) {
    VIXL_ASSERT(IsValid());
  }

  inline CPURegList(CPURegister::RegisterType type, unsigned size,
                    unsigned first_reg, unsigned last_reg)
    : size_(size), type_(type) {
    VIXL_ASSERT(((type == CPURegister::kRegister) &&
                (last_reg < kNumberOfRegisters)) ||
               ((type == CPURegister::kFPRegister) &&
                (last_reg < kNumberOfFPRegisters)));
    VIXL_ASSERT(last_reg >= first_reg);
    list_ = (UINT64_C(1) << (last_reg + 1)) - 1;
    list_ &= ~((UINT64_C(1) << first_reg) - 1);
    VIXL_ASSERT(IsValid());
  }

  inline CPURegister::RegisterType type() const {
    VIXL_ASSERT(IsValid());
    return type_;
  }

  
  
  
  void Combine(const CPURegList& other) {
    VIXL_ASSERT(IsValid());
    VIXL_ASSERT(other.type() == type_);
    VIXL_ASSERT(other.RegisterSizeInBits() == size_);
    list_ |= other.list();
  }

  
  
  
  void Remove(const CPURegList& other) {
    VIXL_ASSERT(IsValid());
    VIXL_ASSERT(other.type() == type_);
    VIXL_ASSERT(other.RegisterSizeInBits() == size_);
    list_ &= ~other.list();
  }

  
  inline void Combine(const CPURegister& other) {
    VIXL_ASSERT(other.type() == type_);
    VIXL_ASSERT(other.size() == size_);
    Combine(other.code());
  }

  inline void Remove(const CPURegister& other) {
    VIXL_ASSERT(other.type() == type_);
    VIXL_ASSERT(other.size() == size_);
    Remove(other.code());
  }

  
  
  inline void Combine(int code) {
    VIXL_ASSERT(IsValid());
    VIXL_ASSERT(CPURegister(code, size_, type_).IsValid());
    list_ |= (UINT64_C(1) << code);
  }

  inline void Remove(int code) {
    VIXL_ASSERT(IsValid());
    VIXL_ASSERT(CPURegister(code, size_, type_).IsValid());
    list_ &= ~(UINT64_C(1) << code);
  }

  inline RegList list() const {
    VIXL_ASSERT(IsValid());
    return list_;
  }

  inline void set_list(RegList new_list) {
    VIXL_ASSERT(IsValid());
    list_ = new_list;
  }

  
  
  void RemoveCalleeSaved();

  CPURegister PopLowestIndex();
  CPURegister PopHighestIndex();

  
  static CPURegList GetCalleeSaved(unsigned size = kXRegSize);
  static CPURegList GetCalleeSavedFP(unsigned size = kDRegSize);

  
  static CPURegList GetCallerSaved(unsigned size = kXRegSize);
  static CPURegList GetCallerSavedFP(unsigned size = kDRegSize);

  inline bool IsEmpty() const {
    VIXL_ASSERT(IsValid());
    return list_ == 0;
  }

  inline bool IncludesAliasOf(const CPURegister& other) const {
    VIXL_ASSERT(IsValid());
    return (type_ == other.type()) && ((other.Bit() & list_) != 0);
  }

  inline bool IncludesAliasOf(int code) const {
    VIXL_ASSERT(IsValid());
    return ((code & list_) != 0);
  }

  inline int Count() const {
    VIXL_ASSERT(IsValid());
    return CountSetBits(list_, kRegListSizeInBits);
  }

  inline unsigned RegisterSizeInBits() const {
    VIXL_ASSERT(IsValid());
    return size_;
  }

  inline unsigned RegisterSizeInBytes() const {
    int size_in_bits = RegisterSizeInBits();
    VIXL_ASSERT((size_in_bits % 8) == 0);
    return size_in_bits / 8;
  }

  inline unsigned TotalSizeInBytes() const {
    VIXL_ASSERT(IsValid());
    return RegisterSizeInBytes() * Count();
  }

 private:
  RegList list_;
  unsigned size_;
  CPURegister::RegisterType type_;

  bool IsValid() const;
};


extern const CPURegList kCalleeSaved;
extern const CPURegList kCalleeSavedFP;


extern const CPURegList kCallerSaved;
extern const CPURegList kCallerSavedFP;


class Operand {
 public:
  
  
  
  
  Operand(int64_t immediate);           

  
  
  
  
  
  Operand(Register reg,
          Shift shift = LSL,
          unsigned shift_amount = 0);   

  
  
  
  explicit Operand(Register reg, Extend extend, unsigned shift_amount = 0);

  bool IsImmediate() const;
  bool IsShiftedRegister() const;
  bool IsExtendedRegister() const;
  bool IsZero() const;

  
  
  Operand ToExtendedRegister() const;

  int64_t immediate() const {
    VIXL_ASSERT(IsImmediate());
    return immediate_;
  }

  Register reg() const {
    VIXL_ASSERT(IsShiftedRegister() || IsExtendedRegister());
    return reg_;
  }

  CPURegister maybeReg() const {
    if (IsShiftedRegister() || IsExtendedRegister())
      return reg_;
    return NoCPUReg;
  }

  Shift shift() const {
    VIXL_ASSERT(IsShiftedRegister());
    return shift_;
  }

  Extend extend() const {
    VIXL_ASSERT(IsExtendedRegister());
    return extend_;
  }

  unsigned shift_amount() const {
    VIXL_ASSERT(IsShiftedRegister() || IsExtendedRegister());
    return shift_amount_;
  }

 private:
  int64_t immediate_;
  Register reg_;
  Shift shift_;
  Extend extend_;
  unsigned shift_amount_;
};



class MemOperand {
 public:
  explicit MemOperand(Register base,
                      ptrdiff_t offset = 0,
                      AddrMode addrmode = Offset);
  explicit MemOperand(Register base,
                      Register regoffset,
                      Shift shift = LSL,
                      unsigned shift_amount = 0);
  explicit MemOperand(Register base,
                      Register regoffset,
                      Extend extend,
                      unsigned shift_amount = 0);
  explicit MemOperand(Register base,
                      const Operand& offset,
                      AddrMode addrmode = Offset);

  
  explicit MemOperand(js::jit::Address addr)
    : MemOperand(Register(addr.base, 64), (ptrdiff_t)addr.offset) {
  }

  const Register& base() const {
    return base_;
  }
  const Register& regoffset() const {
    return regoffset_;
  }
  ptrdiff_t offset() const {
    return offset_;
  }
  AddrMode addrmode() const {
    return addrmode_;
  }
  Shift shift() const {
    return shift_;
  }
  Extend extend() const {
    return extend_;
  }
  unsigned shift_amount() const {
    return shift_amount_;
  }
  bool IsImmediateOffset() const;
  bool IsRegisterOffset() const;
  bool IsPreIndex() const;
  bool IsPostIndex() const;

 private:
  Register base_;
  Register regoffset_;
  ptrdiff_t offset_;
  AddrMode addrmode_;
  Shift shift_;
  Extend extend_;
  unsigned shift_amount_;
};


enum PositionIndependentCodeOption {
  
  
  
  PositionIndependentCode,

  
  
  
  PositionDependentCode,

  
  
  
  
  PageOffsetDependentCode
};


enum LoadStoreScalingOption {
  
  
  PreferScaledOffset,

  
  
  PreferUnscaledOffset,

  
  RequireScaledOffset,

  
  RequireUnscaledOffset
};


class Assembler : public MozBaseAssembler {
 public:
  Assembler()
    : pc_(nullptr) { 
#ifdef DEBUG
    finalized_ = false;
#endif
  }

  
  
  
  
  ~Assembler() {
    
    
  }

  

  
  
  
  
  
  void Reset();

  
  
  void FinalizeCode();

  
  
  
  static const int DoubleConditionBitSpecial = 0x100;

  enum DoubleCondition {
    DoubleOrdered                        = Condition::vc,
    DoubleEqual                          = Condition::eq,
    DoubleNotEqual                       = Condition::ne | DoubleConditionBitSpecial,
    DoubleGreaterThan                    = Condition::gt,
    DoubleGreaterThanOrEqual             = Condition::ge,
    DoubleLessThan                       = Condition::lo, 
    DoubleLessThanOrEqual                = Condition::ls,

    
    DoubleUnordered                      = Condition::vs,
    DoubleEqualOrUnordered               = Condition::eq | DoubleConditionBitSpecial,
    DoubleNotEqualOrUnordered            = Condition::ne,
    DoubleGreaterThanOrUnordered         = Condition::hi,
    DoubleGreaterThanOrEqualOrUnordered  = Condition::hs,
    DoubleLessThanOrUnordered            = Condition::lt,
    DoubleLessThanOrEqualOrUnordered     = Condition::le
  };

  static inline Condition InvertCondition(Condition cond) {
    
    
    VIXL_ASSERT((cond != al) && (cond != nv));
    return static_cast<Condition>(cond ^ 1);
  }
  
  static inline Condition InvertCmpCondition(Condition cond) {
    
    
    switch (cond) {
    case eq:
    case ne:
      return cond;
    case gt:
      return le;
    case le:
      return gt;
    case ge:
      return lt;
    case lt:
      return ge;
    case hi:
      return lo;
    case lo:
      return hi;
    case hs:
      return ls;
    case ls:
      return hs;
    case mi:
      return pl;
    case pl:
      return mi;
    default:
      MOZ_CRASH("TODO: figure this case out.");
    }
    return static_cast<Condition>(cond ^ 1);
  }

  static inline Condition ConditionFromDoubleCondition(DoubleCondition cond) {
    VIXL_ASSERT(!(cond & DoubleConditionBitSpecial));
    return static_cast<Condition>(cond);
  }

  

  
  

  void br(const Register& xn);
  static void br(Instruction* at, const Register& xn);

  
  void blr(const Register& xn);
  static void blr(Instruction* at, const Register& xn);
  
  void ret(const Register& xn = lr);

  
  BufferOffset b(Label* label);

  
  BufferOffset b(Label* label, Condition cond);

  
  BufferOffset b(int imm26);
  static void b(Instruction* at, int imm26);

  
  BufferOffset b(int imm19, Condition cond);
  static void b(Instruction* at, int imm19, Condition cond);

  
  void bl(Label* label);

  
  void bl(int imm26);
  static void bl(Instruction* at, int imm26);

  
  void cbz(const Register& rt, Label* label);

  
  void cbz(const Register& rt, int imm19);
  static void cbz(Instruction* at, const Register& rt, int imm19);

  
  void cbnz(const Register& rt, Label* label);

  
  void cbnz(const Register& rt, int imm19);
  static void cbnz(Instruction* at, const Register& rt, int imm19);

  
  void tbz(const Register& rt, unsigned bit_pos, Label* label);

  
  void tbz(const Register& rt, unsigned bit_pos, int imm14);
  static void tbz(Instruction* at, const Register& rt, unsigned bit_pos, int imm14);

  
  void tbnz(const Register& rt, unsigned bit_pos, Label* label);

  
  void tbnz(const Register& rt, unsigned bit_pos, int imm14);
  static void tbnz(Instruction* at, const Register& rt, unsigned bit_pos, int imm14);

  
  
  

  
  void adr(const Register& rd, Label* label);

  
  void adr(const Register& rd, int imm21);
  static void adr(Instruction* at, const Register& rd, int imm21);

  
  void adrp(const Register& rd, Label* label);

  
  void adrp(const Register& rd, int imm21);
  static void adrp(Instruction* at, const Register& rd, int imm21);

  
  
  void add(const Register& rd, const Register& rn, const Operand& operand);

  
  void adds(const Register& rd, const Register& rn, const Operand& operand);

  
  void cmn(const Register& rn, const Operand& operand);

  
  void sub(const Register& rd, const Register& rn, const Operand& operand);

  
  void subs(const Register& rd, const Register& rn, const Operand& operand);

  
  void cmp(const Register& rn, const Operand& operand);

  
  void neg(const Register& rd, const Operand& operand);

  
  void negs(const Register& rd, const Operand& operand);

  
  void adc(const Register& rd, const Register& rn, const Operand& operand);

  
  void adcs(const Register& rd, const Register& rn, const Operand& operand);

  
  void sbc(const Register& rd, const Register& rn, const Operand& operand);

  
  void sbcs(const Register& rd, const Register& rn, const Operand& operand);

  
  void ngc(const Register& rd, const Operand& operand);

  
  void ngcs(const Register& rd, const Operand& operand);

  
  
  void and_(const Register& rd, const Register& rn, const Operand& operand);

  
  BufferOffset ands(const Register& rd, const Register& rn, const Operand& operand);

  
  BufferOffset tst(const Register& rn, const Operand& operand);

  
  void bic(const Register& rd, const Register& rn, const Operand& operand);

  
  void bics(const Register& rd, const Register& rn, const Operand& operand);

  
  void orr(const Register& rd, const Register& rn, const Operand& operand);

  
  void orn(const Register& rd, const Register& rn, const Operand& operand);

  
  void eor(const Register& rd, const Register& rn, const Operand& operand);

  
  void eon(const Register& rd, const Register& rn, const Operand& operand);

  
  void lslv(const Register& rd, const Register& rn, const Register& rm);

  
  void lsrv(const Register& rd, const Register& rn, const Register& rm);

  
  void asrv(const Register& rd, const Register& rn, const Register& rm);

  
  void rorv(const Register& rd, const Register& rn, const Register& rm);

  
  
  void bfm(const Register& rd, const Register& rn, unsigned immr, unsigned imms);

  
  void sbfm(const Register& rd, const Register& rn, unsigned immr, unsigned imms);

  
  void ubfm(const Register& rd, const Register& rn, unsigned immr, unsigned imms);

  
  
  inline void bfi(const Register& rd, const Register& rn, unsigned lsb, unsigned width) {
    VIXL_ASSERT(width >= 1);
    VIXL_ASSERT(lsb + width <= rn.size());
    bfm(rd, rn, (rd.size() - lsb) & (rd.size() - 1), width - 1);
  }

  
  inline void bfxil(const Register& rd, const Register& rn, unsigned lsb, unsigned width) {
    VIXL_ASSERT(width >= 1);
    VIXL_ASSERT(lsb + width <= rn.size());
    bfm(rd, rn, lsb, lsb + width - 1);
  }

  
  
  inline void asr(const Register& rd, const Register& rn, unsigned shift) {
    VIXL_ASSERT(shift < rd.size());
    sbfm(rd, rn, shift, rd.size() - 1);
  }

  
  inline void sbfiz(const Register& rd, const Register& rn, unsigned lsb, unsigned width) {
    VIXL_ASSERT(width >= 1);
    VIXL_ASSERT(lsb + width <= rn.size());
    sbfm(rd, rn, (rd.size() - lsb) & (rd.size() - 1), width - 1);
  }

  
  inline void sbfx(const Register& rd, const Register& rn, unsigned lsb, unsigned width) {
    VIXL_ASSERT(width >= 1);
    VIXL_ASSERT(lsb + width <= rn.size());
    sbfm(rd, rn, lsb, lsb + width - 1);
  }

  
  inline void sxtb(const Register& rd, const Register& rn) {
    sbfm(rd, rn, 0, 7);
  }

  
  inline void sxth(const Register& rd, const Register& rn) {
    sbfm(rd, rn, 0, 15);
  }

  
  inline void sxtw(const Register& rd, const Register& rn) {
    sbfm(rd, rn, 0, 31);
  }

  
  
  inline void lsl(const Register& rd, const Register& rn, unsigned shift) {
    unsigned reg_size = rd.size();
    VIXL_ASSERT(shift < reg_size);
    ubfm(rd, rn, (reg_size - shift) % reg_size, reg_size - shift - 1);
  }

  
  inline void lsr(const Register& rd, const Register& rn, unsigned shift) {
    VIXL_ASSERT(shift < rd.size());
    ubfm(rd, rn, shift, rd.size() - 1);
  }

  
  inline void ubfiz(const Register& rd, const Register& rn, unsigned lsb, unsigned width) {
    VIXL_ASSERT(width >= 1);
    VIXL_ASSERT(lsb + width <= rn.size());
    ubfm(rd, rn, (rd.size() - lsb) & (rd.size() - 1), width - 1);
  }

  
  inline void ubfx(const Register& rd, const Register& rn, unsigned lsb, unsigned width) {
    VIXL_ASSERT(width >= 1);
    VIXL_ASSERT(lsb + width <= rn.size());
    ubfm(rd, rn, lsb, lsb + width - 1);
  }

  
  inline void uxtb(const Register& rd, const Register& rn) {
    ubfm(rd, rn, 0, 7);
  }

  
  inline void uxth(const Register& rd, const Register& rn) {
    ubfm(rd, rn, 0, 15);
  }

  
  inline void uxtw(const Register& rd, const Register& rn) {
    ubfm(rd, rn, 0, 31);
  }

  
  void extr(const Register& rd, const Register& rn, const Register& rm, unsigned lsb);

  
  void csel(const Register& rd, const Register& rn, const Register& rm, Condition cond);

  
  void csinc(const Register& rd, const Register& rn, const Register& rm, Condition cond);

  
  void csinv(const Register& rd, const Register& rn, const Register& rm, Condition cond);

  
  void csneg(const Register& rd, const Register& rn, const Register& rm, Condition cond);

  
  void cset(const Register& rd, Condition cond);

  
  void csetm(const Register& rd, Condition cond);

  
  void cinc(const Register& rd, const Register& rn, Condition cond);

  
  void cinv(const Register& rd, const Register& rn, Condition cond);

  
  void cneg(const Register& rd, const Register& rn, Condition cond);

  
  inline void ror(const Register& rd, const Register& rs, unsigned shift) {
    extr(rd, rs, rs, shift);
  }

  
  
  void ccmn(const Register& rn, const Operand& operand, StatusFlags nzcv, Condition cond);

  
  void ccmp(const Register& rn, const Operand& operand, StatusFlags nzcv, Condition cond);

  
  void mul(const Register& rd, const Register& rn, const Register& rm);

  
  void mneg(const Register& rd, const Register& rn, const Register& rm);

  
  void smull(const Register& rd, const Register& rn, const Register& rm);

  
  void smulh(const Register& xd, const Register& xn, const Register& xm);

  
  void madd(const Register& rd, const Register& rn,
            const Register& rm, const Register& ra);

  
  void msub(const Register& rd, const Register& rn,
            const Register& rm, const Register& ra);

  
  void smaddl(const Register& rd, const Register& rn,
              const Register& rm, const Register& ra);

  
  void umaddl(const Register& rd, const Register& rn,
              const Register& rm, const Register& ra);

  
  void smsubl(const Register& rd, const Register& rn,
              const Register& rm, const Register& ra);

  
  void umsubl(const Register& rd, const Register& rn,
              const Register& rm, const Register& ra);

  
  void sdiv(const Register& rd, const Register& rn, const Register& rm);

  
  void udiv(const Register& rd, const Register& rn, const Register& rm);

  
  void rbit(const Register& rd, const Register& rn);

  
  void rev16(const Register& rd, const Register& rn);

  
  void rev32(const Register& rd, const Register& rn);

  
  void rev(const Register& rd, const Register& rn);

  
  void clz(const Register& rd, const Register& rn);

  
  void cls(const Register& rd, const Register& rn);

  
  
  void ldr(const CPURegister& rt, const MemOperand& src,
           LoadStoreScalingOption option = PreferScaledOffset);

  
  void str(const CPURegister& rt, const MemOperand& dst,
           LoadStoreScalingOption option = PreferScaledOffset);

  
  void ldrsw(const Register& rt, const MemOperand& src,
             LoadStoreScalingOption option = PreferScaledOffset);

  
  void ldrb(const Register& rt, const MemOperand& src,
            LoadStoreScalingOption option = PreferScaledOffset);

  
  void strb(const Register& rt, const MemOperand& dst,
            LoadStoreScalingOption option = PreferScaledOffset);

  
  void ldrsb(const Register& rt, const MemOperand& src,
             LoadStoreScalingOption option = PreferScaledOffset);

  
  void ldrh(const Register& rt, const MemOperand& src,
            LoadStoreScalingOption option = PreferScaledOffset);

  
  void strh(const Register& rt, const MemOperand& dst,
            LoadStoreScalingOption option = PreferScaledOffset);

  
  void ldrsh(const Register& rt, const MemOperand& src,
             LoadStoreScalingOption option = PreferScaledOffset);

  
  void ldur(const CPURegister& rt, const MemOperand& src,
            LoadStoreScalingOption option = PreferUnscaledOffset);

  
  void stur(const CPURegister& rt, const MemOperand& src,
            LoadStoreScalingOption option = PreferUnscaledOffset);

  
  void ldursw(const Register& rt, const MemOperand& src,
              LoadStoreScalingOption option = PreferUnscaledOffset);

  
  void ldurb(const Register& rt, const MemOperand& src,
             LoadStoreScalingOption option = PreferUnscaledOffset);

  
  void sturb(const Register& rt, const MemOperand& dst,
             LoadStoreScalingOption option = PreferUnscaledOffset);

  
  void ldursb(const Register& rt, const MemOperand& src,
              LoadStoreScalingOption option = PreferUnscaledOffset);

  
  void ldurh(const Register& rt, const MemOperand& src,
             LoadStoreScalingOption option = PreferUnscaledOffset);

  
  void sturh(const Register& rt, const MemOperand& dst,
             LoadStoreScalingOption option = PreferUnscaledOffset);

  
  void ldursh(const Register& rt, const MemOperand& src,
              LoadStoreScalingOption option = PreferUnscaledOffset);

  
  void ldp(const CPURegister& rt, const CPURegister& rt2, const MemOperand& src);

  
  void stp(const CPURegister& rt, const CPURegister& rt2, const MemOperand& dst);

  
  void ldpsw(const Register& rt, const Register& rt2, const MemOperand& src);

  
  void ldnp(const CPURegister& rt, const CPURegister& rt2, const MemOperand& src);

  
  void stnp(const CPURegister& rt, const CPURegister& rt2, const MemOperand& dst);

  
  void ldr(const CPURegister& rt, int imm19);
  static void ldr(Instruction* at, const CPURegister& rt, int imm19);

  
  void ldrsw(const Register& rt, int imm19);

  
  void stxrb(const Register& rs, const Register& rt, const MemOperand& dst);

  
  void stxrh(const Register& rs, const Register& rt, const MemOperand& dst);

  
  void stxr(const Register& rs, const Register& rt, const MemOperand& dst);

  
  void ldxrb(const Register& rt, const MemOperand& src);

  
  void ldxrh(const Register& rt, const MemOperand& src);

  
  void ldxr(const Register& rt, const MemOperand& src);

  
  void stxp(const Register& rs, const Register& rt,
            const Register& rt2, const MemOperand& dst);

  
  void ldxp(const Register& rt, const Register& rt2, const MemOperand& src);

  
  void stlxrb(const Register& rs, const Register& rt, const MemOperand& dst);

  
  void stlxrh(const Register& rs, const Register& rt, const MemOperand& dst);

  
  void stlxr(const Register& rs, const Register& rt, const MemOperand& dst);

  
  void ldaxrb(const Register& rt, const MemOperand& src);

  
  void ldaxrh(const Register& rt, const MemOperand& src);

  
  void ldaxr(const Register& rt, const MemOperand& src);

  
  void stlxp(const Register& rs, const Register& rt,
             const Register& rt2, const MemOperand& dst);

  
  void ldaxp(const Register& rt, const Register& rt2, const MemOperand& src);

  
  void stlrb(const Register& rt, const MemOperand& dst);

  
  void stlrh(const Register& rt, const MemOperand& dst);

  
  void stlr(const Register& rt, const MemOperand& dst);

  
  void ldarb(const Register& rt, const MemOperand& src);

  
  void ldarh(const Register& rt, const MemOperand& src);

  
  void ldar(const Register& rt, const MemOperand& src);

  
  
  
  
  
  
  
  
  

  
  void movk(const Register& rd, uint64_t imm, int shift = -1) {
    MoveWide(rd, imm, shift, MOVK);
  }

  
  void movn(const Register& rd, uint64_t imm, int shift = -1) {
    MoveWide(rd, imm, shift, MOVN);
  }

  
  void movz(const Register& rd, uint64_t imm, int shift = -1) {
    MoveWide(rd, imm, shift, MOVZ);
  }

  
  
  void brk(int code);

  
  void svc(int code);
  static void svc(Instruction* at, int code);

  
  void hlt(int code);

  
  void mov(const Register& rd, const Register& rn);

  
  void mvn(const Register& rd, const Operand& operand);

  
  
  void mrs(const Register& rt, SystemRegister sysreg);

  
  void msr(SystemRegister sysreg, const Register& rt);

  
  BufferOffset hint(SystemHint code);
  static void hint(Instruction* at, SystemHint code);
  
  void clrex(int imm4 = 0xf);

  
  void dmb(BarrierDomain domain, BarrierType type);

  
  void dsb(BarrierDomain domain, BarrierType type);

  
  void isb();

  
  
  BufferOffset nop() {
    return hint(NOP);
  }
  static void nop(Instruction* at);
  
  
  void fmov(const FPRegister& fd, double imm);

  
  void fmov(const FPRegister& fd, float imm);

  
  void fmov(const Register& rd, const FPRegister& fn);

  
  void fmov(const FPRegister& fd, const Register& rn);

  
  void fmov(const FPRegister& fd, const FPRegister& fn);

  
  void fadd(const FPRegister& fd, const FPRegister& fn, const FPRegister& fm);

  
  void fsub(const FPRegister& fd, const FPRegister& fn, const FPRegister& fm);

  
  void fmul(const FPRegister& fd, const FPRegister& fn, const FPRegister& fm);

  
  void fmadd(const FPRegister& fd, const FPRegister& fn,
             const FPRegister& fm, const FPRegister& fa);

  
  void fmsub(const FPRegister& fd, const FPRegister& fn,
             const FPRegister& fm, const FPRegister& fa);

  
  void fnmadd(const FPRegister& fd, const FPRegister& fn,
              const FPRegister& fm, const FPRegister& fa);

  
  void fnmsub(const FPRegister& fd, const FPRegister& fn,
              const FPRegister& fm, const FPRegister& fa);

  
  void fdiv(const FPRegister& fd, const FPRegister& fn, const FPRegister& fm);

  
  void fmax(const FPRegister& fd, const FPRegister& fn, const FPRegister& fm);

  
  void fmin(const FPRegister& fd, const FPRegister& fn, const FPRegister& fm);

  
  void fmaxnm(const FPRegister& fd, const FPRegister& fn, const FPRegister& fm);

  
  void fminnm(const FPRegister& fd, const FPRegister& fn, const FPRegister& fm);

  
  void fabs(const FPRegister& fd, const FPRegister& fn);

  
  void fneg(const FPRegister& fd, const FPRegister& fn);

  
  void fsqrt(const FPRegister& fd, const FPRegister& fn);

  
  void frinta(const FPRegister& fd, const FPRegister& fn);

  
  void frintm(const FPRegister& fd, const FPRegister& fn);

  
  void frintn(const FPRegister& fd, const FPRegister& fn);

  
  void frintz(const FPRegister& fd, const FPRegister& fn);

  
  void fcmp(const FPRegister& fn, const FPRegister& fm);

  
  void fcmp(const FPRegister& fn, double value);

  
  void fccmp(const FPRegister& fn, const FPRegister& fm, StatusFlags nzcv, Condition cond);

  
  void fcsel(const FPRegister& fd, const FPRegister& fn,
             const FPRegister& fm, Condition cond);

  
  void FPConvertToInt(const Register& rd, const FPRegister& fn, FPIntegerConvertOp op);

  
  void fcvt(const FPRegister& fd, const FPRegister& fn);

  
  void fcvtas(const Register& rd, const FPRegister& fn);

  
  void fcvtau(const Register& rd, const FPRegister& fn);

  
  void fcvtms(const Register& rd, const FPRegister& fn);

  
  void fcvtmu(const Register& rd, const FPRegister& fn);

  
  void fcvtps(const Register& rd, const FPRegister& fn);

  
  void fcvtpu(const Register& rd, const FPRegister& fn);

  
  void fcvtns(const Register& rd, const FPRegister& fn);

  
  void fcvtnu(const Register& rd, const FPRegister& fn);

  
  void fcvtzs(const Register& rd, const FPRegister& fn);

  
  void fcvtzu(const Register& rd, const FPRegister& fn);

  
  void scvtf(const FPRegister& fd, const Register& rn, unsigned fbits = 0);

  
  void ucvtf(const FPRegister& fd, const Register& rn, unsigned fbits = 0);

  
  
  inline void dci(Instr raw_inst) {
    Emit(raw_inst);
  }

  
  inline void dc32(uint32_t data) {
    EmitData(&data, sizeof(data));
  }

  
  inline void dc64(uint64_t data) {
    EmitData(&data, sizeof(data));
  }

  
  
  
  void EmitStringData(const char * string) {
    VIXL_ASSERT(string != NULL);

    size_t len = strlen(string) + 1;
    EmitData(string, len);

    
    const char pad[] = {'\0', '\0', '\0', '\0'};
    JS_STATIC_ASSERT(sizeof(pad) == kInstructionSize);
    Instruction* next_pc = AlignUp(pc_, kInstructionSize);
    EmitData(&pad, next_pc - pc_);
  }

  

  
  static Instr Rd(CPURegister rd) {
    VIXL_ASSERT(rd.code() != kSPRegInternalCode);
    return rd.code() << Rd_offset;
  }

  static Instr Rn(CPURegister rn) {
    VIXL_ASSERT(rn.code() != kSPRegInternalCode);
    return rn.code() << Rn_offset;
  }

  static Instr Rm(CPURegister rm) {
    VIXL_ASSERT(rm.code() != kSPRegInternalCode);
    return rm.code() << Rm_offset;
  }

  static Instr Ra(CPURegister ra) {
    VIXL_ASSERT(ra.code() != kSPRegInternalCode);
    return ra.code() << Ra_offset;
  }

  static Instr Rt(CPURegister rt) {
    VIXL_ASSERT(rt.code() != kSPRegInternalCode);
    return rt.code() << Rt_offset;
  }

  static Instr Rt2(CPURegister rt2) {
    VIXL_ASSERT(rt2.code() != kSPRegInternalCode);
    return rt2.code() << Rt2_offset;
  }

  static Instr Rs(CPURegister rs) {
    VIXL_ASSERT(rs.code() != kSPRegInternalCode);
    return rs.code() << Rs_offset;
  }

  
  
  static Instr RdSP(Register rd) {
    VIXL_ASSERT(!rd.IsZero());
    return (rd.code() & kRegCodeMask) << Rd_offset;
  }

  static Instr RnSP(Register rn) {
    VIXL_ASSERT(!rn.IsZero());
    return (rn.code() & kRegCodeMask) << Rn_offset;
  }

  
  static Instr Flags(FlagsUpdate S) {
    if (S == SetFlags)
      return 1 << FlagsUpdate_offset;

    if (S == LeaveFlags)
      return 0 << FlagsUpdate_offset;

    VIXL_UNREACHABLE();
    return 0;
  }

  static Instr Cond(Condition cond) {
    return cond << Condition_offset;
  }

  
  static Instr ImmPCRelAddress(int imm21) {
    VIXL_ASSERT(is_int21(imm21));
    Instr imm = static_cast<Instr>(truncate_to_int21(imm21));
    Instr immhi = (imm >> ImmPCRelLo_width) << ImmPCRelHi_offset;
    Instr immlo = imm << ImmPCRelLo_offset;
    return (immhi & ImmPCRelHi_mask) | (immlo & ImmPCRelLo_mask);
  }

  
  static Instr ImmUncondBranch(int imm26) {
    VIXL_ASSERT(is_int26(imm26));
    return truncate_to_int26(imm26) << ImmUncondBranch_offset;
  }

  static Instr ImmCondBranch(int imm19) {
    VIXL_ASSERT(is_int19(imm19));
    return truncate_to_int19(imm19) << ImmCondBranch_offset;
  }

  static Instr ImmCmpBranch(int imm19) {
    VIXL_ASSERT(is_int19(imm19));
    return truncate_to_int19(imm19) << ImmCmpBranch_offset;
  }

  static Instr ImmTestBranch(int imm14) {
    VIXL_ASSERT(is_int14(imm14));
    return truncate_to_int14(imm14) << ImmTestBranch_offset;
  }

  static Instr ImmTestBranchBit(unsigned bit_pos) {
    VIXL_ASSERT(is_uint6(bit_pos));
    
    unsigned b5 = bit_pos << (ImmTestBranchBit5_offset - 5);
    unsigned b40 = bit_pos << ImmTestBranchBit40_offset;
    b5 &= ImmTestBranchBit5_mask;
    b40 &= ImmTestBranchBit40_mask;
    return b5 | b40;
  }

  
  static Instr SF(Register rd) {
    return rd.Is64Bits() ? SixtyFourBits : ThirtyTwoBits;
  }

  static Instr ImmAddSub(int64_t imm) {
    VIXL_ASSERT(IsImmAddSub(imm));
    if (is_uint12(imm)) 
      return imm << ImmAddSub_offset;
    return ((imm >> 12) << ImmAddSub_offset) | (1 << ShiftAddSub_offset);
  }

  static inline Instr ImmS(unsigned imms, unsigned reg_size) {
    VIXL_ASSERT(((reg_size == kXRegSize) && is_uint6(imms)) ||
               ((reg_size == kWRegSize) && is_uint5(imms)));
    USE(reg_size);
    return imms << ImmS_offset;
  }

  static inline Instr ImmR(unsigned immr, unsigned reg_size) {
    VIXL_ASSERT(((reg_size == kXRegSize) && is_uint6(immr)) ||
               ((reg_size == kWRegSize) && is_uint5(immr)));
    USE(reg_size);
    VIXL_ASSERT(is_uint6(immr));
    return immr << ImmR_offset;
  }

  static inline Instr ImmSetBits(unsigned imms, unsigned reg_size) {
    VIXL_ASSERT((reg_size == kWRegSize) || (reg_size == kXRegSize));
    VIXL_ASSERT(is_uint6(imms));
    VIXL_ASSERT((reg_size == kXRegSize) || is_uint6(imms + 3));
    USE(reg_size);
    return imms << ImmSetBits_offset;
  }

  static inline Instr ImmRotate(unsigned immr, unsigned reg_size) {
    VIXL_ASSERT((reg_size == kWRegSize) || (reg_size == kXRegSize));
    VIXL_ASSERT(((reg_size == kXRegSize) && is_uint6(immr)) ||
               ((reg_size == kWRegSize) && is_uint5(immr)));
    USE(reg_size);
    return immr << ImmRotate_offset;
  }

  static inline Instr ImmLLiteral(int imm19) {
    VIXL_ASSERT(is_int19(imm19));
    return truncate_to_int19(imm19) << ImmLLiteral_offset;
  }

  static inline Instr BitN(unsigned bitn, unsigned reg_size) {
    VIXL_ASSERT((reg_size == kWRegSize) || (reg_size == kXRegSize));
    VIXL_ASSERT((reg_size == kXRegSize) || (bitn == 0));
    USE(reg_size);
    return bitn << BitN_offset;
  }

  static Instr ShiftDP(Shift shift) {
    VIXL_ASSERT(shift == LSL || shift == LSR || shift == ASR || shift == ROR);
    return shift << ShiftDP_offset;
  }

  static Instr ImmDPShift(unsigned amount) {
    VIXL_ASSERT(is_uint6(amount));
    return amount << ImmDPShift_offset;
  }

  static Instr ExtendMode(Extend extend) {
    return extend << ExtendMode_offset;
  }

  static Instr ImmExtendShift(unsigned left_shift) {
    VIXL_ASSERT(left_shift <= 4);
    return left_shift << ImmExtendShift_offset;
  }

  static Instr ImmCondCmp(unsigned imm) {
    VIXL_ASSERT(is_uint5(imm));
    return imm << ImmCondCmp_offset;
  }

  static Instr Nzcv(StatusFlags nzcv) {
    return ((nzcv >> Flags_offset) & 0xf) << Nzcv_offset;
  }

  
  static Instr ImmLSUnsigned(int imm12) {
    VIXL_ASSERT(is_uint12(imm12));
    return imm12 << ImmLSUnsigned_offset;
  }

  static Instr ImmLS(int imm9) {
    VIXL_ASSERT(is_int9(imm9));
    return truncate_to_int9(imm9) << ImmLS_offset;
  }

  static Instr ImmLSPair(int imm7, LSDataSize size) {
    VIXL_ASSERT(((imm7 >> size) << size) == imm7);
    int scaled_imm7 = imm7 >> size;
    VIXL_ASSERT(is_int7(scaled_imm7));
    return truncate_to_int7(scaled_imm7) << ImmLSPair_offset;
  }

  static Instr ImmShiftLS(unsigned shift_amount) {
    VIXL_ASSERT(is_uint1(shift_amount));
    return shift_amount << ImmShiftLS_offset;
  }

  static Instr ImmException(int imm16) {
    VIXL_ASSERT(is_uint16(imm16));
    return imm16 << ImmException_offset;
  }

  static Instr ImmSystemRegister(int imm15) {
    VIXL_ASSERT(is_uint15(imm15));
    return imm15 << ImmSystemRegister_offset;
  }

  static Instr ImmHint(int imm7) {
    VIXL_ASSERT(is_uint7(imm7));
    return imm7 << ImmHint_offset;
  }

  static Instr CRm(int imm4) {
    VIXL_ASSERT(is_uint4(imm4));
    return imm4 << CRm_offset;
  }

  static Instr ImmBarrierDomain(int imm2) {
    VIXL_ASSERT(is_uint2(imm2));
    return imm2 << ImmBarrierDomain_offset;
  }

  static Instr ImmBarrierType(int imm2) {
    VIXL_ASSERT(is_uint2(imm2));
    return imm2 << ImmBarrierType_offset;
  }

  static LSDataSize CalcLSDataSize(LoadStoreOp op) {
    VIXL_ASSERT((SizeLS_offset + SizeLS_width) == (kInstructionSize * 8));
    return static_cast<LSDataSize>(op >> SizeLS_offset);
  }

  
  static Instr ImmMoveWide(uint64_t imm) {
    VIXL_ASSERT(is_uint16(imm));
    return imm << ImmMoveWide_offset;
  }

  static Instr ShiftMoveWide(int64_t shift) {
    VIXL_ASSERT(is_uint2(shift));
    return shift << ShiftMoveWide_offset;
  }

  
  static Instr ImmFP32(float imm);
  static Instr ImmFP64(double imm);

  
  static Instr FPType(FPRegister fd) {
    return fd.Is64Bits() ? FP64 : FP32;
  }

  static Instr FPScale(unsigned scale) {
    VIXL_ASSERT(is_uint6(scale));
    return scale << FPScale_offset;
  }

  size_t size() const {
    return SizeOfCodeGenerated();
  }

  
  size_t SizeOfCodeGenerated() const {
    return armbuffer_.size();
  }

  inline PositionIndependentCodeOption pic() {
    return pic_;
  }

  inline bool AllowPageOffsetDependentCode() {
    return (pic() == PageOffsetDependentCode) ||
           (pic() == PositionDependentCode);
  }

 protected:
  inline const Register& AppropriateZeroRegFor(const CPURegister& reg) const {
    return reg.Is64Bits() ? xzr : wzr;
  }

  BufferOffset LoadStore(const CPURegister& rt, const MemOperand& addr, LoadStoreOp op,
                         LoadStoreScalingOption option = PreferScaledOffset);

  static bool IsImmLSUnscaled(ptrdiff_t offset);
  static bool IsImmLSScaled(ptrdiff_t offset, LSDataSize size);

  BufferOffset Logical(const Register& rd, const Register& rn,
                       const Operand& operand, LogicalOp op);
  BufferOffset LogicalImmediate(const Register& rd, const Register& rn, unsigned n,
                                unsigned imm_s, unsigned imm_r, LogicalOp op);
  static bool IsImmLogical(uint64_t value, unsigned width, unsigned* n = nullptr,
                           unsigned* imm_s = nullptr, unsigned* imm_r = nullptr);

  void ConditionalCompare(const Register& rn, const Operand& operand, StatusFlags nzcv,
                          Condition cond, ConditionalCompareOp op);
  static bool IsImmConditionalCompare(int64_t immediate);

  void AddSubWithCarry(const Register& rd, const Register& rn, const Operand& operand,
                       FlagsUpdate S, AddSubWithCarryOp op);

  static bool IsImmFP32(float imm);
  static bool IsImmFP64(double imm);

  
  
  void EmitShift(const Register& rd, const Register& rn, Shift shift, unsigned amount);
  void EmitExtendShift(const Register& rd, const Register& rn,
                       Extend extend, unsigned left_shift);

  void AddSub(const Register& rd, const Register& rn, const Operand& operand,
              FlagsUpdate S, AddSubOp op);
  static bool IsImmAddSub(int64_t immediate);

  
  
  
  static LoadStoreOp LoadOpFor(const CPURegister& rt);
  static LoadStorePairOp LoadPairOpFor(const CPURegister& rt, const CPURegister& rt2);
  static LoadStoreOp StoreOpFor(const CPURegister& rt);
  static LoadStorePairOp StorePairOpFor(const CPURegister& rt, const CPURegister& rt2);

  static LoadStorePairNonTemporalOp LoadPairNonTemporalOpFor(
    const CPURegister& rt, const CPURegister& rt2);

  static LoadStorePairNonTemporalOp StorePairNonTemporalOpFor(
    const CPURegister& rt, const CPURegister& rt2);

  static LoadLiteralOp LoadLiteralOpFor(const CPURegister& rt);


 private:
  
  void MoveWide(const Register& rd, uint64_t imm, int shift, MoveWideImmediateOp mov_op);
  BufferOffset DataProcShiftedRegister(const Register& rd, const Register& rn,
                                       const Operand& operand, FlagsUpdate S, Instr op);
  void DataProcExtendedRegister(const Register& rd, const Register& rn,
                                const Operand& operand, FlagsUpdate S, Instr op);
  void LoadStorePair(const CPURegister& rt, const CPURegister& rt2,
                     const MemOperand& addr, LoadStorePairOp op);
  void LoadStorePairNonTemporal(const CPURegister& rt, const CPURegister& rt2,
                                const MemOperand& addr, LoadStorePairNonTemporalOp op);
  void LoadLiteral(const CPURegister& rt, uint64_t imm, LoadLiteralOp op);
  void LoadPCLiteral(const CPURegister& rt, ptrdiff_t PCInsOffset, LoadLiteralOp op);
  void ConditionalSelect(const Register& rd, const Register& rn,
                         const Register& rm, Condition cond, ConditionalSelectOp op);
  void DataProcessing1Source(const Register& rd, const Register& rn,
                             DataProcessing1SourceOp op);
  void DataProcessing3Source(const Register& rd, const Register& rn,
                             const Register& rm, const Register& ra,
                             DataProcessing3SourceOp op);
  void FPDataProcessing1Source(const FPRegister& fd, const FPRegister& fn,
                               FPDataProcessing1SourceOp op);
  void FPDataProcessing2Source(const FPRegister& fd, const FPRegister& fn,
                               const FPRegister& fm, FPDataProcessing2SourceOp op);
  void FPDataProcessing3Source(const FPRegister& fd, const FPRegister& fn,
                               const FPRegister& fm, const FPRegister& fa,
                               FPDataProcessing3SourceOp op);

  void RecordLiteral(int64_t imm, unsigned size);

  
  
  
  ptrdiff_t LinkAndGetByteOffsetTo(BufferOffset branch, Label* label);
  ptrdiff_t LinkAndGetInstructionOffsetTo(BufferOffset branch, Label* label);
  ptrdiff_t LinkAndGetPageOffsetTo(BufferOffset branch, Label* label);

  
  template <int element_size>
  ptrdiff_t LinkAndGetOffsetTo(BufferOffset branch, Label* label);

 private:
  inline void CheckBufferSpace() {
    VIXL_ASSERT(!armbuffer_.oom());
  }

 protected:
  
  
  class AutoBlockLiteralPool {
    ARMBuffer* armbuffer_;

   public:
    AutoBlockLiteralPool(Assembler* assembler, size_t maxInst)
      : armbuffer_(&assembler->armbuffer_) {
      armbuffer_->enterNoPool(maxInst);
    }
    ~AutoBlockLiteralPool() {
      armbuffer_->leaveNoPool();
    }
  };

 protected:
  PositionIndependentCodeOption pic_;

  
  Instruction* pc_;

#ifdef DEBUG
  bool finalized_;
#endif
};

} 

#endif  
