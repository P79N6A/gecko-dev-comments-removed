

























#ifndef VIXL_A64_SIMULATOR_A64_H_
#define VIXL_A64_SIMULATOR_A64_H_

#include "mozilla/Vector.h"

#include "jsalloc.h"

#include "jit/arm64/vixl/Assembler-vixl.h"
#include "jit/arm64/vixl/Disasm-vixl.h"
#include "jit/arm64/vixl/Globals-vixl.h"
#include "jit/arm64/vixl/Instructions-vixl.h"
#include "jit/arm64/vixl/Instrument-vixl.h"
#include "jit/arm64/vixl/Utils-vixl.h"
#include "jit/IonTypes.h"
#include "vm/PosixNSPR.h"

#define JS_CHECK_SIMULATOR_RECURSION_WITH_EXTRA(cx, extra, onerror)             \
    JS_BEGIN_MACRO                                                              \
        if (cx->mainThread().simulator()->overRecursedWithExtra(extra)) {       \
            js::ReportOverRecursed(cx);                                         \
            onerror;                                                            \
        }                                                                       \
    JS_END_MACRO

namespace vixl {


























const Instr kUnreachableOpcode = 0xdeb0;


















const Instr kPrintfOpcode = 0xdeb1;
const unsigned kPrintfArgCountOffset = 1 * kInstructionSize;
const unsigned kPrintfArgPatternListOffset = 2 * kInstructionSize;
const unsigned kPrintfLength = 3 * kInstructionSize;

const unsigned kPrintfMaxArgCount = 4;



enum PrintfArgPattern {
  kPrintfArgW = 1,
  kPrintfArgX = 2,
  
  
  kPrintfArgD = 3
};
static const unsigned kPrintfArgPatternBits = 2;








const Instr kTraceOpcode = 0xdeb2;
const unsigned kTraceParamsOffset = 1 * kInstructionSize;
const unsigned kTraceCommandOffset = 2 * kInstructionSize;
const unsigned kTraceLength = 3 * kInstructionSize;


enum TraceParameters {
  LOG_DISASM     = 1 << 0,  
  LOG_REGS       = 1 << 1,  
  LOG_FP_REGS    = 1 << 2,  
  LOG_SYS_REGS   = 1 << 3,  
  LOG_WRITE      = 1 << 4,  

  LOG_NONE       = 0,
  LOG_STATE      = LOG_REGS | LOG_FP_REGS | LOG_SYS_REGS,
  LOG_ALL        = LOG_DISASM | LOG_STATE | LOG_WRITE
};


enum TraceCommand {
  TRACE_ENABLE   = 1,
  TRACE_DISABLE  = 2
};










const Instr kLogOpcode = 0xdeb3;
const unsigned kLogParamsOffset = 1 * kInstructionSize;
const unsigned kLogLength = 2 * kInstructionSize;




class SimSystemRegister {
 public:
  
  
  SimSystemRegister() : value_(0), write_ignore_mask_(0xffffffff) { }

  uint32_t RawValue() const {
    return value_;
  }

  void SetRawValue(uint32_t new_value) {
    value_ = (value_ & write_ignore_mask_) | (new_value & ~write_ignore_mask_);
  }

  uint32_t Bits(int msb, int lsb) const {
    return unsigned_bitextract_32(msb, lsb, value_);
  }

  int32_t SignedBits(int msb, int lsb) const {
    return signed_bitextract_32(msb, lsb, value_);
  }

  void SetBits(int msb, int lsb, uint32_t bits);

  
  static SimSystemRegister DefaultValueFor(SystemRegister id);

#define DEFINE_GETTER(Name, HighBit, LowBit, Func)                            \
  uint32_t Name() const { return Func(HighBit, LowBit); }              \
  void Set##Name(uint32_t bits) { SetBits(HighBit, LowBit, bits); }
#define DEFINE_WRITE_IGNORE_MASK(Name, Mask)                                  \
  static const uint32_t Name##WriteIgnoreMask = ~static_cast<uint32_t>(Mask);

  SYSTEM_REGISTER_FIELDS_LIST(DEFINE_GETTER, DEFINE_WRITE_IGNORE_MASK)

#undef DEFINE_ZERO_BITS
#undef DEFINE_GETTER

 protected:
  
  
  
  SimSystemRegister(uint32_t value, uint32_t write_ignore_mask)
      : value_(value), write_ignore_mask_(write_ignore_mask) { }

