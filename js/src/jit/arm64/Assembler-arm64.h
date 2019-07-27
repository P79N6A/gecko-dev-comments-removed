





#ifndef A64_ASSEMBLER_A64_H_
#define A64_ASSEMBLER_A64_H_

#include "jit/arm64/vixl/Assembler-vixl.h"

#include "jit/JitCompartment.h"

namespace js {
namespace jit {


typedef vixl::Register ARMRegister;
typedef vixl::FPRegister ARMFPRegister;
using vixl::ARMBuffer;
using vixl::Instruction;

static const uint32_t AlignmentAtPrologue = 0;
static const uint32_t AlignmentMidPrologue = 8;
static const Scale ScalePointer = TimesEight;
static const uint32_t AlignmentAtAsmJSPrologue = sizeof(void*);




static constexpr Register ScratchReg = { Registers::ip0 };
static constexpr ARMRegister ScratchReg64 = { ScratchReg, 64 };

static constexpr Register ScratchReg2 = { Registers::ip1 };
static constexpr ARMRegister ScratchReg2_64 = { ScratchReg2, 64 };

static constexpr FloatRegister ScratchDoubleReg = { FloatRegisters::d31 };
static constexpr FloatRegister ReturnDoubleReg = { FloatRegisters::d0 };

static constexpr FloatRegister ReturnFloat32Reg = { FloatRegisters::s0 , FloatRegisters::Single };
static constexpr FloatRegister ScratchFloat32Reg = { FloatRegisters::s31 , FloatRegisters::Single };

static constexpr Register InvalidReg = { Registers::invalid_reg };
static constexpr FloatRegister InvalidFloatReg = { FloatRegisters::invalid_fpreg };

static constexpr FloatRegister ReturnInt32x4Reg = InvalidFloatReg;
static constexpr FloatRegister ReturnFloat32x4Reg = InvalidFloatReg;

static constexpr Register OsrFrameReg = { Registers::x3 };
static constexpr Register ArgumentsRectifierReg = { Registers::x8 };
static constexpr Register CallTempReg0 = { Registers::x9 };
static constexpr Register CallTempReg1 = { Registers::x10 };
static constexpr Register CallTempReg2 = { Registers::x11 };
static constexpr Register CallTempReg3 = { Registers::x12 };
static constexpr Register CallTempReg4 = { Registers::x13 };
static constexpr Register CallTempReg5 = { Registers::x14 };

static constexpr Register PreBarrierReg = { Registers::x1 };

static constexpr Register ReturnReg = { Registers::x0 };
static constexpr Register JSReturnReg = { Registers::x2 };
static constexpr Register FramePointer = { Registers::fp };
static constexpr Register ZeroRegister = { Registers::sp };
static constexpr ARMRegister ZeroRegister64 = { Registers::sp, 64 };
static constexpr ARMRegister ZeroRegister32 = { Registers::sp, 32 };

static constexpr FloatRegister ReturnFloatReg = { FloatRegisters::d0 };
static constexpr FloatRegister ScratchFloatReg = { FloatRegisters::d31 };

static constexpr FloatRegister ReturnSimdReg = InvalidFloatReg;
static constexpr FloatRegister ScratchSimdReg = InvalidFloatReg;



static constexpr Register RealStackPointer = { Registers::sp };

static constexpr Register StackPointer = { Registers::sp };

static constexpr Register PseudoStackPointer = { Registers::x28 };
static constexpr ARMRegister PseudoStackPointer64 = { Registers::x28, 64 };
static constexpr ARMRegister PseudoStackPointer32 = { Registers::x28, 32 };


static constexpr Register RegExpStackPointer = PseudoStackPointer;

static constexpr Register IntArgReg0 = { Registers::x0 };
static constexpr Register IntArgReg1 = { Registers::x1 };
static constexpr Register IntArgReg2 = { Registers::x2 };
static constexpr Register IntArgReg3 = { Registers::x3 };
static constexpr Register IntArgReg4 = { Registers::x4 };
static constexpr Register IntArgReg5 = { Registers::x5 };
static constexpr Register IntArgReg6 = { Registers::x6 };
static constexpr Register IntArgReg7 = { Registers::x7 };
static constexpr Register GlobalReg =  { Registers::x20 };
static constexpr Register HeapReg = { Registers::x21 };
static constexpr Register HeapLenReg = { Registers::x22 };


#define DEFINE_UNSIZED_REGISTERS(N)  \
static constexpr Register r##N = { Registers::x##N };
REGISTER_CODE_LIST(DEFINE_UNSIZED_REGISTERS)
#undef DEFINE_UNSIZED_REGISTERS
static constexpr Register ip0 = { Registers::x16 };
static constexpr Register ip1 = { Registers::x16 };
static constexpr Register fp  = { Registers::x30 };
static constexpr Register lr  = { Registers::x30 };
static constexpr Register rzr = { Registers::xzr };


#define IMPORT_VIXL_REGISTERS(N)  \
static constexpr ARMRegister w##N = vixl::w##N;  \
static constexpr ARMRegister x##N = vixl::x##N;
REGISTER_CODE_LIST(IMPORT_VIXL_REGISTERS)
#undef IMPORT_VIXL_REGISTERS
static constexpr ARMRegister wzr = vixl::wzr;
static constexpr ARMRegister xzr = vixl::xzr;
static constexpr ARMRegister wsp = vixl::wsp;
static constexpr ARMRegister sp = vixl::sp;


#define IMPORT_VIXL_VREGISTERS(N)  \
static constexpr ARMFPRegister s##N = vixl::s##N;  \
static constexpr ARMFPRegister d##N = vixl::d##N;
REGISTER_CODE_LIST(IMPORT_VIXL_VREGISTERS)
#undef IMPORT_VIXL_VREGISTERS

static constexpr ValueOperand JSReturnOperand = ValueOperand(JSReturnReg);


static constexpr Register AsmJSIonExitRegCallee = r8;
static constexpr Register AsmJSIonExitRegE0 = r0;
static constexpr Register AsmJSIonExitRegE1 = r1;
static constexpr Register AsmJSIonExitRegE2 = r2;
static constexpr Register AsmJSIonExitRegE3 = r3;



static constexpr Register AsmJSIonExitRegReturnData = r2;
static constexpr Register AsmJSIonExitRegReturnType = r3;
static constexpr Register AsmJSIonExitRegD0 = r0;
static constexpr Register AsmJSIonExitRegD1 = r1;
static constexpr Register AsmJSIonExitRegD2 = r4;

static constexpr Register JSReturnReg_Type = r3;
static constexpr Register JSReturnReg_Data = r2;

static constexpr FloatRegister NANReg = { FloatRegisters::d14 };


static constexpr Register CallTempNonArgRegs[] = { r8, r9, r10, r11, r12, r13, r14, r15 };
static const uint32_t NumCallTempNonArgRegs =
    mozilla::ArrayLength(CallTempNonArgRegs);

static constexpr uint32_t JitStackAlignment = 16;

static constexpr uint32_t JitStackValueAlignment = JitStackAlignment / sizeof(Value);
static_assert(JitStackAlignment % sizeof(Value) == 0 && JitStackValueAlignment >= 1,
  "Stack alignment should be a non-zero multiple of sizeof(Value)");





static constexpr bool SupportsSimd = false;
static constexpr uint32_t SimdMemoryAlignment = 16;

static_assert(CodeAlignment % SimdMemoryAlignment == 0,
  "Code alignment should be larger than any of the alignments which are used for "
  "the constant sections of the code buffer.  Thus it should be larger than the "
  "alignment for SIMD constants.");

static const uint32_t AsmJSStackAlignment = SimdMemoryAlignment;
static const int32_t AsmJSGlobalRegBias = 1024;

class Assembler : public vixl::Assembler
{
  public:
    Assembler()
      : vixl::Assembler()
    { }

