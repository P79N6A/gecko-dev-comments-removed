



























#include "jit/mips/Simulator-mips.h"

#include "mozilla/Casting.h"
#include "mozilla/FloatingPoint.h"
#include "mozilla/Likely.h"
#include "mozilla/MathAlgorithms.h"

#include <float.h>

#include "asmjs/AsmJSValidate.h"
#include "jit/mips/Assembler-mips.h"
#include "vm/Runtime.h"

namespace js {
namespace jit {

static const Instr kCallRedirInstr = op_special | MAX_BREAK_CODE << RTShift | ff_break;


static bool
HaveSameSign(int32_t a, int32_t b)
{
    return ((a ^ b) >= 0);
}

static uint32_t
GetFCSRConditionBit(uint32_t cc)
{
    if (cc == 0) {
        return 23;
    } else {
        return 24 + cc;
    }
}

static const int32_t kRegisterskMaxValue = 0x7fffffff;
static const int32_t kRegisterskMinValue = 0x80000000;




class SimInstruction
{
  public:
    enum {
        kInstrSize = 4,
        
        
        kPCReadOffset = 0
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

    
    enum Type {
        kRegisterType,
        kImmediateType,
        kJumpType,
        kUnsupported = -1
    };

    
    Type instructionType() const;


    
    inline Opcode opcodeValue() const {
        return static_cast<Opcode>(bits(OpcodeShift + OpcodeBits - 1, OpcodeShift));
    }

    inline int rsValue() const {
        MOZ_ASSERT(instructionType() == kRegisterType || instructionType() == kImmediateType);
        return bits(RSShift + RSBits - 1, RSShift);
    }

    inline int rtValue() const {
        MOZ_ASSERT(instructionType() == kRegisterType || instructionType() == kImmediateType);
        return bits(RTShift + RTBits - 1, RTShift);
    }

    inline int rdValue() const {
        MOZ_ASSERT(instructionType() == kRegisterType);
        return bits(RDShift + RDBits - 1, RDShift);
    }

    inline int saValue() const {
        MOZ_ASSERT(instructionType() == kRegisterType);
        return bits(SAShift + SABits - 1, SAShift);
    }

    inline int functionValue() const {
        MOZ_ASSERT(instructionType() == kRegisterType || instructionType() == kImmediateType);
        return bits(FunctionShift + FunctionBits - 1, FunctionShift);
    }

    inline int fdValue() const {
        return bits(FDShift + FDBits - 1, FDShift);
    }

    inline int fsValue() const {
        return bits(FSShift + FSBits - 1, FSShift);
    }

    inline int ftValue() const {
        return bits(FTShift + FTBits - 1, FTShift);
    }

    inline int frValue() const {
        return bits(FRShift + FRBits - 1, FRShift);
    }

    
    inline int fcccValue() const {
        return bits(FCccShift + FCccBits - 1, FCccShift);
    }

    
    inline int fbccValue() const {
        return bits(FBccShift + FBccBits - 1, FBccShift);
    }

    
    inline int fbtrueValue() const {
        return bits(FBtrueShift + FBtrueBits - 1, FBtrueShift);
    }

    
    inline Opcode opcodeFieldRaw() const {
        return static_cast<Opcode>(instructionBits() & OpcodeMask);
    }

    inline int rsFieldRaw() const {
        MOZ_ASSERT(instructionType() == kRegisterType || instructionType() == kImmediateType);
        return instructionBits() & RSMask;
    }

    
    inline int rsFieldRawNoAssert() const {
        return instructionBits() & RSMask;
    }

    inline int rtFieldRaw() const {
        MOZ_ASSERT(instructionType() == kRegisterType || instructionType() == kImmediateType);
        return instructionBits() & RTMask;
    }

    inline int rdFieldRaw() const {
        MOZ_ASSERT(instructionType() == kRegisterType);
        return instructionBits() & RDMask;
    }

    inline int saFieldRaw() const {
        MOZ_ASSERT(instructionType() == kRegisterType);
        return instructionBits() & SAMask;
    }

    inline int functionFieldRaw() const {
        return instructionBits() & FunctionMask;
    }

    
    inline int secondaryValue() const {
        Opcode op = opcodeFieldRaw();
        switch (op) {
          case op_special:
          case op_special2:
            return functionValue();
          case op_cop1:
            return rsValue();
          case op_regimm:
            return rtValue();
          default:
            return ff_null;
        }
    }

    inline int32_t imm16Value() const {
        MOZ_ASSERT(instructionType() == kImmediateType);
        return bits(Imm16Shift + Imm16Bits - 1, Imm16Shift);
    }

    inline int32_t imm26Value() const {
        MOZ_ASSERT(instructionType() == kJumpType);
        return bits(Imm26Shift + Imm26Bits - 1, Imm26Shift);
    }

    
    bool isForbiddenInBranchDelay() const;
    
    bool isLinkingInstruction() const;
    
    bool isTrap() const;

  private:

    SimInstruction() MOZ_DELETE;
    SimInstruction(const SimInstruction &other) MOZ_DELETE;
    void operator=(const SimInstruction &other) MOZ_DELETE;
};

bool
SimInstruction::isForbiddenInBranchDelay() const
{
    const int op = opcodeFieldRaw();
    switch (op) {
      case op_j:
      case op_jal:
      case op_beq:
      case op_bne:
      case op_blez:
      case op_bgtz:
      case op_beql:
      case op_bnel:
      case op_blezl:
      case op_bgtzl:
        return true;
      case op_regimm:
        switch (rtFieldRaw()) {
          case rt_bltz:
          case rt_bgez:
          case rt_bltzal:
          case rt_bgezal:
            return true;
          default:
            return false;
        };
        break;
      case op_special:
        switch (functionFieldRaw()) {
          case ff_jr:
          case ff_jalr:
            return true;
          default:
            return false;
        };
        break;
      default:
        return false;
    };
}

bool
SimInstruction::isLinkingInstruction() const
{
    const int op = opcodeFieldRaw();
    switch (op) {
      case op_jal:
        return true;
      case op_regimm:
        switch (rtFieldRaw()) {
          case rt_bgezal:
          case rt_bltzal:
            return true;
          default:
            return false;
        };
      case op_special:
        switch (functionFieldRaw()) {
          case ff_jalr:
            return true;
          default:
            return false;
        };
      default:
        return false;
    };
}

bool
SimInstruction::isTrap() const
{
    if (opcodeFieldRaw() != op_special) {
        return false;
    } else {
        switch (functionFieldRaw()) {
          case ff_break:
          case ff_tge:
          case ff_tgeu:
          case ff_tlt:
          case ff_tltu:
          case ff_teq:
          case ff_tne:
            return true;
          default:
            return false;
        };
    }
}

SimInstruction::Type
SimInstruction::instructionType() const
{
    switch (opcodeFieldRaw()) {
      case op_special:
        switch (functionFieldRaw()) {
          case ff_jr:
          case ff_jalr:
          case ff_break:
          case ff_sll:
          case ff_srl:
          case ff_sra:
          case ff_sllv:
          case ff_srlv:
          case ff_srav:
          case ff_mfhi:
          case ff_mflo:
          case ff_mult:
          case ff_multu:
          case ff_div:
          case ff_divu:
          case ff_add:
          case ff_addu:
          case ff_sub:
          case ff_subu:
          case ff_and:
          case ff_or:
          case ff_xor:
          case ff_nor:
          case ff_slt:
          case ff_sltu:
          case ff_tge:
          case ff_tgeu:
          case ff_tlt:
          case ff_tltu:
          case ff_teq:
          case ff_tne:
          case ff_movz:
          case ff_movn:
          case ff_movci:
            return kRegisterType;
          default:
            return kUnsupported;
        };
        break;
      case op_special2:
        switch (functionFieldRaw()) {
          case ff_mul:
          case ff_clz:
            return kRegisterType;
          default:
            return kUnsupported;
        };
        break;
      case op_special3:
        switch (functionFieldRaw()) {
          case ff_ins:
          case ff_ext:
            return kRegisterType;
          default:
            return kUnsupported;
        };
        break;
      case op_cop1:    
        switch (rsFieldRawNoAssert()) {
          case rs_bc1:   
            return kImmediateType;
          default:
            return kRegisterType;
        };
        break;
      case op_cop1x:
        return kRegisterType;
        
      case op_regimm:
      case op_beq:
      case op_bne:
      case op_blez:
      case op_bgtz:
      case op_addi:
      case op_addiu:
      case op_slti:
      case op_sltiu:
      case op_andi:
      case op_ori:
      case op_xori:
      case op_lui:
      case op_beql:
      case op_bnel:
      case op_blezl:
      case op_bgtzl:
      case op_lb:
      case op_lh:
      case op_lwl:
      case op_lw:
      case op_lbu:
      case op_lhu:
      case op_lwr:
      case op_sb:
      case op_sh:
      case op_swl:
      case op_sw:
      case op_swr:
      case op_lwc1:
      case op_ldc1:
      case op_swc1:
      case op_sdc1:
        return kImmediateType;
        
      case op_j:
      case op_jal:
        return kJumpType;
      default:
        return kUnsupported;
    };
    return kUnsupported;
}


const int kCArgSlotCount = 4;
const int kCArgsSlotsSize = kCArgSlotCount * SimInstruction::kInstrSize;
const int kBranchReturnOffset = 2 * SimInstruction::kInstrSize;

class CachePage {
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

    char *validityByte(int offset) {
        return &validity_map_[offset >> kLineShift];
    }

    char *cachedData(int offset) {
        return &data_[offset];
    }

  private:
    char data_[kPageSize];   
    static const int kValidityMapSize = kPageSize >> kLineShift;
    char validity_map_[kValidityMapSize];  
};

class Redirection;

class SimulatorRuntime
{
    friend class AutoLockSimulatorRuntime;

    Redirection *redirection_;

    
    struct ICacheHasher {
        typedef void *Key;
        typedef void *Lookup;
        static HashNumber hash(const Lookup &l);
        static bool match(const Key &k, const Lookup &l);
    };

  public:
    typedef HashMap<void *, CachePage *, ICacheHasher, SystemAllocPolicy> ICacheMap;

  protected:
    ICacheMap icache_;

    
    PRLock *lock_;
    mozilla::DebugOnly<PRThread *> lockOwner_;

  public:
    SimulatorRuntime()
      : redirection_(nullptr),
        lock_(nullptr),
        lockOwner_(nullptr) {}
    ~SimulatorRuntime();
    bool init() {
        lock_ = PR_NewLock();
        if (!lock_)
            return false;
        if (!icache_.init())
            return false;
        return true;
    }
    ICacheMap &icache() {
        MOZ_ASSERT(lockOwner_ == PR_GetCurrentThread());
        return icache_;
    }
    Redirection *redirection() const {
        MOZ_ASSERT(lockOwner_ == PR_GetCurrentThread());
        return redirection_;
    }
    void setRedirection(js::jit::Redirection *redirection) {
        MOZ_ASSERT(lockOwner_ == PR_GetCurrentThread());
        redirection_ = redirection;
    }
};

class AutoLockSimulatorRuntime
{
  protected:
    SimulatorRuntime *srt_;

  public:
    AutoLockSimulatorRuntime(SimulatorRuntime *srt)
        : srt_(srt) {
        PR_Lock(srt_->lock_);
        MOZ_ASSERT(!srt_->lockOwner_);
#ifdef DEBUG
        srt_->lockOwner_ = PR_GetCurrentThread();
#endif
    }