  uint32_t value_;
  uint32_t write_ignore_mask_;
};



template<int kSizeInBytes>
class SimRegisterBase {
 public:
  
  template<typename T>
  void Set(T new_value) {
    VIXL_STATIC_ASSERT(sizeof(new_value) <= kSizeInBytes);
    if (sizeof(new_value) < kSizeInBytes) {
      
      memset(value_ + sizeof(new_value), 0, kSizeInBytes - sizeof(new_value));
    }
    memcpy(value_, &new_value, sizeof(new_value));
  }

  
  template<typename T>
  T Get() const {
    T result;
    VIXL_STATIC_ASSERT(sizeof(result) <= kSizeInBytes);
    memcpy(&result, value_, sizeof(T));
    return result;
  }

 protected:
  uint8_t value_[kSizeInBytes];
};
typedef SimRegisterBase<kXRegSizeInBytes> SimRegister;      
typedef SimRegisterBase<kDRegSizeInBytes> SimFPRegister;    


class SimExclusiveLocalMonitor {
 public:
  SimExclusiveLocalMonitor() : kSkipClearProbability(8), seed_(0x87654321) {
    Clear();
  }

  
  void Clear() {
    address_ = 0;
    size_ = 0;
  }

  
  void MaybeClear() {
    if ((seed_ % kSkipClearProbability) != 0) {
      Clear();
    }

    
    seed_ = (seed_ * 48271) % 2147483647;
  }

  
  void MarkExclusive(uint64_t address, size_t size) {
    address_ = address;
    size_ = size;
  }

  
  
  bool IsExclusive(uint64_t address, size_t size) {
    VIXL_ASSERT(size > 0);
    
    return (size == size_) && (address == address_);
  }

 private:
  uint64_t address_;
  size_t size_;

  const int kSkipClearProbability;
  uint32_t seed_;
};





class SimExclusiveGlobalMonitor {
 public:
  SimExclusiveGlobalMonitor() : kPassProbability(8), seed_(0x87654321) {}

  bool IsExclusive(uint64_t address, size_t size) {
    USE(address);
    USE(size);

    bool pass = (seed_ % kPassProbability) != 0;
    
    seed_ = (seed_ * 48271) % 2147483647;
    return pass;
  }

 private:
  const int kPassProbability;
  uint32_t seed_;
};

class Redirection;

class Simulator : public DecoderVisitor {
  friend class AutoLockSimulatorCache;

 public:
  explicit Simulator(Decoder* decoder, FILE* stream = stdout);
  ~Simulator();

  
  explicit Simulator();
  void init(Decoder* decoder, FILE* stream);
  static Simulator* Current();
  static Simulator* Create();
  static void Destroy(Simulator* sim);
  uintptr_t stackLimit() const;
  uintptr_t* addressOfStackLimit();
  bool overRecursed(uintptr_t newsp = 0) const;
  bool overRecursedWithExtra(uint32_t extra) const;
  int64_t call(uint8_t* entry, int argument_count, ...);
  void setRedirection(Redirection* redirection);
  Redirection* redirection() const;
  static void* RedirectNativeFunction(void* nativeFunction, js::jit::ABIFunctionType type);
  void setGPR32Result(int32_t result);
  void setGPR64Result(int64_t result);
  void setFP32Result(float result);
  void setFP64Result(double result);
  void VisitCallRedirection(const Instruction* instr);

  void ResetState();

  
  virtual void Run();
  void RunFrom(const Instruction* first);

  
  const Instruction* pc() const { return pc_; }
  void set_pc(const Instruction* new_pc) {
    pc_ = AddressUntag(new_pc);
    pc_modified_ = true;
  }
  void set_resume_pc(const Instruction* new_resume_pc);

  void increment_pc() {
    if (!pc_modified_) {
      pc_ = pc_->NextInstruction();
    }

    pc_modified_ = false;
  }

  void ExecuteInstruction();

  
  #define DECLARE(A) virtual void Visit##A(const Instruction* instr);
  VISITOR_LIST(DECLARE)
  #undef DECLARE

  

  
  template<typename T>
  T reg(unsigned code, Reg31Mode r31mode = Reg31IsZeroRegister) const {
    VIXL_ASSERT(code < kNumberOfRegisters);
    if ((code == 31) && (r31mode == Reg31IsZeroRegister)) {
      T result;
      memset(&result, 0, sizeof(result));
      return result;
    }
    return registers_[code].Get<T>();
  }

  
  int32_t wreg(unsigned code,
               Reg31Mode r31mode = Reg31IsZeroRegister) const {
    return reg<int32_t>(code, r31mode);
  }