    typedef vixl::Condition Condition;

    void finish();
    void trace(JSTracer* trc);

    
    BufferOffset emitExtendedJumpTable();
    BufferOffset ExtendedJumpTable_;
    void executableCopy(uint8_t* buffer);

    BufferOffset immPool(ARMRegister dest, uint8_t* value, vixl::LoadLiteralOp op,
                         ARMBuffer::PoolEntry* pe = nullptr);
    BufferOffset immPool64(ARMRegister dest, uint64_t value, ARMBuffer::PoolEntry* pe = nullptr);
    BufferOffset immPool64Branch(RepatchLabel* label, ARMBuffer::PoolEntry* pe, vixl::Condition c);
    BufferOffset fImmPool(ARMFPRegister dest, uint8_t* value, vixl::LoadLiteralOp op);
    BufferOffset fImmPool64(ARMFPRegister dest, double value);
    BufferOffset fImmPool32(ARMFPRegister dest, float value);

    void bind(Label* label) { bind(label, nextOffset()); }
    void bind(Label* label, BufferOffset boff);
    void bind(RepatchLabel* label);

    bool oom() const {
        return AssemblerShared::oom() ||
            armbuffer_.oom() ||
            jumpRelocations_.oom() ||
            dataRelocations_.oom() ||
            preBarriers_.oom();
    }

