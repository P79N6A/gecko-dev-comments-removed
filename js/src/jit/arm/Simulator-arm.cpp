



























#include "jit/arm/Simulator-arm.h"

#include "mozilla/Casting.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/FloatingPoint.h"
#include "mozilla/Likely.h"
#include "mozilla/MathAlgorithms.h"
#include "mozilla/SizePrintfMacros.h"

#include "asmjs/AsmJSValidate.h"
#include "jit/arm/Assembler-arm.h"
#include "vm/Runtime.h"

extern "C" {

int64_t
__aeabi_idivmod(int x, int y)
{
    uint32_t lo = uint32_t(x / y);
    uint32_t hi = uint32_t(x % y);
    return (int64_t(hi) << 32) | lo;
}

int64_t
__aeabi_uidivmod(int x, int y)
{
    uint32_t lo = uint32_t(x) / uint32_t(y);
    uint32_t hi = uint32_t(x) % uint32_t(y);
    return (int64_t(hi) << 32) | lo;
}
}

namespace js {
namespace jit {


enum BlockAddrMode {
    
    da_x         = (0|0|0) << 21,  
    ia_x         = (0|4|0) << 21,  
    db_x         = (8|0|0) << 21,  
    ib_x         = (8|4|0) << 21,  
};


enum VFPRegPrecision {
    kSinglePrecision = 0,
    kDoublePrecision = 1
};

enum NeonListType {
    nlt_1 = 0x7,
    nlt_2 = 0xA,
    nlt_3 = 0x6,
    nlt_4 = 0x2
};







enum SoftwareInterruptCodes {
    kCallRtRedirected = 0x10,  
    kBreakpoint= 0x20, 
    kStopCode = 1 << 23 
};

const uint32_t kStopCodeMask = kStopCode - 1;
const uint32_t kMaxStopCode = kStopCode - 1;

















class SimInstruction {
  public:
    enum {
        kInstrSize = 4,
        kPCReadOffset = 8
    };

    
    inline Instr instructionBits() const {
        return *reinterpret_cast<const Instr*>(this);
    }

    
    inline void setInstructionBits(Instr value) {
        *reinterpret_cast<Instr*>(this) = value;
    }

    
    inline int bit(int nr) const {
        return (instructionBits() >> nr) & 1;
    }

    
    inline int bits(int hi, int lo) const {
        return (instructionBits() >> lo) & ((2 << (hi - lo)) - 1);
    }

    
    inline int bitField(int hi, int lo) const {
        return instructionBits() & (((2 << (hi - lo)) - 1) << lo);
    }

    
    
    
    
    
    
    
    
    
    
    

    
    inline Assembler::ARMCondition conditionField() const {
        return static_cast<Assembler::ARMCondition>(bitField(31, 28));
    }
    inline int typeValue() const { return bits(27, 25); }
    inline int specialValue() const { return bits(27, 23); }

    inline int rnValue() const { return bits(19, 16); }
    inline int rdValue() const { return bits(15, 12); }

    inline int coprocessorValue() const { return bits(11, 8); }

    
    
    inline int vnValue() const { return bits(19, 16); }
    inline int vmValue() const { return bits(3, 0); }
    inline int vdValue() const { return bits(15, 12); }
    inline int nValue() const { return bit(7); }
    inline int mValue() const { return bit(5); }
    inline int dValue() const { return bit(22); }
    inline int rtValue() const { return bits(15, 12); }
    inline int pValue() const { return bit(24); }
    inline int uValue() const { return bit(23); }
    inline int opc1Value() const { return (bit(23) << 2) | bits(21, 20); }
    inline int opc2Value() const { return bits(19, 16); }
    inline int opc3Value() const { return bits(7, 6); }
    inline int szValue() const { return bit(8); }
    inline int VLValue() const { return bit(20); }
    inline int VCValue() const { return bit(8); }
    inline int VAValue() const { return bits(23, 21); }
    inline int VBValue() const { return bits(6, 5); }
    inline int VFPNRegValue(VFPRegPrecision pre) { return VFPGlueRegValue(pre, 16, 7); }
    inline int VFPMRegValue(VFPRegPrecision pre) { return VFPGlueRegValue(pre, 0, 5); }
    inline int VFPDRegValue(VFPRegPrecision pre) { return VFPGlueRegValue(pre, 12, 22); }

    
    inline int opcodeValue() const { return static_cast<ALUOp>(bits(24, 21)); }
    inline ALUOp opcodeField() const { return static_cast<ALUOp>(bitField(24, 21)); }
    inline int sValue() const { return bit(20); }

    
    inline int rmValue() const { return bits(3, 0); }
    inline ShiftType shifttypeValue() const { return static_cast<ShiftType>(bits(6, 5)); }
    inline int rsValue() const { return bits(11, 8); }
    inline int shiftAmountValue() const { return bits(11, 7); }

    
    inline int rotateValue() const { return bits(11, 8); }
    inline int immed8Value() const { return bits(7, 0); }
    inline int immed4Value() const { return bits(19, 16); }
    inline int immedMovwMovtValue() const { return immed4Value() << 12 | offset12Value(); }

    
    inline int PUValue() const { return bits(24, 23); }
    inline int PUField() const { return bitField(24, 23); }
    inline int bValue() const { return bit(22); }
    inline int wValue() const { return bit(21); }
    inline int lValue() const { return bit(20); }

    
    
    inline int offset12Value() const { return bits(11, 0); }

    
    inline int rlistValue() const { return bits(15, 0); }

    
    inline int signValue() const { return bit(6); }
    inline int hValue() const { return bit(5); }
    inline int immedHValue() const { return bits(11, 8); }
    inline int immedLValue() const { return bits(3, 0); }

    
    inline int linkValue() const { return bit(24); }
    inline int sImmed24Value() const { return ((instructionBits() << 8) >> 8); }

    
    inline SoftwareInterruptCodes svcValue() const {
        return static_cast<SoftwareInterruptCodes>(bits(23, 0));
    }

    
    
    inline bool isSpecialType0() const { return (bit(7) == 1) && (bit(4) == 1); }

    
    inline bool isMiscType0() const {
        return bit(24) == 1 && bit(23) == 0 && bit(20) == 0 && (bit(7) == 0);
    }

    
    inline bool isNopType1() const { return bits(24, 0) == 0x0120F000; }

    
    inline bool isStop() const {
        return typeValue() == 7 && bit(24) == 1 && svcValue() >= kStopCode;
    }

    
    inline bool hasS()    const { return sValue() == 1; }
    inline bool hasB()    const { return bValue() == 1; }
    inline bool hasW()    const { return wValue() == 1; }
    inline bool hasL()    const { return lValue() == 1; }
    inline bool hasU()    const { return uValue() == 1; }
    inline bool hasSign() const { return signValue() == 1; }
    inline bool hasH()    const { return hValue() == 1; }
    inline bool hasLink() const { return linkValue() == 1; }

    
    double doubleImmedVmov() const;
    
    float float32ImmedVmov() const;

  private:
    
    
    
    
    inline int VFPGlueRegValue(VFPRegPrecision pre, int four_bit, int one_bit) {
        if (pre == kSinglePrecision)
            return (bits(four_bit + 3, four_bit) << 1) | bit(one_bit);
        return (bit(one_bit) << 4) | bits(four_bit + 3, four_bit);
    }

    SimInstruction() = delete;
    SimInstruction(const SimInstruction& other) = delete;
    void operator=(const SimInstruction& other) = delete;
};

double
SimInstruction::doubleImmedVmov() const
{
    
    
    
    
    
    
    
    uint64_t high16;
    high16  = (bits(17, 16) << 4) | bits(3, 0);   
    high16 |= (0xff * bit(18)) << 6;              
    high16 |= (bit(18) ^ 1) << 14;                
    high16 |= bit(19) << 15;                      

    uint64_t imm = high16 << 48;
    return mozilla::BitwiseCast<double>(imm);
}

float
SimInstruction::float32ImmedVmov() const
{
    
    
    
    
    
    
    uint32_t imm;
    imm  = (bits(17, 16) << 23) | (bits(3, 0) << 19); 
    imm |= (0x1f * bit(18)) << 25;                    
    imm |= (bit(18) ^ 1) << 30;                       
    imm |= bit(19) << 31;                             

    return mozilla::BitwiseCast<float>(imm);
}

class CachePage
{
  public:
    static const int LINE_VALID = 0;
    static const int LINE_INVALID = 1;
    static const int kPageShift = 12;
    static const int kPageSize = 1 << kPageShift;
    static const int kPageMask = kPageSize - 1;
    static const int kLineShift = 2;  
    static const int kLineLength = 1 << kLineShift;
    static const int kLineMask = kLineLength - 1;

    CachePage() {
        memset(&validity_map_, LINE_INVALID, sizeof(validity_map_));
    }
    char* validityByte(int offset) {
        return &validity_map_[offset >> kLineShift];
    }
    char* cachedData(int offset) {
        return &data_[offset];
    }

  private:
    char data_[kPageSize];   
    static const int kValidityMapSize = kPageSize >> kLineShift;
    char validity_map_[kValidityMapSize];  
};



class AutoLockSimulatorCache
{
  public:
    explicit AutoLockSimulatorCache(Simulator* sim) : sim_(sim) {
        PR_Lock(sim_->cacheLock_);
        MOZ_ASSERT(!sim_->cacheLockHolder_);
#ifdef DEBUG
        sim_->cacheLockHolder_ = PR_GetCurrentThread();
#endif
    }

    ~AutoLockSimulatorCache() {
        MOZ_ASSERT(sim_->cacheLockHolder_);
#ifdef DEBUG
        sim_->cacheLockHolder_ = nullptr;
#endif
        PR_Unlock(sim_->cacheLock_);
    }

  private:
    Simulator* const sim_;
};

bool Simulator::ICacheCheckingEnabled = false;

int64_t Simulator::StopSimAt = -1L;

Simulator*
Simulator::Create()
{
    Simulator* sim = js_new<Simulator>();
    if (!sim)
        return nullptr;

    if (!sim->init()) {
        js_delete(sim);
        return nullptr;
    }

    if (getenv("ARM_SIM_ICACHE_CHECKS"))
        Simulator::ICacheCheckingEnabled = true;

    char* stopAtStr = getenv("ARM_SIM_STOP_AT");
    int64_t stopAt;
    if (stopAtStr && sscanf(stopAtStr, "%lld", &stopAt) == 1) {
        fprintf(stderr, "\nStopping simulation at icount %lld\n", stopAt);
        Simulator::StopSimAt = stopAt;
    }

    return sim;
}

void
Simulator::Destroy(Simulator* sim)
{
    js_delete(sim);
}



class ArmDebugger {
  public:
    explicit ArmDebugger(Simulator* sim) : sim_(sim) { }

    void stop(SimInstruction* instr);
    void debug();

  private:
    static const Instr kBreakpointInstr = (Assembler::AL | (7 * (1 << 25)) | (1* (1 << 24)) | kBreakpoint);
    static const Instr kNopInstr = (Assembler::AL | (13 * (1 << 21)));

    Simulator* sim_;

    int32_t getRegisterValue(int regnum);
    double getRegisterPairDoubleValue(int regnum);
    double getVFPDoubleRegisterValue(int regnum);
    bool getValue(const char* desc, int32_t* value);
    bool getVFPDoubleValue(const char* desc, double* value);

    
    bool setBreakpoint(SimInstruction* breakpc);
    bool deleteBreakpoint(SimInstruction* breakpc);

    
    