  int64_t xreg(unsigned code,
               Reg31Mode r31mode = Reg31IsZeroRegister) const {
    return reg<int64_t>(code, r31mode);
  }

  
  
  template<typename T>
  T reg(unsigned size, unsigned code,
        Reg31Mode r31mode = Reg31IsZeroRegister) const {
    uint64_t raw;
    switch (size) {
      case kWRegSize: raw = reg<uint32_t>(code, r31mode); break;
      case kXRegSize: raw = reg<uint64_t>(code, r31mode); break;
      default:
        VIXL_UNREACHABLE();
        return 0;
    }

    T result;
    VIXL_STATIC_ASSERT(sizeof(result) <= sizeof(raw));
    
    memcpy(&result, &raw, sizeof(result));
    return result;
  }

  
  int64_t reg(unsigned size, unsigned code,
              Reg31Mode r31mode = Reg31IsZeroRegister) const {
    return reg<int64_t>(size, code, r31mode);
  }

  enum RegLogMode {
    LogRegWrites,
    NoRegLog
  };

  
  
  template<typename T>
  void set_reg(unsigned code, T value,
               RegLogMode log_mode = LogRegWrites,
               Reg31Mode r31mode = Reg31IsZeroRegister) {
    VIXL_STATIC_ASSERT((sizeof(T) == kWRegSizeInBytes) ||
                       (sizeof(T) == kXRegSizeInBytes));
    VIXL_ASSERT(code < kNumberOfRegisters);

    if ((code == 31) && (r31mode == Reg31IsZeroRegister)) {
      return;
    }

    registers_[code].Set(value);

    if (log_mode == LogRegWrites) LogRegister(code, r31mode);
  }

  
  void set_wreg(unsigned code, int32_t value,
                RegLogMode log_mode = LogRegWrites,
                Reg31Mode r31mode = Reg31IsZeroRegister) {
    set_reg(code, value, log_mode, r31mode);
  }

  void set_xreg(unsigned code, int64_t value,
               RegLogMode log_mode = LogRegWrites,
                Reg31Mode r31mode = Reg31IsZeroRegister) {
    set_reg(code, value, log_mode, r31mode);
  }

  
  
  template<typename T>
  void set_reg(unsigned size, unsigned code, T value,
               RegLogMode log_mode = LogRegWrites,
               Reg31Mode r31mode = Reg31IsZeroRegister) {
    
    uint64_t raw = 0;
    VIXL_STATIC_ASSERT(sizeof(value) <= sizeof(raw));
    memcpy(&raw, &value, sizeof(value));

    
    switch (size) {
      case kWRegSize: set_reg<uint32_t>(code, raw, log_mode, r31mode); break;
      case kXRegSize: set_reg<uint64_t>(code, raw, log_mode, r31mode); break;
      default:
        VIXL_UNREACHABLE();
        return;
    }
  }

  

  
  template<typename T>
  void set_lr(T value) {
    set_reg(kLinkRegCode, value);
  }

  template<typename T>
  void set_sp(T value) {
    set_reg(31, value, LogRegWrites, Reg31IsStackPointer);
  }

  
  
  

  
  template<typename T>
  T fpreg(unsigned code) const {
    VIXL_STATIC_ASSERT((sizeof(T) == kSRegSizeInBytes) ||
                       (sizeof(T) == kDRegSizeInBytes));
    VIXL_ASSERT(code < kNumberOfFPRegisters);

    return fpregisters_[code].Get<T>();
  }

  
  float sreg(unsigned code) const {
    return fpreg<float>(code);
  }

  uint32_t sreg_bits(unsigned code) const {
    return fpreg<uint32_t>(code);
  }

  double dreg(unsigned code) const {
    return fpreg<double>(code);
  }

  uint64_t dreg_bits(unsigned code) const {
    return fpreg<uint64_t>(code);
  }

  
  