    void copyJumpRelocationTable(uint8_t* dest) const {
        if (jumpRelocations_.length())
            memcpy(dest, jumpRelocations_.buffer(), jumpRelocations_.length());
    }
    void copyDataRelocationTable(uint8_t* dest) const {
        if (dataRelocations_.length())
            memcpy(dest, dataRelocations_.buffer(), dataRelocations_.length());
    }
    void copyPreBarrierTable(uint8_t* dest) const {
        if (preBarriers_.length())
            memcpy(dest, preBarriers_.buffer(), preBarriers_.length());
    }

    size_t jumpRelocationTableBytes() const {
        return jumpRelocations_.length();
    }
    size_t dataRelocationTableBytes() const {
        return dataRelocations_.length();
    }
    size_t preBarrierTableBytes() const {
        return preBarriers_.length();
    }
    size_t bytesNeeded() const {
        return SizeOfCodeGenerated() +
            jumpRelocationTableBytes() +
            dataRelocationTableBytes() +
            preBarrierTableBytes();
    }

    BufferOffset nextOffset() const {
        return armbuffer_.nextOffset();
    }

    void addCodeLabel(CodeLabel label) {
        propagateOOM(codeLabels_.append(label));
    }
    size_t numCodeLabels() const {
        return codeLabels_.length();
    }
    CodeLabel codeLabel(size_t i) {
        return codeLabels_[i];
    }
    void processCodeLabels(uint8_t* rawCode) {
        for (size_t i = 0; i < codeLabels_.length(); i++) {
            CodeLabel label = codeLabels_[i];
            Bind(rawCode, label.dest(), rawCode + actualOffset(label.src()->offset()));
        }
    }

    void Bind(uint8_t* rawCode, AbsoluteLabel* label, const void* address) {
        uint32_t off = actualOffset(label->offset());
        *reinterpret_cast<const void**>(rawCode + off) = address;
    }
    bool nextLink(BufferOffset cur, BufferOffset* next) {
        Instruction* link = getInstructionAt(cur);
        uint32_t nextLinkOffset = uint32_t(link->ImmPCRawOffset());
        if (nextLinkOffset == uint32_t(LabelBase::INVALID_OFFSET))
            return false;
        *next = BufferOffset(nextLinkOffset + cur.getOffset());
        return true;
    }
    void retarget(Label* cur, Label* next);

    
    
    void flush() {
        armbuffer_.flushPool();
    }

    int actualOffset(int curOffset) {
        return curOffset + armbuffer_.poolSizeBefore(curOffset);
    }
    int actualIndex(int curOffset) {
        ARMBuffer::PoolEntry pe(curOffset);
        return armbuffer_.poolEntryOffset(pe);
    }
    int labelOffsetToPatchOffset(int labelOff) {
        return actualOffset(labelOff);
    }
    static uint8_t* PatchableJumpAddress(JitCode* code, uint32_t index) {
        return code->raw() + index;
    }
    void setPrinter(Sprinter* sp) {
    }

    static bool SupportsFloatingPoint() { return true; }
    static bool SupportsSimd() { return js::jit::SupportsSimd; }

    
    void addJumpRelocation(BufferOffset src, Relocation::Kind reloc);