    ~AutoLockSimulatorRuntime() {
        MOZ_ASSERT(srt_->lockOwner_ == PR_GetCurrentThread());
        srt_->lockOwner_ = nullptr;
        PR_Unlock(srt_->lock_);
    }
};

bool Simulator::ICacheCheckingEnabled = false;

int Simulator::StopSimAt = -1;

SimulatorRuntime *
CreateSimulatorRuntime()
{
    SimulatorRuntime *srt = js_new<SimulatorRuntime>();
    if (!srt)
        return nullptr;

    if (!srt->init()) {
        js_delete(srt);
        return nullptr;
    }

    if (getenv("MIPS_SIM_ICACHE_CHECKS"))
        Simulator::ICacheCheckingEnabled = true;

    char *stopAtStr = getenv("MIPS_SIM_STOP_AT");
    int64_t stopAt;
    if (stopAtStr && sscanf(stopAtStr, "%lld", &stopAt) == 1) {
        fprintf(stderr, "\nStopping simulation at icount %lld\n", stopAt);
        Simulator::StopSimAt = stopAt;
    }

    return srt;
}

void
DestroySimulatorRuntime(SimulatorRuntime *srt)
{
    js_delete(srt);
}



class MipsDebugger
{
  public:
    explicit MipsDebugger(Simulator *sim) : sim_(sim) { }

    void stop(SimInstruction *instr);
    void debug();
    
    void printAllRegs();
    void printAllRegsIncludingFPU();

  private:
    
    static const Instr kBreakpointInstr = op_special | ff_break | 0xfffff << 6;
    static const Instr kNopInstr =  op_special | ff_sll;

    Simulator *sim_;

    int32_t getRegisterValue(int regnum);
    int32_t getFPURegisterValueInt(int regnum);
    int64_t getFPURegisterValueLong(int regnum);
    float getFPURegisterValueFloat(int regnum);
    double getFPURegisterValueDouble(int regnum);
    bool getValue(const char *desc, int32_t *value);

    
    bool setBreakpoint(SimInstruction *breakpc);
    bool deleteBreakpoint(SimInstruction *breakpc);

    
    