  template<typename T>
  T fpreg(unsigned size, unsigned code) const {
    uint64_t raw;
    switch (size) {
      case kSRegSize: raw = fpreg<uint32_t>(code); break;
      case kDRegSize: raw = fpreg<uint64_t>(code); break;
      default:
        VIXL_UNREACHABLE();
        raw = 0;
        break;
    }

    T result;
    VIXL_STATIC_ASSERT(sizeof(result) <= sizeof(raw));
    
    memcpy(&result, &raw, sizeof(result));
    return result;
  }

  
  template<typename T>
  void set_fpreg(unsigned code, T value,
                RegLogMode log_mode = LogRegWrites) {
    VIXL_STATIC_ASSERT((sizeof(value) == kSRegSizeInBytes) ||
                       (sizeof(value) == kDRegSizeInBytes));
    VIXL_ASSERT(code < kNumberOfFPRegisters);
    fpregisters_[code].Set(value);

    if (log_mode == LogRegWrites) {
      if (sizeof(value) <= kSRegSizeInBytes) {
        LogFPRegister(code, kPrintSRegValue);
      } else {
        LogFPRegister(code, kPrintDRegValue);
      }
    }
  }

  
  void set_sreg(unsigned code, float value,
                RegLogMode log_mode = LogRegWrites) {
    set_fpreg(code, value, log_mode);
  }

  void set_sreg_bits(unsigned code, uint32_t value,
                     RegLogMode log_mode = LogRegWrites) {
    set_fpreg(code, value, log_mode);
  }

  void set_dreg(unsigned code, double value,
                RegLogMode log_mode = LogRegWrites) {
    set_fpreg(code, value, log_mode);
  }

  void set_dreg_bits(unsigned code, uint64_t value,
                     RegLogMode log_mode = LogRegWrites) {
    set_fpreg(code, value, log_mode);
  }

  bool N() const { return nzcv_.N() != 0; }
  bool Z() const { return nzcv_.Z() != 0; }
  bool C() const { return nzcv_.C() != 0; }
  bool V() const { return nzcv_.V() != 0; }
  SimSystemRegister& nzcv() { return nzcv_; }

  
  
  FPRounding RMode() { return static_cast<FPRounding>(fpcr_.RMode()); }
  bool DN() { return fpcr_.DN() != 0; }
  SimSystemRegister& fpcr() { return fpcr_; }

  
  void PrintRegisters();
  void PrintFPRegisters();
  void PrintSystemRegisters();

  
  void LogSystemRegisters() {
    if (trace_parameters() & LOG_SYS_REGS) PrintSystemRegisters();
  }
  void LogRegisters() {
    if (trace_parameters() & LOG_REGS) PrintRegisters();
  }
  void LogFPRegisters() {
    if (trace_parameters() & LOG_FP_REGS) PrintFPRegisters();
  }

  
  
  
  
  enum PrintFPRegisterSizes {
    kPrintDRegValue = 1 << kDRegSizeInBytes,
    kPrintSRegValue = 1 << kSRegSizeInBytes,
    kPrintAllFPRegValues = kPrintDRegValue | kPrintSRegValue
  };

  
  void PrintRegister(unsigned code, Reg31Mode r31mode = Reg31IsStackPointer);
  void PrintFPRegister(unsigned code,
                       PrintFPRegisterSizes sizes = kPrintAllFPRegValues);
  void PrintSystemRegister(SystemRegister id);

  
  void LogRegister(unsigned code, Reg31Mode r31mode = Reg31IsStackPointer) {
    if (trace_parameters() & LOG_REGS) PrintRegister(code, r31mode);
  }
  void LogFPRegister(unsigned code,
                     PrintFPRegisterSizes sizes = kPrintAllFPRegValues) {
    if (trace_parameters() & LOG_FP_REGS) PrintFPRegister(code, sizes);
  }
  void LogSystemRegister(SystemRegister id) {
    if (trace_parameters() & LOG_SYS_REGS) PrintSystemRegister(id);
  }

  
  void PrintRead(uintptr_t address, size_t size, unsigned reg_code);
  void PrintReadFP(uintptr_t address, size_t size, unsigned reg_code);
  void PrintWrite(uintptr_t address, size_t size, unsigned reg_code);
  void PrintWriteFP(uintptr_t address, size_t size, unsigned reg_code);

  
  void LogRead(uintptr_t address, size_t size, unsigned reg_code) {
    if (trace_parameters() & LOG_REGS) PrintRead(address, size, reg_code);
  }
  void LogReadFP(uintptr_t address, size_t size, unsigned reg_code) {
    if (trace_parameters() & LOG_FP_REGS) PrintReadFP(address, size, reg_code);
  }
  void LogWrite(uintptr_t address, size_t size, unsigned reg_code) {
    if (trace_parameters() & LOG_WRITE) PrintWrite(address, size, reg_code);
  }
  void LogWriteFP(uintptr_t address, size_t size, unsigned reg_code) {
    if (trace_parameters() & LOG_WRITE) PrintWriteFP(address, size, reg_code);
  }