  protected:
    
    
    void addPendingJump(BufferOffset src, ImmPtr target, Relocation::Kind kind);

    
    
    size_t addPatchableJump(BufferOffset src, Relocation::Kind kind);

  public:
    static uint32_t PatchWrite_NearCallSize() {
        return 4;
    }

    static uint32_t NopSize() {
        return 4;
    }

    static void PatchWrite_NearCall(CodeLocationLabel start, CodeLocationLabel toCall) {
        Instruction* dest = (Instruction*)start.raw();
        
        bl(dest, ((Instruction*)toCall.raw() - dest)>>2);

    }
    static void PatchDataWithValueCheck(CodeLocationLabel label,
                                        PatchedImmPtr newValue,
                                        PatchedImmPtr expected);

    static void PatchDataWithValueCheck(CodeLocationLabel label,
                                        ImmPtr newValue,
                                        ImmPtr expected);

    static void PatchWrite_Imm32(CodeLocationLabel label, Imm32 imm) {
        
        uint32_t* raw = (uint32_t*)label.raw();
        
        
        *(raw - 1) = imm.value;
    }
    static uint32_t AlignDoubleArg(uint32_t offset) {
        MOZ_CRASH("AlignDoubleArg()");
    }
    static Instruction* NextInstruction(Instruction* instruction, uint32_t* count = nullptr) {
        if (count != nullptr)
            *count += 4;
        Instruction* cur = instruction;
        Instruction* next = cur + 4;
        
        if (next->IsUncondB()) {
            uint32_t* snd = (uint32_t*)(instruction + 8);
            
            
            if ((*snd & 0xffff8000) == 0xffff0000) {
                
                int poolSize =  (*snd & 0x7fff);
                return (Instruction*)(snd + poolSize);
            }
        } else if (cur->IsBR() || cur->IsUncondB()) {
            
            
            if ((next->InstructionBits() & 0xffff0000) == 0xffff0000) {
                int poolSize = (next->InstructionBits() & 0x7fff);
                Instruction* ret = (next + (poolSize << 2));
                return ret;
            }
        }
        return (instruction + 4);

    }
    static uint8_t* NextInstruction(uint8_t* instruction, uint32_t* count = nullptr) {
        return (uint8_t*)NextInstruction((Instruction*)instruction, count);
    }
    static uintptr_t GetPointer(uint8_t* ptr) {
        Instruction* i = reinterpret_cast<Instruction*>(ptr);
        uint64_t ret = i->Literal64();
        return ret;
    }

    
    static void ToggleToJmp(CodeLocationLabel inst_);
    static void ToggleToCmp(CodeLocationLabel inst_);
    static void ToggleCall(CodeLocationLabel inst_, bool enabled);

    static void TraceJumpRelocations(JSTracer* trc, JitCode* code, CompactBufferReader& reader);
    static void TraceDataRelocations(JSTracer* trc, JitCode* code, CompactBufferReader& reader);

    static int32_t ExtractCodeLabelOffset(uint8_t* code);
    static void PatchInstructionImmediate(uint8_t* code, PatchedImmPtr imm);

    static void FixupNurseryObjects(JSContext* cx, JitCode* code, CompactBufferReader& reader,
                                    const ObjectVector& nurseryObjects);

    
    size_t toFinalOffset(BufferOffset offset) {
        return size_t(offset.getOffset() + armbuffer_.poolSizeBefore(offset.getOffset()));
    }

  public:
    
    static const size_t SizeOfJumpTableEntry = 16;

    struct JumpTableEntry
    {
        uint32_t ldr;
        uint32_t br;
        void* data;

        Instruction* getLdr() {
            return reinterpret_cast<Instruction*>(&ldr);
        }
    };

    
    static const size_t OffsetOfJumpTableEntryPointer = 8;

  public:
    static void UpdateBoundsCheck(uint32_t logHeapSize, Instruction* inst);

    void writeCodePointer(AbsoluteLabel* absoluteLabel) {
        MOZ_ASSERT(!absoluteLabel->bound());
        uintptr_t x = LabelBase::INVALID_OFFSET;
        BufferOffset off = EmitData(&x, sizeof(uintptr_t));

        
        
        
        
        
        LabelBase* label = absoluteLabel;
        label->bind(off.getOffset());
    }

