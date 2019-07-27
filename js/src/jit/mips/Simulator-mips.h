



























#ifndef jit_mips_Simulator_mips_h
#define jit_mips_Simulator_mips_h

#ifdef JS_MIPS_SIMULATOR

#include "jslock.h"

#include "jit/IonTypes.h"

namespace js {
namespace jit {

class Simulator;
class Redirection;
class CachePage;
class AutoLockSimulator;

const intptr_t kPointerAlignment = 4;
const intptr_t kPointerAlignmentMask = kPointerAlignment - 1;

const intptr_t kDoubleAlignment = 8;
const intptr_t kDoubleAlignmentMask = kDoubleAlignment - 1;



const int kNumRegisters = 32;


const int kPCRegister = 34;


const int kNumFPURegisters = 32;


const int kFCSRRegister = 31;
const int kInvalidFPUControlRegister = -1;
const uint32_t kFPUInvalidResult = static_cast<uint32_t>(1 << 31) - 1;


const uint32_t kFCSRInexactFlagBit = 2;
const uint32_t kFCSRUnderflowFlagBit = 3;
const uint32_t kFCSROverflowFlagBit = 4;
const uint32_t kFCSRDivideByZeroFlagBit = 5;
const uint32_t kFCSRInvalidOpFlagBit = 6;

const uint32_t kFCSRInexactFlagMask = 1 << kFCSRInexactFlagBit;
const uint32_t kFCSRUnderflowFlagMask = 1 << kFCSRUnderflowFlagBit;
const uint32_t kFCSROverflowFlagMask = 1 << kFCSROverflowFlagBit;
const uint32_t kFCSRDivideByZeroFlagMask = 1 << kFCSRDivideByZeroFlagBit;
const uint32_t kFCSRInvalidOpFlagMask = 1 << kFCSRInvalidOpFlagBit;

const uint32_t kFCSRFlagMask =
    kFCSRInexactFlagMask |
    kFCSRUnderflowFlagMask |
    kFCSROverflowFlagMask |
    kFCSRDivideByZeroFlagMask |
    kFCSRInvalidOpFlagMask;

const uint32_t kFCSRExceptionFlagMask = kFCSRFlagMask ^ kFCSRInexactFlagMask;








const uint32_t kMaxWatchpointCode = 31;
const uint32_t kMaxStopCode = 127;




typedef uint32_t Instr;
class SimInstruction;

class Simulator {
    friend class Redirection;
    friend class MipsDebugger;
    friend class AutoLockSimulatorCache;
  public:

    
    enum Register {
        no_reg = -1,
        zero_reg = 0,
        at,
        v0, v1,
        a0, a1, a2, a3,
        t0, t1, t2, t3, t4, t5, t6, t7,
        s0, s1, s2, s3, s4, s5, s6, s7,
        t8, t9,
        k0, k1,
        gp,
        sp,
        s8,
        ra,
        
        LO,
        HI,
        pc,   
        kNumSimuRegisters,
        
        fp = s8
    };

    
    enum FPURegister {
        f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11,
        f12, f13, f14, f15,   
        f16, f17, f18, f19, f20, f21, f22, f23, f24, f25,
        f26, f27, f28, f29, f30, f31,
        kNumFPURegisters
    };

    
    static Simulator *Create();

    static void Destroy(Simulator *simulator);

    
    Simulator();
    ~Simulator();

    
    
    static Simulator *Current();

    static inline uintptr_t StackLimit() {
        return Simulator::Current()->stackLimit();
    }

    uintptr_t *addressOfStackLimit();

    
    
    
    void setRegister(int reg, int32_t value);
    int32_t getRegister(int reg) const;
    double getDoubleFromRegisterPair(int reg);
    
    void setFpuRegister(int fpureg, int32_t value);
    void setFpuRegisterFloat(int fpureg, float value);
    void setFpuRegisterFloat(int fpureg, int64_t value);
    void setFpuRegisterDouble(int fpureg, double value);
    void setFpuRegisterDouble(int fpureg, int64_t value);
    int32_t getFpuRegister(int fpureg) const;
    int64_t getFpuRegisterLong(int fpureg) const;
    float getFpuRegisterFloat(int fpureg) const;
    double getFpuRegisterDouble(int fpureg) const;
    void setFCSRBit(uint32_t cc, bool value);
    bool testFCSRBit(uint32_t cc);
    bool setFCSRRoundError(double original, double rounded);

    
    void set_pc(int32_t value);
    int32_t get_pc() const;

    void set_resume_pc(int32_t value) {
        resume_pc_ = value;
    }

    
    uintptr_t stackLimit() const;
    bool overRecursed(uintptr_t newsp = 0) const;
    bool overRecursedWithExtra(uint32_t extra) const;

    
    template<bool enableStopSimAt>
    void execute();

    
    int64_t call(uint8_t *entry, int argument_count, ...);

    
    uintptr_t pushAddress(uintptr_t address);

    
    uintptr_t popAddress();

    
    void setLastDebuggerInput(char *input);
    char *lastDebuggerInput() { return lastDebuggerInput_; }
    