    void undoBreakpoints();
    void redoBreakpoints();
};

static void
UNSUPPORTED()
{
    printf("Unsupported instruction.\n");
    MOZ_CRASH();
}

void
MipsDebugger::stop(SimInstruction *instr)
{
    
    uint32_t code = instr->bits(25, 6);
    
    char *msg = *reinterpret_cast<char**>(sim_->get_pc() +
                                          SimInstruction::kInstrSize);
    
    if (!sim_->watchedStops_[code].desc_) {
        sim_->watchedStops_[code].desc_ = msg;
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
MipsDebugger::getRegisterValue(int regnum)
{
    if (regnum == kPCRegister)
        return sim_->get_pc();
    return sim_->getRegister(regnum);
}

int32_t MipsDebugger::getFPURegisterValueInt(int regnum)
{
    return sim_->getFpuRegister(regnum);
}

int64_t
MipsDebugger::getFPURegisterValueLong(int regnum)
{
    return sim_->getFpuRegisterLong(regnum);
}

float
MipsDebugger::getFPURegisterValueFloat(int regnum)
{
    return sim_->getFpuRegisterFloat(regnum);
}

double
MipsDebugger::getFPURegisterValueDouble(int regnum)
{
    return sim_->getFpuRegisterDouble(regnum);
}

bool
MipsDebugger::getValue(const char *desc, int32_t *value)
{
    Register reg = Register::FromName(desc);
    if (reg != InvalidReg) {
        *value = getRegisterValue(reg.code());
        return true;
    }

    if (strncmp(desc, "0x", 2) == 0) {
        return sscanf(desc, "%x", reinterpret_cast<uint32_t*>(value)) == 1;
    }
    return sscanf(desc, "%i", value) == 1;
}

bool
MipsDebugger::setBreakpoint(SimInstruction *breakpc)
{
    
    if (sim_->break_pc_ != nullptr)
        return false;

    
    sim_->break_pc_ = breakpc;
    sim_->break_instr_ = breakpc->instructionBits();
    
    
    return true;

}

bool
MipsDebugger::deleteBreakpoint(SimInstruction *breakpc)
{
    if (sim_->break_pc_ != nullptr)
        sim_->break_pc_->setInstructionBits(sim_->break_instr_);

    sim_->break_pc_ = nullptr;
    sim_->break_instr_ = 0;
    return true;
}

void
MipsDebugger::undoBreakpoints()
{
    if (sim_->break_pc_)
        sim_->break_pc_->setInstructionBits(sim_->break_instr_);
}

void
MipsDebugger::redoBreakpoints()
{
    if (sim_->break_pc_)
        sim_->break_pc_->setInstructionBits(kBreakpointInstr);
}

void
MipsDebugger::printAllRegs()
{
    int32_t value;
    for (uint32_t i = 0; i < Registers::Total; i++) {
        value = getRegisterValue(i);
        printf("%3s: 0x%08x %10d   ", Registers::GetName(i), value, value);

        if (i % 2)
            printf("\n");
    }
    printf("\n");

    value = getRegisterValue(Simulator::LO);
    printf(" LO: 0x%08x %10d   ", value, value);
    value = getRegisterValue(Simulator::HI);
    printf(" HI: 0x%08x %10d\n", value, value);
    value = getRegisterValue(Simulator::pc);
    printf(" pc: 0x%08x\n", value);
}

void
MipsDebugger::printAllRegsIncludingFPU()
{
    printAllRegs();

    printf("\n\n");
    
    for (uint32_t i = 0; i < FloatRegisters::TotalSingle; i++) {
        if (i & 0x1) {
            printf("%3s: 0x%08x\tflt: %-8.4g\n",
                   FloatRegisters::GetName(i),
                   getFPURegisterValueInt(i),
                   getFPURegisterValueFloat(i));
        } else {
            printf("%3s: 0x%08x\tflt: %-8.4g\tdbl: %-16.4g\n",
                   FloatRegisters::GetName(i),
                   getFPURegisterValueInt(i),
                   getFPURegisterValueFloat(i),
                   getFPURegisterValueDouble(i));
        }
    }

}

static char *
ReadLine(const char *prompt)
{
    char *result = nullptr;
    char lineBuf[256];
    int offset = 0;
    bool keepGoing = true;
    fprintf(stdout, "%s", prompt);
    fflush(stdout);
    while (keepGoing) {
        if (fgets(lineBuf, sizeof(lineBuf), stdin) == nullptr) {
            
            if (result)
                js_delete(result);
            return nullptr;
        }
        int len = strlen(lineBuf);
        if (len > 0 && lineBuf[len - 1] == '\n') {
            
            
            keepGoing = false;
        }
        if (!result) {
            
            result = (char *)js_malloc(len + 1);
            if (!result)
                return nullptr;
        } else {
            
            int new_len = offset + len + 1;
            char *new_result = (char *)js_malloc(new_len);
            if (!new_result)
                return nullptr;
            
            
            memcpy(new_result, result, offset * sizeof(char));
            js_free(result);
            result = new_result;
        }
        
        memcpy(result + offset, lineBuf, len * sizeof(char));
        offset += len;
    }

    MOZ_ASSERT(result);
    result[offset] = '\0';
    return result;
}

static void
DisassembleInstruction(uint32_t pc)
{
    uint8_t *bytes = reinterpret_cast<uint8_t *>(pc);
    char hexbytes[256];
    sprintf(hexbytes, "0x%x 0x%x 0x%x 0x%x", bytes[0], bytes[1], bytes[2], bytes[3]);
    char llvmcmd[1024];
    sprintf(llvmcmd, "bash -c \"echo -n '%p'; echo '%s' | "
            "llvm-mc -disassemble -arch=mipsel -mcpu=mips32r2 | "
            "grep -v pure_instructions | grep -v .text\"", static_cast<void*>(bytes), hexbytes);
    if (system(llvmcmd))
        printf("Cannot disassemble instruction.\n");
}

void
MipsDebugger::debug()
{
    intptr_t lastPC = -1;
    bool done = false;

#define COMMAND_SIZE 63
#define ARG_SIZE 255

#define STR(a) #a
#define XSTR(a) STR(a)

    char cmd[COMMAND_SIZE + 1];
    char arg1[ARG_SIZE + 1];
    char arg2[ARG_SIZE + 1];
    char *argv[3] = { cmd, arg1, arg2 };

    
    cmd[COMMAND_SIZE] = 0;
    arg1[ARG_SIZE] = 0;
    arg2[ARG_SIZE] = 0;

    
    
    undoBreakpoints();

    while (!done && (sim_->get_pc() != Simulator::end_sim_pc)) {
        if (lastPC != sim_->get_pc()) {
            DisassembleInstruction(sim_->get_pc());
            lastPC = sim_->get_pc();
        }
        char *line = ReadLine("sim> ");
        if (line == nullptr) {
            break;
        } else {
            char *last_input = sim_->lastDebuggerInput();
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
            if ((strcmp(cmd, "si") == 0) || (strcmp(cmd, "stepi") == 0)) {
                SimInstruction *instr = reinterpret_cast<SimInstruction*>(sim_->get_pc());
                if (!(instr->isTrap()) ||
                        instr->instructionBits() == kCallRedirInstr) {
                    sim_->instructionDecode(
                        reinterpret_cast<SimInstruction*>(sim_->get_pc()));
                } else {
                    
                    printf("/!\\ Jumping over generated breakpoint.\n");
                    sim_->set_pc(sim_->get_pc() + SimInstruction::kInstrSize);
                }
            } else if ((strcmp(cmd, "c") == 0) || (strcmp(cmd, "cont") == 0)) {
                
                sim_->instructionDecode(reinterpret_cast<SimInstruction*>(sim_->get_pc()));
                
                done = true;
            } else if ((strcmp(cmd, "p") == 0) || (strcmp(cmd, "print") == 0)) {
                if (argc == 2) {
                    int32_t value;
                    if (strcmp(arg1, "all") == 0) {
                        printAllRegs();
                    } else if (strcmp(arg1, "allf") == 0) {
                        printAllRegsIncludingFPU();
                    } else {
                        Register reg = Register::FromName(arg1);
                        FloatRegisters::Code fCode = FloatRegister::FromName(arg1);
                        if (reg != InvalidReg) {
                            value = getRegisterValue(reg.code());
                            printf("%s: 0x%08x %d \n", arg1, value, value);
                        } else if (fCode != FloatRegisters::Invalid) {
                            if (fCode & 0x1) {
                                printf("%3s: 0x%08x\tflt: %-8.4g\n",
                                       FloatRegisters::GetName(fCode),
                                       getFPURegisterValueInt(fCode),
                                       getFPURegisterValueFloat(fCode));
                            } else {
                                printf("%3s: 0x%08x\tflt: %-8.4g\tdbl: %-16.4g\n",
                                       FloatRegisters::GetName(fCode),
                                       getFPURegisterValueInt(fCode),
                                       getFPURegisterValueFloat(fCode),
                                       getFPURegisterValueDouble(fCode));
                            }
                        } else {
                            printf("%s unrecognized\n", arg1);
                        }
                    }
                } else {
                    printf("print <register> or print <fpu register> single\n");
                }
            } else if (strcmp(cmd, "stack") == 0 || strcmp(cmd, "mem") == 0) {
                int32_t *cur = nullptr;
                int32_t *end = nullptr;
                int next_arg = 1;

                if (strcmp(cmd, "stack") == 0) {
                    cur = reinterpret_cast<int32_t*>(sim_->getRegister(Simulator::sp));
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

            } else if ((strcmp(cmd, "disasm") == 0) ||
                       (strcmp(cmd, "dpc") == 0) ||
                       (strcmp(cmd, "di") == 0)) {
                uint8_t *cur = nullptr;
                uint8_t *end = nullptr;

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
                printf("No flags on MIPS !\n");
            } else if (strcmp(cmd, "stop") == 0) {
                int32_t value;
                intptr_t stop_pc = sim_->get_pc() -
                                   2 * SimInstruction::kInstrSize;
                SimInstruction *stop_instr = reinterpret_cast<SimInstruction*>(stop_pc);
                SimInstruction *msg_address =
                    reinterpret_cast<SimInstruction*>(stop_pc +
                                                      SimInstruction::kInstrSize);
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
                            for (uint32_t i = kMaxWatchpointCode + 1;
                                    i <= kMaxStopCode;
                                    i++) {
                                sim_->printStopInfo(i);
                            }
                        } else if (getValue(arg2, &value)) {
                            sim_->printStopInfo(value);
                        } else {
                            printf("Unrecognized argument.\n");
                        }
                    } else if (strcmp(arg1, "enable") == 0) {
                        
                        if (strcmp(arg2, "all") == 0) {
                            for (uint32_t i = kMaxWatchpointCode + 1;
                                    i <= kMaxStopCode;
                                    i++) {
                                sim_->enableStop(i);
                            }
                        } else if (getValue(arg2, &value)) {
                            sim_->enableStop(value);
                        } else {
                            printf("Unrecognized argument.\n");
                        }
                    } else if (strcmp(arg1, "disable") == 0) {
                        
                        if (strcmp(arg2, "all") == 0) {
                            for (uint32_t i = kMaxWatchpointCode + 1;
                                    i <= kMaxStopCode;
                                    i++) {
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
                printf("stepi\n");
                printf("  step one instruction (alias 'si')\n");
                printf("print <register>\n");
                printf("  print register content (alias 'p')\n");
                printf("  use register name 'all' to print all registers\n");
                printf("printobject <register>\n");
                printf("  print an object from a register (alias 'po')\n");
                printf("stack [<words>]\n");
                printf("  dump stack content, default dump 10 words)\n");
                printf("mem <address> [<words>]\n");
                printf("  dump memory content, default dump 10 words)\n");
                printf("flags\n");
                printf("  print flags\n");
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
                printf("    stop and and give control to the Debugger.\n");
                printf("    All stop codes are watched:\n");
                printf("    - They can be enabled / disabled: the Simulator\n");
                printf("       will / won't stop when hitting them.\n");
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

void
Simulator::setLastDebuggerInput(char *input)
{
    js_free(lastDebuggerInput_);
    lastDebuggerInput_ = input;
}

static CachePage *
GetCachePage(SimulatorRuntime::ICacheMap &i_cache, void *page)
{
    SimulatorRuntime::ICacheMap::AddPtr p = i_cache.lookupForAdd(page);
    if (p)
        return p->value();

    CachePage *new_page = js_new<CachePage>();
    if (!i_cache.add(p, page, new_page))
        return nullptr;
    return new_page;
}


static void
FlushOnePage(SimulatorRuntime::ICacheMap &i_cache, intptr_t start, int size)
{
    MOZ_ASSERT(size <= CachePage::kPageSize);
    MOZ_ASSERT(AllOnOnePage(start, size - 1));
    MOZ_ASSERT((start & CachePage::kLineMask) == 0);
    MOZ_ASSERT((size & CachePage::kLineMask) == 0);
    void *page = reinterpret_cast<void*>(start & (~CachePage::kPageMask));
    int offset = (start & CachePage::kPageMask);
    CachePage *cache_page = GetCachePage(i_cache, page);
    char *valid_bytemap = cache_page->validityByte(offset);
    memset(valid_bytemap, CachePage::LINE_INVALID, size >> CachePage::kLineShift);
}

static void
FlushICache(SimulatorRuntime::ICacheMap &i_cache, void *start_addr, size_t size)
{
    intptr_t start = reinterpret_cast<intptr_t>(start_addr);
    int intra_line = (start & CachePage::kLineMask);
    start -= intra_line;
    size += intra_line;
    size = ((size - 1) | CachePage::kLineMask) + 1;
    int offset = (start & CachePage::kPageMask);
    while (!AllOnOnePage(start, size - 1)) {
        int bytes_to_flush = CachePage::kPageSize - offset;
        FlushOnePage(i_cache, start, bytes_to_flush);
        start += bytes_to_flush;
        size -= bytes_to_flush;
        MOZ_ASSERT((start & CachePage::kPageMask) == 0);
        offset = 0;
    }
    if (size != 0) {
        FlushOnePage(i_cache, start, size);
    }
}

static void
CheckICache(SimulatorRuntime::ICacheMap &i_cache, SimInstruction *instr)
{
    intptr_t address = reinterpret_cast<intptr_t>(instr);
    void *page = reinterpret_cast<void*>(address & (~CachePage::kPageMask));
    void *line = reinterpret_cast<void*>(address & (~CachePage::kLineMask));
    int offset = (address & CachePage::kPageMask);
    CachePage *cache_page = GetCachePage(i_cache, page);
    char *cache_valid_byte = cache_page->validityByte(offset);
    bool cache_hit = (*cache_valid_byte == CachePage::LINE_VALID);
    char *cached_line = cache_page->cachedData(offset & ~CachePage::kLineMask);
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
SimulatorRuntime::ICacheHasher::hash(const Lookup &l)
{
    return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(l)) >> 2;
}

bool
SimulatorRuntime::ICacheHasher::match(const Key &k, const Lookup &l)
{
    MOZ_ASSERT((reinterpret_cast<intptr_t>(k) & CachePage::kPageMask) == 0);
    MOZ_ASSERT((reinterpret_cast<intptr_t>(l) & CachePage::kPageMask) == 0);
    return k == l;
}

void
Simulator::FlushICache(void *start_addr, size_t size)
{
    SimulatorRuntime *srt = TlsPerThreadData.get()->simulatorRuntime();
    AutoLockSimulatorRuntime alsr(srt);
    js::jit::FlushICache(srt->icache(), start_addr, size);
}

Simulator::~Simulator()
{
    js_free(stack_);
}

Simulator::Simulator(SimulatorRuntime *srt)
    : srt_(srt)
{
    
    

    
    
    static const size_t stackSize = 2 * 1024 * 1024;
    stack_ = static_cast<char*>(js_malloc(stackSize));
    if (!stack_) {
        MOZ_ReportAssertionFailure("[unhandlable oom] Simulator stack", __FILE__, __LINE__);
        MOZ_CRASH();
    }
    pc_modified_ = false;
    icount_ = 0;
    break_count_ = 0;
    resume_pc_ = 0;
    break_pc_ = nullptr;
    break_instr_ = 0;

    
    
    for (int i = 0; i < Register::kNumSimuRegisters; i++) {
        registers_[i] = 0;
    }
    for (int i = 0; i < Simulator::FPURegister::kNumFPURegisters; i++) {
        FPUregisters_[i] = 0;
    }
    FCSR_ = 0;

    
    
    
    registers_[sp] = reinterpret_cast<int32_t>(stack_) + stackSize - 64;
    
    
    registers_[pc] = bad_ra;
    registers_[ra] = bad_ra;

    for (int i = 0; i < kNumExceptions; i++)
        exceptions[i] = 0;

    lastDebuggerInput_ = nullptr;
}








class Redirection
{
    friend class SimulatorRuntime;

    Redirection(void* nativeFunction, ABIFunctionType type, SimulatorRuntime *srt)
      : nativeFunction_(nativeFunction),
        swiInstruction_(kCallRedirInstr),
        type_(type),
        next_(nullptr)
    {
        next_ = srt->redirection();
	if (Simulator::ICacheCheckingEnabled)
	    FlushICache(srt->icache(), addressOfSwiInstruction(), SimInstruction::kInstrSize);
        srt->setRedirection(this);
    }

  public:
    void *addressOfSwiInstruction() { return &swiInstruction_; }
    void *nativeFunction() const { return nativeFunction_; }
    ABIFunctionType type() const { return type_; }

    static Redirection *Get(void *nativeFunction, ABIFunctionType type) {
        PerThreadData *pt = TlsPerThreadData.get();
        SimulatorRuntime *srt = pt->simulatorRuntime();
        AutoLockSimulatorRuntime alsr(srt);

        JS_ASSERT_IF(pt->simulator(), pt->simulator()->srt_ == srt);

        Redirection *current = srt->redirection();
        for (; current != nullptr; current = current->next_) {
            if (current->nativeFunction_ == nativeFunction) {
                MOZ_ASSERT(current->type() == type);
                return current;
            }
        }

        Redirection *redir = (Redirection *)js_malloc(sizeof(Redirection));
        if (!redir) {
            MOZ_ReportAssertionFailure("[unhandlable oom] Simulator redirection",
                                       __FILE__, __LINE__);
            MOZ_CRASH();
        }
        new(redir) Redirection(nativeFunction, type, srt);
        return redir;
    }

    static Redirection *FromSwiInstruction(SimInstruction *swiInstruction) {
        uint8_t *addrOfSwi = reinterpret_cast<uint8_t*>(swiInstruction);
        uint8_t *addrOfRedirection = addrOfSwi - offsetof(Redirection, swiInstruction_);
        return reinterpret_cast<Redirection*>(addrOfRedirection);
    }

  private:
    void *nativeFunction_;
    uint32_t swiInstruction_;
    ABIFunctionType type_;
    Redirection *next_;
};

 void *
Simulator::RedirectNativeFunction(void *nativeFunction, ABIFunctionType type)
{
    Redirection *redirection = Redirection::Get(nativeFunction, type);
    return redirection->addressOfSwiInstruction();
}

SimulatorRuntime::~SimulatorRuntime()
{
    Redirection *r = redirection_;
    while (r) {
        Redirection *next = r->next_;
        js_delete(r);
        r = next;
    }
    if (lock_)
        PR_DestroyLock(lock_);
}


Simulator *
Simulator::Current()
{
    PerThreadData *pt = TlsPerThreadData.get();
    Simulator *sim = pt->simulator();
    if (!sim) {
        sim = js_new<Simulator>(pt->simulatorRuntime());
        pt->setSimulator(sim);
    }

    return sim;
}



void Simulator::setRegister(int reg, int32_t value)
{
    MOZ_ASSERT((reg >= 0) && (reg < Register::kNumSimuRegisters));
    if (reg == pc) {
        pc_modified_ = true;
    }

    
    registers_[reg] = (reg == 0) ? 0 : value;
}

void
Simulator::setFpuRegister(int fpureg, int32_t value)
{
    MOZ_ASSERT((fpureg >= 0) && (fpureg < Simulator::FPURegister::kNumFPURegisters));
    FPUregisters_[fpureg] = value;
}

void
Simulator::setFpuRegisterFloat(int fpureg, float value)
{
    MOZ_ASSERT((fpureg >= 0) && (fpureg < Simulator::FPURegister::kNumFPURegisters));
    *mozilla::BitwiseCast<float*>(&FPUregisters_[fpureg]) = value;
}

void
Simulator::setFpuRegisterFloat(int fpureg, int64_t value)
{
    setFpuRegister(fpureg, value & 0xffffffff);
    setFpuRegister(fpureg + 1, value >> 32);
}

void
Simulator::setFpuRegisterDouble(int fpureg, double value)
{
    MOZ_ASSERT((fpureg >= 0) && (fpureg < Simulator::FPURegister::kNumFPURegisters)
           && ((fpureg % 2) == 0));
    *mozilla::BitwiseCast<double*>(&FPUregisters_[fpureg]) = value;
}

void
Simulator::setFpuRegisterDouble(int fpureg, int64_t value)
{
    setFpuRegister(fpureg, value & 0xffffffff);
    setFpuRegister(fpureg + 1, value >> 32);
}



int32_t
Simulator::getRegister(int reg) const
{
    MOZ_ASSERT((reg >= 0) && (reg < Register::kNumSimuRegisters));
    if (reg == 0)
        return 0;
    return registers_[reg] + ((reg == pc) ? SimInstruction::kPCReadOffset : 0);
}

double
Simulator::getDoubleFromRegisterPair(int reg)
{
    MOZ_ASSERT((reg >= 0) && (reg < Register::kNumSimuRegisters) && ((reg % 2) == 0));

    double dm_val = 0.0;
    
    
    memcpy(&dm_val, &registers_[reg], sizeof(dm_val));
    return(dm_val);
}

int32_t
Simulator::getFpuRegister(int fpureg) const
{
    MOZ_ASSERT((fpureg >= 0) && (fpureg < Simulator::FPURegister::kNumFPURegisters));
    return FPUregisters_[fpureg];
}

int64_t
Simulator::getFpuRegisterLong(int fpureg) const
{
    MOZ_ASSERT((fpureg >= 0) && (fpureg < Simulator::FPURegister::kNumFPURegisters)
           && ((fpureg % 2) == 0));
    return *mozilla::BitwiseCast<int64_t*>(const_cast<int32_t*>(&FPUregisters_[fpureg]));
}

float
Simulator::getFpuRegisterFloat(int fpureg) const
{
    MOZ_ASSERT((fpureg >= 0) && (fpureg < Simulator::FPURegister::kNumFPURegisters));
    return *mozilla::BitwiseCast<float*>(const_cast<int32_t*>(&FPUregisters_[fpureg]));
}

double
Simulator::getFpuRegisterDouble(int fpureg) const
{
    MOZ_ASSERT((fpureg >= 0) && (fpureg < Simulator::FPURegister::kNumFPURegisters)
           && ((fpureg % 2) == 0));
    return *mozilla::BitwiseCast<double*>(const_cast<int32_t*>(&FPUregisters_[fpureg]));
}




void
Simulator::getFpArgs(double *x, double *y, int32_t *z)
{
    *x = getFpuRegisterDouble(12);
    *y = getFpuRegisterDouble(14);
    *z = getRegister(a2);
}

void
Simulator::setCallResultDouble(double result)
{
    setFpuRegisterDouble(f0, result);
}

void
Simulator::setCallResultFloat(float result)
{
    setFpuRegisterFloat(f0, result);
}

void
Simulator::setCallResult(int64_t res)
{
    setRegister(v0, static_cast<int32_t>(res));
    setRegister(v1, static_cast<int32_t>(res >> 32));
}


void
Simulator::setFCSRBit(uint32_t cc, bool value)
{
    if (value)
        FCSR_ |= (1 << cc);
    else
        FCSR_ &= ~(1 << cc);
}

bool
Simulator::testFCSRBit(uint32_t cc)
{
    return FCSR_ & (1 << cc);
}



bool
Simulator::setFCSRRoundError(double original, double rounded)
{
    bool ret = false;

    if (!std::isfinite(original) || !std::isfinite(rounded)) {
        setFCSRBit(kFCSRInvalidOpFlagBit, true);
        ret = true;
    }

    if (original != rounded) {
        setFCSRBit(kFCSRInexactFlagBit, true);
    }

    if (rounded < DBL_MIN && rounded > -DBL_MIN && rounded != 0) {
        setFCSRBit(kFCSRUnderflowFlagBit, true);
        ret = true;
    }

    if (rounded > INT_MAX || rounded < INT_MIN) {
        setFCSRBit(kFCSROverflowFlagBit, true);
        
        setFCSRBit(kFCSRInvalidOpFlagBit, true);
        ret = true;
    }

    return ret;
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
    return ((registers_[pc] == bad_ra) || (registers_[pc] == end_sim_pc));
}


int32_t
Simulator::get_pc() const
{
    return registers_[pc];
}









int
Simulator::readW(uint32_t addr, SimInstruction *instr)
{
    if (addr < 0x400) {
        
        printf("Memory read from bad address: 0x%08x, pc=0x%08x\n",
               addr, reinterpret_cast<intptr_t>(instr));
        MOZ_CRASH();
    }
    if ((addr & kPointerAlignmentMask) == 0) {
        intptr_t *ptr = reinterpret_cast<intptr_t*>(addr);
        return *ptr;
    }
    printf("Unaligned read at 0x%08x, pc=0x%08x\n",
           addr,
           reinterpret_cast<intptr_t>(instr));
    MOZ_CRASH();
    return 0;
}

void
Simulator::writeW(uint32_t addr, int value, SimInstruction *instr)
{
    if (addr < 0x400) {
        
        printf("Memory write to bad address: 0x%08x, pc=0x%08x\n",
               addr, reinterpret_cast<intptr_t>(instr));
        MOZ_CRASH();
    }
    if ((addr & kPointerAlignmentMask) == 0) {
        intptr_t *ptr = reinterpret_cast<intptr_t*>(addr);
        *ptr = value;
        return;
    }
    printf("Unaligned write at 0x%08x, pc=0x%08x\n",
           addr,
           reinterpret_cast<intptr_t>(instr));
    MOZ_CRASH();
}

double
Simulator::readD(uint32_t addr, SimInstruction* instr)
{
    if ((addr & kDoubleAlignmentMask) == 0) {
        double *ptr = reinterpret_cast<double*>(addr);
        return *ptr;
    }
    printf("Unaligned (double) read at 0x%08x, pc=0x%08x\n",
           addr,
           reinterpret_cast<intptr_t>(instr));
    MOZ_CRASH();
    return 0;
}

void
Simulator::writeD(uint32_t addr, double value, SimInstruction *instr)
{
    if ((addr & kDoubleAlignmentMask) == 0) {
        double *ptr = reinterpret_cast<double*>(addr);
        *ptr = value;
        return;
    }
    printf("Unaligned (double) write at 0x%08x, pc=0x%08x\n",
           addr,
           reinterpret_cast<intptr_t>(instr));
    MOZ_CRASH();
}

uint16_t
Simulator::readHU(uint32_t addr, SimInstruction *instr)
{
    if ((addr & 1) == 0) {
        uint16_t *ptr = reinterpret_cast<uint16_t*>(addr);
        return *ptr;
    }
    printf("Unaligned unsigned halfword read at 0x%08x, pc=0x%08x\n",
           addr,
           reinterpret_cast<intptr_t>(instr));
    MOZ_CRASH();
    return 0;
}

int16_t
Simulator::readH(uint32_t addr, SimInstruction* instr)
{
    if ((addr & 1) == 0) {
        int16_t* ptr = reinterpret_cast<int16_t*>(addr);
        return *ptr;
    }
    printf("Unaligned signed halfword read at 0x%08x, pc=0x%08x\n",
           addr,
           reinterpret_cast<intptr_t>(instr));
    MOZ_CRASH();
    return 0;
}

void
Simulator::writeH(uint32_t addr, uint16_t value, SimInstruction *instr)
{
    if ((addr & 1) == 0) {
        uint16_t *ptr = reinterpret_cast<uint16_t*>(addr);
        *ptr = value;
        return;
    }
    printf("Unaligned unsigned halfword write at 0x%08x, pc=0x%08x\n",
           addr,
           reinterpret_cast<intptr_t>(instr));
    MOZ_CRASH();
}

void
Simulator::writeH(uint32_t addr, int16_t value, SimInstruction *instr)
{
    if ((addr & 1) == 0) {
        int16_t *ptr = reinterpret_cast<int16_t*>(addr);
        *ptr = value;
        return;
    }
    printf("Unaligned halfword write at 0x%08x, pc=0x%08x\n",
           addr,
           reinterpret_cast<intptr_t>(instr));
    MOZ_CRASH();
}

uint32_t
Simulator::readBU(uint32_t addr)
{
    uint8_t *ptr = reinterpret_cast<uint8_t*>(addr);
    return *ptr;
}

int32_t
Simulator::readB(uint32_t addr)
{
    int8_t *ptr = reinterpret_cast<int8_t*>(addr);
    return *ptr;
}

void
Simulator::writeB(uint32_t addr, uint8_t value)
{
    uint8_t *ptr = reinterpret_cast<uint8_t*>(addr);
    *ptr = value;
}

void
Simulator::writeB(uint32_t addr, int8_t value)
{
    int8_t *ptr = reinterpret_cast<int8_t*>(addr);
    *ptr = value;
}

uintptr_t
Simulator::stackLimit() const
{
    
    
    return reinterpret_cast<uintptr_t>(stack_) + 1024 * 1024;
}

bool
Simulator::overRecursed(uintptr_t newsp) const
{
    if (newsp == 0)
        newsp = getRegister(sp);
    return newsp <= stackLimit();
}

bool
Simulator::overRecursedWithExtra(uint32_t extra) const
{
    uintptr_t newsp = getRegister(sp) - extra;
    return newsp <= stackLimit();
}


void
Simulator::format(SimInstruction *instr, const char *format)
{
    printf("Simulator found unsupported instruction:\n 0x%08x: %s\n",
           reinterpret_cast<intptr_t>(instr), format);
    MOZ_CRASH();
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


void
Simulator::softwareInterrupt(SimInstruction *instr)
{
    int32_t func = instr->functionFieldRaw();
    uint32_t code = (func == ff_break) ? instr->bits(25, 6) : -1;

    
    if (instr->instructionBits() == kCallRedirInstr) {
#if !defined(USES_O32_ABI)
        MOZ_CRASH("Only O32 ABI supported.");
#else
        Redirection *redirection = Redirection::FromSwiInstruction(instr);
        int32_t arg0 = getRegister(a0);
        int32_t arg1 = getRegister(a1);
        int32_t arg2 = getRegister(a2);
        int32_t arg3 = getRegister(a3);

        int32_t *stack_pointer = reinterpret_cast<int32_t*>(getRegister(sp));
        
        int32_t arg4 = stack_pointer[4];
        int32_t arg5 = stack_pointer[5];

        
        
        int32_t saved_ra = getRegister(ra);

        intptr_t external = reinterpret_cast<intptr_t>(redirection->nativeFunction());

        bool stack_aligned = (getRegister(sp) & (ABIStackAlignment - 1)) == 0;
        if (!stack_aligned) {
            fprintf(stderr, "Runtime call with unaligned stack!\n");
            MOZ_CRASH();
        }

        switch (redirection->type()) {
          case Args_General0: {
            Prototype_General0 target = reinterpret_cast<Prototype_General0>(external);
            int64_t result = target();
            setCallResult(result);
            break;
          }
          case Args_General1: {
            Prototype_General1 target = reinterpret_cast<Prototype_General1>(external);
            int64_t result = target(arg0);
            setCallResult(result);
            break;
          }
          case Args_General2: {
            Prototype_General2 target = reinterpret_cast<Prototype_General2>(external);
            int64_t result = target(arg0, arg1);
            setCallResult(result);
            break;
          }
          case Args_General3: {
            Prototype_General3 target = reinterpret_cast<Prototype_General3>(external);
            int64_t result = target(arg0, arg1, arg2);
            setCallResult(result);
            break;
          }
          case Args_General4: {
            Prototype_General4 target = reinterpret_cast<Prototype_General4>(external);
            int64_t result = target(arg0, arg1, arg2, arg3);
            setCallResult(result);
            break;
          }
          case Args_General5: {
            Prototype_General5 target = reinterpret_cast<Prototype_General5>(external);
            int64_t result = target(arg0, arg1, arg2, arg3, arg4);
            setCallResult(result);
            break;
          }
          case Args_General6: {
            Prototype_General6 target = reinterpret_cast<Prototype_General6>(external);
            int64_t result = target(arg0, arg1, arg2, arg3, arg4, arg5);
            setCallResult(result);
            break;
          }
          case Args_General7: {
            Prototype_General7 target = reinterpret_cast<Prototype_General7>(external);
            int32_t arg6 = stack_pointer[6];
            int64_t result = target(arg0, arg1, arg2, arg3, arg4, arg5, arg6);
            setCallResult(result);
            break;
          }
          case Args_General8: {
            Prototype_General8 target = reinterpret_cast<Prototype_General8>(external);
            int32_t arg6 = stack_pointer[6];
            int32_t arg7 = stack_pointer[7];
            int64_t result = target(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
            setCallResult(result);
            break;
          }
          case Args_Double_None: {
            Prototype_Double_None target = reinterpret_cast<Prototype_Double_None>(external);
            double dresult = target();
            setCallResultDouble(dresult);
            break;
          }
          case Args_Int_Double: {
            double dval0, dval1;
            int32_t ival;
            getFpArgs(&dval0, &dval1, &ival);
            Prototype_Int_Double target = reinterpret_cast<Prototype_Int_Double>(external);
            int32_t res = target(dval0);
            setRegister(v0, res);
            break;
          }
          case Args_Double_Double: {
            double dval0, dval1;
            int32_t ival;
            getFpArgs(&dval0, &dval1, &ival);
            Prototype_Double_Double target = reinterpret_cast<Prototype_Double_Double>(external);
            double dresult = target(dval0);
            setCallResultDouble(dresult);
            break;
          }
          case Args_Float32_Float32: {
            float fval0;
            fval0 = getFpuRegisterFloat(12);
            Prototype_Float32_Float32 target = reinterpret_cast<Prototype_Float32_Float32>(external);
            float fresult = target(fval0);
            setCallResultFloat(fresult);
            break;
          }
          case Args_Double_Int: {
            Prototype_Double_Int target = reinterpret_cast<Prototype_Double_Int>(external);
            double dresult = target(arg0);
            setCallResultDouble(dresult);
            break;
          }
          case Args_Double_DoubleInt: {
            double dval0, dval1;
            int32_t ival;
            getFpArgs(&dval0, &dval1, &ival);
            Prototype_DoubleInt target = reinterpret_cast<Prototype_DoubleInt>(external);
            double dresult = target(dval0, ival);
            setCallResultDouble(dresult);
            break;
          }
          case Args_Double_DoubleDouble: {
            double dval0, dval1;
            int32_t ival;
            getFpArgs(&dval0, &dval1, &ival);
            Prototype_Double_DoubleDouble target = reinterpret_cast<Prototype_Double_DoubleDouble>(external);
            double dresult = target(dval0, dval1);
            setCallResultDouble(dresult);
            break;
          }
          case Args_Double_IntDouble: {
            int32_t ival = getRegister(a0);
            double dval0 = getDoubleFromRegisterPair(a2);
            Prototype_Double_IntDouble target = reinterpret_cast<Prototype_Double_IntDouble>(external);
            double dresult = target(ival, dval0);
            setCallResultDouble(dresult);
            break;
          }
          case Args_Int_IntDouble: {
            int32_t ival = getRegister(a0);
            double dval0 = getDoubleFromRegisterPair(a2);
            Prototype_Int_IntDouble target = reinterpret_cast<Prototype_Int_IntDouble>(external);
            int32_t result = target(ival, dval0);
            setRegister(v0, result);
            break;
          }
          default:
            MOZ_CRASH("call");
        }

        setRegister(ra, saved_ra);
        set_pc(getRegister(ra));
#endif
    } else if (func == ff_break && code <= kMaxStopCode) {
        if (isWatchpoint(code)) {
            printWatchpoint(code);
        } else {
            increaseStopCounter(code);
            handleStop(code, instr);
        }
    } else {
        
        MipsDebugger dbg(this);
        dbg.debug();
    }
}


bool
Simulator::isWatchpoint(uint32_t code)
{
    return (code <= kMaxWatchpointCode);
}

void
Simulator::printWatchpoint(uint32_t code)
{
    MipsDebugger dbg(this);
    ++break_count_;
    printf("\n---- break %d marker: %3d  (instr count: %8d) ----------"
           "----------------------------------",
           code, break_count_, icount_);
    dbg.printAllRegs();  
}

void
Simulator::handleStop(uint32_t code, SimInstruction *instr)
{
    
    
    if (isEnabledStop(code)) {
        MipsDebugger dbg(this);
        dbg.stop(instr);
    } else {
        set_pc(get_pc() + 2 * SimInstruction::kInstrSize);
    }
}

bool
Simulator::isStopInstruction(SimInstruction *instr)
{
    int32_t func = instr->functionFieldRaw();
    uint32_t code = static_cast<uint32_t>(instr->bits(25, 6));
    return (func == ff_break) && code > kMaxWatchpointCode && code <= kMaxStopCode;
}

bool
Simulator::isEnabledStop(uint32_t code)
{
    MOZ_ASSERT(code <= kMaxStopCode);
    MOZ_ASSERT(code > kMaxWatchpointCode);
    return !(watchedStops_[code].count_ & kStopDisabledBit);
}

void
Simulator::enableStop(uint32_t code)
{
    if (!isEnabledStop(code))
        watchedStops_[code].count_ &= ~kStopDisabledBit;
}

void
Simulator::disableStop(uint32_t code)
{
    if (isEnabledStop(code))
        watchedStops_[code].count_ |= kStopDisabledBit;
}

void
Simulator::increaseStopCounter(uint32_t code)
{
    MOZ_ASSERT(code <= kMaxStopCode);
    if ((watchedStops_[code].count_ & ~(1 << 31)) == 0x7fffffff) {
        printf("Stop counter for code %i has overflowed.\n"
               "Enabling this code and reseting the counter to 0.\n", code);
        watchedStops_[code].count_ = 0;
        enableStop(code);
    } else {
        watchedStops_[code].count_++;
    }
}


void
Simulator::printStopInfo(uint32_t code)
{
    if (code <= kMaxWatchpointCode) {
        printf("That is a watchpoint, not a stop.\n");
        return;
    } else if (code > kMaxStopCode) {
        printf("Code too large, only %u stops can be used\n", kMaxStopCode + 1);
        return;
    }
    const char *state = isEnabledStop(code) ? "Enabled" : "Disabled";
    int32_t count = watchedStops_[code].count_ & ~kStopDisabledBit;
    
    if (count != 0) {
        if (watchedStops_[code].desc_) {
            printf("stop %i - 0x%x: \t%s, \tcounter = %i, \t%s\n",
                   code, code, state, count, watchedStops_[code].desc_);
        } else {
            printf("stop %i - 0x%x: \t%s, \tcounter = %i\n",
                   code, code, state, count);
        }
    }
}

void
Simulator::signalExceptions()
{
    for (int i = 1; i < kNumExceptions; i++) {
        if (exceptions[i] != 0)
            MOZ_CRASH("Error: Exception raised.");
    }
}


void
Simulator::configureTypeRegister(SimInstruction *instr,
                                 int32_t& alu_out,
                                 int64_t& i64hilo,
                                 uint64_t& u64hilo,
                                 int32_t& next_pc,
                                 int32_t& return_addr_reg,
                                 bool& do_interrupt)
{
    
    
    

    
    const Opcode   op     = instr->opcodeFieldRaw();
    const int32_t  rs_reg = instr->rsValue();
    const int32_t  rs     = getRegister(rs_reg);
    const uint32_t rs_u   = static_cast<uint32_t>(rs);
    const int32_t  rt_reg = instr->rtValue();
    const int32_t  rt     = getRegister(rt_reg);
    const uint32_t rt_u   = static_cast<uint32_t>(rt);
    const int32_t  rd_reg = instr->rdValue();
    const uint32_t sa     = instr->saValue();

    const int32_t  fs_reg = instr->fsValue();


    
    switch (op) {
      case op_cop1:    
        switch (instr->rsFieldRaw()) {
          case rs_bc1:   
            MOZ_CRASH();
            break;
          case rs_cfc1:
            
            MOZ_ASSERT(fs_reg == kFCSRRegister);
            alu_out = FCSR_;
            break;
          case rs_mfc1:
            alu_out = getFpuRegister(fs_reg);
            break;
          case rs_mfhc1:
            MOZ_CRASH();
            break;
          case rs_ctc1:
          case rs_mtc1:
          case rs_mthc1:
            
            break;
          case rs_s:
          case rs_d:
          case rs_w:
          case rs_l:
          case rs_ps:
            
            break;
          default:
            MOZ_CRASH();
        };
        break;
      case op_cop1x:
        break;
      case op_special:
        switch (instr->functionFieldRaw()) {
          case ff_jr:
          case ff_jalr:
            next_pc = getRegister(instr->rsValue());
            return_addr_reg = instr->rdValue();
            break;
          case ff_sll:
            alu_out = rt << sa;
            break;
          case ff_srl:
            if (rs_reg == 0) {
                
                
                alu_out = rt_u >> sa;
            } else {
                
                
                
                alu_out = (rt_u >> sa) | (rt_u << (32 - sa));
            }
            break;
          case ff_sra:
            alu_out = rt >> sa;
            break;
          case ff_sllv:
            alu_out = rt << rs;
            break;
          case ff_srlv:
            if (sa == 0) {
                
                
                alu_out = rt_u >> rs;
            } else {
                
                
                
                alu_out = (rt_u >> rs_u) | (rt_u << (32 - rs_u));
            }
            break;
          case ff_srav:
            alu_out = rt >> rs;
            break;
          case ff_mfhi:
            alu_out = getRegister(HI);
            break;
          case ff_mflo:
            alu_out = getRegister(LO);
            break;
          case ff_mult:
            i64hilo = static_cast<int64_t>(rs) * static_cast<int64_t>(rt);
            break;
          case ff_multu:
            u64hilo = static_cast<uint64_t>(rs_u) * static_cast<uint64_t>(rt_u);
            break;
          case ff_add:
            if (HaveSameSign(rs, rt)) {
                if (rs > 0) {
                    exceptions[kIntegerOverflow] = rs > (kRegisterskMaxValue - rt);
                } else if (rs < 0) {
                    exceptions[kIntegerUnderflow] = rs < (kRegisterskMinValue - rt);
                }
            }
            alu_out = rs + rt;
            break;
          case ff_addu:
            alu_out = rs + rt;
            break;
          case ff_sub:
            if (!HaveSameSign(rs, rt)) {
                if (rs > 0) {
                    exceptions[kIntegerOverflow] = rs > (kRegisterskMaxValue + rt);
                } else if (rs < 0) {
                    exceptions[kIntegerUnderflow] = rs < (kRegisterskMinValue + rt);
                }
            }
            alu_out = rs - rt;
            break;
          case ff_subu:
            alu_out = rs - rt;
            break;
          case ff_and:
            alu_out = rs & rt;
            break;
          case ff_or:
            alu_out = rs | rt;
            break;
          case ff_xor:
            alu_out = rs ^ rt;
            break;
          case ff_nor:
            alu_out = ~(rs | rt);
            break;
          case ff_slt:
            alu_out = rs < rt ? 1 : 0;
            break;
          case ff_sltu:
            alu_out = rs_u < rt_u ? 1 : 0;
            break;
            
          case ff_break:
            do_interrupt = true;
            break;
          case ff_tge:
            do_interrupt = rs >= rt;
            break;
          case ff_tgeu:
            do_interrupt = rs_u >= rt_u;
            break;
          case ff_tlt:
            do_interrupt = rs < rt;
            break;
          case ff_tltu:
            do_interrupt = rs_u < rt_u;
            break;
          case ff_teq:
            do_interrupt = rs == rt;
            break;
          case ff_tne:
            do_interrupt = rs != rt;
            break;
          case ff_movn:
          case ff_movz:
          case ff_movci:
            
            break;
          case ff_div:
          case ff_divu:
            
            break;
          default:
            MOZ_CRASH();
        };
        break;
      case op_special2:
        switch (instr->functionFieldRaw()) {
          case ff_mul:
            alu_out = rs_u * rt_u;  
            break;
          case ff_clz:
            alu_out = __builtin_clz(rs_u);
            break;
          default:
            MOZ_CRASH();
        };
        break;
      case op_special3:
        switch (instr->functionFieldRaw()) {
          case ff_ins: {   
            
            uint16_t msb = rd_reg;
            
            uint16_t lsb = sa;
            uint16_t size = msb - lsb + 1;
            uint32_t mask = (1 << size) - 1;
            alu_out = (rt_u & ~(mask << lsb)) | ((rs_u & mask) << lsb);
            break;
          }
          case ff_ext: {   
            
            uint16_t msb = rd_reg;
            
            uint16_t lsb = sa;
            uint16_t size = msb + 1;
            uint32_t mask = (1 << size) - 1;
            alu_out = (rs_u & (mask << lsb)) >> lsb;
            break;
          }
          default:
            MOZ_CRASH();
        };
        break;
      default:
        MOZ_CRASH();
    };
}

void
Simulator::decodeTypeRegister(SimInstruction *instr)
{
    
    const Opcode   op     = instr->opcodeFieldRaw();
    const int32_t  rs_reg = instr->rsValue();
    const int32_t  rs     = getRegister(rs_reg);
    const uint32_t rs_u   = static_cast<uint32_t>(rs);
    const int32_t  rt_reg = instr->rtValue();
    const int32_t  rt     = getRegister(rt_reg);
    const uint32_t rt_u   = static_cast<uint32_t>(rt);
    const int32_t  rd_reg = instr->rdValue();

    const int32_t  fr_reg = instr->frValue();
    const int32_t  fs_reg = instr->fsValue();
    const int32_t  ft_reg = instr->ftValue();
    const int32_t  fd_reg = instr->fdValue();
    int64_t  i64hilo = 0;
    uint64_t u64hilo = 0;

    
    
    
    int32_t alu_out = 0x12345678;

    
    bool do_interrupt = false;

    
    
    int32_t current_pc = get_pc();
    
    int32_t next_pc = 0;
    int32_t return_addr_reg = 31;

    
    configureTypeRegister(instr,
                          alu_out,
                          i64hilo,
                          u64hilo,
                          next_pc,
                          return_addr_reg,
                          do_interrupt);

    
    signalExceptions();

    
    switch (op) {
      case op_cop1:
        switch (instr->rsFieldRaw()) {
          case rs_bc1:   
            MOZ_CRASH();
            break;
          case rs_cfc1:
            setRegister(rt_reg, alu_out);
          case rs_mfc1:
            setRegister(rt_reg, alu_out);
            break;
          case rs_mfhc1:
            MOZ_CRASH();
            break;
          case rs_ctc1:
            
            MOZ_ASSERT(fs_reg == kFCSRRegister);
            FCSR_ = registers_[rt_reg];
            break;
          case rs_mtc1:
            FPUregisters_[fs_reg] = registers_[rt_reg];
            break;
          case rs_mthc1:
            MOZ_CRASH();
            break;
          case rs_s:
            float f, ft_value, fs_value;
            uint32_t cc, fcsr_cc;
            int64_t  i64;
            fs_value = getFpuRegisterFloat(fs_reg);
            ft_value = getFpuRegisterFloat(ft_reg);
            cc = instr->fcccValue();
            fcsr_cc = GetFCSRConditionBit(cc);
            switch (instr->functionFieldRaw()) {
              case ff_add_fmt:
                setFpuRegisterFloat(fd_reg, fs_value + ft_value);
                break;
              case ff_sub_fmt:
                setFpuRegisterFloat(fd_reg, fs_value - ft_value);
                break;
              case ff_mul_fmt:
                setFpuRegisterFloat(fd_reg, fs_value * ft_value);
                break;
              case ff_div_fmt:
                setFpuRegisterFloat(fd_reg, fs_value / ft_value);
                break;
              case ff_abs_fmt:
                setFpuRegisterFloat(fd_reg, fabsf(fs_value));
                break;
              case ff_mov_fmt:
                setFpuRegisterFloat(fd_reg, fs_value);
                break;
              case ff_neg_fmt:
                setFpuRegisterFloat(fd_reg, -fs_value);
                break;
              case ff_sqrt_fmt:
                setFpuRegisterFloat(fd_reg, sqrtf(fs_value));
                break;
              case ff_c_un_fmt:
                setFCSRBit(fcsr_cc, mozilla::IsNaN(fs_value) || mozilla::IsNaN(ft_value));
                break;
              case ff_c_eq_fmt:
                setFCSRBit(fcsr_cc, (fs_value == ft_value));
                break;
              case ff_c_ueq_fmt:
                setFCSRBit(fcsr_cc,
                           (fs_value == ft_value) || (mozilla::IsNaN(fs_value) || mozilla::IsNaN(ft_value)));
                break;
              case ff_c_olt_fmt:
                setFCSRBit(fcsr_cc, (fs_value < ft_value));
                break;
              case ff_c_ult_fmt:
                setFCSRBit(fcsr_cc,
                           (fs_value < ft_value) || (mozilla::IsNaN(fs_value) || mozilla::IsNaN(ft_value)));
                break;
              case ff_c_ole_fmt:
                setFCSRBit(fcsr_cc, (fs_value <= ft_value));
                break;
              case ff_c_ule_fmt:
                setFCSRBit(fcsr_cc,
                           (fs_value <= ft_value) || (mozilla::IsNaN(fs_value) || mozilla::IsNaN(ft_value)));
                break;
              case ff_cvt_d_fmt:
                f = getFpuRegisterFloat(fs_reg);
                setFpuRegisterDouble(fd_reg, static_cast<double>(f));
                break;
              case ff_cvt_w_fmt:   
                
                MOZ_ASSERT((FCSR_ & 3) == 0);
                
              case ff_round_w_fmt: { 
                float rounded = std::floor(fs_value + 0.5);
                int32_t result = static_cast<int32_t>(rounded);
                if ((result & 1) != 0 && result - fs_value == 0.5) {
                    
                    
                    result--;
                }
                setFpuRegister(fd_reg, result);
                if (setFCSRRoundError(fs_value, rounded)) {
                    setFpuRegister(fd_reg, kFPUInvalidResult);
                }
                break;
              }
              case ff_trunc_w_fmt: { 
                float rounded = truncf(fs_value);
                int32_t result = static_cast<int32_t>(rounded);
                setFpuRegister(fd_reg, result);
                if (setFCSRRoundError(fs_value, rounded)) {
                    setFpuRegister(fd_reg, kFPUInvalidResult);
                }
                break;
              }
              case ff_floor_w_fmt: { 
                float rounded = std::floor(fs_value);
                int32_t result = static_cast<int32_t>(rounded);
                setFpuRegister(fd_reg, result);
                if (setFCSRRoundError(fs_value, rounded)) {
                    setFpuRegister(fd_reg, kFPUInvalidResult);
                }
                break;
              }
              case ff_ceil_w_fmt: { 
                float rounded = std::ceil(fs_value);
                int32_t result = static_cast<int32_t>(rounded);
                setFpuRegister(fd_reg, result);
                if (setFCSRRoundError(fs_value, rounded)) {
                    setFpuRegister(fd_reg, kFPUInvalidResult);
                }
                break;
              }
              case ff_cvt_l_fmt: {  
                float rounded = truncf(fs_value);
                i64 = static_cast<int64_t>(rounded);
                setFpuRegisterFloat(fd_reg, i64);
                break;
              }
              case ff_round_l_fmt: {  
                float rounded =
                    fs_value > 0 ? std::floor(fs_value + 0.5) : std::ceil(fs_value - 0.5);
                i64 = static_cast<int64_t>(rounded);
                setFpuRegisterFloat(fd_reg, i64);
                break;
              }
              case ff_trunc_l_fmt: {  
                float rounded = truncf(fs_value);
                i64 = static_cast<int64_t>(rounded);
                setFpuRegisterFloat(fd_reg, i64);
                break;
              }
              case ff_floor_l_fmt:  
                i64 = static_cast<int64_t>(std::floor(fs_value));
                setFpuRegisterFloat(fd_reg, i64);
                break;
              case ff_ceil_l_fmt:  
                i64 = static_cast<int64_t>(std::ceil(fs_value));
                setFpuRegisterFloat(fd_reg, i64);
                break;
              case ff_cvt_ps_s:
              case ff_c_f_fmt:
                MOZ_CRASH();
                break;
              default:
                MOZ_CRASH();
            }
            break;
          case rs_d:
            double dt_value, ds_value;
            ds_value = getFpuRegisterDouble(fs_reg);
            dt_value = getFpuRegisterDouble(ft_reg);
            cc = instr->fcccValue();
            fcsr_cc = GetFCSRConditionBit(cc);
            switch (instr->functionFieldRaw()) {
              case ff_add_fmt:
                setFpuRegisterDouble(fd_reg, ds_value + dt_value);
                break;
              case ff_sub_fmt:
                setFpuRegisterDouble(fd_reg, ds_value - dt_value);
                break;
              case ff_mul_fmt:
                setFpuRegisterDouble(fd_reg, ds_value * dt_value);
                break;
              case ff_div_fmt:
                setFpuRegisterDouble(fd_reg, ds_value / dt_value);
                break;
              case ff_abs_fmt:
                setFpuRegisterDouble(fd_reg, fabs(ds_value));
                break;
              case ff_mov_fmt:
                setFpuRegisterDouble(fd_reg, ds_value);
                break;
              case ff_neg_fmt:
                setFpuRegisterDouble(fd_reg, -ds_value);
                break;
              case ff_sqrt_fmt:
                setFpuRegisterDouble(fd_reg, sqrt(ds_value));
                break;
              case ff_c_un_fmt:
                setFCSRBit(fcsr_cc, mozilla::IsNaN(ds_value) || mozilla::IsNaN(dt_value));
                break;
              case ff_c_eq_fmt:
                setFCSRBit(fcsr_cc, (ds_value == dt_value));
                break;
              case ff_c_ueq_fmt:
                setFCSRBit(fcsr_cc,
                            (ds_value == dt_value) || (mozilla::IsNaN(ds_value) || mozilla::IsNaN(dt_value)));
                break;
              case ff_c_olt_fmt:
                setFCSRBit(fcsr_cc, (ds_value < dt_value));
                break;
              case ff_c_ult_fmt:
                setFCSRBit(fcsr_cc,
                            (ds_value < dt_value) || (mozilla::IsNaN(ds_value) || mozilla::IsNaN(dt_value)));
                break;
              case ff_c_ole_fmt:
                setFCSRBit(fcsr_cc, (ds_value <= dt_value));
                break;
              case ff_c_ule_fmt:
                setFCSRBit(fcsr_cc,
                            (ds_value <= dt_value) || (mozilla::IsNaN(ds_value) || mozilla::IsNaN(dt_value)));
                break;
              case ff_cvt_w_fmt:   
                
                MOZ_ASSERT((FCSR_ & 3) == 0);
                
              case ff_round_w_fmt: { 
                double rounded = std::floor(ds_value + 0.5);
                int32_t result = static_cast<int32_t>(rounded);
                if ((result & 1) != 0 && result - ds_value == 0.5) {
                    
                    
                    result--;
                }
                setFpuRegister(fd_reg, result);
                if (setFCSRRoundError(ds_value, rounded)) {
                    setFpuRegister(fd_reg, kFPUInvalidResult);
                }
                break;
              }
              case ff_trunc_w_fmt: { 
                double rounded = trunc(ds_value);
                int32_t result = static_cast<int32_t>(rounded);
                setFpuRegister(fd_reg, result);
                if (setFCSRRoundError(ds_value, rounded)) {
                    setFpuRegister(fd_reg, kFPUInvalidResult);
                }
                break;
              }
              case ff_floor_w_fmt: { 
                double rounded = std::floor(ds_value);
                int32_t result = static_cast<int32_t>(rounded);
                setFpuRegister(fd_reg, result);
                if (setFCSRRoundError(ds_value, rounded)) {
                    setFpuRegister(fd_reg, kFPUInvalidResult);
                }
                break;
              }
              case ff_ceil_w_fmt: { 
                double rounded = std::ceil(ds_value);
                int32_t result = static_cast<int32_t>(rounded);
                setFpuRegister(fd_reg, result);
                if (setFCSRRoundError(ds_value, rounded)) {
                    setFpuRegister(fd_reg, kFPUInvalidResult);
                }
                break;
              }
              case ff_cvt_s_fmt:  
                setFpuRegisterFloat(fd_reg, static_cast<float>(ds_value));
                break;
              case ff_cvt_l_fmt: {  
                double rounded = trunc(ds_value);
                i64 = static_cast<int64_t>(rounded);
                setFpuRegisterDouble(fd_reg, i64);
                break;
              }
              case ff_trunc_l_fmt: {  
                double rounded = trunc(ds_value);
                i64 = static_cast<int64_t>(rounded);
                setFpuRegisterDouble(fd_reg, i64);
                break;
              }
              case ff_round_l_fmt: {  
                double rounded =
                    ds_value > 0 ? std::floor(ds_value + 0.5) : std::ceil(ds_value - 0.5);
                i64 = static_cast<int64_t>(rounded);
                setFpuRegisterDouble(fd_reg, i64);
                break;
              }
              case ff_floor_l_fmt:  
                i64 = static_cast<int64_t>(std::floor(ds_value));
                setFpuRegisterDouble(fd_reg, i64);
                break;
              case ff_ceil_l_fmt:  
                i64 = static_cast<int64_t>(std::ceil(ds_value));
                setFpuRegisterDouble(fd_reg, i64);
                break;
              case ff_c_f_fmt:
                MOZ_CRASH();
                break;
              default:
                MOZ_CRASH();
            }
            break;
          case rs_w:
            switch (instr->functionFieldRaw()) {
              case ff_cvt_s_fmt:   
                alu_out = getFpuRegister(fs_reg);
                setFpuRegisterFloat(fd_reg, static_cast<float>(alu_out));
                break;
              case ff_cvt_d_fmt:   
                alu_out = getFpuRegister(fs_reg);
                setFpuRegisterDouble(fd_reg, static_cast<double>(alu_out));
                break;
              default:
                MOZ_CRASH();
            };
            break;
          case rs_l:
            switch (instr->functionFieldRaw()) {
              case ff_cvt_d_fmt:  
                
                
                i64 = static_cast<uint32_t>(getFpuRegister(fs_reg));
                i64 |= static_cast<int64_t>(getFpuRegister(fs_reg + 1)) << 32;
                setFpuRegisterDouble(fd_reg, static_cast<double>(i64));
                break;
              case ff_cvt_s_fmt:
                MOZ_CRASH();
                break;
              default:
                MOZ_CRASH();
            }
            break;
          case rs_ps:
            break;
          default:
            MOZ_CRASH();
        };
        break;
      case op_cop1x:
        switch (instr->functionFieldRaw()) {
          case ff_madd_s:
            float fr, ft, fs;
            fr = getFpuRegisterFloat(fr_reg);
            fs = getFpuRegisterFloat(fs_reg);
            ft = getFpuRegisterFloat(ft_reg);
            setFpuRegisterFloat(fd_reg, fs * ft + fr);
            break;
          case ff_madd_d:
            double dr, dt, ds;
            dr = getFpuRegisterDouble(fr_reg);
            ds = getFpuRegisterDouble(fs_reg);
            dt = getFpuRegisterDouble(ft_reg);
            setFpuRegisterDouble(fd_reg, ds * dt + dr);
            break;
          default:
            MOZ_CRASH();
        };
        break;
      case op_special:
        switch (instr->functionFieldRaw()) {
          case ff_jr: {
            SimInstruction *branch_delay_instr = reinterpret_cast<SimInstruction*>(
                    current_pc + SimInstruction::kInstrSize);
            branchDelayInstructionDecode(branch_delay_instr);
            set_pc(next_pc);
            pc_modified_ = true;
            break;
          }
          case ff_jalr: {
            SimInstruction *branch_delay_instr = reinterpret_cast<SimInstruction*>(
                    current_pc + SimInstruction::kInstrSize);
            setRegister(return_addr_reg, current_pc + 2 * SimInstruction::kInstrSize);
            branchDelayInstructionDecode(branch_delay_instr);
            set_pc(next_pc);
            pc_modified_ = true;
            break;
          }
          
          case ff_mult:
            setRegister(LO, static_cast<int32_t>(i64hilo & 0xffffffff));
            setRegister(HI, static_cast<int32_t>(i64hilo >> 32));
            break;
          case ff_multu:
            setRegister(LO, static_cast<int32_t>(u64hilo & 0xffffffff));
            setRegister(HI, static_cast<int32_t>(u64hilo >> 32));
            break;
          case ff_div:
            
            
            
            
            if (rs == INT_MIN && rt == -1) {
                setRegister(LO, INT_MIN);
                setRegister(HI, 0);
            } else if (rt != 0) {
                setRegister(LO, rs / rt);
                setRegister(HI, rs % rt);
            }
            break;
          case ff_divu:
            if (rt_u != 0) {
                setRegister(LO, rs_u / rt_u);
                setRegister(HI, rs_u % rt_u);
            }
            break;
            
          case ff_break:
          case ff_tge:
          case ff_tgeu:
          case ff_tlt:
          case ff_tltu:
          case ff_teq:
          case ff_tne:
            if (do_interrupt) {
                softwareInterrupt(instr);
            }
            break;
            
          case ff_movn:
            if (rt) setRegister(rd_reg, rs);
            break;
          case ff_movci: {
            uint32_t cc = instr->fbccValue();
            uint32_t fcsr_cc = GetFCSRConditionBit(cc);
            if (instr->bit(16)) {  
                if (testFCSRBit(fcsr_cc)) setRegister(rd_reg, rs);
            } else {
                if (!testFCSRBit(fcsr_cc)) setRegister(rd_reg, rs);
            }
            break;
          }
          case ff_movz:
            if (!rt) setRegister(rd_reg, rs);
            break;
          default:  
            setRegister(rd_reg, alu_out);
          };
          break;
      case op_special2:
        switch (instr->functionFieldRaw()) {
          case ff_mul:
            setRegister(rd_reg, alu_out);
            
            setRegister(LO, Unpredictable);
            setRegister(HI, Unpredictable);
            break;
          default:  
            setRegister(rd_reg, alu_out);
        }
        break;
      case op_special3:
        switch (instr->functionFieldRaw()) {
          case ff_ins:
            
            setRegister(rt_reg, alu_out);
            break;
          case ff_ext:
            
            setRegister(rt_reg, alu_out);
            break;
          default:
            MOZ_CRASH();
        };
        break;
        
        
        
      default:
        setRegister(rd_reg, alu_out);
      };
}


void
Simulator::decodeTypeImmediate(SimInstruction *instr)
{
    
    Opcode   op     = instr->opcodeFieldRaw();
    int32_t  rs     = getRegister(instr->rsValue());
    uint32_t rs_u   = static_cast<uint32_t>(rs);
    int32_t  rt_reg = instr->rtValue();  
    int32_t  rt     = getRegister(rt_reg);
    int16_t  imm16  = instr->imm16Value();

    int32_t  ft_reg = instr->ftValue();  

    
    uint32_t  oe_imm16 = 0xffff & imm16;
    
    int32_t   se_imm16 = imm16;

    
    int32_t current_pc = get_pc();
    
    int32_t next_pc = bad_ra;

    
    bool do_branch = false;
    bool execute_branch_delay_instruction = false;

    
    int32_t alu_out = 0;
    
    double fp_out = 0.0;
    uint32_t cc, cc_value, fcsr_cc;

    
    uint32_t addr = 0x0;
    
    uint32_t mem_value = 0x0;

    
    switch (op) {
          
      case op_cop1:
        switch (instr->rsFieldRaw()) {
          case rs_bc1:   
            cc = instr->fbccValue();
            fcsr_cc = GetFCSRConditionBit(cc);
            cc_value = testFCSRBit(fcsr_cc);
            do_branch = (instr->fbtrueValue()) ? cc_value : !cc_value;
            execute_branch_delay_instruction = true;
            
            if (do_branch) {
                next_pc = current_pc + (imm16 << 2) + SimInstruction::kInstrSize;
            } else {
                next_pc = current_pc + kBranchReturnOffset;
            }
            break;
          default:
            MOZ_CRASH();
        };
        break;
        
      case op_regimm:
        switch (instr->rtFieldRaw()) {
          case rt_bltz:
            do_branch = (rs  < 0);
            break;
          case rt_bltzal:
            do_branch = rs  < 0;
            break;
          case rt_bgez:
            do_branch = rs >= 0;
            break;
          case rt_bgezal:
            do_branch = rs >= 0;
            break;
          default:
            MOZ_CRASH();
        };
        switch (instr->rtFieldRaw()) {
          case rt_bltz:
          case rt_bltzal:
          case rt_bgez:
          case rt_bgezal:
            
            execute_branch_delay_instruction = true;
            
            if (do_branch) {
                next_pc = current_pc + (imm16 << 2) + SimInstruction::kInstrSize;
                if (instr->isLinkingInstruction()) {
                    setRegister(31, current_pc + kBranchReturnOffset);
                }
            } else {
                next_pc = current_pc + kBranchReturnOffset;
            }
          default:
            break;
        };
        break;  
        
        
        
      case op_beq:
        do_branch = (rs == rt);
        break;
      case op_bne:
        do_branch = rs != rt;
        break;
      case op_blez:
        do_branch = rs <= 0;
        break;
      case op_bgtz:
        do_branch = rs  > 0;
        break;
        
      case op_addi:
        if (HaveSameSign(rs, se_imm16)) {
            if (rs > 0) {
                exceptions[kIntegerOverflow] = rs > (kRegisterskMaxValue - se_imm16);
            } else if (rs < 0) {
                exceptions[kIntegerUnderflow] =
                    rs < (kRegisterskMinValue - se_imm16);
            }
        }
        alu_out = rs + se_imm16;
        break;
      case op_addiu:
        alu_out = rs + se_imm16;
        break;
      case op_slti:
        alu_out = (rs < se_imm16) ? 1 : 0;
        break;
      case op_sltiu:
        alu_out = (rs_u < static_cast<uint32_t>(se_imm16)) ? 1 : 0;
        break;
      case op_andi:
        alu_out = rs & oe_imm16;
        break;
      case op_ori:
        alu_out = rs | oe_imm16;
        break;
      case op_xori:
        alu_out = rs ^ oe_imm16;
        break;
      case op_lui:
        alu_out = (oe_imm16 << 16);
        break;
        
      case op_lb:
        addr = rs + se_imm16;
        alu_out = readB(addr);
        break;
      case op_lh:
        addr = rs + se_imm16;
        alu_out = readH(addr, instr);
        break;
      case op_lwl: {
        
        uint8_t al_offset = (rs + se_imm16) & kPointerAlignmentMask;
        uint8_t byte_shift = kPointerAlignmentMask - al_offset;
        uint32_t mask = (1 << byte_shift * 8) - 1;
        addr = rs + se_imm16 - al_offset;
        alu_out = readW(addr, instr);
        alu_out <<= byte_shift * 8;
        alu_out |= rt & mask;
        break;
      }
      case op_lw:
        addr = rs + se_imm16;
        alu_out = readW(addr, instr);
        break;
      case op_lbu:
        addr = rs + se_imm16;
        alu_out = readBU(addr);
        break;
      case op_lhu:
        addr = rs + se_imm16;
        alu_out = readHU(addr, instr);
        break;
      case op_lwr: {
        
        uint8_t al_offset = (rs + se_imm16) & kPointerAlignmentMask;
        uint8_t byte_shift = kPointerAlignmentMask - al_offset;
        uint32_t mask = al_offset ? (~0 << (byte_shift + 1) * 8) : 0;
        addr = rs + se_imm16 - al_offset;
        alu_out = readW(addr, instr);
        alu_out = static_cast<uint32_t> (alu_out) >> al_offset * 8;
        alu_out |= rt & mask;
        break;
      }
      case op_sb:
        addr = rs + se_imm16;
        break;
      case op_sh:
        addr = rs + se_imm16;
        break;
      case op_swl: {
        uint8_t al_offset = (rs + se_imm16) & kPointerAlignmentMask;
        uint8_t byte_shift = kPointerAlignmentMask - al_offset;
        uint32_t mask = byte_shift ? (~0 << (al_offset + 1) * 8) : 0;
        addr = rs + se_imm16 - al_offset;
        mem_value = readW(addr, instr) & mask;
        mem_value |= static_cast<uint32_t>(rt) >> byte_shift * 8;
        break;
      }
      case op_sw:
        addr = rs + se_imm16;
        break;
      case op_swr: {
        uint8_t al_offset = (rs + se_imm16) & kPointerAlignmentMask;
        uint32_t mask = (1 << al_offset * 8) - 1;
        addr = rs + se_imm16 - al_offset;
        mem_value = readW(addr, instr);
        mem_value = (rt << al_offset * 8) | (mem_value & mask);
        break;
      }
      case op_lwc1:
        addr = rs + se_imm16;
        alu_out = readW(addr, instr);
        break;
      case op_ldc1:
        addr = rs + se_imm16;
        fp_out = readD(addr, instr);
        break;
      case op_swc1:
      case op_sdc1:
        addr = rs + se_imm16;
        break;
      default:
        MOZ_CRASH();
    };

    
    signalExceptions();

    
    switch (op) {
          
      case op_beq:
      case op_bne:
      case op_blez:
      case op_bgtz:
        
        execute_branch_delay_instruction = true;
        
        if (do_branch) {
            next_pc = current_pc + (imm16 << 2) + SimInstruction::kInstrSize;
            if (instr->isLinkingInstruction()) {
                setRegister(31, current_pc + 2 * SimInstruction::kInstrSize);
            }
        } else {
            next_pc = current_pc + 2 * SimInstruction::kInstrSize;
        }
        break;
        
      case op_addi:
      case op_addiu:
      case op_slti:
      case op_sltiu:
      case op_andi:
      case op_ori:
      case op_xori:
      case op_lui:
        setRegister(rt_reg, alu_out);
        break;
        
      case op_lb:
      case op_lh:
      case op_lwl:
      case op_lw:
      case op_lbu:
      case op_lhu:
      case op_lwr:
        setRegister(rt_reg, alu_out);
        break;
      case op_sb:
        writeB(addr, static_cast<int8_t>(rt));
        break;
      case op_sh:
        writeH(addr, static_cast<uint16_t>(rt), instr);
        break;
      case op_swl:
        writeW(addr, mem_value, instr);
        break;
      case op_sw:
        writeW(addr, rt, instr);
        break;
      case op_swr:
        writeW(addr, mem_value, instr);
        break;
      case op_lwc1:
        setFpuRegister(ft_reg, alu_out);
        break;
      case op_ldc1:
        setFpuRegisterDouble(ft_reg, fp_out);
        break;
      case op_swc1:
        addr = rs + se_imm16;
        writeW(addr, getFpuRegister(ft_reg), instr);
        break;
      case op_sdc1:
        addr = rs + se_imm16;
        writeD(addr, getFpuRegisterDouble(ft_reg), instr);
        break;
      default:
        break;
    };


    if (execute_branch_delay_instruction) {
        
        
        
        SimInstruction *branch_delay_instr =
            reinterpret_cast<SimInstruction*>(current_pc + SimInstruction::kInstrSize);
        branchDelayInstructionDecode(branch_delay_instr);
    }

    
    if (next_pc != bad_ra)
        set_pc(next_pc);
}


void
Simulator::decodeTypeJump(SimInstruction *instr)
{
    
    int32_t current_pc = get_pc();
    
    int32_t pc_high_bits = current_pc & 0xf0000000;
    
    int32_t next_pc = pc_high_bits | (instr->imm26Value() << 2);

    
    
    
    SimInstruction *branch_delay_instr =
        reinterpret_cast<SimInstruction*>(current_pc + SimInstruction::kInstrSize);
    branchDelayInstructionDecode(branch_delay_instr);

    
    
    if (instr->isLinkingInstruction())
        setRegister(31, current_pc + 2 * SimInstruction::kInstrSize);
    set_pc(next_pc);
    pc_modified_ = true;
}


void
Simulator::instructionDecode(SimInstruction *instr)
{
    if (Simulator::ICacheCheckingEnabled) {
        AutoLockSimulatorRuntime alsr(srt_);
        CheckICache(srt_->icache(), instr);
    }
    pc_modified_ = false;

    switch (instr->instructionType()) {
      case SimInstruction::kRegisterType:
        decodeTypeRegister(instr);
        break;
      case SimInstruction::kImmediateType:
        decodeTypeImmediate(instr);
        break;
      case SimInstruction::kJumpType:
        decodeTypeJump(instr);
        break;
      default:
        UNSUPPORTED();
    }
    if (!pc_modified_)
        setRegister(pc, reinterpret_cast<int32_t>(instr) + SimInstruction::kInstrSize);
}

void
Simulator::branchDelayInstructionDecode(SimInstruction *instr)
{
    if (instr->instructionBits() == NopInst) {
        
        
        return;
    }

    if (instr->isForbiddenInBranchDelay()) {
        MOZ_CRASH("Eror:Unexpected opcode in a branch delay slot.");
    }
    instructionDecode(instr);
}

template<bool enableStopSimAt>
void
Simulator::execute()
{
    
    
    int program_counter = get_pc();
    AsmJSActivation *activation = TlsPerThreadData.get()->asmJSActivationStack();

    while (program_counter != end_sim_pc) {
        if (enableStopSimAt && (icount_ == Simulator::StopSimAt)) {
            MipsDebugger dbg(this);
            dbg.debug();
        } else {
            SimInstruction *instr = reinterpret_cast<SimInstruction *>(program_counter);
            instructionDecode(instr);
            icount_++;

            int32_t rpc = resume_pc_;
            if (MOZ_UNLIKELY(rpc != 0)) {
                
                activation->setResumePC((void *)get_pc());
                set_pc(rpc);
                resume_pc_ = 0;
            }
        }
        program_counter = get_pc();
    }
}

void
Simulator::callInternal(uint8_t *entry)
{
    
    setRegister(pc, reinterpret_cast<int32_t>(entry));
    
    
    
    setRegister(ra, end_sim_pc);

    
    
    
    int32_t s0_val = getRegister(s0);
    int32_t s1_val = getRegister(s1);
    int32_t s2_val = getRegister(s2);
    int32_t s3_val = getRegister(s3);
    int32_t s4_val = getRegister(s4);
    int32_t s5_val = getRegister(s5);
    int32_t s6_val = getRegister(s6);
    int32_t s7_val = getRegister(s7);
    int32_t gp_val = getRegister(gp);
    int32_t sp_val = getRegister(sp);
    int32_t fp_val = getRegister(fp);

    
    
    int32_t callee_saved_value = icount_;
    setRegister(s0, callee_saved_value);
    setRegister(s1, callee_saved_value);
    setRegister(s2, callee_saved_value);
    setRegister(s3, callee_saved_value);
    setRegister(s4, callee_saved_value);
    setRegister(s5, callee_saved_value);
    setRegister(s6, callee_saved_value);
    setRegister(s7, callee_saved_value);
    setRegister(gp, callee_saved_value);
    setRegister(fp, callee_saved_value);

    
    if (Simulator::StopSimAt != -1)
        execute<true>();
    else
        execute<false>();

    
    MOZ_ASSERT(callee_saved_value == getRegister(s0));
    MOZ_ASSERT(callee_saved_value == getRegister(s1));
    MOZ_ASSERT(callee_saved_value == getRegister(s2));
    MOZ_ASSERT(callee_saved_value == getRegister(s3));
    MOZ_ASSERT(callee_saved_value == getRegister(s4));
    MOZ_ASSERT(callee_saved_value == getRegister(s5));
    MOZ_ASSERT(callee_saved_value == getRegister(s6));
    MOZ_ASSERT(callee_saved_value == getRegister(s7));
    MOZ_ASSERT(callee_saved_value == getRegister(gp));
    MOZ_ASSERT(callee_saved_value == getRegister(fp));

    
    setRegister(s0, s0_val);
    setRegister(s1, s1_val);
    setRegister(s2, s2_val);
    setRegister(s3, s3_val);
    setRegister(s4, s4_val);
    setRegister(s5, s5_val);
    setRegister(s6, s6_val);
    setRegister(s7, s7_val);
    setRegister(gp, gp_val);
    setRegister(sp, sp_val);
    setRegister(fp, fp_val);
}

int64_t
Simulator::call(uint8_t *entry, int argument_count, ...)
{
    va_list parameters;
    va_start(parameters, argument_count);

    int original_stack = getRegister(sp);
    
    int entry_stack = original_stack;
    if (argument_count > kCArgSlotCount)
        entry_stack = entry_stack - argument_count * sizeof(int32_t);
    else
        entry_stack = entry_stack - kCArgsSlotsSize;

    entry_stack &= ~ABIStackAlignment;

    intptr_t *stack_argument = reinterpret_cast<intptr_t*>(entry_stack);

    
    for (int i = 0; i < argument_count; i++) {
        js::jit::Register argReg;
        if (GetIntArgReg(i, &argReg))
            setRegister(argReg.code(), va_arg(parameters, int32_t));
        else
            stack_argument[i] = va_arg(parameters, int32_t);
    }

    va_end(parameters);
    setRegister(sp, entry_stack);

    callInternal(entry);

    
    MOZ_ASSERT(entry_stack == getRegister(sp));
    setRegister(sp, original_stack);

    int64_t result = (int64_t(getRegister(v1)) << 32) | getRegister(v0);
    return result;
}

uintptr_t
Simulator::pushAddress(uintptr_t address)
{
    int new_sp = getRegister(sp) - sizeof(uintptr_t);
    uintptr_t *stack_slot = reinterpret_cast<uintptr_t*>(new_sp);
    *stack_slot = address;
    setRegister(sp, new_sp);
    return new_sp;
}

uintptr_t
Simulator::popAddress()
{
    int current_sp = getRegister(sp);
    uintptr_t *stack_slot = reinterpret_cast<uintptr_t*>(current_sp);
    uintptr_t address = *stack_slot;
    setRegister(sp, current_sp + sizeof(uintptr_t));
    return address;
}

} 
} 

js::jit::Simulator *
js::PerThreadData::simulator() const
{
    return simulator_;
}

void
js::PerThreadData::setSimulator(js::jit::Simulator *sim)
{
    simulator_ = sim;
    simulatorStackLimit_ = sim->stackLimit();
}

js::jit::SimulatorRuntime *
js::PerThreadData::simulatorRuntime() const
{
    return runtime_->simulatorRuntime();
}

uintptr_t *
js::PerThreadData::addressOfSimulatorStackLimit()
{
    return &simulatorStackLimit_;
}

js::jit::SimulatorRuntime *
JSRuntime::simulatorRuntime() const
{
    return simulatorRuntime_;
}

void
JSRuntime::setSimulatorRuntime(js::jit::SimulatorRuntime *srt)
{
    MOZ_ASSERT(!simulatorRuntime_);
    simulatorRuntime_ = srt;
}