    void verifyHeapAccessDisassembly(uint32_t begin, uint32_t end,
                                     const Disassembler::HeapAccess& heapAccess)
    {
        MOZ_CRASH("verifyHeapAccessDisassembly");
    }

  protected:
    
    
    
    struct JumpRelocation
    {
        BufferOffset jump; 
        uint32_t extendedTableIndex; 

        JumpRelocation(BufferOffset jump, uint32_t extendedTableIndex)
          : jump(jump), extendedTableIndex(extendedTableIndex)
        { }
    };

    
    
    
    js::Vector<BufferOffset, 0, SystemAllocPolicy> tmpDataRelocations_;
    js::Vector<BufferOffset, 0, SystemAllocPolicy> tmpPreBarriers_;
    js::Vector<JumpRelocation, 0, SystemAllocPolicy> tmpJumpRelocations_;

    
    
    struct RelativePatch
    {
        BufferOffset offset;
        void* target;
        Relocation::Kind kind;

        RelativePatch(BufferOffset offset, void* target, Relocation::Kind kind)
          : offset(offset), target(target), kind(kind)
        { }
    };

    js::Vector<CodeLabel, 0, SystemAllocPolicy> codeLabels_;

    
    
    
    js::Vector<RelativePatch, 8, SystemAllocPolicy> pendingJumps_;

    
    CompactBufferWriter jumpRelocations_;
    CompactBufferWriter dataRelocations_;
    CompactBufferWriter preBarriers_;
};

static const uint32_t NumIntArgRegs = 8;
static const uint32_t NumFloatArgRegs = 8;

class ABIArgGenerator
{
  public:
    ABIArgGenerator()
      : intRegIndex_(0),
        floatRegIndex_(0),
        stackOffset_(0),
        current_()
    { }

    ABIArg next(MIRType argType);
    ABIArg& current() { return current_; }
    uint32_t stackBytesConsumedSoFar() const { return stackOffset_; }

  public:
    static const Register NonArgReturnReg0;
    static const Register NonArgReturnReg1;
    static const Register NonVolatileReg;
    static const Register NonArg_VolatileReg;
    static const Register NonReturn_VolatileReg0;
    static const Register NonReturn_VolatileReg1;

  protected:
    unsigned intRegIndex_;
    unsigned floatRegIndex_;
    uint32_t stackOffset_;
    ABIArg current_;
};

static inline bool
GetIntArgReg(uint32_t usedIntArgs, uint32_t usedFloatArgs, Register* out)
{
    if (usedIntArgs >= NumIntArgRegs)
        return false;
    *out = Register::FromCode(usedIntArgs);
    return true;
}

static inline bool
GetFloatArgReg(uint32_t usedIntArgs, uint32_t usedFloatArgs, FloatRegister* out)
{
    if (usedFloatArgs >= NumFloatArgRegs)
        return false;
    *out = FloatRegister::FromCode(usedFloatArgs);
    return true;
}






static inline bool
GetTempRegForIntArg(uint32_t usedIntArgs, uint32_t usedFloatArgs, Register* out)
{
    if (GetIntArgReg(usedIntArgs, usedFloatArgs, out))
        return true;
    
    
    
    usedIntArgs -= NumIntArgRegs;
    if (usedIntArgs >= NumCallTempNonArgRegs)
        return false;
    *out = CallTempNonArgRegs[usedIntArgs];
    return true;

}

void PatchJump(CodeLocationJump& jump_, CodeLocationLabel label);

static inline void
PatchBackedge(CodeLocationJump& jump_, CodeLocationLabel label, JitRuntime::BackedgeTarget target)
{
    PatchJump(jump_, label);
}


class AutoForbidPools
{
    Assembler* asm_;

  public:
    AutoForbidPools(Assembler* asm_, size_t maxInst)
      : asm_(asm_)
    {
        asm_->enterNoPool(maxInst);
    }

    ~AutoForbidPools() {
        asm_->leaveNoPool();
    }
};

} 
} 

#endif 
