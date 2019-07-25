








































#ifndef jsion_codegen_inl_h__
#define jsion_codegen_inl_h__

namespace js {
namespace ion {

static inline Register
ToRegister(const LAllocation &a)
{
    JS_ASSERT(a.isGeneralReg());
    return a.toGeneralReg()->reg();
}

static inline Register
ToRegister(const LAllocation *a)
{
    return ToRegister(*a);
}

static inline Register
ToRegister(const LDefinition *def)
{
    return ToRegister(*def->output());
}

static inline FloatRegister
ToFloatRegister(const LAllocation &a)
{
    JS_ASSERT(a.isFloatReg());
    return a.toFloatReg()->reg();
}

static inline FloatRegister
ToFloatRegister(const LAllocation *a)
{
    return ToFloatRegister(*a);
}

static inline FloatRegister
ToFloatRegister(const LDefinition *def)
{
    return ToFloatRegister(*def->output());
}

static inline AnyRegister
ToAnyRegister(const LAllocation &a)
{
    JS_ASSERT(a.isGeneralReg() || a.isFloatReg());
    if (a.isGeneralReg())
        return AnyRegister(ToRegister(a));
    return AnyRegister(ToFloatRegister(a));
}

static inline AnyRegister
ToAnyRegister(const LAllocation *a)
{
    return ToAnyRegister(*a);
}

static inline AnyRegister
ToAnyRegister(const LDefinition *def)
{
    return ToAnyRegister(def->output());
}

static inline ValueOperand
GetValueOutput(LInstruction *ins)
{
#if defined(JS_NUNBOX32)
    return ValueOperand(ToRegister(ins->getDef(TYPE_INDEX)),
                        ToRegister(ins->getDef(PAYLOAD_INDEX)));
#elif defined(JS_PUNBOX64)
    return ValueOperand(ToRegister(ins->getDef(0)));
#else
#error "Unknown"
#endif
}

void
CodeGeneratorShared::saveLive(LInstruction *ins)
{
    LSafepoint *safepoint = ins->safepoint();

    masm.PushRegsInMask(safepoint->restRegs());
    masm.PushRegsInMask(safepoint->gcRegs());
}

void
CodeGeneratorShared::restoreLive(LInstruction *ins)
{
    LSafepoint *safepoint = ins->safepoint();

    masm.PopRegsInMask(safepoint->restRegs());
    masm.PopRegsInMask(safepoint->gcRegs());
}

} 
} 

#endif 

