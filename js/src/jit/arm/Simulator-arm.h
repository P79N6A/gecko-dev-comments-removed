



























#ifndef jit_arm_Simulator_arm_h
#define jit_arm_Simulator_arm_h

#ifdef JS_ARM_SIMULATOR

#include "jslock.h"

#include "jit/arm/Architecture-arm.h"
#include "jit/IonTypes.h"

namespace js {
namespace jit {

class Simulator;
class Redirection;
class CachePage;
class AutoLockSimulator;




typedef void (*SingleStepCallback)(void *arg, Simulator *sim, void *pc);


enum VFPRoundingMode {
    SimRN = 0 << 22,   
    SimRP = 1 << 22,   
    SimRM = 2 << 22,   
    SimRZ = 3 << 22,   

    
    kRoundToNearest = SimRN,
    kRoundToPlusInf = SimRP,
    kRoundToMinusInf = SimRM,
    kRoundToZero = SimRZ
};

const uint32_t kVFPRoundingModeMask = 3 << 22;

typedef int32_t Instr;
class SimInstruction;

class Simulator
{
    friend class Redirection;
    friend class AutoLockSimulatorCache;

  public:
    friend class ArmDebugger;
    enum Register {
        no_reg = -1,
        r0 = 0, r1, r2, r3, r4, r5, r6, r7,
        r8, r9, r10, r11, r12, r13, r14, r15,
        num_registers,
        sp = 13,
        lr = 14,
        pc = 15,
        s0 = 0, s1, s2, s3, s4, s5, s6, s7,
        s8, s9, s10, s11, s12, s13, s14, s15,
        s16, s17, s18, s19, s20, s21, s22, s23,
        s24, s25, s26, s27, s28, s29, s30, s31,
        num_s_registers = 32,
        d0 = 0, d1, d2, d3, d4, d5, d6, d7,
        d8, d9, d10, d11, d12, d13, d14, d15,
        d16, d17, d18, d19, d20, d21, d22, d23,
        d24, d25, d26, d27, d28, d29, d30, d31,
        num_d_registers = 32,
        q0 = 0, q1, q2, q3, q4, q5, q6, q7,
        q8, q9, q10, q11, q12, q13, q14, q15,
        num_q_registers = 16
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

    
    
    
    void set_register(int reg, int32_t value);
    int32_t get_register(int reg) const;
    double get_double_from_register_pair(int reg);
    void set_register_pair_from_double(int reg, double* value);
    void set_dw_register(int dreg, const int* dbl);

    
    void get_d_register(int dreg, uint64_t* value);
    void set_d_register(int dreg, const uint64_t* value);
    void get_d_register(int dreg, uint32_t* value);
    void set_d_register(int dreg, const uint32_t* value);
    void get_q_register(int qreg, uint64_t* value);
    void set_q_register(int qreg, const uint64_t* value);
    void get_q_register(int qreg, uint32_t* value);
    void set_q_register(int qreg, const uint32_t* value);
    void set_s_register(int reg, unsigned int value);
    unsigned int get_s_register(int reg) const;

    void set_d_register_from_double(int dreg, const double& dbl) {
        setVFPRegister<double, 2>(dreg, dbl);
    }
    double get_double_from_d_register(int dreg) {
        return getFromVFPRegister<double, 2>(dreg);
    }
    void set_s_register_from_float(int sreg, const float flt) {
        setVFPRegister<float, 1>(sreg, flt);
    }
    float get_float_from_s_register(int sreg) {
        return getFromVFPRegister<float, 1>(sreg);
    }
    void set_s_register_from_sinteger(int sreg, const int sint) {
        setVFPRegister<int, 1>(sreg, sint);
    }
    int get_sinteger_from_s_register(int sreg) {
        return getFromVFPRegister<int, 1>(sreg);
    }

    
    void set_pc(int32_t value);
    int32_t get_pc() const;

    void set_resume_pc(int32_t value) {
        resume_pc_ = value;
    }

    void enable_single_stepping(SingleStepCallback cb, void *arg);
    void disable_single_stepping();

    uintptr_t stackLimit() const;
    bool overRecursed(uintptr_t newsp = 0) const;
    bool overRecursedWithExtra(uint32_t extra) const;

    
    template<bool EnableStopSimAt>
    void execute();

    
    int64_t call(uint8_t* entry, int argument_count, ...);

    
    void setLastDebuggerInput(char *input);
    char *lastDebuggerInput() { return lastDebuggerInput_; }

    
    
    bool has_bad_pc() const;

  private:
    enum special_values {
        
        
        bad_lr = -1,
        
        
        
        
        end_sim_pc = -2
    };

    bool init();

    
    
    inline bool conditionallyExecute(SimInstruction* instr);

    
    void setNZFlags(int32_t val);
    void setCFlag(bool val);
    void setVFlag(bool val);
    bool carryFrom(int32_t left, int32_t right, int32_t carry = 0);
    bool borrowFrom(int32_t left, int32_t right);
    bool overflowFrom(int32_t alu_out, int32_t left, int32_t right, bool addition);