  void DoUnreachable(const Instruction* instr);
  void DoTrace(const Instruction* instr);
  void DoLog(const Instruction* instr);

  static const char* WRegNameForCode(unsigned code,
                                     Reg31Mode mode = Reg31IsZeroRegister);
  static const char* XRegNameForCode(unsigned code,
                                     Reg31Mode mode = Reg31IsZeroRegister);
  static const char* SRegNameForCode(unsigned code);
  static const char* DRegNameForCode(unsigned code);
  static const char* VRegNameForCode(unsigned code);

  bool coloured_trace() const { return coloured_trace_; }
  void set_coloured_trace(bool value);

  int trace_parameters() const { return trace_parameters_; }
  void set_trace_parameters(int parameters);

  void set_instruction_stats(bool value);

  
  
  void ClearLocalMonitor() {
    local_monitor_.Clear();
  }

  void SilenceExclusiveAccessWarning() {
    print_exclusive_access_warning_ = false;
  }

 protected:
  const char* clr_normal;
  const char* clr_flag_name;
  const char* clr_flag_value;
  const char* clr_reg_name;
  const char* clr_reg_value;
  const char* clr_fpreg_name;
  const char* clr_fpreg_value;
  const char* clr_memory_address;
  const char* clr_warning;
  const char* clr_warning_message;
  const char* clr_printf;

  
  bool ConditionPassed(Condition cond) {
    switch (cond) {
      case eq:
        return Z();
      case ne:
        return !Z();
      case hs:
        return C();
      case lo:
        return !C();
      case mi:
        return N();
      case pl:
        return !N();
      case vs:
        return V();
      case vc:
        return !V();
      case hi:
        return C() && !Z();
      case ls:
        return !(C() && !Z());
      case ge:
        return N() == V();
      case lt:
        return N() != V();
      case gt:
        return !Z() && (N() == V());
      case le:
        return !(!Z() && (N() == V()));
      case nv:  
      case al:
        return true;
      default:
        VIXL_UNREACHABLE();
        return false;
    }
  }

  bool ConditionPassed(Instr cond) {
    return ConditionPassed(static_cast<Condition>(cond));
  }

  bool ConditionFailed(Condition cond) {
    return !ConditionPassed(cond);
  }

  void AddSubHelper(const Instruction* instr, int64_t op2);
  int64_t AddWithCarry(unsigned reg_size,
                       bool set_flags,
                       int64_t src1,
                       int64_t src2,
                       int64_t carry_in = 0);
  void LogicalHelper(const Instruction* instr, int64_t op2);
  void ConditionalCompareHelper(const Instruction* instr, int64_t op2);
  void LoadStoreHelper(const Instruction* instr,
                       int64_t offset,
                       AddrMode addrmode);
  void LoadStorePairHelper(const Instruction* instr, AddrMode addrmode);
  uintptr_t AddressModeHelper(unsigned addr_reg,
                              int64_t offset,
                              AddrMode addrmode);

  uint64_t AddressUntag(uint64_t address) {
    return address & ~kAddressTagMask;
  }

  template <typename T>
  T* AddressUntag(T* address) {
    uintptr_t address_raw = reinterpret_cast<uintptr_t>(address);
    return reinterpret_cast<T*>(AddressUntag(address_raw));
  }

  template <typename T, typename A>
  T MemoryRead(A address) {
    T value;
    address = AddressUntag(address);
    VIXL_ASSERT((sizeof(value) == 1) || (sizeof(value) == 2) ||
                (sizeof(value) == 4) || (sizeof(value) == 8));
    memcpy(&value, reinterpret_cast<const char *>(address), sizeof(value));
    return value;
  }

  template <typename T, typename A>
  void MemoryWrite(A address, T value) {
    address = AddressUntag(address);
    VIXL_ASSERT((sizeof(value) == 1) || (sizeof(value) == 2) ||
                (sizeof(value) == 4) || (sizeof(value) == 8));
    memcpy(reinterpret_cast<char *>(address), &value, sizeof(value));
  }