    void undoBreakpoints();
    void redoBreakpoints();
};

void
ArmDebugger::stop(SimInstruction * instr)
{
    
    uint32_t code = instr->svcValue() & kStopCodeMask;
    
    char* msg = *reinterpret_cast<char**>(sim_->get_pc()
                                          + SimInstruction::kInstrSize);
    
    if (sim_->isWatchedStop(code) && !sim_->watched_stops_[code].desc) {
        sim_->watched_stops_[code].desc = msg;
    }
    
    if (code != kMaxStopCode) {
        printf("Simulator hit stop %u: %s\n", code, msg);
    } else {
        printf("Simulator hit %s\n", msg);
    }
    sim_->set_pc(sim_->get_pc() + 2 * SimInstruction::kInstrSize);
    debug();
}

int32_t
ArmDebugger::getRegisterValue(int regnum)
{
    if (regnum == Registers::pc)
        return sim_->get_pc();
    return sim_->get_register(regnum);
}

double
ArmDebugger::getRegisterPairDoubleValue(int regnum)
{
    return sim_->get_double_from_register_pair(regnum);
}

double
ArmDebugger::getVFPDoubleRegisterValue(int regnum)
{
    return sim_->get_double_from_d_register(regnum);
}

bool
ArmDebugger::getValue(const char* desc, int32_t* value)
{
    Register reg = Register::FromName(desc);
    if (reg != InvalidReg) {
        *value = getRegisterValue(reg.code());
        return true;
    }
    if (strncmp(desc, "0x", 2) == 0)
        return sscanf(desc + 2, "%x", reinterpret_cast<uint32_t*>(value)) == 1;
    return sscanf(desc, "%u", reinterpret_cast<uint32_t*>(value)) == 1;
}

bool
ArmDebugger::getVFPDoubleValue(const char* desc, double* value)
{
    FloatRegister reg(FloatRegister::FromName(desc));
    if (reg != InvalidFloatReg) {
        *value = sim_->get_double_from_d_register(reg.code());
        return true;
    }
    return false;
}

bool
ArmDebugger::setBreakpoint(SimInstruction* breakpc)
{
    
    if (sim_->break_pc_)
        return false;

    
    sim_->break_pc_ = breakpc;
    sim_->break_instr_ = breakpc->instructionBits();
    
    
    return true;
}

bool
ArmDebugger::deleteBreakpoint(SimInstruction* breakpc)
{
    if (sim_->break_pc_ != nullptr)
        sim_->break_pc_->setInstructionBits(sim_->break_instr_);

    sim_->break_pc_ = nullptr;
    sim_->break_instr_ = 0;
    return true;
}

void
ArmDebugger::undoBreakpoints()
{
    if (sim_->break_pc_)
        sim_->break_pc_->setInstructionBits(sim_->break_instr_);
}

void
ArmDebugger::redoBreakpoints()
{
    if (sim_->break_pc_)
        sim_->break_pc_->setInstructionBits(kBreakpointInstr);
}

static char*
ReadLine(const char* prompt)
{
    char* result = nullptr;
    char line_buf[256];
    int offset = 0;
    bool keep_going = true;
    fprintf(stdout, "%s", prompt);
    fflush(stdout);
    while (keep_going) {
        if (fgets(line_buf, sizeof(line_buf), stdin) == nullptr) {
            
            if (result)
                js_delete(result);
            return nullptr;
        }
        int len = strlen(line_buf);
        if (len > 0 && line_buf[len - 1] == '\n') {
            
            
            keep_going = false;
        }
        if (!result) {
            
            
            result = (char*)js_malloc(len + 1);
            if (!result)
                return nullptr;
        } else {
            
            int new_len = offset + len + 1;
            char* new_result = (char*)js_malloc(new_len);
            if (!new_result)
                return nullptr;
            
            
            memcpy(new_result, result, offset * sizeof(char));
            js_free(result);
            result = new_result;
        }
        
        memcpy(result + offset, line_buf, len * sizeof(char));
        offset += len;
    }

    MOZ_ASSERT(result);
    result[offset] = '\0';
    return result;
}


static void
DisassembleInstruction(uint32_t pc)
{
    uint8_t* bytes = reinterpret_cast<uint8_t*>(pc);
    char hexbytes[256];
    sprintf(hexbytes, "0x%x 0x%x 0x%x 0x%x", bytes[0], bytes[1], bytes[2], bytes[3]);
    char llvmcmd[1024];
    sprintf(llvmcmd, "bash -c \"echo -n '%p'; echo '%s' | "
            "llvm-mc -disassemble -arch=arm -mcpu=cortex-a9 | "
            "grep -v pure_instructions | grep -v .text\"",
            reinterpret_cast<void*>(pc), hexbytes);
    system(llvmcmd);
}

void
ArmDebugger::debug()
{
    intptr_t last_pc = -1;
    bool done = false;

#define COMMAND_SIZE 63
#define ARG_SIZE 255

#define STR(a) #a
#define XSTR(a) STR(a)

    char cmd[COMMAND_SIZE + 1];
    char arg1[ARG_SIZE + 1];
    char arg2[ARG_SIZE + 1];
    char* argv[3] = { cmd, arg1, arg2 };

    
    cmd[COMMAND_SIZE] = 0;
    arg1[ARG_SIZE] = 0;
    arg2[ARG_SIZE] = 0;

    
    
    undoBreakpoints();

    while (!done && !sim_->has_bad_pc()) {
        if (last_pc != sim_->get_pc()) {
            DisassembleInstruction(sim_->get_pc());
            last_pc = sim_->get_pc();
        }
        char* line = ReadLine("sim> ");
        if (line == nullptr) {
            break;
        } else {
            char* last_input = sim_->lastDebuggerInput();
            if (strcmp(line, "\n") == 0 && last_input != nullptr) {
                line = last_input;
            } else {
                
                sim_->setLastDebuggerInput(line);
            }

            
            
            int argc = sscanf(line,
                              "%" XSTR(COMMAND_SIZE) "s "
                              "%" XSTR(ARG_SIZE) "s "
                              "%" XSTR(ARG_SIZE) "s",
                              cmd, arg1, arg2);
            if (argc < 0) {
                continue;
            } else if ((strcmp(cmd, "si") == 0) || (strcmp(cmd, "stepi") == 0)) {
                sim_->instructionDecode(reinterpret_cast<SimInstruction*>(sim_->get_pc()));
                sim_->icount_++;
            } else if ((strcmp(cmd, "skip") == 0)) {
                sim_->set_pc(sim_->get_pc() + 4);
                sim_->icount_++;
            } else if ((strcmp(cmd, "c") == 0) || (strcmp(cmd, "cont") == 0)) {
                
                
                sim_->instructionDecode(reinterpret_cast<SimInstruction*>(sim_->get_pc()));
                sim_->icount_++;
                
                done = true;
            } else if ((strcmp(cmd, "p") == 0) || (strcmp(cmd, "print") == 0)) {
                if (argc == 2 || (argc == 3 && strcmp(arg2, "fp") == 0)) {
                    int32_t value;
                    double dvalue;
                    if (strcmp(arg1, "all") == 0) {
                        for (uint32_t i = 0; i < Registers::Total; i++) {
                            value = getRegisterValue(i);
                            printf("%3s: 0x%08x %10d", Registers::GetName(i), value, value);
                            if ((argc == 3 && strcmp(arg2, "fp") == 0) &&
                                i < 8 &&
                                (i % 2) == 0) {
                                dvalue = getRegisterPairDoubleValue(i);
                                printf(" (%f)\n", dvalue);
                            } else {
                                printf("\n");
                            }
                        }
                        for (uint32_t i = 0; i < FloatRegisters::TotalPhys; i++) {
                            dvalue = getVFPDoubleRegisterValue(i);
                            uint64_t as_words = mozilla::BitwiseCast<uint64_t>(dvalue);
                            printf("%3s: %f 0x%08x %08x\n",
                                   FloatRegister::FromCode(i).name(),
                                   dvalue,
                                   static_cast<uint32_t>(as_words >> 32),
                                   static_cast<uint32_t>(as_words & 0xffffffff));
                        }
                    } else {
                        if (getValue(arg1, &value)) {
                            printf("%s: 0x%08x %d \n", arg1, value, value);
                        } else if (getVFPDoubleValue(arg1, &dvalue)) {
                            uint64_t as_words = mozilla::BitwiseCast<uint64_t>(dvalue);
                            printf("%s: %f 0x%08x %08x\n",
                                   arg1,
                                   dvalue,
                                   static_cast<uint32_t>(as_words >> 32),
                                   static_cast<uint32_t>(as_words & 0xffffffff));
                        } else {
                            printf("%s unrecognized\n", arg1);
                        }
                    }
                } else {
                    printf("print <register>\n");
                }
            } else if (strcmp(cmd, "stack") == 0 || strcmp(cmd, "mem") == 0) {
                int32_t* cur = nullptr;
                int32_t* end = nullptr;
                int next_arg = 1;

                if (strcmp(cmd, "stack") == 0) {
                    cur = reinterpret_cast<int32_t*>(sim_->get_register(Simulator::sp));
                } else {  
                    int32_t value;
                    if (!getValue(arg1, &value)) {
                        printf("%s unrecognized\n", arg1);
                        continue;
                    }
                    cur = reinterpret_cast<int32_t*>(value);
                    next_arg++;
                }

                int32_t words;
                if (argc == next_arg) {
                    words = 10;
                } else {
                    if (!getValue(argv[next_arg], &words)) {
                        words = 10;
                    }
                }
                end = cur + words;

                while (cur < end) {
                    printf("  %p:  0x%08x %10d", cur, *cur, *cur);
                    printf("\n");
                    cur++;
                }
            } else if (strcmp(cmd, "disasm") == 0 || strcmp(cmd, "di") == 0) {
                uint8_t* cur = nullptr;
                uint8_t* end = nullptr;
                if (argc == 1) {
                    cur = reinterpret_cast<uint8_t*>(sim_->get_pc());
                    end = cur + (10 * SimInstruction::kInstrSize);
                } else if (argc == 2) {
                    Register reg = Register::FromName(arg1);
                    if (reg != InvalidReg || strncmp(arg1, "0x", 2) == 0) {
                        
                        int32_t value;
                        if (getValue(arg1, &value)) {
                            cur = reinterpret_cast<uint8_t*>(value);
                            
                            end = cur + (10 * SimInstruction::kInstrSize);
                        }
                    } else {
                        
                        int32_t value;
                        if (getValue(arg1, &value)) {
                            cur = reinterpret_cast<uint8_t*>(sim_->get_pc());
                            
                            end = cur + (value * SimInstruction::kInstrSize);
                        }
                    }
                } else {
                    int32_t value1;
                    int32_t value2;
                    if (getValue(arg1, &value1) && getValue(arg2, &value2)) {
                        cur = reinterpret_cast<uint8_t*>(value1);
                        end = cur + (value2 * SimInstruction::kInstrSize);
                    }
                }
                while (cur < end) {
                    DisassembleInstruction(uint32_t(cur));
                    cur += SimInstruction::kInstrSize;
                }
            } else if (strcmp(cmd, "gdb") == 0) {
                printf("relinquishing control to gdb\n");
                asm("int $3");
                printf("regaining control from gdb\n");
            } else if (strcmp(cmd, "break") == 0) {
                if (argc == 2) {
                    int32_t value;
                    if (getValue(arg1, &value)) {
                        if (!setBreakpoint(reinterpret_cast<SimInstruction*>(value)))
                            printf("setting breakpoint failed\n");
                    } else {
                        printf("%s unrecognized\n", arg1);
                    }
                } else {
                    printf("break <address>\n");
                }
            } else if (strcmp(cmd, "del") == 0) {
                if (!deleteBreakpoint(nullptr)) {
                    printf("deleting breakpoint failed\n");
                }
            } else if (strcmp(cmd, "flags") == 0) {
                printf("N flag: %d; ", sim_->n_flag_);
                printf("Z flag: %d; ", sim_->z_flag_);
                printf("C flag: %d; ", sim_->c_flag_);
                printf("V flag: %d\n", sim_->v_flag_);
                printf("INVALID OP flag: %d; ", sim_->inv_op_vfp_flag_);
                printf("DIV BY ZERO flag: %d; ", sim_->div_zero_vfp_flag_);
                printf("OVERFLOW flag: %d; ", sim_->overflow_vfp_flag_);
                printf("UNDERFLOW flag: %d; ", sim_->underflow_vfp_flag_);
                printf("INEXACT flag: %d;\n", sim_->inexact_vfp_flag_);
            } else if (strcmp(cmd, "stop") == 0) {
                int32_t value;
                intptr_t stop_pc = sim_->get_pc() - 2 * SimInstruction::kInstrSize;
                SimInstruction* stop_instr = reinterpret_cast<SimInstruction*>(stop_pc);
                SimInstruction* msg_address =
                    reinterpret_cast<SimInstruction*>(stop_pc + SimInstruction::kInstrSize);
                if ((argc == 2) && (strcmp(arg1, "unstop") == 0)) {
                    
                    if (sim_->isStopInstruction(stop_instr)) {
                        stop_instr->setInstructionBits(kNopInstr);
                        msg_address->setInstructionBits(kNopInstr);
                    } else {
                        printf("Not at debugger stop.\n");
                    }
                } else if (argc == 3) {
                    
                    if (strcmp(arg1, "info") == 0) {
                        if (strcmp(arg2, "all") == 0) {
                            printf("Stop information:\n");
                            for (uint32_t i = 0; i < sim_->kNumOfWatchedStops; i++)
                                sim_->printStopInfo(i);
                        } else if (getValue(arg2, &value)) {
                            sim_->printStopInfo(value);
                        } else {
                            printf("Unrecognized argument.\n");
                        }
                    } else if (strcmp(arg1, "enable") == 0) {
                        
                        if (strcmp(arg2, "all") == 0) {
                            for (uint32_t i = 0; i < sim_->kNumOfWatchedStops; i++)
                                sim_->enableStop(i);
                        } else if (getValue(arg2, &value)) {
                            sim_->enableStop(value);
                        } else {
                            printf("Unrecognized argument.\n");
                        }
                    } else if (strcmp(arg1, "disable") == 0) {
                        
                        if (strcmp(arg2, "all") == 0) {
                            for (uint32_t i = 0; i < sim_->kNumOfWatchedStops; i++) {
                                sim_->disableStop(i);
                            }
                        } else if (getValue(arg2, &value)) {
                            sim_->disableStop(value);
                        } else {
                            printf("Unrecognized argument.\n");
                        }
                    }
                } else {
                    printf("Wrong usage. Use help command for more information.\n");
                }
            } else if ((strcmp(cmd, "h") == 0) || (strcmp(cmd, "help") == 0)) {
                printf("cont\n");
                printf("  continue execution (alias 'c')\n");
                printf("skip\n");
                printf("  skip one instruction (set pc to next instruction)\n");
                printf("stepi\n");
                printf("  step one instruction (alias 'si')\n");
                printf("print <register>\n");
                printf("  print register content (alias 'p')\n");
                printf("  use register name 'all' to print all registers\n");
                printf("  add argument 'fp' to print register pair double values\n");
                printf("flags\n");
                printf("  print flags\n");
                printf("stack [<words>]\n");
                printf("  dump stack content, default dump 10 words)\n");
                printf("mem <address> [<words>]\n");
                printf("  dump memory content, default dump 10 words)\n");
                printf("disasm [<instructions>]\n");
                printf("disasm [<address/register>]\n");
                printf("disasm [[<address/register>] <instructions>]\n");
                printf("  disassemble code, default is 10 instructions\n");
                printf("  from pc (alias 'di')\n");
                printf("gdb\n");
                printf("  enter gdb\n");
                printf("break <address>\n");
                printf("  set a break point on the address\n");
                printf("del\n");
                printf("  delete the breakpoint\n");
                printf("stop feature:\n");
                printf("  Description:\n");
                printf("    Stops are debug instructions inserted by\n");
                printf("    the Assembler::stop() function.\n");
                printf("    When hitting a stop, the Simulator will\n");
                printf("    stop and and give control to the ArmDebugger.\n");
                printf("    The first %d stop codes are watched:\n",
                       Simulator::kNumOfWatchedStops);
                printf("    - They can be enabled / disabled: the Simulator\n");
                printf("      will / won't stop when hitting them.\n");
                printf("    - The Simulator keeps track of how many times they \n");
                printf("      are met. (See the info command.) Going over a\n");
                printf("      disabled stop still increases its counter. \n");
                printf("  Commands:\n");
                printf("    stop info all/<code> : print infos about number <code>\n");
                printf("      or all stop(s).\n");
                printf("    stop enable/disable all/<code> : enables / disables\n");
                printf("      all or number <code> stop(s)\n");
                printf("    stop unstop\n");
                printf("      ignore the stop instruction at the current location\n");
                printf("      from now on\n");
            } else {
                printf("Unknown command: %s\n", cmd);
            }
        }
    }

    
    
    redoBreakpoints();

#undef COMMAND_SIZE
#undef ARG_SIZE

#undef STR
#undef XSTR
}

static bool
AllOnOnePage(uintptr_t start, int size)
{
    intptr_t start_page = (start & ~CachePage::kPageMask);
    intptr_t end_page = ((start + size) & ~CachePage::kPageMask);
    return start_page == end_page;
}

static CachePage*
GetCachePageLocked(Simulator::ICacheMap& i_cache, void* page)
{
    MOZ_ASSERT(Simulator::ICacheCheckingEnabled);

    Simulator::ICacheMap::AddPtr p = i_cache.lookupForAdd(page);
    if (p)
        return p->value();

    CachePage* new_page = js_new<CachePage>();
    if (!i_cache.add(p, page, new_page))
        return nullptr;
    return new_page;
}


static void
FlushOnePageLocked(Simulator::ICacheMap& i_cache, intptr_t start, int size)
{
    MOZ_ASSERT(size <= CachePage::kPageSize);
    MOZ_ASSERT(AllOnOnePage(start, size - 1));
    MOZ_ASSERT((start & CachePage::kLineMask) == 0);
    MOZ_ASSERT((size & CachePage::kLineMask) == 0);

    void* page = reinterpret_cast<void*>(start & (~CachePage::kPageMask));
    int offset = (start & CachePage::kPageMask);
    CachePage* cache_page = GetCachePageLocked(i_cache, page);
    char* valid_bytemap = cache_page->validityByte(offset);
    memset(valid_bytemap, CachePage::LINE_INVALID, size >> CachePage::kLineShift);
}

static void
FlushICacheLocked(Simulator::ICacheMap& i_cache, void* start_addr, size_t size)
{
    intptr_t start = reinterpret_cast<intptr_t>(start_addr);
    int intra_line = (start & CachePage::kLineMask);
    start -= intra_line;
    size += intra_line;
    size = ((size - 1) | CachePage::kLineMask) + 1;
    int offset = (start & CachePage::kPageMask);
    while (!AllOnOnePage(start, size - 1)) {
        int bytes_to_flush = CachePage::kPageSize - offset;
        FlushOnePageLocked(i_cache, start, bytes_to_flush);
        start += bytes_to_flush;
        size -= bytes_to_flush;
        MOZ_ASSERT((start & CachePage::kPageMask) == 0);
        offset = 0;
    }
    if (size != 0)
        FlushOnePageLocked(i_cache, start, size);
}

static void
CheckICacheLocked(Simulator::ICacheMap& i_cache, SimInstruction* instr)
{
    intptr_t address = reinterpret_cast<intptr_t>(instr);
    void* page = reinterpret_cast<void*>(address & (~CachePage::kPageMask));
    void* line = reinterpret_cast<void*>(address & (~CachePage::kLineMask));
    int offset = (address & CachePage::kPageMask);
    CachePage* cache_page = GetCachePageLocked(i_cache, page);
    char* cache_valid_byte = cache_page->validityByte(offset);
    bool cache_hit = (*cache_valid_byte == CachePage::LINE_VALID);
    char* cached_line = cache_page->cachedData(offset & ~CachePage::kLineMask);
    if (cache_hit) {
        
        MOZ_ASSERT(memcmp(reinterpret_cast<void*>(instr),
                          cache_page->cachedData(offset),
                          SimInstruction::kInstrSize) == 0);
    } else {
        
        memcpy(cached_line, line, CachePage::kLineLength);
        *cache_valid_byte = CachePage::LINE_VALID;
    }
}

HashNumber
Simulator::ICacheHasher::hash(const Lookup& l)
{
    return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(l)) >> 2;
}

bool
Simulator::ICacheHasher::match(const Key& k, const Lookup& l)
{
    MOZ_ASSERT((reinterpret_cast<intptr_t>(k) & CachePage::kPageMask) == 0);
    MOZ_ASSERT((reinterpret_cast<intptr_t>(l) & CachePage::kPageMask) == 0);
    return k == l;
}

void
Simulator::setLastDebuggerInput(char* input)
{
    js_free(lastDebuggerInput_);
    lastDebuggerInput_ = input;
}

void
Simulator::FlushICache(void* start_addr, size_t size)
{
    JitSpewCont(JitSpew_CacheFlush, "[%p %" PRIxSIZE "]", start_addr, size);
    if (Simulator::ICacheCheckingEnabled) {
        Simulator* sim = Simulator::Current();

        AutoLockSimulatorCache als(sim);

        js::jit::FlushICacheLocked(sim->icache(), start_addr, size);
    }
}

Simulator::Simulator()
{
    
    

    
    

    stack_ = nullptr;
    stackLimit_ = 0;
    pc_modified_ = false;
    icount_ = 0L;
    resume_pc_ = 0;
    break_pc_ = nullptr;
    break_instr_ = 0;
    single_stepping_ = false;
    single_step_callback_ = nullptr;
    single_step_callback_arg_ = nullptr;
    skipCalleeSavedRegsCheck = false;

    
    
    for (int i = 0; i < num_registers; i++)
        registers_[i] = 0;

    n_flag_ = false;
    z_flag_ = false;
    c_flag_ = false;
    v_flag_ = false;

    for (int i = 0; i < num_d_registers * 2; i++)
        vfp_registers_[i] = 0;

    n_flag_FPSCR_ = false;
    z_flag_FPSCR_ = false;
    c_flag_FPSCR_ = false;
    v_flag_FPSCR_ = false;
    FPSCR_rounding_mode_ = SimRZ;
    FPSCR_default_NaN_mode_ = true;

    inv_op_vfp_flag_ = false;
    div_zero_vfp_flag_ = false;
    overflow_vfp_flag_ = false;
    underflow_vfp_flag_ = false;
    inexact_vfp_flag_ = false;

    
    
    registers_[pc] = bad_lr;
    registers_[lr] = bad_lr;

    lastDebuggerInput_ = nullptr;

    cacheLock_ = nullptr;
#ifdef DEBUG
    cacheLockHolder_ = nullptr;
#endif
    redirection_ = nullptr;
}

bool
Simulator::init()
{
    cacheLock_ = PR_NewLock();
    if (!cacheLock_)
        return false;

    if (!icache_.init())
        return false;

    
    static const size_t stackSize = 2 * 1024*1024;
    stack_ = reinterpret_cast<char*>(js_malloc(stackSize));
    if (!stack_)
        return false;

    
    
    stackLimit_ = reinterpret_cast<uintptr_t>(stack_) + 1024 * 1024;

    
    
    
    registers_[sp] = reinterpret_cast<int32_t>(stack_) + stackSize - 64;

    return true;
}







class Redirection
{
    friend class Simulator;

    
    Redirection(void* nativeFunction, ABIFunctionType type, Simulator* sim)
      : nativeFunction_(nativeFunction),
        swiInstruction_(Assembler::AL | (0xf * (1 << 24)) | kCallRtRedirected),
        type_(type),
        next_(nullptr)
    {
        next_ = sim->redirection();
        if (Simulator::ICacheCheckingEnabled)
            FlushICacheLocked(sim->icache(), addressOfSwiInstruction(), SimInstruction::kInstrSize);
        sim->setRedirection(this);
    }