    static void FlushICache(void *start, size_t size);

    
    
    bool has_bad_pc() const;

  private:
    enum SpecialValues {
        
        
        bad_ra = -1,
        
        
        
        
        end_sim_pc = -2,
        
        Unpredictable = 0xbadbeaf
    };

    bool init();

    
    void format(SimInstruction* instr, const char* format);

    
    inline uint32_t readBU(uint32_t addr);
    inline int32_t readB(uint32_t addr);
    inline void writeB(uint32_t addr, uint8_t value);
    inline void writeB(uint32_t addr, int8_t value);

    inline uint16_t readHU(uint32_t addr, SimInstruction *instr);
    inline int16_t readH(uint32_t addr, SimInstruction *instr);
    
    inline void writeH(uint32_t addr, uint16_t value, SimInstruction *instr);
    inline void writeH(uint32_t addr, int16_t value, SimInstruction *instr);

    inline int readW(uint32_t addr, SimInstruction *instr);
    inline void writeW(uint32_t addr, int value, SimInstruction *instr);

    inline double readD(uint32_t addr, SimInstruction *instr);
    inline void writeD(uint32_t addr, double value, SimInstruction *instr);

    
    void decodeTypeRegister(SimInstruction *instr);

    
    void configureTypeRegister(SimInstruction *instr,
                               int32_t& alu_out,
                               int64_t& i64hilo,
                               uint64_t& u64hilo,
                               int32_t& next_pc,
                               int32_t& return_addr_reg,
                               bool& do_interrupt);

    void decodeTypeImmediate(SimInstruction *instr);
    void decodeTypeJump(SimInstruction *instr);

    
    void softwareInterrupt(SimInstruction *instr);

    
    bool isWatchpoint(uint32_t code);
    void printWatchpoint(uint32_t code);
    void handleStop(uint32_t code, SimInstruction *instr);
    bool isStopInstruction(SimInstruction *instr);
    bool isEnabledStop(uint32_t code);
    void enableStop(uint32_t code);
    void disableStop(uint32_t code);
    void increaseStopCounter(uint32_t code);
    void printStopInfo(uint32_t code);


    
    void instructionDecode(SimInstruction *instr);
    
    void branchDelayInstructionDecode(SimInstruction *instr);

  public:
    static bool ICacheCheckingEnabled;

    static int StopSimAt;

    
    static void *RedirectNativeFunction(void *nativeFunction, ABIFunctionType type);

  private:
    enum Exception {
        kNone,
        kIntegerOverflow,
        kIntegerUnderflow,
        kDivideByZero,
        kNumExceptions
    };
    int16_t exceptions[kNumExceptions];

    
    void signalExceptions();

    
    void getFpArgs(double *x, double *y, int32_t *z);
    void getFpFromStack(int32_t *stack, double *x);

    void setCallResultDouble(double result);
    void setCallResultFloat(float result);
    void setCallResult(int64_t res);

    void callInternal(uint8_t *entry);

    
    
    int32_t registers_[kNumSimuRegisters];
    
    int32_t FPUregisters_[kNumFPURegisters];
    
    uint32_t FCSR_;

    
    char *stack_;
    uintptr_t stackLimit_;
    bool pc_modified_;
    int icount_;
    int break_count_;

    int32_t resume_pc_;

    
    char *lastDebuggerInput_;

    
    SimInstruction *break_pc_;
    Instr break_instr_;

    
    
    static const uint32_t kNumOfWatchedStops = 256;


    
    static const uint32_t kStopDisabledBit = 1U << 31;

    
    
    
    
    struct StopCountAndDesc {
        uint32_t count_;
        char *desc_;
    };
    StopCountAndDesc watchedStops_[kNumOfWatchedStops];

  private:
    
    struct ICacheHasher {
        typedef void *Key;
        typedef void *Lookup;
        static HashNumber hash(const Lookup &l);
        static bool match(const Key &k, const Lookup &l);
    };

  public:
    typedef HashMap<void *, CachePage *, ICacheHasher, SystemAllocPolicy> ICacheMap;

  private:
    
    
    
    PRLock *cacheLock_;
#ifdef DEBUG
    PRThread *cacheLockHolder_;
#endif

    Redirection *redirection_;
    ICacheMap icache_;

  public:
    ICacheMap &icache() {
        
        
        
        MOZ_ASSERT(cacheLockHolder_);
        return icache_;
    }

    Redirection *redirection() const {
        MOZ_ASSERT(cacheLockHolder_);
        return redirection_;
    }

    void setRedirection(js::jit::Redirection *redirection) {
        MOZ_ASSERT(cacheLockHolder_);
        redirection_ = redirection;
    }
};

#define JS_CHECK_SIMULATOR_RECURSION_WITH_EXTRA(cx, extra, onerror)             \
    JS_BEGIN_MACRO                                                              \
        if (cx->mainThread().simulator()->overRecursedWithExtra(extra)) {       \
            js_ReportOverRecursed(cx);                                          \
            onerror;                                                            \
        }                                                                       \
    JS_END_MACRO

} 
} 

#endif

#endif