  int64_t ShiftOperand(unsigned reg_size,
                       int64_t value,
                       Shift shift_type,
                       unsigned amount);
  int64_t Rotate(unsigned reg_width,
                 int64_t value,
                 Shift shift_type,
                 unsigned amount);
  int64_t ExtendValue(unsigned reg_width,
                      int64_t value,
                      Extend extend_type,
                      unsigned left_shift = 0);

  enum ReverseByteMode {
    Reverse16 = 0,
    Reverse32 = 1,
    Reverse64 = 2
  };
  uint64_t ReverseBytes(uint64_t value, ReverseByteMode mode);
  uint64_t ReverseBits(uint64_t value, unsigned num_bits);

  template <typename T>
  T FPDefaultNaN() const;

  void FPCompare(double val0, double val1);
  double FPRoundInt(double value, FPRounding round_mode);
  double FPToDouble(float value);
  float FPToFloat(double value, FPRounding round_mode);
  double FixedToDouble(int64_t src, int fbits, FPRounding round_mode);
  double UFixedToDouble(uint64_t src, int fbits, FPRounding round_mode);
  float FixedToFloat(int64_t src, int fbits, FPRounding round_mode);
  float UFixedToFloat(uint64_t src, int fbits, FPRounding round_mode);
  int32_t FPToInt32(double value, FPRounding rmode);
  int64_t FPToInt64(double value, FPRounding rmode);
  uint32_t FPToUInt32(double value, FPRounding rmode);
  uint64_t FPToUInt64(double value, FPRounding rmode);

  template <typename T>
  T FPAdd(T op1, T op2);

  template <typename T>
  T FPDiv(T op1, T op2);

  template <typename T>
  T FPMax(T a, T b);

  template <typename T>
  T FPMaxNM(T a, T b);

  template <typename T>
  T FPMin(T a, T b);

  template <typename T>
  T FPMinNM(T a, T b);

  template <typename T>
  T FPMul(T op1, T op2);

  template <typename T>
  T FPMulAdd(T a, T op1, T op2);

  template <typename T>
  T FPSqrt(T op);

  template <typename T>
  T FPSub(T op1, T op2);

  
  
  void FPProcessException() { }

  
  template <typename T>
  T FPProcessNaN(T op);

  bool FPProcessNaNs(const Instruction* instr);

  template <typename T>
  T FPProcessNaNs(T op1, T op2);

  template <typename T>
  T FPProcessNaNs3(T op1, T op2, T op3);

  
  void DoPrintf(const Instruction* instr);

  

  
  SimExclusiveLocalMonitor local_monitor_;
  SimExclusiveGlobalMonitor global_monitor_;

  
  FILE* stream_;
  PrintDisassembler* print_disasm_;

  
  Instrument* instrumentation_;

  
  SimRegister registers_[kNumberOfRegisters];

  
  SimFPRegister fpregisters_[kNumberOfFPRegisters];

  
  
  
  SimSystemRegister nzcv_;

  
  SimSystemRegister fpcr_;

  
  
  
  
  
  
  
  void AssertSupportedFPCR() {
    VIXL_ASSERT(fpcr().FZ() == 0);             
    VIXL_ASSERT(fpcr().RMode() == FPTieEven);  

    
    
  }

  static int CalcNFlag(uint64_t result, unsigned reg_size) {
    return (result >> (reg_size - 1)) & 1;
  }

  static int CalcZFlag(uint64_t result) {
    return result == 0;
  }

  static const uint32_t kConditionFlagsMask = 0xf0000000;

  
  byte* stack_;
  static const int stack_protection_size_ = 256;
  
  static const int stack_size_ = 2 * 1024 + 2 * stack_protection_size_;
  byte* stack_limit_;

  Decoder* decoder_;
  
  
  bool pc_modified_;
  const Instruction* pc_;
  const Instruction* resume_pc_;

  static const char* xreg_names[];
  static const char* wreg_names[];
  static const char* sreg_names[];
  static const char* dreg_names[];
  static const char* vreg_names[];

  static const Instruction* kEndOfSimAddress;

 private:
  bool coloured_trace_;

  
  int trace_parameters_;

  
  bool instruction_stats_;

  
  bool print_exclusive_access_warning_;
  void PrintExclusiveAccessWarning();

 protected:
  
  PRLock* lock_;
#ifdef DEBUG
  PRThread* lockOwner_;
#endif
  Redirection* redirection_;
  mozilla::Vector<int64_t, 0, js::SystemAllocPolicy> spStack_;
};
}  

#endif  