  public:
    void* addressOfSwiInstruction() { return &swiInstruction_; }
    void* nativeFunction() const { return nativeFunction_; }
    ABIFunctionType type() const { return type_; }

    static Redirection* Get(void* nativeFunction, ABIFunctionType type) {
        Simulator* sim = Simulator::Current();

        AutoLockSimulatorCache als(sim);

        Redirection* current = sim->redirection();
        for (; current != nullptr; current = current->next_) {
            if (current->nativeFunction_ == nativeFunction) {
                MOZ_ASSERT(current->type() == type);
                return current;
            }
        }

        Redirection* redir = (Redirection*)js_malloc(sizeof(Redirection));
        if (!redir) {
            MOZ_ReportAssertionFailure("[unhandlable oom] Simulator redirection",
                                       __FILE__, __LINE__);
            MOZ_CRASH();
        }
        new(redir) Redirection(nativeFunction, type, sim);
        return redir;
    }

    static Redirection* FromSwiInstruction(SimInstruction* swiInstruction) {
        uint8_t* addrOfSwi = reinterpret_cast<uint8_t*>(swiInstruction);
        uint8_t* addrOfRedirection = addrOfSwi - offsetof(Redirection, swiInstruction_);
        return reinterpret_cast<Redirection*>(addrOfRedirection);
    }

  private:
    void* nativeFunction_;
    uint32_t swiInstruction_;
    ABIFunctionType type_;
    Redirection* next_;
};

Simulator::~Simulator()
{
    js_free(stack_);
    PR_DestroyLock(cacheLock_);
    Redirection* r = redirection_;
    while (r) {
        Redirection* next = r->next_;
        js_delete(r);
        r = next;
    }
}

