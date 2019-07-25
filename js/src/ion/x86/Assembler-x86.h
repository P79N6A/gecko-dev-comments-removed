








































#ifndef jsion_cpu_x86_assembler_h__
#define jsion_cpu_x86_assembler_h__

#include "ion/shared/Assembler-shared.h"
#include "assembler/assembler/X86Assembler.h"
#include "ion/CompactBuffer.h"
#include "ion/IonCode.h"

namespace js {
namespace ion {

static const Register eax = { JSC::X86Registers::eax };
static const Register ecx = { JSC::X86Registers::ecx };
static const Register edx = { JSC::X86Registers::edx };
static const Register ebx = { JSC::X86Registers::ebx };
static const Register esp = { JSC::X86Registers::esp };
static const Register ebp = { JSC::X86Registers::ebp };
static const Register esi = { JSC::X86Registers::esi };
static const Register edi = { JSC::X86Registers::edi };

static const Register InvalidReg = { JSC::X86Registers::invalid_reg };
static const FloatRegister InvalidFloatReg = { JSC::X86Registers::invalid_xmm };

static const Register JSReturnReg_Type = ecx;
static const Register JSReturnReg_Data = edx;
static const Register StackPointer = esp;
static const Register ReturnReg = eax;
static const FloatRegister ScratchFloatReg = { JSC::X86Registers::xmm7 };

static const Register ArgumentsRectifierReg = { JSC::X86Registers::esi };

struct ImmTag : public Imm32
{
    ImmTag(JSValueTag mask)
      : Imm32(int32(mask))
    { }
};

struct ImmType : public ImmTag
{
    ImmType(JSValueType type)
      : ImmTag(JSVAL_TYPE_TO_TAG(type))
    { }
};

enum Scale {
    TimesOne,
    TimesTwo,
    TimesFour,
    TimesEight
};

static const Scale ScalePointer = TimesFour;

class Operand
{
  public:
    enum Kind {
        REG,
        REG_DISP,
        FPREG,
        SCALE
    };

    Kind kind_ : 2;
    int32 base_ : 5;
    Scale scale_ : 2;
    int32 disp_;
    int32 index_ : 5;

  public:
    explicit Operand(const Register &reg)
      : kind_(REG),
        base_(reg.code())
    { }
    explicit Operand(const FloatRegister &reg)
      : kind_(FPREG),
        base_(reg.code())
    { }
    explicit Operand(const Register &base, const Register &index, Scale scale, int32 disp = 0)
      : kind_(SCALE),
        base_(base.code()),
        scale_(scale),
        disp_(disp),
        index_(index.code())
    { }
    Operand(const Register &reg, int32 disp)
      : kind_(REG_DISP),
        base_(reg.code()),
        disp_(disp)
    { }

    Kind kind() const {
        return kind_;
    }
    Registers::Code reg() const {
        JS_ASSERT(kind() == REG);
        return (Registers::Code)base_;
    }
    Registers::Code base() const {
        JS_ASSERT(kind() == REG_DISP || kind() == SCALE);
        return (Registers::Code)base_;
    }
    Registers::Code index() const {
        JS_ASSERT(kind() == SCALE);
        return (Registers::Code)index_;
    }
    Scale scale() const {
        JS_ASSERT(kind() == SCALE);
        return scale_;
    }
    FloatRegisters::Code fpu() const {
        JS_ASSERT(kind() == FPREG);
        return (FloatRegisters::Code)base_;
    }
    int32 disp() const {
        JS_ASSERT(kind() == REG_DISP || kind() == SCALE);
        return disp_;
    }
};

} 
} 

#include "ion/shared/Assembler-x86-shared.h"

namespace js {
namespace ion {

class ValueOperand
{
    Register type_;
    Register payload_;

  public:
    ValueOperand(Register type, Register payload)
      : type_(type), payload_(payload)
    { }

    Operand type() const {
        return Operand(type_);
    }
    Register typeReg() const {
        return type_;
    }
    Operand payload() const {
        return Operand(payload_);
    }
    Register payloadReg() const {
        return payload_;
    }
};

class Assembler : public AssemblerX86Shared
{
    void writeRelocation(JmpSrc src) {
        relocations_.writeUnsigned(src.offset());
    }
    void addPendingJump(JmpSrc src, void *target, Relocation::Kind kind) {
        enoughMemory_ &= jumps_.append(RelativePatch(src.offset(), target, kind));
        if (kind == Relocation::CODE)
            writeRelocation(src);
    }

  public:
    using AssemblerX86Shared::movl;
    using AssemblerX86Shared::j;
    using AssemblerX86Shared::jmp;
    using AssemblerX86Shared::movsd;
    using AssemblerX86Shared::retarget;

    static void TraceRelocations(JSTracer *trc, IonCode *code, CompactBufferReader &reader);

    
    
    void flush() { }

    
    
    void executableCopy(uint8 *buffer);

    

    void movl(const ImmGCPtr &ptr, const Register &dest) {
        masm.movl_i32r(ptr.value, dest.code());
    }

    void mov(const Imm32 &imm32, const Register &dest) {
        movl(imm32, dest);
    }
    void mov(const Operand &src, const Register &dest) {
        movl(src, dest);
    }
    void mov(const Register &src, const Operand &dest) {
        movl(src, dest);
    }
    void mov(AbsoluteLabel *label, const Register &dest) {
        JS_ASSERT(!label->bound());
        
        
        masm.movl_i32r(label->prev(), dest.code());
        label->setPrev(masm.size());
    }
    void mov(const Register &src, const Register &dest) {
        movl(src, dest);
    }
    void lea(const Operand &src, const Register &dest) {
        switch (src.kind()) {
          case Operand::REG_DISP:
            masm.leal_mr(src.disp(), src.base(), dest.code());
            break;
          case Operand::SCALE:
            masm.leal_mr(src.disp(), src.base(), src.index(), src.scale(), dest.code());
            break;
          default:
            JS_NOT_REACHED("unexpected operand kind");
        }
    }
    void cvttsd2s(const FloatRegister &src, const Register &dest) {
        cvttsd2si(src, dest);
    }

    void jmp(void *target, Relocation::Kind reloc) {
        JmpSrc src = masm.jmp();
        addPendingJump(src, target, reloc);
    }
    void j(Condition cond, void *target, Relocation::Kind reloc) {
        JmpSrc src = masm.jCC(static_cast<JSC::X86Assembler::Condition>(cond));
        addPendingJump(src, target, reloc);
    }

    
    
    void retarget(Label *label, void *target, Relocation::Kind reloc) {
        JSC::MacroAssembler::Label jsclabel;
        if (label->used()) {
            bool more;
            JSC::X86Assembler::JmpSrc jmp(label->offset());
            do {
                JSC::X86Assembler::JmpSrc next;
                more = masm.nextJump(jmp, &next);
                addPendingJump(jmp, target, reloc);
                jmp = next;
            } while (more);
        }
        label->reset();
    }

    void movsd(const double *dp, const FloatRegister &dest) {
        masm.movsd_mr((const void *)dp, dest.code());
    }
    void movsd(AbsoluteLabel *label, const FloatRegister &dest) {
        JS_ASSERT(!label->bound());
        
        
        masm.movsd_mr(reinterpret_cast<void *>(label->prev()), dest.code());
        label->setPrev(masm.size());
    }
};

static const uint32 NumArgRegs = 0;

static inline bool
GetArgReg(uint32 arg, Register *out)
{
    return false;
}

} 
} 

#endif