    inline int getCarry() { return c_flag_ ? 1 : 0; };

    
    void compute_FPSCR_Flags(double val1, double val2);
    void copy_FPSCR_to_APSR();
    inline double canonicalizeNaN(double value);

    
    int32_t getShiftRm(SimInstruction *instr, bool* carry_out);
    int32_t getImm(SimInstruction *instr, bool* carry_out);
    int32_t processPU(SimInstruction *instr, int num_regs, int operand_size,
                      intptr_t *start_address, intptr_t *end_address);
    void handleRList(SimInstruction *instr, bool load);
    void handleVList(SimInstruction *inst);
    void softwareInterrupt(SimInstruction *instr);

    
    inline bool isStopInstruction(SimInstruction *instr);
    inline bool isWatchedStop(uint32_t bkpt_code);
    inline bool isEnabledStop(uint32_t bkpt_code);
    inline void enableStop(uint32_t bkpt_code);
    inline void disableStop(uint32_t bkpt_code);
    inline void increaseStopCounter(uint32_t bkpt_code);
    void printStopInfo(uint32_t code);

    
    inline uint8_t readBU(int32_t addr);
    inline int8_t readB(int32_t addr);
    inline void writeB(int32_t addr, uint8_t value);
    inline void writeB(int32_t addr, int8_t value);

    inline uint16_t readHU(int32_t addr, SimInstruction *instr);
    inline int16_t readH(int32_t addr, SimInstruction *instr);
    
    inline void writeH(int32_t addr, uint16_t value, SimInstruction *instr);
    inline void writeH(int32_t addr, int16_t value, SimInstruction *instr);

    inline int readW(int32_t addr, SimInstruction *instr);
    inline void writeW(int32_t addr, int value, SimInstruction *instr);

    int32_t *readDW(int32_t addr);
    void writeDW(int32_t addr, int32_t value1, int32_t value2);

    
    
    void decodeType01(SimInstruction *instr);
    void decodeType2(SimInstruction *instr);
    void decodeType3(SimInstruction *instr);
    void decodeType4(SimInstruction *instr);
    void decodeType5(SimInstruction *instr);
    void decodeType6(SimInstruction *instr);
    void decodeType7(SimInstruction *instr);

    
    void decodeTypeVFP(SimInstruction *instr);
    void decodeType6CoprocessorIns(SimInstruction *instr);
    void decodeSpecialCondition(SimInstruction *instr);

    void decodeVMOVBetweenCoreAndSinglePrecisionRegisters(SimInstruction *instr);
    void decodeVCMP(SimInstruction *instr);
    void decodeVCVTBetweenDoubleAndSingle(SimInstruction *instr);
    void decodeVCVTBetweenFloatingPointAndInteger(SimInstruction *instr);
    void decodeVCVTBetweenFloatingPointAndIntegerFrac(SimInstruction *instr);

    
    void decodeType7CoprocessorIns(SimInstruction *instr);

    
    void instructionDecode(SimInstruction *instr);

  public:
    static bool ICacheCheckingEnabled;
    static void FlushICache(void *start, size_t size);

    static int64_t StopSimAt;

    
    
    
    
    
    bool skipCalleeSavedRegsCheck;

    
    static void *RedirectNativeFunction(void *nativeFunction, ABIFunctionType type);

  private:
    
    void getFpArgs(double *x, double *y, int32_t *z);
    void getFpFromStack(int32_t *stack, double *x1);
    void setCallResultDouble(double result);
    void setCallResultFloat(float result);
    void setCallResult(int64_t res);
    void scratchVolatileRegisters(bool scratchFloat = true);

    template<class ReturnType, int register_size>
    ReturnType getFromVFPRegister(int reg_index);

    template<class InputType, int register_size>
    void setVFPRegister(int reg_index, const InputType& value);

    void callInternal(uint8_t* entry);

    
    
    
    
    int32_t registers_[16];
    bool n_flag_;
    bool z_flag_;
    bool c_flag_;
    bool v_flag_;

    
    uint32_t vfp_registers_[num_d_registers * 2];
    bool n_flag_FPSCR_;
    bool z_flag_FPSCR_;
    bool c_flag_FPSCR_;
    bool v_flag_FPSCR_;

    
    VFPRoundingMode FPSCR_rounding_mode_;
    bool FPSCR_default_NaN_mode_;

    
    bool inv_op_vfp_flag_;
    bool div_zero_vfp_flag_;
    bool overflow_vfp_flag_;
    bool underflow_vfp_flag_;
    bool inexact_vfp_flag_;

    
    char *stack_;
    uintptr_t stackLimit_;
    bool pc_modified_;
    int64_t icount_;

    int32_t resume_pc_;

    
    char *lastDebuggerInput_;

    
    SimInstruction *break_pc_;
    Instr break_instr_;

    
    bool single_stepping_;
    SingleStepCallback single_step_callback_;
    void *single_step_callback_arg_;

    
    
    static const uint32_t kNumOfWatchedStops = 256;

    
    static const uint32_t kStopDisabledBit = 1 << 31;

    
    
    
    
    struct StopCountAndDesc {
        uint32_t count;
        char *desc;
    };
    StopCountAndDesc watched_stops_[kNumOfWatchedStops];

  public:
    int64_t icount() {
        return icount_;
    }

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
        if (cx->runtime()->simulator()->overRecursedWithExtra(extra)) {         \
            js::ReportOverRecursed(cx);                                         \
            onerror;                                                            \
        }                                                                       \
    JS_END_MACRO

} 
} 

#endif

#endif