 void*
Simulator::RedirectNativeFunction(void* nativeFunction, ABIFunctionType type)
{
    Redirection* redirection = Redirection::Get(nativeFunction, type);
    return redirection->addressOfSwiInstruction();
}



void
Simulator::set_register(int reg, int32_t value)
{
    MOZ_ASSERT(reg >= 0 && reg < num_registers);
    if (reg == pc)
        pc_modified_ = true;
    registers_[reg] = value;
}



int32_t
Simulator::get_register(int reg) const
{
    MOZ_ASSERT(reg >= 0 && reg < num_registers);
    
    if (reg >= num_registers) return 0;
    return registers_[reg] + ((reg == pc) ? SimInstruction::kPCReadOffset : 0);
}

double
Simulator::get_double_from_register_pair(int reg)
{
    MOZ_ASSERT(reg >= 0 && reg < num_registers && (reg % 2) == 0);

    
    
    double dm_val = 0.0;
    char buffer[2 * sizeof(vfp_registers_[0])];
    memcpy(buffer, &registers_[reg], 2 * sizeof(registers_[0]));
    memcpy(&dm_val, buffer, 2 * sizeof(registers_[0]));
    return dm_val;
}

void
Simulator::set_register_pair_from_double(int reg, double* value)
{
    MOZ_ASSERT(reg >= 0 && reg < num_registers && (reg % 2) == 0);
    memcpy(registers_ + reg, value, sizeof(*value));
}

void
Simulator::set_dw_register(int dreg, const int* dbl)
{
    MOZ_ASSERT(dreg >= 0 && dreg < num_d_registers);
    registers_[dreg] = dbl[0];
    registers_[dreg + 1] = dbl[1];
}

void
Simulator::get_d_register(int dreg, uint64_t* value)
{
    MOZ_ASSERT(dreg >= 0 && dreg < int(FloatRegisters::TotalPhys));
    memcpy(value, vfp_registers_ + dreg * 2, sizeof(*value));
}

void
Simulator::set_d_register(int dreg, const uint64_t* value)
{
    MOZ_ASSERT(dreg >= 0 && dreg < int(FloatRegisters::TotalPhys));
    memcpy(vfp_registers_ + dreg * 2, value, sizeof(*value));
}

void
Simulator::get_d_register(int dreg, uint32_t* value)
{
    MOZ_ASSERT(dreg >= 0 && dreg < int(FloatRegisters::TotalPhys));
    memcpy(value, vfp_registers_ + dreg * 2, sizeof(*value) * 2);
}

void
Simulator::set_d_register(int dreg, const uint32_t* value)
{
    MOZ_ASSERT(dreg >= 0 && dreg < int(FloatRegisters::TotalPhys));
    memcpy(vfp_registers_ + dreg * 2, value, sizeof(*value) * 2);
}

void
Simulator::get_q_register(int qreg, uint64_t* value)
{
    MOZ_ASSERT(qreg >= 0 && qreg < num_q_registers);
    memcpy(value, vfp_registers_ + qreg * 4, sizeof(*value) * 2);
}

void
Simulator::set_q_register(int qreg, const uint64_t* value)
{
    MOZ_ASSERT(qreg >= 0 && qreg < num_q_registers);
    memcpy(vfp_registers_ + qreg * 4, value, sizeof(*value) * 2);
}

void
Simulator::get_q_register(int qreg, uint32_t* value)
{
    MOZ_ASSERT(qreg >= 0 && qreg < num_q_registers);
    memcpy(value, vfp_registers_ + qreg * 4, sizeof(*value) * 4);
}

void
Simulator::set_q_register(int qreg, const uint32_t* value)
{
    MOZ_ASSERT((qreg >= 0) && (qreg < num_q_registers));
    memcpy(vfp_registers_ + qreg * 4, value, sizeof(*value) * 4);
}

void
Simulator::set_pc(int32_t value)
{
    pc_modified_ = true;
    registers_[pc] = value;
}

bool
Simulator::has_bad_pc() const
{
    return registers_[pc] == bad_lr || registers_[pc] == end_sim_pc;
}


int32_t
Simulator::get_pc() const
{
    return registers_[pc];
}

void
Simulator::set_s_register(int sreg, unsigned int value)
{
    MOZ_ASSERT(sreg >= 0 && sreg < num_s_registers);
    vfp_registers_[sreg] = value;
}

unsigned
Simulator::get_s_register(int sreg) const
{
    MOZ_ASSERT(sreg >= 0 && sreg < num_s_registers);
    return vfp_registers_[sreg];
}

template<class InputType, int register_size>
void
Simulator::setVFPRegister(int reg_index, const InputType& value)
{
    MOZ_ASSERT(reg_index >= 0);
    MOZ_ASSERT_IF(register_size == 1, reg_index < num_s_registers);
    MOZ_ASSERT_IF(register_size == 2, reg_index < int(FloatRegisters::TotalPhys));

    char buffer[register_size * sizeof(vfp_registers_[0])];
    memcpy(buffer, &value, register_size * sizeof(vfp_registers_[0]));
    memcpy(&vfp_registers_[reg_index * register_size], buffer,
           register_size * sizeof(vfp_registers_[0]));
}

template<class ReturnType, int register_size>
ReturnType Simulator::getFromVFPRegister(int reg_index)
{
    MOZ_ASSERT(reg_index >= 0);
    MOZ_ASSERT_IF(register_size == 1, reg_index < num_s_registers);
    MOZ_ASSERT_IF(register_size == 2, reg_index < int(FloatRegisters::TotalPhys));

    ReturnType value = 0;
    char buffer[register_size * sizeof(vfp_registers_[0])];
    memcpy(buffer, &vfp_registers_[register_size * reg_index],
           register_size * sizeof(vfp_registers_[0]));
    memcpy(&value, buffer, register_size * sizeof(vfp_registers_[0]));
    return value;
}



template double Simulator::getFromVFPRegister<double, 2>(int reg_index);
template float Simulator::getFromVFPRegister<float, 1>(int reg_index);
template void Simulator::setVFPRegister<double, 2>(int reg_index, const double& value);
template void Simulator::setVFPRegister<float, 1>(int reg_index, const float& value);

void
Simulator::getFpArgs(double* x, double* y, int32_t* z)
{
    if (UseHardFpABI()) {
        *x = get_double_from_d_register(0);
        *y = get_double_from_d_register(1);
        *z = get_register(0);
    } else {
        *x = get_double_from_register_pair(0);
        *y = get_double_from_register_pair(2);
        *z = get_register(2);
    }
}

void
Simulator::getFpFromStack(int32_t* stack, double* x)
{
    MOZ_ASSERT(stack && x);
    char buffer[2 * sizeof(stack[0])];
    memcpy(buffer, stack, 2 * sizeof(stack[0]));
    memcpy(x, buffer, 2 * sizeof(stack[0]));
}

void
Simulator::setCallResultDouble(double result)
{
    
    if (UseHardFpABI()) {
        char buffer[2 * sizeof(vfp_registers_[0])];
        memcpy(buffer, &result, sizeof(buffer));
        
        memcpy(vfp_registers_, buffer, sizeof(buffer));
    } else {
        char buffer[2 * sizeof(registers_[0])];
        memcpy(buffer, &result, sizeof(buffer));
        
        memcpy(registers_, buffer, sizeof(buffer));
    }
}

void
Simulator::setCallResultFloat(float result)
{
    if (UseHardFpABI()) {
        char buffer[sizeof(registers_[0])];
        memcpy(buffer, &result, sizeof(buffer));
        
        memcpy(vfp_registers_, buffer, sizeof(buffer));
    } else {
        char buffer[sizeof(registers_[0])];
        memcpy(buffer, &result, sizeof(buffer));
        
        memcpy(registers_, buffer, sizeof(buffer));
    }
}

void
Simulator::setCallResult(int64_t res)
{
    set_register(r0, static_cast<int32_t>(res));
    set_register(r1, static_cast<int32_t>(res >> 32));
}

int
Simulator::readW(int32_t addr, SimInstruction* instr)
{
    
    
    if ((addr & 3) == 0 || !HasAlignmentFault()) {
        intptr_t* ptr = reinterpret_cast<intptr_t*>(addr);
        return *ptr;
    } else {
        printf("Unaligned write at 0x%08x, pc=%p\n", addr, instr);
        MOZ_CRASH();
    }
}

void
Simulator::writeW(int32_t addr, int value, SimInstruction* instr)
{
    if ((addr & 3) == 0) {
        intptr_t* ptr = reinterpret_cast<intptr_t*>(addr);
        *ptr = value;
    } else {
        printf("Unaligned write at 0x%08x, pc=%p\n", addr, instr);
        MOZ_CRASH();
    }
}

uint16_t
Simulator::readHU(int32_t addr, SimInstruction* instr)
{
    
    
    if ((addr & 1) == 0 || !HasAlignmentFault()) {
       uint16_t* ptr = reinterpret_cast<uint16_t*>(addr);
       return *ptr;
    }
    printf("Unaligned unsigned halfword read at 0x%08x, pc=%p\n", addr, instr);
    MOZ_CRASH();
    return 0;
}

int16_t
Simulator::readH(int32_t addr, SimInstruction* instr)
{
    if ((addr & 1) == 0) {
        int16_t* ptr = reinterpret_cast<int16_t*>(addr);
        return *ptr;
    }
    printf("Unaligned signed halfword read at 0x%08x\n", addr);
    MOZ_CRASH();
    return 0;
}

void
Simulator::writeH(int32_t addr, uint16_t value, SimInstruction* instr)
{
    if ((addr & 1) == 0) {
        uint16_t* ptr = reinterpret_cast<uint16_t*>(addr);
        *ptr = value;
    } else {
        printf("Unaligned unsigned halfword write at 0x%08x, pc=%p\n", addr, instr);
        MOZ_CRASH();
    }
}

void
Simulator::writeH(int32_t addr, int16_t value, SimInstruction* instr)
{
    if ((addr & 1) == 0) {
        int16_t* ptr = reinterpret_cast<int16_t*>(addr);
        *ptr = value;
    } else {
        printf("Unaligned halfword write at 0x%08x, pc=%p\n", addr, instr);
        MOZ_CRASH();
    }
}

uint8_t
Simulator::readBU(int32_t addr)
{
    uint8_t* ptr = reinterpret_cast<uint8_t*>(addr);
    return *ptr;
}

int8_t
Simulator::readB(int32_t addr)
{
    int8_t* ptr = reinterpret_cast<int8_t*>(addr);
    return *ptr;
}

void
Simulator::writeB(int32_t addr, uint8_t value)
{
    uint8_t* ptr = reinterpret_cast<uint8_t*>(addr);
    *ptr = value;
}

void
Simulator::writeB(int32_t addr, int8_t value)
{
    int8_t* ptr = reinterpret_cast<int8_t*>(addr);
    *ptr = value;
}

int32_t*
Simulator::readDW(int32_t addr)
{
    if ((addr & 3) == 0) {
        int32_t* ptr = reinterpret_cast<int32_t*>(addr);
        return ptr;
    }
    printf("Unaligned read at 0x%08x\n", addr);
    MOZ_CRASH();
    return 0;
}

void
Simulator::writeDW(int32_t addr, int32_t value1, int32_t value2)
{
    if ((addr & 3) == 0) {
        int32_t* ptr = reinterpret_cast<int32_t*>(addr);
        *ptr++ = value1;
        *ptr = value2;
    } else {
        printf("Unaligned write at 0x%08x\n", addr);
        MOZ_CRASH();
    }
}

uintptr_t
Simulator::stackLimit() const
{
    return stackLimit_;
}

uintptr_t*
Simulator::addressOfStackLimit()
{
    return &stackLimit_;
}

bool
Simulator::overRecursed(uintptr_t newsp) const
{
    if (newsp == 0)
        newsp = get_register(sp);
    return newsp <= stackLimit();
}

bool
Simulator::overRecursedWithExtra(uint32_t extra) const
{
    uintptr_t newsp = get_register(sp) - extra;
    return newsp <= stackLimit();
}



bool
Simulator::conditionallyExecute(SimInstruction* instr)
{
    switch (instr->conditionField()) {
      case Assembler::EQ: return z_flag_;
      case Assembler::NE: return !z_flag_;
      case Assembler::CS: return c_flag_;
      case Assembler::CC: return !c_flag_;
      case Assembler::MI: return n_flag_;
      case Assembler::PL: return !n_flag_;
      case Assembler::VS: return v_flag_;
      case Assembler::VC: return !v_flag_;
      case Assembler::HI: return c_flag_ && !z_flag_;
      case Assembler::LS: return !c_flag_ || z_flag_;
      case Assembler::GE: return n_flag_ == v_flag_;
      case Assembler::LT: return n_flag_ != v_flag_;
      case Assembler::GT: return !z_flag_ && (n_flag_ == v_flag_);
      case Assembler::LE: return z_flag_ || (n_flag_ != v_flag_);
      case Assembler::AL: return true;
      default: MOZ_CRASH();
    }
    return false;
}


void
Simulator::setNZFlags(int32_t val)
{
    n_flag_ = (val < 0);
    z_flag_ = (val == 0);
}


void
Simulator::setCFlag(bool val)
{
    c_flag_ = val;
}


void
Simulator::setVFlag(bool val)
{
    v_flag_ = val;
}


bool
Simulator::carryFrom(int32_t left, int32_t right, int32_t carry)
{
    uint32_t uleft = static_cast<uint32_t>(left);
    uint32_t uright = static_cast<uint32_t>(right);
    uint32_t urest  = 0xffffffffU - uleft;
    return (uright > urest) ||
           (carry && (((uright + 1) > urest) || (uright > (urest - 1))));
}


bool
Simulator::borrowFrom(int32_t left, int32_t right)
{
    uint32_t uleft = static_cast<uint32_t>(left);
    uint32_t uright = static_cast<uint32_t>(right);
    return (uright > uleft);
}


bool
Simulator::overflowFrom(int32_t alu_out, int32_t left, int32_t right, bool addition)
{
    bool overflow;
    if (addition) {
        
        overflow = ((left >= 0 && right >= 0) || (left < 0 && right < 0))
            
            && ((left < 0 && alu_out >= 0) || (left >= 0 && alu_out < 0));
    } else {
        
        overflow = ((left < 0 && right >= 0) || (left >= 0 && right < 0))
            
            && ((left < 0 && alu_out >= 0) || (left >= 0 && alu_out < 0));
    }
    return overflow;
}


void
Simulator::compute_FPSCR_Flags(double val1, double val2)
{
    if (mozilla::IsNaN(val1) || mozilla::IsNaN(val2)) {
        n_flag_FPSCR_ = false;
        z_flag_FPSCR_ = false;
        c_flag_FPSCR_ = true;
        v_flag_FPSCR_ = true;
        
    } else if (val1 == val2) {
        n_flag_FPSCR_ = false;
        z_flag_FPSCR_ = true;
        c_flag_FPSCR_ = true;
        v_flag_FPSCR_ = false;
    } else if (val1 < val2) {
        n_flag_FPSCR_ = true;
        z_flag_FPSCR_ = false;
        c_flag_FPSCR_ = false;
        v_flag_FPSCR_ = false;
    } else {
        
        n_flag_FPSCR_ = false;
        z_flag_FPSCR_ = false;
        c_flag_FPSCR_ = true;
        v_flag_FPSCR_ = false;
    }
}

void
Simulator::copy_FPSCR_to_APSR()
{
    n_flag_ = n_flag_FPSCR_;
    z_flag_ = z_flag_FPSCR_;
    c_flag_ = c_flag_FPSCR_;
    v_flag_ = v_flag_FPSCR_;
}



int32_t
Simulator::getShiftRm(SimInstruction* instr, bool* carry_out)
{
    ShiftType shift = instr->shifttypeValue();
    int shift_amount = instr->shiftAmountValue();
    int32_t result = get_register(instr->rmValue());
    if (instr->bit(4) == 0) {
        
        if (shift == ROR && shift_amount == 0) {
            MOZ_CRASH("NYI");
            return result;
        }
        if ((shift == LSR || shift == ASR) && shift_amount == 0)
            shift_amount = 32;
        switch (shift) {
          case ASR: {
            if (shift_amount == 0) {
                if (result < 0) {
                    result = 0xffffffff;
                    *carry_out = true;
                } else {
                    result = 0;
                    *carry_out = false;
                }
            } else {
                result >>= (shift_amount - 1);
                *carry_out = (result & 1) == 1;
                result >>= 1;
            }
            break;
          }

          case LSL: {
            if (shift_amount == 0) {
                *carry_out = c_flag_;
            } else {
                result <<= (shift_amount - 1);
                *carry_out = (result < 0);
                result <<= 1;
            }
            break;
          }

          case LSR: {
            if (shift_amount == 0) {
                result = 0;
                *carry_out = c_flag_;
            } else {
                uint32_t uresult = static_cast<uint32_t>(result);
                uresult >>= (shift_amount - 1);
                *carry_out = (uresult & 1) == 1;
                uresult >>= 1;
                result = static_cast<int32_t>(uresult);
            }
            break;
          }

          case ROR: {
            if (shift_amount == 0) {
                *carry_out = c_flag_;
            } else {
                uint32_t left = static_cast<uint32_t>(result) >> shift_amount;
                uint32_t right = static_cast<uint32_t>(result) << (32 - shift_amount);
                result = right | left;
                *carry_out = (static_cast<uint32_t>(result) >> 31) != 0;
            }
            break;
          }

          default:
            MOZ_CRASH();
        }
    } else {
        
        int rs = instr->rsValue();
        shift_amount = get_register(rs) &0xff;
        switch (shift) {
          case ASR: {
            if (shift_amount == 0) {
                *carry_out = c_flag_;
            } else if (shift_amount < 32) {
                result >>= (shift_amount - 1);
                *carry_out = (result & 1) == 1;
                result >>= 1;
            } else {
                MOZ_ASSERT(shift_amount >= 32);
                if (result < 0) {
                    *carry_out = true;
                    result = 0xffffffff;
                } else {
                    *carry_out = false;
                    result = 0;
                }
            }
            break;
          }

          case LSL: {
            if (shift_amount == 0) {
                *carry_out = c_flag_;
            } else if (shift_amount < 32) {
                result <<= (shift_amount - 1);
                *carry_out = (result < 0);
                result <<= 1;
            } else if (shift_amount == 32) {
                *carry_out = (result & 1) == 1;
                result = 0;
            } else {
                MOZ_ASSERT(shift_amount > 32);
                *carry_out = false;
                result = 0;
            }
            break;
          }

          case LSR: {
            if (shift_amount == 0) {
                *carry_out = c_flag_;
            } else if (shift_amount < 32) {
                uint32_t uresult = static_cast<uint32_t>(result);
                uresult >>= (shift_amount - 1);
                *carry_out = (uresult & 1) == 1;
                uresult >>= 1;
                result = static_cast<int32_t>(uresult);
            } else if (shift_amount == 32) {
                *carry_out = (result < 0);
                result = 0;
            } else {
                *carry_out = false;
                result = 0;
            }
            break;
          }

          case ROR: {
            if (shift_amount == 0) {
                *carry_out = c_flag_;
            } else {
                uint32_t left = static_cast<uint32_t>(result) >> shift_amount;
                uint32_t right = static_cast<uint32_t>(result) << (32 - shift_amount);
                result = right | left;
                *carry_out = (static_cast<uint32_t>(result) >> 31) != 0;
            }
            break;
          }

          default:
            MOZ_CRASH();
        }
    }
    return result;
}



int32_t
Simulator::getImm(SimInstruction* instr, bool* carry_out)
{
    int rotate = instr->rotateValue() * 2;
    int immed8 = instr->immed8Value();
    int imm = (immed8 >> rotate) | (immed8 << (32 - rotate));
    *carry_out = (rotate == 0) ? c_flag_ : (imm < 0);
    return imm;
}

int32_t
Simulator::processPU(SimInstruction* instr, int num_regs, int reg_size,
                     intptr_t* start_address, intptr_t* end_address)
{
    int rn = instr->rnValue();
    int32_t rn_val = get_register(rn);
    switch (instr->PUField()) {
      case da_x:
        MOZ_CRASH();
        break;
      case ia_x:
        *start_address = rn_val;
        *end_address = rn_val + (num_regs * reg_size) - reg_size;
        rn_val = rn_val + (num_regs * reg_size);
        break;
      case db_x:
        *start_address = rn_val - (num_regs * reg_size);
        *end_address = rn_val - reg_size;
        rn_val = *start_address;
        break;
      case ib_x:
        *start_address = rn_val + reg_size;
        *end_address = rn_val + (num_regs * reg_size);
        rn_val = *end_address;
        break;
      default:
        MOZ_CRASH();
    }
    return rn_val;
}


void
Simulator::handleRList(SimInstruction* instr, bool load)
{
    int rlist = instr->rlistValue();
    int num_regs = mozilla::CountPopulation32(rlist);

    intptr_t start_address = 0;
    intptr_t end_address = 0;
    int32_t rn_val = processPU(instr, num_regs, sizeof(void*), &start_address, &end_address);
    intptr_t* address = reinterpret_cast<intptr_t*>(start_address);

    
    MOZ_ASSERT(start_address > 8191 || start_address < 0);

    int reg = 0;
    while (rlist != 0) {
        if ((rlist & 1) != 0) {
            if (load) {
                set_register(reg, *address);
            } else {
                *address = get_register(reg);
            }
            address += 1;
        }
        reg++;
        rlist >>= 1;
    }
    MOZ_ASSERT(end_address == ((intptr_t)address) - 4);
    if (instr->hasW())
        set_register(instr->rnValue(), rn_val);
}


void
Simulator::handleVList(SimInstruction* instr)
{
    VFPRegPrecision precision = (instr->szValue() == 0) ? kSinglePrecision : kDoublePrecision;
    int operand_size = (precision == kSinglePrecision) ? 4 : 8;
    bool load = (instr->VLValue() == 0x1);

    int vd;
    int num_regs;
    vd = instr->VFPDRegValue(precision);
    if (precision == kSinglePrecision)
        num_regs = instr->immed8Value();
    else
        num_regs = instr->immed8Value() / 2;

    intptr_t start_address = 0;
    intptr_t end_address = 0;
    int32_t rn_val = processPU(instr, num_regs, operand_size, &start_address, &end_address);

    intptr_t* address = reinterpret_cast<intptr_t*>(start_address);
    for (int reg = vd; reg < vd + num_regs; reg++) {
        if (precision == kSinglePrecision) {
            if (load)
                set_s_register_from_sinteger(reg, readW(reinterpret_cast<int32_t>(address), instr));
            else
                writeW(reinterpret_cast<int32_t>(address), get_sinteger_from_s_register(reg), instr);
            address += 1;
        } else {
            if (load) {
                int32_t data[] = {
                    readW(reinterpret_cast<int32_t>(address), instr),
                    readW(reinterpret_cast<int32_t>(address + 1), instr)
                };
                double d;
                memcpy(&d, data, 8);
                set_d_register_from_double(reg, d);
            } else {
                int32_t data[2];
                double d = get_double_from_d_register(reg);
                memcpy(data, &d, 8);
                writeW(reinterpret_cast<int32_t>(address), data[0], instr);
                writeW(reinterpret_cast<int32_t>(address + 1), data[1], instr);
            }
            address += 2;
        }
    }
    MOZ_ASSERT(reinterpret_cast<intptr_t>(address) - operand_size == end_address);
    if (instr->hasW())
        set_register(instr->rnValue(), rn_val);
}





typedef int64_t (*Prototype_General0)();
typedef int64_t (*Prototype_General1)(int32_t arg0);
typedef int64_t (*Prototype_General2)(int32_t arg0, int32_t arg1);
typedef int64_t (*Prototype_General3)(int32_t arg0, int32_t arg1, int32_t arg2);
typedef int64_t (*Prototype_General4)(int32_t arg0, int32_t arg1, int32_t arg2, int32_t arg3);
typedef int64_t (*Prototype_General5)(int32_t arg0, int32_t arg1, int32_t arg2, int32_t arg3,
                                      int32_t arg4);
typedef int64_t (*Prototype_General6)(int32_t arg0, int32_t arg1, int32_t arg2, int32_t arg3,
                                      int32_t arg4, int32_t arg5);
typedef int64_t (*Prototype_General7)(int32_t arg0, int32_t arg1, int32_t arg2, int32_t arg3,
                                      int32_t arg4, int32_t arg5, int32_t arg6);
typedef int64_t (*Prototype_General8)(int32_t arg0, int32_t arg1, int32_t arg2, int32_t arg3,
                                      int32_t arg4, int32_t arg5, int32_t arg6, int32_t arg7);

typedef double (*Prototype_Double_None)();
typedef double (*Prototype_Double_Double)(double arg0);
typedef double (*Prototype_Double_Int)(int32_t arg0);
typedef int32_t (*Prototype_Int_Double)(double arg0);
typedef float (*Prototype_Float32_Float32)(float arg0);

typedef double (*Prototype_DoubleInt)(double arg0, int32_t arg1);
typedef double (*Prototype_Double_IntDouble)(int32_t arg0, double arg1);
typedef double (*Prototype_Double_DoubleDouble)(double arg0, double arg1);
typedef int32_t (*Prototype_Int_IntDouble)(int32_t arg0, double arg1);

typedef double (*Prototype_Double_DoubleDoubleDouble)(double arg0, double arg1, double arg2);
typedef double (*Prototype_Double_DoubleDoubleDoubleDouble)(double arg0, double arg1,
                                                            double arg2, double arg3);









void
Simulator::scratchVolatileRegisters(bool scratchFloat)
{
    int32_t scratch_value = 0xa5a5a5a5 ^ uint32_t(icount_);
    set_register(r0, scratch_value);
    set_register(r1, scratch_value);
    set_register(r2, scratch_value);
    set_register(r3, scratch_value);
    set_register(r12, scratch_value); 
    set_register(r14, scratch_value); 

    if (scratchFloat) {
        uint64_t scratch_value_d = 0x5a5a5a5a5a5a5a5aLU ^ uint64_t(icount_) ^ (uint64_t(icount_) << 30);
        for (uint32_t i = d0; i < d8; i++)
            set_d_register(i, &scratch_value_d);
        for (uint32_t i = d16; i < FloatRegisters::TotalPhys; i++)
            set_d_register(i, &scratch_value_d);
    }
}


void
Simulator::softwareInterrupt(SimInstruction* instr)
{
    int svc = instr->svcValue();
    switch (svc) {
      case kCallRtRedirected: {
        Redirection* redirection = Redirection::FromSwiInstruction(instr);
        int32_t arg0 = get_register(r0);
        int32_t arg1 = get_register(r1);
        int32_t arg2 = get_register(r2);
        int32_t arg3 = get_register(r3);
        int32_t* stack_pointer = reinterpret_cast<int32_t*>(get_register(sp));
        int32_t arg4 = stack_pointer[0];
        int32_t arg5 = stack_pointer[1];

        int32_t saved_lr = get_register(lr);
        intptr_t external = reinterpret_cast<intptr_t>(redirection->nativeFunction());

        bool stack_aligned = (get_register(sp) & (ABIStackAlignment - 1)) == 0;
        if (!stack_aligned) {
            fprintf(stderr, "Runtime call with unaligned stack!\n");
            MOZ_CRASH();
        }

        if (single_stepping_)
            single_step_callback_(single_step_callback_arg_, this, nullptr);

        switch (redirection->type()) {
          case Args_General0: {
            Prototype_General0 target = reinterpret_cast<Prototype_General0>(external);
            int64_t result = target();
            scratchVolatileRegisters();
            setCallResult(result);
            break;
          }
          case Args_General1: {
            Prototype_General1 target = reinterpret_cast<Prototype_General1>(external);
            int64_t result = target(arg0);
            scratchVolatileRegisters();
            setCallResult(result);
            break;
          }
          case Args_General2: {
            Prototype_General2 target = reinterpret_cast<Prototype_General2>(external);
            int64_t result = target(arg0, arg1);
            
            
            
            
            bool scratchFloat = target != __aeabi_idivmod && target != __aeabi_uidivmod;
            scratchVolatileRegisters( scratchFloat);
            setCallResult(result);
            break;
          }
          case Args_General3: {
            Prototype_General3 target = reinterpret_cast<Prototype_General3>(external);
            int64_t result = target(arg0, arg1, arg2);
            scratchVolatileRegisters();
            setCallResult(result);
            break;
          }
          case Args_General4: {
            Prototype_General4 target = reinterpret_cast<Prototype_General4>(external);
            int64_t result = target(arg0, arg1, arg2, arg3);
            scratchVolatileRegisters();
            setCallResult(result);
            break;
          }
          case Args_General5: {
            Prototype_General5 target = reinterpret_cast<Prototype_General5>(external);
            int64_t result = target(arg0, arg1, arg2, arg3, arg4);
            scratchVolatileRegisters();
            setCallResult(result);
            break;
          }
          case Args_General6: {
            Prototype_General6 target = reinterpret_cast<Prototype_General6>(external);
            int64_t result = target(arg0, arg1, arg2, arg3, arg4, arg5);
            scratchVolatileRegisters();
            setCallResult(result);
            break;
          }
          case Args_General7: {
            Prototype_General7 target = reinterpret_cast<Prototype_General7>(external);
            int32_t arg6 = stack_pointer[2];
            int64_t result = target(arg0, arg1, arg2, arg3, arg4, arg5, arg6);
            scratchVolatileRegisters();
            setCallResult(result);
            break;
          }
          case Args_General8: {
            Prototype_General8 target = reinterpret_cast<Prototype_General8>(external);
            int32_t arg6 = stack_pointer[2];
            int32_t arg7 = stack_pointer[3];
            int64_t result = target(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
            scratchVolatileRegisters();
            setCallResult(result);
            break;
          }
          case Args_Double_None: {
            Prototype_Double_None target = reinterpret_cast<Prototype_Double_None>(external);
            double dresult = target();
            scratchVolatileRegisters();
            setCallResultDouble(dresult);
            break;
          }
          case Args_Int_Double: {
            double dval0, dval1;
            int32_t ival;
            getFpArgs(&dval0, &dval1, &ival);
            Prototype_Int_Double target = reinterpret_cast<Prototype_Int_Double>(external);
            int32_t res = target(dval0);
            scratchVolatileRegisters();
            set_register(r0, res);
            break;
          }
          case Args_Double_Double: {
            double dval0, dval1;
            int32_t ival;
            getFpArgs(&dval0, &dval1, &ival);
            Prototype_Double_Double target = reinterpret_cast<Prototype_Double_Double>(external);
            double dresult = target(dval0);
            scratchVolatileRegisters();
            setCallResultDouble(dresult);
            break;
          }
          case Args_Float32_Float32: {
            float fval0;
            if (UseHardFpABI())
                fval0 = get_float_from_s_register(0);
            else
                fval0 = mozilla::BitwiseCast<float>(arg0);
            Prototype_Float32_Float32 target = reinterpret_cast<Prototype_Float32_Float32>(external);
            float fresult = target(fval0);
            scratchVolatileRegisters();
            setCallResultFloat(fresult);
            break;
          }
          case Args_Double_Int: {
            Prototype_Double_Int target = reinterpret_cast<Prototype_Double_Int>(external);
            double dresult = target(arg0);
            scratchVolatileRegisters();
            setCallResultDouble(dresult);
            break;
          }
          case Args_Double_DoubleInt: {
            double dval0, dval1;
            int32_t ival;
            getFpArgs(&dval0, &dval1, &ival);
            Prototype_DoubleInt target = reinterpret_cast<Prototype_DoubleInt>(external);
            double dresult = target(dval0, ival);
            scratchVolatileRegisters();
            setCallResultDouble(dresult);
            break;
          }
          case Args_Double_DoubleDouble: {
            double dval0, dval1;
            int32_t ival;
            getFpArgs(&dval0, &dval1, &ival);
            Prototype_Double_DoubleDouble target = reinterpret_cast<Prototype_Double_DoubleDouble>(external);
            double dresult = target(dval0, dval1);
            scratchVolatileRegisters();
            setCallResultDouble(dresult);
            break;
          }
          case Args_Double_IntDouble: {
            int32_t ival = get_register(0);
            double dval0;
            if (UseHardFpABI())
                dval0 = get_double_from_d_register(0);
            else
                dval0 = get_double_from_register_pair(2);
            Prototype_Double_IntDouble target = reinterpret_cast<Prototype_Double_IntDouble>(external);
            double dresult = target(ival, dval0);
            scratchVolatileRegisters();
            setCallResultDouble(dresult);
            break;
          }
          case Args_Int_IntDouble: {
            int32_t ival = get_register(0);
            double dval0;
            if (UseHardFpABI())
                dval0 = get_double_from_d_register(0);
            else
                dval0 = get_double_from_register_pair(2);
            Prototype_Int_IntDouble target = reinterpret_cast<Prototype_Int_IntDouble>(external);
            int32_t result = target(ival, dval0);
            scratchVolatileRegisters();
            set_register(r0, result);
            break;
          }
          case Args_Double_DoubleDoubleDouble: {
            double dval0, dval1, dval2;
            int32_t ival;
            getFpArgs(&dval0, &dval1, &ival);
            
            getFpFromStack(stack_pointer, &dval2);
            Prototype_Double_DoubleDoubleDouble target = reinterpret_cast<Prototype_Double_DoubleDoubleDouble>(external);
            double dresult = target(dval0, dval1, dval2);
            scratchVolatileRegisters();
            setCallResultDouble(dresult);
            break;
         }
         case Args_Double_DoubleDoubleDoubleDouble: {
            double dval0, dval1, dval2, dval3;
            int32_t ival;
            getFpArgs(&dval0, &dval1, &ival);
            
            getFpFromStack(stack_pointer, &dval2);
            getFpFromStack(stack_pointer + 2, &dval3);
            Prototype_Double_DoubleDoubleDoubleDouble target = reinterpret_cast<Prototype_Double_DoubleDoubleDoubleDouble>(external);
            double dresult = target(dval0, dval1, dval2, dval3);
            scratchVolatileRegisters();
            setCallResultDouble(dresult);
            break;
          }
          default:
            MOZ_CRASH("call");
        }

        if (single_stepping_)
            single_step_callback_(single_step_callback_arg_, this, nullptr);

        set_register(lr, saved_lr);
        set_pc(get_register(lr));
        break;
      }
      case kBreakpoint: {
        ArmDebugger dbg(this);
        dbg.debug();
        break;
      }
      default: { 
        if (svc >= (1 << 23)) {
            uint32_t code = svc & kStopCodeMask;
            if (isWatchedStop(code))
                increaseStopCounter(code);

            
            
            if (isEnabledStop(code)) {
                ArmDebugger dbg(this);
                dbg.stop(instr);
            } else {
                set_pc(get_pc() + 2 * SimInstruction::kInstrSize);
            }
        } else {
            
            MOZ_CRASH();
            break;
        }
      }
    }
}

double
Simulator::canonicalizeNaN(double value)
{
    return FPSCR_default_NaN_mode_ ? JS::CanonicalizeNaN(value) : value;
}


bool
Simulator::isStopInstruction(SimInstruction* instr)
{
    return (instr->bits(27, 24) == 0xF) && (instr->svcValue() >= kStopCode);
}

bool Simulator::isWatchedStop(uint32_t code)
{
    MOZ_ASSERT(code <= kMaxStopCode);
    return code < kNumOfWatchedStops;
}

bool
Simulator::isEnabledStop(uint32_t code)
{
    MOZ_ASSERT(code <= kMaxStopCode);
    
    return !isWatchedStop(code) || !(watched_stops_[code].count & kStopDisabledBit);
}

void
Simulator::enableStop(uint32_t code)
{
    MOZ_ASSERT(isWatchedStop(code));
    if (!isEnabledStop(code))
        watched_stops_[code].count &= ~kStopDisabledBit;
}

void
Simulator::disableStop(uint32_t code)
{
    MOZ_ASSERT(isWatchedStop(code));
    if (isEnabledStop(code))
        watched_stops_[code].count |= kStopDisabledBit;
}

void
Simulator::increaseStopCounter(uint32_t code)
{
    MOZ_ASSERT(code <= kMaxStopCode);
    MOZ_ASSERT(isWatchedStop(code));
    if ((watched_stops_[code].count & ~(1 << 31)) == 0x7fffffff) {
        printf("Stop counter for code %i has overflowed.\n"
               "Enabling this code and reseting the counter to 0.\n", code);
        watched_stops_[code].count = 0;
        enableStop(code);
    } else {
        watched_stops_[code].count++;
    }
}


void
Simulator::printStopInfo(uint32_t code)
{
    MOZ_ASSERT(code <= kMaxStopCode);
    if (!isWatchedStop(code)) {
        printf("Stop not watched.");
    } else {
        const char* state = isEnabledStop(code) ? "Enabled" : "Disabled";
        int32_t count = watched_stops_[code].count & ~kStopDisabledBit;
        
        if (count != 0) {
            if (watched_stops_[code].desc) {
                printf("stop %i - 0x%x: \t%s, \tcounter = %i, \t%s\n",
                       code, code, state, count, watched_stops_[code].desc);
            } else {
                printf("stop %i - 0x%x: \t%s, \tcounter = %i\n",
                       code, code, state, count);
            }
        }
    }
}



void
Simulator::decodeType01(SimInstruction* instr)
{
    int type = instr->typeValue();
    if (type == 0 && instr->isSpecialType0()) {
        
        if (instr->bits(7, 4) == 9) {
            if (instr->bit(24) == 0) {
                
                
                int rn = instr->rnValue();
                int rm = instr->rmValue();
                int rs = instr->rsValue();
                int32_t rs_val = get_register(rs);
                int32_t rm_val = get_register(rm);
                if (instr->bit(23) == 0) {
                    if (instr->bit(21) == 0) {
                        
                        
                        
                        int rd = rn;  
                        int32_t alu_out = rm_val * rs_val;
                        set_register(rd, alu_out);
                        if (instr->hasS())
                            setNZFlags(alu_out);
                    } else {
                        int rd = instr->rdValue();
                        int32_t acc_value = get_register(rd);
                        if (instr->bit(22) == 0) {
                            
                            
                            
                            
                            
                            int32_t mul_out = rm_val * rs_val;
                            int32_t result = acc_value + mul_out;
                            set_register(rn, result);
                        } else {
                            int32_t mul_out = rm_val * rs_val;
                            int32_t result = acc_value - mul_out;
                            set_register(rn, result);
                        }
                    }
                } else {
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    int rd_hi = rn;  
                    int rd_lo = instr->rdValue();
                    int32_t hi_res = 0;
                    int32_t lo_res = 0;
                    if (instr->bit(22) == 1) {
                        int64_t left_op  = static_cast<int32_t>(rm_val);
                        int64_t right_op = static_cast<int32_t>(rs_val);
                        uint64_t result = left_op * right_op;
                        hi_res = static_cast<int32_t>(result >> 32);
                        lo_res = static_cast<int32_t>(result & 0xffffffff);
                    } else {
                        
                        uint64_t left_op  = static_cast<uint32_t>(rm_val);
                        uint64_t right_op = static_cast<uint32_t>(rs_val);
                        uint64_t result = left_op * right_op;
                        hi_res = static_cast<int32_t>(result >> 32);
                        lo_res = static_cast<int32_t>(result & 0xffffffff);
                    }
                    set_register(rd_lo, lo_res);
                    set_register(rd_hi, hi_res);
                    if (instr->hasS())
                        MOZ_CRASH();
                }
            } else {
                if (instr->bit(23)) {
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    if (instr->bit(20)) {
                        
                        int rn = instr->rnValue();
                        int rt = instr->rtValue();
                        int32_t address = get_register(rn);
                        switch (instr->bits(22,21)) {
                          case 0:
                            set_register(rt, readW(address, instr));
                            break;
                          case 1:
                            set_dw_register(rt, readDW(address));
                            break;
                          case 2:
                            set_register(rt, readBU(address));
                            break;
                          case 3:
                            set_register(rt, readHU(address, instr));
                            break;
                        }
                    } else {
                        
                        int rn = instr->rnValue();
                        int rd = instr->rdValue();
                        int rt = instr->bits(3,0);
                        int32_t address = get_register(rn);
                        int32_t value = get_register(rt);
                        switch (instr->bits(22,21)) {
                          case 0:
                            writeW(address, value, instr);
                            break;
                          case 1: {
                              MOZ_ASSERT((rt % 2) == 0);
                              int32_t value2 = get_register(rt+1);
                              writeDW(address, value, value2);
                              break;
                          }
                          case 2:
                            writeB(address, (uint8_t)value);
                            break;
                          case 3:
                            writeH(address, (uint16_t)value, instr);
                            break;
                        }
                        set_register(rd, 0);
                    }
                } else {
                    MOZ_CRASH(); 
                }
            }
        } else {
            
            int rd = instr->rdValue();
            int rn = instr->rnValue();
            int32_t rn_val = get_register(rn);
            int32_t addr = 0;
            if (instr->bit(22) == 0) {
                int rm = instr->rmValue();
                int32_t rm_val = get_register(rm);
                switch (instr->PUField()) {
                  case da_x:
                    MOZ_ASSERT(!instr->hasW());
                    addr = rn_val;
                    rn_val -= rm_val;
                    set_register(rn, rn_val);
                    break;
                  case ia_x:
                    MOZ_ASSERT(!instr->hasW());
                    addr = rn_val;
                    rn_val += rm_val;
                    set_register(rn, rn_val);
                    break;
                  case db_x:
                    rn_val -= rm_val;
                    addr = rn_val;
                    if (instr->hasW())
                        set_register(rn, rn_val);
                    break;
                  case ib_x:
                    rn_val += rm_val;
                    addr = rn_val;
                    if (instr->hasW())
                        set_register(rn, rn_val);
                    break;
                  default:
                    
                    MOZ_CRASH();
                    break;
                }
            } else {
                int32_t imm_val = (instr->immedHValue() << 4) | instr->immedLValue();
                switch (instr->PUField()) {
                  case da_x:
                    MOZ_ASSERT(!instr->hasW());
                    addr = rn_val;
                    rn_val -= imm_val;
                    set_register(rn, rn_val);
                    break;
                  case ia_x:
                    MOZ_ASSERT(!instr->hasW());
                    addr = rn_val;
                    rn_val += imm_val;
                    set_register(rn, rn_val);
                    break;
                  case db_x:
                    rn_val -= imm_val;
                    addr = rn_val;
                    if (instr->hasW())
                        set_register(rn, rn_val);
                    break;
                  case ib_x:
                    rn_val += imm_val;
                    addr = rn_val;
                    if (instr->hasW())
                        set_register(rn, rn_val);
                    break;
                  default:
                    
                    MOZ_CRASH();
                    break;
                }
            }
            if ((instr->bits(7, 4) & 0xd) == 0xd && instr->bit(20) == 0) {
                MOZ_ASSERT((rd % 2) == 0);
                if (instr->hasH()) {
                    
                    int32_t value1 = get_register(rd);
                    int32_t value2 = get_register(rd+1);
                    writeDW(addr, value1, value2);
                } else {
                    
                    int* rn_data = readDW(addr);
                    set_dw_register(rd, rn_data);
                }
            } else if (instr->hasH()) {
                if (instr->hasSign()) {
                    if (instr->hasL()) {
                        int16_t val = readH(addr, instr);
                        set_register(rd, val);
                    } else {
                        int16_t val = get_register(rd);
                        writeH(addr, val, instr);
                    }
                } else {
                    if (instr->hasL()) {
                        uint16_t val = readHU(addr, instr);
                        set_register(rd, val);
                    } else {
                        uint16_t val = get_register(rd);
                        writeH(addr, val, instr);
                    }
                }
            } else {
                
                MOZ_ASSERT(instr->hasSign());
                MOZ_ASSERT(instr->hasL());
                int8_t val = readB(addr);
                set_register(rd, val);
            }
            return;
        }
    } else if ((type == 0) && instr->isMiscType0()) {
        if (instr->bits(7, 4) == 0) {
            if (instr->bit(21) == 0) {
                
                int rd = instr->rdValue();
                uint32_t flags;
                if (instr->bit(22) == 0) {
                    
                    flags = (n_flag_ << 31) |
                        (z_flag_ << 30) |
                        (c_flag_ << 29) |
                        (v_flag_ << 28);
                } else {
                    
                    MOZ_CRASH();
                }
                set_register(rd, flags);
            } else {
                
                if (instr->bits(27, 23) == 2) {
                    
                    int rm = instr->rmValue();
                    mozilla::DebugOnly<uint32_t> mask = instr->bits(19, 16);
                    MOZ_ASSERT(mask == (3 << 2));

                    uint32_t flags = get_register(rm);
                    n_flag_ = (flags >> 31) & 1;
                    z_flag_ = (flags >> 30) & 1;
                    c_flag_ = (flags >> 29) & 1;
                    v_flag_ = (flags >> 28) & 1;
                } else {
                    MOZ_CRASH();
                }
            }
        } else if (instr->bits(22, 21) == 1) {
            int rm = instr->rmValue();
            switch (instr->bits(7, 4)) {
              case 1:   
                set_pc(get_register(rm));
                break;
              case 3: { 
                uint32_t old_pc = get_pc();
                set_pc(get_register(rm));
                set_register(lr, old_pc + SimInstruction::kInstrSize);
                break;
              }
              case 7: { 
                fprintf(stderr, "Simulator hit BKPT.\n");
                if (getenv("ARM_SIM_DEBUGGER")) {
                    ArmDebugger dbg(this);
                    dbg.debug();
                } else {
                    fprintf(stderr, "Use ARM_SIM_DEBUGGER=1 to enter the builtin debugger.\n");
                    MOZ_CRASH("ARM simulator breakpoint");
                }
                break;
              }
              default:
                MOZ_CRASH();
            }
        } else if (instr->bits(22, 21) == 3) {
            int rm = instr->rmValue();
            int rd = instr->rdValue();
            switch (instr->bits(7, 4)) {
              case 1: { 
                uint32_t bits = get_register(rm);
                int leading_zeros = 0;
                if (bits == 0)
                    leading_zeros = 32;
                else
                    leading_zeros = mozilla::CountLeadingZeroes32(bits);
                set_register(rd, leading_zeros);
                break;
              }
              default:
                MOZ_CRASH();
                break;
            }
        } else {
            printf("%08x\n", instr->instructionBits());
            MOZ_CRASH();
        }
    } else if ((type == 1) && instr->isNopType1()) {
        
    } else {
        int rd = instr->rdValue();
        int rn = instr->rnValue();
        int32_t rn_val = get_register(rn);
        int32_t shifter_operand = 0;
        bool shifter_carry_out = 0;
        if (type == 0) {
            shifter_operand = getShiftRm(instr, &shifter_carry_out);
        } else {
            MOZ_ASSERT(instr->typeValue() == 1);
            shifter_operand = getImm(instr, &shifter_carry_out);
        }
        int32_t alu_out;
        switch (instr->opcodeField()) {
          case OpAnd:
            alu_out = rn_val & shifter_operand;
            set_register(rd, alu_out);
            if (instr->hasS()) {
                setNZFlags(alu_out);
                setCFlag(shifter_carry_out);
            }
            break;
          case OpEor:
            alu_out = rn_val ^ shifter_operand;
            set_register(rd, alu_out);
            if (instr->hasS()) {
                setNZFlags(alu_out);
                setCFlag(shifter_carry_out);
            }
            break;
          case OpSub:
            alu_out = rn_val - shifter_operand;
            set_register(rd, alu_out);
            if (instr->hasS()) {
                setNZFlags(alu_out);
                setCFlag(!borrowFrom(rn_val, shifter_operand));
                setVFlag(overflowFrom(alu_out, rn_val, shifter_operand, false));
            }
            break;
          case OpRsb:
            alu_out = shifter_operand - rn_val;
            set_register(rd, alu_out);
            if (instr->hasS()) {
                setNZFlags(alu_out);
                setCFlag(!borrowFrom(shifter_operand, rn_val));
                setVFlag(overflowFrom(alu_out, shifter_operand, rn_val, false));
            }
            break;
          case OpAdd:
            alu_out = rn_val + shifter_operand;
            set_register(rd, alu_out);
            if (instr->hasS()) {
                setNZFlags(alu_out);
                setCFlag(carryFrom(rn_val, shifter_operand));
                setVFlag(overflowFrom(alu_out, rn_val, shifter_operand, true));
            }
            break;
          case OpAdc:
            alu_out = rn_val + shifter_operand + getCarry();
            set_register(rd, alu_out);
            if (instr->hasS()) {
                setNZFlags(alu_out);
                setCFlag(carryFrom(rn_val, shifter_operand, getCarry()));
                setVFlag(overflowFrom(alu_out, rn_val, shifter_operand, true));
            }
            break;
          case OpSbc:
          case OpRsc:
            MOZ_CRASH();
            break;
          case OpTst:
            if (instr->hasS()) {
                alu_out = rn_val & shifter_operand;
                setNZFlags(alu_out);
                setCFlag(shifter_carry_out);
            } else {
                alu_out = instr->immedMovwMovtValue();
                set_register(rd, alu_out);
            }
            break;
          case OpTeq:
            if (instr->hasS()) {
                alu_out = rn_val ^ shifter_operand;
                setNZFlags(alu_out);
                setCFlag(shifter_carry_out);
            } else {
                
                
                MOZ_CRASH();
            }
            break;
          case OpCmp:
            if (instr->hasS()) {
                alu_out = rn_val - shifter_operand;
                setNZFlags(alu_out);
                setCFlag(!borrowFrom(rn_val, shifter_operand));
                setVFlag(overflowFrom(alu_out, rn_val, shifter_operand, false));
            } else {
                alu_out = (get_register(rd) & 0xffff) |
                    (instr->immedMovwMovtValue() << 16);
                set_register(rd, alu_out);
            }
            break;
          case OpCmn:
            if (instr->hasS()) {
                alu_out = rn_val + shifter_operand;
                setNZFlags(alu_out);
                setCFlag(carryFrom(rn_val, shifter_operand));
                setVFlag(overflowFrom(alu_out, rn_val, shifter_operand, true));
            } else {
                
                
                MOZ_CRASH();
            }
            break;
          case OpOrr:
            alu_out = rn_val | shifter_operand;
            set_register(rd, alu_out);
            if (instr->hasS()) {
                setNZFlags(alu_out);
                setCFlag(shifter_carry_out);
            }
            break;
          case OpMov:
            alu_out = shifter_operand;
            set_register(rd, alu_out);
            if (instr->hasS()) {
                setNZFlags(alu_out);
                setCFlag(shifter_carry_out);
            }
            break;
          case OpBic:
            alu_out = rn_val & ~shifter_operand;
            set_register(rd, alu_out);
            if (instr->hasS()) {
                setNZFlags(alu_out);
                setCFlag(shifter_carry_out);
            }
            break;
          case OpMvn:
            alu_out = ~shifter_operand;
            set_register(rd, alu_out);
            if (instr->hasS()) {
                setNZFlags(alu_out);
                setCFlag(shifter_carry_out);
            }
            break;
          default:
            MOZ_CRASH();
            break;
        }
    }
}

void
Simulator::decodeType2(SimInstruction* instr)
{
    int rd = instr->rdValue();
    int rn = instr->rnValue();
    int32_t rn_val = get_register(rn);
    int32_t im_val = instr->offset12Value();
    int32_t addr = 0;
    switch (instr->PUField()) {
      case da_x:
        MOZ_ASSERT(!instr->hasW());
        addr = rn_val;
        rn_val -= im_val;
        set_register(rn, rn_val);
        break;
      case ia_x:
        MOZ_ASSERT(!instr->hasW());
        addr = rn_val;
        rn_val += im_val;
        set_register(rn, rn_val);
        break;
      case db_x:
        rn_val -= im_val;
        addr = rn_val;
        if (instr->hasW())
            set_register(rn, rn_val);
        break;
      case ib_x:
        rn_val += im_val;
        addr = rn_val;
        if (instr->hasW())
            set_register(rn, rn_val);
        break;
      default:
        MOZ_CRASH();
        break;
    }
    if (instr->hasB()) {
        if (instr->hasL()) {
            uint8_t val = readBU(addr);
            set_register(rd, val);
        } else {
            uint8_t val = get_register(rd);
            writeB(addr, val);
        }
    } else {
        if (instr->hasL())
            set_register(rd, readW(addr, instr));
        else
            writeW(addr, get_register(rd), instr);
    }
}

static uint32_t
rotateBytes(uint32_t val, int32_t rotate)
{
    switch (rotate) {
      default:
        return val;
      case 1:
        return (val >> 8) | (val << 24);
      case 2:
        return (val >> 16) | (val << 16);
      case 3:
        return (val >> 24) | (val << 8);
    }
}

void
Simulator::decodeType3(SimInstruction* instr)
{
    int rd = instr->rdValue();
    int rn = instr->rnValue();
    int32_t rn_val = get_register(rn);
    bool shifter_carry_out = 0;
    int32_t shifter_operand = getShiftRm(instr, &shifter_carry_out);
    int32_t addr = 0;
    switch (instr->PUField()) {
      case da_x:
        MOZ_ASSERT(!instr->hasW());
        MOZ_CRASH();
        break;
      case ia_x: {
        if (instr->bit(4) == 0) {
            
        } else {
            if (instr->bit(5) == 0) {
                switch (instr->bits(22, 21)) {
                  case 0:
                    if (instr->bit(20) == 0) {
                        if (instr->bit(6) == 0) {
                            
                            uint32_t rn_val = get_register(rn);
                            uint32_t rm_val = get_register(instr->rmValue());
                            int32_t shift = instr->bits(11, 7);
                            rm_val <<= shift;
                            set_register(rd, (rn_val & 0xFFFF) | (rm_val & 0xFFFF0000U));
                        } else {
                            
                            uint32_t rn_val = get_register(rn);
                            int32_t rm_val = get_register(instr->rmValue());
                            int32_t shift = instr->bits(11, 7);
                            if (shift == 0)
                                shift = 32;
                            rm_val >>= shift;
                            set_register(rd, (rn_val & 0xFFFF0000U) | (rm_val & 0xFFFF));
                        }
                    } else {
                        MOZ_CRASH();
                    }
                    break;
                  case 1:
                    MOZ_CRASH();
                    break;
                  case 2:
                    MOZ_CRASH();
                    break;
                  case 3: {
                    
                      int32_t sat_pos = instr->bits(20, 16);
                      int32_t sat_val = (1 << sat_pos) - 1;
                      int32_t shift = instr->bits(11, 7);
                      int32_t shift_type = instr->bit(6);
                      int32_t rm_val = get_register(instr->rmValue());
                      if (shift_type == 0) 
                          rm_val <<= shift;
                      else 
                          rm_val >>= shift;

                      
                      
                      
                      if (rm_val > sat_val)
                          rm_val = sat_val;
                      else if (rm_val < 0)
                          rm_val = 0;
                      set_register(rd, rm_val);
                      break;
                  }
                }
            } else {
                switch (instr->bits(22, 21)) {
                  case 0:
                    MOZ_CRASH();
                    break;
                  case 1:
                    if (instr->bits(7,4) == 7 && instr->bits(19,16) == 15) {
                        uint32_t rm_val = rotateBytes(get_register(instr->rmValue()),
                                                      instr->bits(11, 10));
                        if (instr->bit(20)) {
                            
                            set_register(rd, (int32_t)(int16_t)(rm_val & 0xFFFF));
                        }
                        else {
                            
                            set_register(rd, (int32_t)(int8_t)(rm_val & 0xFF));
                        }
                    } else {
                        MOZ_CRASH();
                    }
                    break;
                  case 2:
                    if ((instr->bit(20) == 0) && (instr->bits(9, 6) == 1)) {
                        if (instr->bits(19, 16) == 0xF) {
                            
                            uint32_t rm_val = rotateBytes(get_register(instr->rmValue()),
                                                          instr->bits(11, 10));
                            set_register(rd, (rm_val & 0xFF) | (rm_val & 0xFF0000));
                        } else {
                            MOZ_CRASH();
                        }
                    } else {
                        MOZ_CRASH();
                    }
                    break;
                  case 3:
                    if ((instr->bit(20) == 0) && (instr->bits(9, 6) == 1)) {
                        if (instr->bits(19, 16) == 0xF) {
                            
                            uint32_t rm_val = rotateBytes(get_register(instr->rmValue()),
                                                          instr->bits(11, 10));
                            set_register(rd, (rm_val & 0xFF));
                        } else {
                            
                            uint32_t rn_val = get_register(rn);
                            uint32_t rm_val = rotateBytes(get_register(instr->rmValue()),
                                                          instr->bits(11, 10));
                            set_register(rd, rn_val + (rm_val & 0xFF));
                        }
                    } else if ((instr->bit(20) == 1) && (instr->bits(9, 6) == 1)) {
                        if (instr->bits(19, 16) == 0xF) {
                            
                            uint32_t rm_val = rotateBytes(get_register(instr->rmValue()),
                                                          instr->bits(11, 10));
                            set_register(rd, (rm_val & 0xFFFF));
                        } else {
                            
                            uint32_t rn_val = get_register(rn);
                            uint32_t rm_val = rotateBytes(get_register(instr->rmValue()),
                                                          instr->bits(11, 10));
                            set_register(rd, rn_val + (rm_val & 0xFFFF));
                        }
                    } else {
                        MOZ_CRASH();
                    }
                    break;
                }
            }
            return;
        }
        break;
      }
      case db_x: { 
        if (instr->bit(22) == 0x0 && instr->bit(20) == 0x1 &&
            instr->bits(15,12) == 0x0f && instr->bits(7, 4) == 0x1) {
            if (!instr->hasW()) {
                
                int rm = instr->rmValue();
                int32_t rm_val = get_register(rm);
                int rs = instr->rsValue();
                int32_t rs_val = get_register(rs);
                int32_t ret_val = 0;
                MOZ_ASSERT(rs_val != 0);
                if ((rm_val == INT32_MIN) && (rs_val == -1))
                    ret_val = INT32_MIN;
                else
                    ret_val = rm_val / rs_val;
                set_register(rn, ret_val);
                return;
            } else {
                
                int rm = instr->rmValue();
                uint32_t rm_val = get_register(rm);
                int rs = instr->rsValue();
                uint32_t rs_val = get_register(rs);
                uint32_t ret_val = 0;
                MOZ_ASSERT(rs_val != 0);
                ret_val = rm_val / rs_val;
                set_register(rn, ret_val);
                return;
            }
        }

        addr = rn_val - shifter_operand;
        if (instr->hasW())
            set_register(rn, addr);
        break;
      }
      case ib_x: {
        if (instr->hasW() && (instr->bits(6, 4) == 0x5)) {
            uint32_t widthminus1 = static_cast<uint32_t>(instr->bits(20, 16));
            uint32_t lsbit = static_cast<uint32_t>(instr->bits(11, 7));
            uint32_t msbit = widthminus1 + lsbit;
            if (msbit <= 31) {
                if (instr->bit(22)) {
                    
                    uint32_t rm_val = static_cast<uint32_t>(get_register(instr->rmValue()));
                    uint32_t extr_val = rm_val << (31 - msbit);
                    extr_val = extr_val >> (31 - widthminus1);
                    set_register(instr->rdValue(), extr_val);
                } else {
                    
                    int32_t rm_val = get_register(instr->rmValue());
                    int32_t extr_val = rm_val << (31 - msbit);
                    extr_val = extr_val >> (31 - widthminus1);
                    set_register(instr->rdValue(), extr_val);
                }
            } else {
                MOZ_CRASH();
            }
            return;
        } else if (!instr->hasW() && (instr->bits(6, 4) == 0x1)) {
            uint32_t lsbit = static_cast<uint32_t>(instr->bits(11, 7));
            uint32_t msbit = static_cast<uint32_t>(instr->bits(20, 16));
            if (msbit >= lsbit) {
                
                uint32_t rd_val =
                    static_cast<uint32_t>(get_register(instr->rdValue()));
                uint32_t bitcount = msbit - lsbit + 1;
                uint32_t mask = (1 << bitcount) - 1;
                rd_val &= ~(mask << lsbit);
                if (instr->rmValue() != 15) {
                    
                    uint32_t rm_val =
                        static_cast<uint32_t>(get_register(instr->rmValue()));
                    rm_val &= mask;
                    rd_val |= rm_val << lsbit;
                }
                set_register(instr->rdValue(), rd_val);
            } else {
                MOZ_CRASH();
            }
            return;
        } else {
            addr = rn_val + shifter_operand;
            if (instr->hasW())
                set_register(rn, addr);
        }
        break;
      }
      default:
        MOZ_CRASH();
        break;
    }
    if (instr->hasB()) {
        if (instr->hasL()) {
            uint8_t byte = readB(addr);
            set_register(rd, byte);
        } else {
            uint8_t byte = get_register(rd);
            writeB(addr, byte);
        }
    } else {
        if (instr->hasL())
            set_register(rd, readW(addr, instr));
        else
            writeW(addr, get_register(rd), instr);
    }
}

void
Simulator::decodeType4(SimInstruction* instr)
{
    
    MOZ_ASSERT(instr->bit(22) == 0);
    bool load = instr->hasL();
    handleRList(instr, load);
}

void
Simulator::decodeType5(SimInstruction* instr)
{
    int off = instr->sImmed24Value() << 2;
    intptr_t pc_address = get_pc();
    if (instr->hasLink())
        set_register(lr, pc_address + SimInstruction::kInstrSize);
    int pc_reg = get_register(pc);
    set_pc(pc_reg + off);
}

void
Simulator::decodeType6(SimInstruction* instr)
{
    decodeType6CoprocessorIns(instr);
}

void
Simulator::decodeType7(SimInstruction* instr)
{
    if (instr->bit(24) == 1)
        softwareInterrupt(instr);
    else if (instr->bit(4) == 1 && instr->bits(11,9) != 5)
        decodeType7CoprocessorIns(instr);
    else
        decodeTypeVFP(instr);
}

void
Simulator::decodeType7CoprocessorIns(SimInstruction* instr)
{
    if (instr->bit(20) == 0) {
        
        if (instr->coprocessorValue() == 15) {
            int opc1 = instr->bits(23,21);
            int opc2 = instr->bits(7,5);
            int CRn = instr->bits(19,16);
            int CRm = instr->bits(3,0);
            if (opc1 == 0 && opc2 == 4 && CRn == 7 && CRm == 10) {
                
            } else if (opc1 == 0 && opc2 == 5 && CRn == 7 && CRm == 10) {
                
            }
            else if (opc1 == 0 && opc2 == 4 && CRn == 7 && CRm == 5) {
                
            }
            else {
                MOZ_CRASH();
            }
        } else {
            MOZ_CRASH();
        }
    } else {
        
        MOZ_CRASH();
    }
}

void
Simulator::decodeTypeVFP(SimInstruction* instr)
{
    MOZ_ASSERT(instr->typeValue() == 7 && instr->bit(24) == 0);
    MOZ_ASSERT(instr->bits(11, 9) == 0x5);

    
    VFPRegPrecision precision = (instr->szValue() == 1) ? kDoublePrecision : kSinglePrecision;
    int vm = instr->VFPMRegValue(precision);
    int vd = instr->VFPDRegValue(precision);
    int vn = instr->VFPNRegValue(precision);

    if (instr->bit(4) == 0) {
        if (instr->opc1Value() == 0x7) {
            
            if ((instr->opc2Value() == 0x0) && (instr->opc3Value() == 0x1)) {
                
                if (instr->szValue() == 0x1) {
                    int m = instr->VFPMRegValue(kDoublePrecision);
                    int d = instr->VFPDRegValue(kDoublePrecision);
                    set_d_register_from_double(d, get_double_from_d_register(m));
                } else {
                    int m = instr->VFPMRegValue(kSinglePrecision);
                    int d = instr->VFPDRegValue(kSinglePrecision);
                    set_s_register_from_float(d, get_float_from_s_register(m));
                }
            } else if ((instr->opc2Value() == 0x0) && (instr->opc3Value() == 0x3)) {
                
                if (instr->szValue() == 0x1) {
                    double dm_value = get_double_from_d_register(vm);
                    double dd_value = std::fabs(dm_value);
                    dd_value = canonicalizeNaN(dd_value);
                    set_d_register_from_double(vd, dd_value);
                } else {
                    float fm_value = get_float_from_s_register(vm);
                    float fd_value = std::fabs(fm_value);
                    fd_value = canonicalizeNaN(fd_value);
                    set_s_register_from_float(vd, fd_value);
                }
            } else if ((instr->opc2Value() == 0x1) && (instr->opc3Value() == 0x1)) {
                
                if (instr->szValue() == 0x1) {
                    double dm_value = get_double_from_d_register(vm);
                    double dd_value = -dm_value;
                    dd_value = canonicalizeNaN(dd_value);
                    set_d_register_from_double(vd, dd_value);
                } else {
                    float fm_value = get_float_from_s_register(vm);
                    float fd_value = -fm_value;
                    fd_value = canonicalizeNaN(fd_value);
                    set_s_register_from_float(vd, fd_value);
                }
            } else if ((instr->opc2Value() == 0x7) && (instr->opc3Value() == 0x3)) {
                decodeVCVTBetweenDoubleAndSingle(instr);
            } else if ((instr->opc2Value() == 0x8) && (instr->opc3Value() & 0x1)) {
                decodeVCVTBetweenFloatingPointAndInteger(instr);
            } else if ((instr->opc2Value() == 0xA) && (instr->opc3Value() == 0x3) &&
                       (instr->bit(8) == 1)) {
                
                int fraction_bits = 32 - ((instr->bits(3, 0) << 1) | instr->bit(5));
                int fixed_value = get_sinteger_from_s_register(vd * 2);
                double divide = 1 << fraction_bits;
                set_d_register_from_double(vd, fixed_value / divide);
            } else if (((instr->opc2Value() >> 1) == 0x6) &&
                       (instr->opc3Value() & 0x1)) {
                decodeVCVTBetweenFloatingPointAndInteger(instr);
            } else if (((instr->opc2Value() == 0x4) || (instr->opc2Value() == 0x5)) &&
                       (instr->opc3Value() & 0x1)) {
                decodeVCMP(instr);
            } else if (((instr->opc2Value() == 0x1)) && (instr->opc3Value() == 0x3)) {
                
                if (instr->szValue() == 0x1) {
                    double dm_value = get_double_from_d_register(vm);
                    double dd_value = std::sqrt(dm_value);
                    dd_value = canonicalizeNaN(dd_value);
                    set_d_register_from_double(vd, dd_value);
                } else {
                    float fm_value = get_float_from_s_register(vm);
                    float fd_value = std::sqrt(fm_value);
                    fd_value = canonicalizeNaN(fd_value);
                    set_s_register_from_float(vd, fd_value);
                }
            } else if (instr->opc3Value() == 0x0) {
                
                if (instr->szValue() == 0x1) {
                    set_d_register_from_double(vd, instr->doubleImmedVmov());
                } else {
                    
                    set_s_register_from_float(vd, instr->float32ImmedVmov());
                }
            } else {
                decodeVCVTBetweenFloatingPointAndIntegerFrac(instr);
            }
        } else if (instr->opc1Value() == 0x3) {
            if (instr->szValue() != 0x1) {
                if (instr->opc3Value() & 0x1) {
                    
                    float fn_value = get_float_from_s_register(vn);
                    float fm_value = get_float_from_s_register(vm);
                    float fd_value = fn_value - fm_value;
                    fd_value = canonicalizeNaN(fd_value);
                    set_s_register_from_float(vd, fd_value);
                } else {
                    
                    float fn_value = get_float_from_s_register(vn);
                    float fm_value = get_float_from_s_register(vm);
                    float fd_value = fn_value + fm_value;
                    fd_value = canonicalizeNaN(fd_value);
                    set_s_register_from_float(vd, fd_value);
                }
            } else {
                if (instr->opc3Value() & 0x1) {
                    
                    double dn_value = get_double_from_d_register(vn);
                    double dm_value = get_double_from_d_register(vm);
                    double dd_value = dn_value - dm_value;
                    dd_value = canonicalizeNaN(dd_value);
                    set_d_register_from_double(vd, dd_value);
                } else {
                    
                    double dn_value = get_double_from_d_register(vn);
                    double dm_value = get_double_from_d_register(vm);
                    double dd_value = dn_value + dm_value;
                    dd_value = canonicalizeNaN(dd_value);
                    set_d_register_from_double(vd, dd_value);
                }
            }
        } else if ((instr->opc1Value() == 0x2) && !(instr->opc3Value() & 0x1)) {
            
            if (instr->szValue() != 0x1) {
                float fn_value = get_float_from_s_register(vn);
                float fm_value = get_float_from_s_register(vm);
                float fd_value = fn_value * fm_value;
                fd_value = canonicalizeNaN(fd_value);
                set_s_register_from_float(vd, fd_value);
            } else {
                double dn_value = get_double_from_d_register(vn);
                double dm_value = get_double_from_d_register(vm);
                double dd_value = dn_value * dm_value;
                dd_value = canonicalizeNaN(dd_value);
                set_d_register_from_double(vd, dd_value);
            }
        } else if ((instr->opc1Value() == 0x0)) {
            
            const bool is_vmls = (instr->opc3Value() & 0x1);

            if (instr->szValue() != 0x1)
                MOZ_CRASH("Not used by V8.");

            const double dd_val = get_double_from_d_register(vd);
            const double dn_val = get_double_from_d_register(vn);
            const double dm_val = get_double_from_d_register(vm);

            
            
            set_d_register_from_double(vd, dn_val * dm_val);
            if (is_vmls) {
                set_d_register_from_double(vd,
                                           canonicalizeNaN(dd_val - get_double_from_d_register(vd)));
            } else {
                set_d_register_from_double(vd,
                                           canonicalizeNaN(dd_val + get_double_from_d_register(vd)));
            }
        } else if ((instr->opc1Value() == 0x4) && !(instr->opc3Value() & 0x1)) {
            
            if (instr->szValue() != 0x1) {
                float fn_value = get_float_from_s_register(vn);
                float fm_value = get_float_from_s_register(vm);
                float fd_value = fn_value / fm_value;
                div_zero_vfp_flag_ = (fm_value == 0);
                fd_value = canonicalizeNaN(fd_value);
                set_s_register_from_float(vd, fd_value);
            } else {
                double dn_value = get_double_from_d_register(vn);
                double dm_value = get_double_from_d_register(vm);
                double dd_value = dn_value / dm_value;
                div_zero_vfp_flag_ = (dm_value == 0);
                dd_value = canonicalizeNaN(dd_value);
                set_d_register_from_double(vd, dd_value);
            }
        } else {
            MOZ_CRASH();
        }
    } else {
        if (instr->VCValue() == 0x0 && instr->VAValue() == 0x0) {
            decodeVMOVBetweenCoreAndSinglePrecisionRegisters(instr);
        } else if ((instr->VLValue() == 0x0) &&
                   (instr->VCValue() == 0x1) &&
                   (instr->bit(23) == 0x0)) {
            
            int vd = instr->bits(19, 16) | (instr->bit(7) << 4);
            double dd_value = get_double_from_d_register(vd);
            int32_t data[2];
            memcpy(data, &dd_value, 8);
            data[instr->bit(21)] = get_register(instr->rtValue());
            memcpy(&dd_value, data, 8);
            set_d_register_from_double(vd, dd_value);
        } else if ((instr->VLValue() == 0x1) &&
                   (instr->VCValue() == 0x1) &&
                   (instr->bit(23) == 0x0)) {
            
            int vn = instr->bits(19, 16) | (instr->bit(7) << 4);
            double dn_value = get_double_from_d_register(vn);
            int32_t data[2];
            memcpy(data, &dn_value, 8);
            set_register(instr->rtValue(), data[instr->bit(21)]);
        } else if ((instr->VLValue() == 0x1) &&
                   (instr->VCValue() == 0x0) &&
                   (instr->VAValue() == 0x7) &&
                   (instr->bits(19, 16) == 0x1)) {
            
            uint32_t rt = instr->rtValue();
            if (rt == 0xF) {
                copy_FPSCR_to_APSR();
            } else {
                
                uint32_t fpscr = (n_flag_FPSCR_ << 31) |
                    (z_flag_FPSCR_ << 30) |
                    (c_flag_FPSCR_ << 29) |
                    (v_flag_FPSCR_ << 28) |
                    (FPSCR_default_NaN_mode_ << 25) |
                    (inexact_vfp_flag_ << 4) |
                    (underflow_vfp_flag_ << 3) |
                    (overflow_vfp_flag_ << 2) |
                    (div_zero_vfp_flag_ << 1) |
                    (inv_op_vfp_flag_ << 0) |
                    (FPSCR_rounding_mode_);
                set_register(rt, fpscr);
            }
        } else if ((instr->VLValue() == 0x0) &&
                   (instr->VCValue() == 0x0) &&
                   (instr->VAValue() == 0x7) &&
                   (instr->bits(19, 16) == 0x1)) {
            
            uint32_t rt = instr->rtValue();
            if (rt == pc) {
                MOZ_CRASH();
            } else {
                uint32_t rt_value = get_register(rt);
                n_flag_FPSCR_ = (rt_value >> 31) & 1;
                z_flag_FPSCR_ = (rt_value >> 30) & 1;
                c_flag_FPSCR_ = (rt_value >> 29) & 1;
                v_flag_FPSCR_ = (rt_value >> 28) & 1;
                FPSCR_default_NaN_mode_ = (rt_value >> 25) & 1;
                inexact_vfp_flag_ = (rt_value >> 4) & 1;
                underflow_vfp_flag_ = (rt_value >> 3) & 1;
                overflow_vfp_flag_ = (rt_value >> 2) & 1;
                div_zero_vfp_flag_ = (rt_value >> 1) & 1;
                inv_op_vfp_flag_ = (rt_value >> 0) & 1;
                FPSCR_rounding_mode_ =
                    static_cast<VFPRoundingMode>((rt_value) & kVFPRoundingModeMask);
            }
        } else {
            MOZ_CRASH();
        }
    }
}

void
Simulator::decodeVMOVBetweenCoreAndSinglePrecisionRegisters(SimInstruction* instr)
{
    MOZ_ASSERT(instr->bit(4) == 1 &&
               instr->VCValue() == 0x0 &&
               instr->VAValue() == 0x0);

    int t = instr->rtValue();
    int n = instr->VFPNRegValue(kSinglePrecision);
    bool to_arm_register = (instr->VLValue() == 0x1);
    if (to_arm_register) {
        int32_t int_value = get_sinteger_from_s_register(n);
        set_register(t, int_value);
    } else {
        int32_t rs_val = get_register(t);
        set_s_register_from_sinteger(n, rs_val);
    }
}

void
Simulator::decodeVCMP(SimInstruction* instr)
{
    MOZ_ASSERT((instr->bit(4) == 0) && (instr->opc1Value() == 0x7));
    MOZ_ASSERT(((instr->opc2Value() == 0x4) || (instr->opc2Value() == 0x5)) &&
               (instr->opc3Value() & 0x1));
    

    VFPRegPrecision precision = kSinglePrecision;
    if (instr->szValue() == 1)
        precision = kDoublePrecision;

    int d = instr->VFPDRegValue(precision);
    int m = 0;
    if (instr->opc2Value() == 0x4)
        m = instr->VFPMRegValue(precision);

    if (precision == kDoublePrecision) {
        double dd_value = get_double_from_d_register(d);
        double dm_value = 0.0;
        if (instr->opc2Value() == 0x4) {
            dm_value = get_double_from_d_register(m);
        }

        
        if (instr->bit(7) == 1) {
            if (mozilla::IsNaN(dd_value))
                inv_op_vfp_flag_ = true;
        }
        compute_FPSCR_Flags(dd_value, dm_value);
    } else {
        float fd_value = get_float_from_s_register(d);
        float fm_value = 0.0;
        if (instr->opc2Value() == 0x4)
            fm_value = get_float_from_s_register(m);

        
        if (instr->bit(7) == 1) {
            if (mozilla::IsNaN(fd_value))
                inv_op_vfp_flag_ = true;
        }
        compute_FPSCR_Flags(fd_value, fm_value);
    }
}

void
Simulator::decodeVCVTBetweenDoubleAndSingle(SimInstruction* instr)
{
    MOZ_ASSERT(instr->bit(4) == 0 && instr->opc1Value() == 0x7);
    MOZ_ASSERT(instr->opc2Value() == 0x7 && instr->opc3Value() == 0x3);

    VFPRegPrecision dst_precision = kDoublePrecision;
    VFPRegPrecision src_precision = kSinglePrecision;
    if (instr->szValue() == 1) {
        dst_precision = kSinglePrecision;
        src_precision = kDoublePrecision;
    }

    int dst = instr->VFPDRegValue(dst_precision);
    int src = instr->VFPMRegValue(src_precision);

    if (dst_precision == kSinglePrecision) {
        double val = get_double_from_d_register(src);
        set_s_register_from_float(dst, static_cast<float>(val));
    } else {
        float val = get_float_from_s_register(src);
        set_d_register_from_double(dst, static_cast<double>(val));
    }
}

static bool
get_inv_op_vfp_flag(VFPRoundingMode mode, double val, bool unsigned_)
{
    MOZ_ASSERT(mode == SimRN || mode == SimRM || mode == SimRZ);
    double max_uint = static_cast<double>(0xffffffffu);
    double max_int = static_cast<double>(INT32_MAX);
    double min_int = static_cast<double>(INT32_MIN);

    
    if (val != val)
        return true;

    
    
    switch (mode) {
      case SimRN:
        return  unsigned_ ? (val >= (max_uint + 0.5)) ||
                            (val < -0.5)
                          : (val >= (max_int + 0.5)) ||
                            (val < (min_int - 0.5));
      case SimRM:
        return  unsigned_ ? (val >= (max_uint + 1.0)) ||
                            (val < 0)
                          : (val >= (max_int + 1.0)) ||
                            (val < min_int);
      case SimRZ:
        return  unsigned_ ? (val >= (max_uint + 1.0)) ||
                            (val <= -1)
                          : (val >= (max_int + 1.0)) ||
                            (val <= (min_int - 1.0));
      default:
        MOZ_CRASH();
        return true;
    }
}



static int
VFPConversionSaturate(double val, bool unsigned_res)
{
    if (val != val) 
        return 0;
    if (unsigned_res)
        return (val < 0) ? 0 : 0xffffffffu;
    return (val < 0) ? INT32_MIN : INT32_MAX;
}

void
Simulator::decodeVCVTBetweenFloatingPointAndInteger(SimInstruction* instr)
{
    MOZ_ASSERT((instr->bit(4) == 0) && (instr->opc1Value() == 0x7) &&
               (instr->bits(27, 23) == 0x1D));
    MOZ_ASSERT(((instr->opc2Value() == 0x8) && (instr->opc3Value() & 0x1)) ||
               (((instr->opc2Value() >> 1) == 0x6) && (instr->opc3Value() & 0x1)));

    
    bool to_integer = (instr->bit(18) == 1);

    VFPRegPrecision src_precision = (instr->szValue() == 1) ? kDoublePrecision : kSinglePrecision;

    if (to_integer) {
        
        
        
        
        

        int dst = instr->VFPDRegValue(kSinglePrecision);
        int src = instr->VFPMRegValue(src_precision);

        
        
        VFPRoundingMode mode = (instr->bit(7) != 1) ? FPSCR_rounding_mode_ : SimRZ;
        MOZ_ASSERT(mode == SimRM || mode == SimRZ || mode == SimRN);

        bool unsigned_integer = (instr->bit(16) == 0);
        bool double_precision = (src_precision == kDoublePrecision);

        double val = double_precision
                     ? get_double_from_d_register(src)
                     : get_float_from_s_register(src);

        int temp = unsigned_integer ? static_cast<uint32_t>(val) : static_cast<int32_t>(val);

        inv_op_vfp_flag_ = get_inv_op_vfp_flag(mode, val, unsigned_integer);

        double abs_diff = unsigned_integer
                          ? std::fabs(val - static_cast<uint32_t>(temp))
                          : std::fabs(val - temp);

        inexact_vfp_flag_ = (abs_diff != 0);

        if (inv_op_vfp_flag_) {
            temp = VFPConversionSaturate(val, unsigned_integer);
        } else {
            switch (mode) {
              case SimRN: {
                int val_sign = (val > 0) ? 1 : -1;
                if (abs_diff > 0.5) {
                    temp += val_sign;
                } else if (abs_diff == 0.5) {
                    
                    temp = ((temp % 2) == 0) ? temp : temp + val_sign;
                }
                break;
              }

              case SimRM:
                temp = temp > val ? temp - 1 : temp;
                  break;

              case SimRZ:
                
                break;

              default:
                MOZ_CRASH();
            }
        }

        
        set_s_register_from_sinteger(dst, temp);
    } else {
        bool unsigned_integer = (instr->bit(7) == 0);
        int dst = instr->VFPDRegValue(src_precision);
        int src = instr->VFPMRegValue(kSinglePrecision);

        int val = get_sinteger_from_s_register(src);

        if (src_precision == kDoublePrecision) {
            if (unsigned_integer)
                set_d_register_from_double(dst, static_cast<double>(static_cast<uint32_t>(val)));
            else
                set_d_register_from_double(dst, static_cast<double>(val));
        } else {
            if (unsigned_integer)
                set_s_register_from_float(dst, static_cast<float>(static_cast<uint32_t>(val)));
            else
                set_s_register_from_float(dst, static_cast<float>(val));
        }
    }
}


void
Simulator::decodeVCVTBetweenFloatingPointAndIntegerFrac(SimInstruction* instr)
{
    MOZ_ASSERT(instr->bits(27, 24) == 0xE && instr->opc1Value() == 0x7 && instr->bit(19) == 1 &&
               instr->bit(17) == 1 && instr->bits(11,9) == 0x5 && instr->bit(6) == 1 &&
               instr->bit(4) == 0);

    int size = (instr->bit(7) == 1) ? 32 : 16;

    int fraction_bits = size - ((instr->bits(3, 0) << 1) | instr->bit(5));
    double mult = 1 << fraction_bits;

    MOZ_ASSERT(size == 32); 

    
    bool to_fixed = (instr->bit(18) == 1);

    VFPRegPrecision precision = (instr->szValue() == 1) ? kDoublePrecision : kSinglePrecision;

    if (to_fixed) {
        
        
        
        
        

        int dst = instr->VFPDRegValue(precision);

        bool unsigned_integer = (instr->bit(16) == 1);
        bool double_precision = (precision == kDoublePrecision);

        double val = double_precision
                     ? get_double_from_d_register(dst)
                     : get_float_from_s_register(dst);

        
        val *= mult;

        
        
        int temp = unsigned_integer ? static_cast<uint32_t>(val) : static_cast<int32_t>(val);

        inv_op_vfp_flag_ = get_inv_op_vfp_flag(SimRZ, val, unsigned_integer);

        double abs_diff = unsigned_integer
                          ? std::fabs(val - static_cast<uint32_t>(temp))
                          : std::fabs(val - temp);

        inexact_vfp_flag_ = (abs_diff != 0);

        if (inv_op_vfp_flag_)
            temp = VFPConversionSaturate(val, unsigned_integer);

        
        if (double_precision) {
            uint32_t dbl[2];
            dbl[0] = temp; dbl[1] = 0;
            set_d_register(dst, dbl);
        } else {
            set_s_register_from_sinteger(dst, temp);
        }
    } else {
        MOZ_CRASH();  
    }
}

void
Simulator::decodeType6CoprocessorIns(SimInstruction* instr)
{
    MOZ_ASSERT(instr->typeValue() == 6);

    if (instr->coprocessorValue() == 0xA) {
        switch (instr->opcodeValue()) {
          case 0x8:
          case 0xA:
          case 0xC:
          case 0xE: {  
            int rn = instr->rnValue();
            int vd = instr->VFPDRegValue(kSinglePrecision);
            int offset = instr->immed8Value();
            if (!instr->hasU())
                offset = -offset;

            int32_t address = get_register(rn) + 4 * offset;
            if (instr->hasL()) {
                
                set_s_register_from_sinteger(vd, readW(address, instr));
            } else {
                
                writeW(address, get_sinteger_from_s_register(vd), instr);
            }
            break;
          }
          case 0x4:
          case 0x5:
          case 0x6:
          case 0x7:
          case 0x9:
          case 0xB:
            
            handleVList(instr);
            break;
          default:
            MOZ_CRASH();
        }
    } else if (instr->coprocessorValue() == 0xB) {
        switch (instr->opcodeValue()) {
          case 0x2:
            
            if (instr->bits(7, 6) != 0 || instr->bit(4) != 1) {
                MOZ_CRASH();  
            } else {
                int rt = instr->rtValue();
                int rn = instr->rnValue();
                int vm = instr->VFPMRegValue(kDoublePrecision);
                if (instr->hasL()) {
                    int32_t data[2];
                    double d = get_double_from_d_register(vm);
                    memcpy(data, &d, 8);
                    set_register(rt, data[0]);
                    set_register(rn, data[1]);
                } else {
                    int32_t data[] = { get_register(rt), get_register(rn) };
                    double d;
                    memcpy(&d, data, 8);
                    set_d_register_from_double(vm, d);
                }
            }
            break;
          case 0x8:
          case 0xA:
          case 0xC:
          case 0xE: {  
            int rn = instr->rnValue();
            int vd = instr->VFPDRegValue(kDoublePrecision);
            int offset = instr->immed8Value();
            if (!instr->hasU())
                offset = -offset;
            int32_t address = get_register(rn) + 4 * offset;
            if (instr->hasL()) {
                
                int32_t data[] = {
                    readW(address, instr),
                    readW(address + 4, instr)
                };
                double val;
                memcpy(&val, data, 8);
                set_d_register_from_double(vd, val);
            } else {
                
                int32_t data[2];
                double val = get_double_from_d_register(vd);
                memcpy(data, &val, 8);
                writeW(address, data[0], instr);
                writeW(address + 4, data[1], instr);
            }
            break;
          }
          case 0x4:
          case 0x5:
          case 0x6:
          case 0x7:
          case 0x9:
          case 0xB:
            
            handleVList(instr);
            break;
          default:
            MOZ_CRASH();
        }
    } else {
        MOZ_CRASH();
    }
}

void
Simulator::decodeSpecialCondition(SimInstruction* instr)
{
    switch (instr->specialValue()) {
      case 5:
        if (instr->bits(18, 16) == 0 && instr->bits(11, 6) == 0x28 && instr->bit(4) == 1) {
            
            if ((instr->vdValue() & 1) != 0)
                MOZ_CRASH("Undefined behavior");
            int Vd = (instr->bit(22) << 3) | (instr->vdValue() >> 1);
            int Vm = (instr->bit(5) << 4) | instr->vmValue();
            int imm3 = instr->bits(21, 19);
            if (imm3 != 1 && imm3 != 2 && imm3 != 4)
                MOZ_CRASH();
            int esize = 8 * imm3;
            int elements = 64 / esize;
            int8_t from[8];
            get_d_register(Vm, reinterpret_cast<uint64_t*>(from));
            int16_t to[8];
            int e = 0;
            while (e < elements) {
                to[e] = from[e];
                e++;
            }
            set_q_register(Vd, reinterpret_cast<uint64_t*>(to));
        } else {
            MOZ_CRASH();
        }
        break;
      case 7:
        if (instr->bits(18, 16) == 0 && instr->bits(11, 6) == 0x28 && instr->bit(4) == 1) {
            
            if ((instr->vdValue() & 1) != 0)
                MOZ_CRASH("Undefined behavior");
            int Vd = (instr->bit(22) << 3) | (instr->vdValue() >> 1);
            int Vm = (instr->bit(5) << 4) | instr->vmValue();
            int imm3 = instr->bits(21, 19);
            if (imm3 != 1 && imm3 != 2 && imm3 != 4)
                MOZ_CRASH();
            int esize = 8 * imm3;
            int elements = 64 / esize;
            uint8_t from[8];
            get_d_register(Vm, reinterpret_cast<uint64_t*>(from));
            uint16_t to[8];
            int e = 0;
            while (e < elements) {
                to[e] = from[e];
                e++;
            }
            set_q_register(Vd, reinterpret_cast<uint64_t*>(to));
        } else {
            MOZ_CRASH();
        }
        break;
      case 8:
        if (instr->bits(21, 20) == 0) {
            
            int Vd = (instr->bit(22) << 4) | instr->vdValue();
            int Rn = instr->vnValue();
            int type = instr->bits(11, 8);
            int Rm = instr->vmValue();
            int32_t address = get_register(Rn);
            int regs = 0;
            switch (type) {
              case nlt_1:
                regs = 1;
                break;
              case nlt_2:
                regs = 2;
                break;
              case nlt_3:
                regs = 3;
                break;
              case nlt_4:
                regs = 4;
                break;
              default:
                MOZ_CRASH();
                break;
            }
            int r = 0;
            while (r < regs) {
                uint32_t data[2];
                get_d_register(Vd + r, data);
                writeW(address, data[0], instr);
                writeW(address + 4, data[1], instr);
                address += 8;
                r++;
            }
            if (Rm != 15) {
                if (Rm == 13)
                    set_register(Rn, address);
                else
                    set_register(Rn, get_register(Rn) + get_register(Rm));
            }
        } else if (instr->bits(21, 20) == 2) {
            
            int Vd = (instr->bit(22) << 4) | instr->vdValue();
            int Rn = instr->vnValue();
            int type = instr->bits(11, 8);
            int Rm = instr->vmValue();
            int32_t address = get_register(Rn);
            int regs = 0;
            switch (type) {
              case nlt_1:
                regs = 1;
                break;
              case nlt_2:
                regs = 2;
                break;
              case nlt_3:
                regs = 3;
                break;
              case nlt_4:
                regs = 4;
                break;
              default:
                MOZ_CRASH();
                break;
            }
            int r = 0;
            while (r < regs) {
                uint32_t data[2];
                data[0] = readW(address, instr);
                data[1] = readW(address + 4, instr);
                set_d_register(Vd + r, data);
                address += 8;
                r++;
            }
            if (Rm != 15) {
                if (Rm == 13)
                    set_register(Rn, address);
                else
                    set_register(Rn, get_register(Rn) + get_register(Rm));
            }
        } else {
            MOZ_CRASH();
        }
        break;
      case 0xA:
        if (instr->bits(31,20) == 0xf57) {
            
            
            
            
            
            switch (instr->bits(7,4)) {
              case 5: 
              case 4: 
              case 6: 
                break;
              default:
                MOZ_CRASH();
            }
        } else {
            MOZ_CRASH();
        }
        break;
      case 0xB:
        if (instr->bits(22, 20) == 5 && instr->bits(15, 12) == 0xf) {
            
        } else {
            MOZ_CRASH();
        }
        break;
      case 0x1C:
      case 0x1D:
        if (instr->bit(4) == 1 && instr->bits(11,9) != 5) {
            
            decodeType7CoprocessorIns(instr);
        } else {
            MOZ_CRASH();
        }
        break;
      default:
        MOZ_CRASH();
    }
}


void
Simulator::instructionDecode(SimInstruction* instr)
{
    if (Simulator::ICacheCheckingEnabled) {
        AutoLockSimulatorCache als(this);
        CheckICacheLocked(icache(), instr);
    }

    pc_modified_ = false;

    static const uint32_t kSpecialCondition = 15 << 28;
    if (instr->conditionField() == kSpecialCondition) {
        decodeSpecialCondition(instr);
    } else if (conditionallyExecute(instr)) {
        switch (instr->typeValue()) {
          case 0:
          case 1:
            decodeType01(instr);
            break;
          case 2:
            decodeType2(instr);
            break;
          case 3:
            decodeType3(instr);
            break;
          case 4:
            decodeType4(instr);
            break;
          case 5:
            decodeType5(instr);
            break;
          case 6:
            decodeType6(instr);
            break;
          case 7:
            decodeType7(instr);
            break;
          default:
            MOZ_CRASH();
            break;
        }
        
        
    } else if (instr->isStop()) {
        set_pc(get_pc() + 2 * SimInstruction::kInstrSize);
    }
    if (!pc_modified_)
        set_register(pc, reinterpret_cast<int32_t>(instr) + SimInstruction::kInstrSize);
}

void
Simulator::enable_single_stepping(SingleStepCallback cb, void* arg)
{
    single_stepping_ = true;
    single_step_callback_ = cb;
    single_step_callback_arg_ = arg;
    single_step_callback_(single_step_callback_arg_, this, (void*)get_pc());
}

void
Simulator::disable_single_stepping()
{
    if (!single_stepping_)
        return;
    single_step_callback_(single_step_callback_arg_, this, (void*)get_pc());
    single_stepping_ = false;
    single_step_callback_ = nullptr;
    single_step_callback_arg_ = nullptr;
}

template<bool EnableStopSimAt>
void
Simulator::execute()
{
    if (single_stepping_)
        single_step_callback_(single_step_callback_arg_, this, nullptr);

    
    
    int program_counter = get_pc();

    while (program_counter != end_sim_pc) {
        if (EnableStopSimAt && (icount_ == Simulator::StopSimAt)) {
            fprintf(stderr, "\nStopped simulation at icount %lld\n", icount_);
            ArmDebugger dbg(this);
            dbg.debug();
        } else {
            if (single_stepping_)
                single_step_callback_(single_step_callback_arg_, this, (void*)program_counter);
            SimInstruction* instr = reinterpret_cast<SimInstruction*>(program_counter);
            instructionDecode(instr);
            icount_++;

            int32_t rpc = resume_pc_;
            if (MOZ_UNLIKELY(rpc != 0)) {
                
                JSRuntime::innermostAsmJSActivation()->setResumePC((void*)get_pc());
                set_pc(rpc);
                resume_pc_ = 0;
            }
        }
        program_counter = get_pc();
    }

    if (single_stepping_)
        single_step_callback_(single_step_callback_arg_, this, nullptr);
}

void
Simulator::callInternal(uint8_t* entry)
{
    
    set_register(pc, reinterpret_cast<int32_t>(entry));

    
    
    
    set_register(lr, end_sim_pc);

    
    
    
    int32_t r4_val = get_register(r4);
    int32_t r5_val = get_register(r5);
    int32_t r6_val = get_register(r6);
    int32_t r7_val = get_register(r7);
    int32_t r8_val = get_register(r8);
    int32_t r9_val = get_register(r9);
    int32_t r10_val = get_register(r10);
    int32_t r11_val = get_register(r11);

    
    uint64_t d8_val;
    get_d_register(d8, &d8_val);
    uint64_t d9_val;
    get_d_register(d9, &d9_val);
    uint64_t d10_val;
    get_d_register(d10, &d10_val);
    uint64_t d11_val;
    get_d_register(d11, &d11_val);
    uint64_t d12_val;
    get_d_register(d12, &d12_val);
    uint64_t d13_val;
    get_d_register(d13, &d13_val);
    uint64_t d14_val;
    get_d_register(d14, &d14_val);
    uint64_t d15_val;
    get_d_register(d15, &d15_val);

    
    
    int32_t callee_saved_value = uint32_t(icount_);
    uint64_t callee_saved_value_d = uint64_t(icount_);

    if (!skipCalleeSavedRegsCheck) {
        set_register(r4, callee_saved_value);
        set_register(r5, callee_saved_value);
        set_register(r6, callee_saved_value);
        set_register(r7, callee_saved_value);
        set_register(r8, callee_saved_value);
        set_register(r9, callee_saved_value);
        set_register(r10, callee_saved_value);
        set_register(r11, callee_saved_value);

        set_d_register(d8, &callee_saved_value_d);
        set_d_register(d9, &callee_saved_value_d);
        set_d_register(d10, &callee_saved_value_d);
        set_d_register(d11, &callee_saved_value_d);
        set_d_register(d12, &callee_saved_value_d);
        set_d_register(d13, &callee_saved_value_d);
        set_d_register(d14, &callee_saved_value_d);
        set_d_register(d15, &callee_saved_value_d);

    }
    
    if (Simulator::StopSimAt != -1L)
        execute<true>();
    else
        execute<false>();

    if (!skipCalleeSavedRegsCheck) {
        
        MOZ_ASSERT(callee_saved_value == get_register(r4));
        MOZ_ASSERT(callee_saved_value == get_register(r5));
        MOZ_ASSERT(callee_saved_value == get_register(r6));
        MOZ_ASSERT(callee_saved_value == get_register(r7));
        MOZ_ASSERT(callee_saved_value == get_register(r8));
        MOZ_ASSERT(callee_saved_value == get_register(r9));
        MOZ_ASSERT(callee_saved_value == get_register(r10));
        MOZ_ASSERT(callee_saved_value == get_register(r11));

        uint64_t value;
        get_d_register(d8, &value);
        MOZ_ASSERT(callee_saved_value_d == value);
        get_d_register(d9, &value);
        MOZ_ASSERT(callee_saved_value_d == value);
        get_d_register(d10, &value);
        MOZ_ASSERT(callee_saved_value_d == value);
        get_d_register(d11, &value);
        MOZ_ASSERT(callee_saved_value_d == value);
        get_d_register(d12, &value);
        MOZ_ASSERT(callee_saved_value_d == value);
        get_d_register(d13, &value);
        MOZ_ASSERT(callee_saved_value_d == value);
        get_d_register(d14, &value);
        MOZ_ASSERT(callee_saved_value_d == value);
        get_d_register(d15, &value);
        MOZ_ASSERT(callee_saved_value_d == value);

        
        set_register(r4, r4_val);
        set_register(r5, r5_val);
        set_register(r6, r6_val);
        set_register(r7, r7_val);
        set_register(r8, r8_val);
        set_register(r9, r9_val);
        set_register(r10, r10_val);
        set_register(r11, r11_val);

        set_d_register(d8, &d8_val);
        set_d_register(d9, &d9_val);
        set_d_register(d10, &d10_val);
        set_d_register(d11, &d11_val);
        set_d_register(d12, &d12_val);
        set_d_register(d13, &d13_val);
        set_d_register(d14, &d14_val);
        set_d_register(d15, &d15_val);
    }
}

int64_t
Simulator::call(uint8_t* entry, int argument_count, ...)
{
    va_list parameters;
    va_start(parameters, argument_count);

    
    MOZ_ASSERT(argument_count >= 1);
    set_register(r0, va_arg(parameters, int32_t));
    if (argument_count >= 2)
        set_register(r1, va_arg(parameters, int32_t));
    if (argument_count >= 3)
        set_register(r2, va_arg(parameters, int32_t));
    if (argument_count >= 4)
        set_register(r3, va_arg(parameters, int32_t));

    
    int original_stack = get_register(sp);
    int entry_stack = original_stack;
    if (argument_count >= 4)
        entry_stack -= (argument_count - 4) * sizeof(int32_t);

    entry_stack &= ~ABIStackAlignment;

    
    intptr_t* stack_argument = reinterpret_cast<intptr_t*>(entry_stack);
    for (int i = 4; i < argument_count; i++)
        stack_argument[i - 4] = va_arg(parameters, int32_t);
    va_end(parameters);
    set_register(sp, entry_stack);

    callInternal(entry);

    
    MOZ_ASSERT(entry_stack == get_register(sp));
    set_register(sp, original_stack);

    int64_t result = (int64_t(get_register(r1)) << 32) | get_register(r0);
    return result;
}

Simulator*
Simulator::Current()
{
    return TlsPerThreadData.get()->simulator();
}

} 
} 

js::jit::Simulator*
JSRuntime::simulator() const
{
    return simulator_;
}

uintptr_t*
JSRuntime::addressOfSimulatorStackLimit()
{
    return simulator_->addressOfStackLimit();
}

js::jit::Simulator*
js::PerThreadData::simulator() const
{
    return runtime_->simulator();
}
