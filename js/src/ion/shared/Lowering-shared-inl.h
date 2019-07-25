








































#ifndef jsion_ion_lowering_inl_h__
#define jsion_ion_lowering_inl_h__

#include "ion/MIR.h"
#include "ion/MIRGraph.h"

namespace js {
namespace ion {

bool
LIRGeneratorShared::emitAtUses(MInstruction *mir)
{
    JS_ASSERT(mir->canEmitAtUses());
    mir->setEmittedAtUses();
    mir->setVirtualRegister(0);
    return true;
}

LUse
LIRGeneratorShared::use(MDefinition *mir, LUse policy)
{
    
#if BOX_PIECES > 1
    JS_ASSERT(mir->type() != MIRType_Value);
#endif
    if (!ensureDefined(mir))
        return policy;
    policy.setVirtualRegister(mir->virtualRegister());
    return policy;
}

template <size_t X, size_t Y> bool
LIRGeneratorShared::define(LInstructionHelper<1, X, Y> *lir, MDefinition *mir, const LDefinition &def)
{
    uint32 vreg = getVirtualRegister();
    if (vreg >= MAX_VIRTUAL_REGISTERS)
        return false;

    
    
    lir->setDef(0, def);
    lir->getDef(0)->setVirtualRegister(vreg);
    lir->setMir(mir);
    mir->setVirtualRegister(vreg);
    return add(lir);
}

template <size_t X, size_t Y> bool
LIRGeneratorShared::define(LInstructionHelper<1, X, Y> *lir, MDefinition *mir, LDefinition::Policy policy)
{
    LDefinition::Type type = LDefinition::TypeFrom(mir->type());
    return define(lir, mir, LDefinition(type, policy));
}

template <size_t X, size_t Y> bool
LIRGeneratorShared::defineFixed(LInstructionHelper<1, X, Y> *lir, MDefinition *mir, const LAllocation &output)
{
    LDefinition::Type type = LDefinition::TypeFrom(mir->type());
    
    LDefinition def(type, LDefinition::PRESET);
    def.setOutput(output);

    return define(lir, mir, def);
}

template <size_t Ops, size_t Temps> bool
LIRGeneratorShared::defineReuseInput(LInstructionHelper<1, Ops, Temps> *lir, MDefinition *mir)
{
    return define(lir, mir, LDefinition::MUST_REUSE_INPUT);
}

template <size_t Ops, size_t Temps> bool
LIRGeneratorShared::defineBox(LInstructionHelper<BOX_PIECES, Ops, Temps> *lir, MDefinition *mir,
                              LDefinition::Policy policy)
{
    uint32 vreg = getVirtualRegister();
    if (vreg >= MAX_VIRTUAL_REGISTERS)
        return false;

#if defined(JS_NUNBOX32)
    lir->setDef(0, LDefinition(vreg + VREG_TYPE_OFFSET, LDefinition::TYPE, policy));
    lir->setDef(1, LDefinition(vreg + VREG_DATA_OFFSET, LDefinition::PAYLOAD, policy));
    if (getVirtualRegister() >= MAX_VIRTUAL_REGISTERS)
        return false;
#elif defined(JS_PUNBOX64)
    lir->setDef(0, LDefinition(vreg, LDefinition::BOX, policy));
#endif
    lir->setMir(mir);

    mir->setVirtualRegister(vreg);
    return add(lir);
}

template <size_t Ops, size_t Temps> bool
LIRGeneratorShared::defineReturn(LInstructionHelper<BOX_PIECES, Ops, Temps> *lir, MDefinition *mir)
{
    defineBox(lir, mir, LDefinition::PRESET);

#if defined(JS_NUNBOX32)
    lir->getDef(TYPE_INDEX)->setOutput(LGeneralReg(JSReturnReg_Type));
    lir->getDef(PAYLOAD_INDEX)->setOutput(LGeneralReg(JSReturnReg_Data));
#elif defined(JS_PUNBOX64)
    lir->getDef(0)->setOutput(LGeneralReg(JSReturnReg));
#endif

    return true;
}

template <size_t Defs, size_t Ops, size_t Temps> bool
LIRGeneratorShared::defineVMReturn(LInstructionHelper<Defs, Ops, Temps> *lir, MDefinition *mir)
{
    uint32 vreg = getVirtualRegister();
    if (vreg >= MAX_VIRTUAL_REGISTERS)
        return false;

    switch (mir->type()) {
      case MIRType_Value:
#if defined(JS_NUNBOX32)
        lir->setDef(TYPE_INDEX, LDefinition(vreg + VREG_TYPE_OFFSET, LDefinition::TYPE,
                                            LGeneralReg(JSReturnReg_Type)));
        lir->setDef(PAYLOAD_INDEX, LDefinition(vreg + VREG_DATA_OFFSET, LDefinition::PAYLOAD,
                                               LGeneralReg(JSReturnReg_Data)));

        if (getVirtualRegister() >= MAX_VIRTUAL_REGISTERS)
            return false;
#elif defined(JS_PUNBOX64)
        lir->setDef(0, LDefinition(vreg, LDefinition::BOX, LGeneralReg(JSCReturnReg)));
#endif
        break;
      default:
        LDefinition::Type type = LDefinition::TypeFrom(mir->type());
        lir->setDef(0, LDefinition(vreg, type, LGeneralReg(ReturnReg)));
        break;
    }

    mir->setVirtualRegister(vreg);
    lir->setMir(mir);
    return add(lir);
}





static inline bool
IsCompatibleLIRCoercion(MIRType to, MIRType from)
{
    if (to == from)
        return true;
    if ((to == MIRType_Int32 || to == MIRType_Boolean) &&
        (from == MIRType_Int32 || from == MIRType_Boolean)) {
        return true;
    }
    return false;
}

bool
LIRGeneratorShared::redefine(MDefinition *def, MDefinition *as)
{
    JS_ASSERT(IsCompatibleLIRCoercion(def->type(), as->type()));
    if (!ensureDefined(as))
        return false;
    def->setVirtualRegister(as->virtualRegister());
    return true;
}

bool
LIRGeneratorShared::defineAs(LInstruction *outLir, MDefinition *outMir, MDefinition *inMir)
{
    uint32 vreg = inMir->virtualRegister();
    LDefinition::Policy policy = LDefinition::REDEFINED;

    if (outMir->type() == MIRType_Value) {
#ifdef JS_NUNBOX32
        outLir->setDef(TYPE_INDEX,
                       LDefinition(vreg + VREG_TYPE_OFFSET, LDefinition::TYPE, policy));
        outLir->setDef(PAYLOAD_INDEX,
                       LDefinition(vreg + VREG_DATA_OFFSET, LDefinition::PAYLOAD, policy));
#elif JS_PUNBOX64
        outLir->setDef(0, LDefinition(vreg, LDefinition::BOX, policy));
#else
# error "Unexpected boxing type"
#endif
    } else {
        outLir->setDef(0, LDefinition(vreg, LDefinition::TypeFrom(inMir->type()), policy));
    }
    outLir->setMir(outMir);
    return redefine(outMir, inMir);
}

bool
LIRGeneratorShared::ensureDefined(MDefinition *mir)
{
    if (mir->isEmittedAtUses()) {
        if (!mir->toInstruction()->accept(this))
            return false;
        JS_ASSERT(mir->isLowered());
    }
    return true;
}

LUse
LIRGeneratorShared::useCopy(MDefinition *mir)
{
    return use(mir, LUse(LUse::COPY));
}

LUse
LIRGeneratorShared::useRegister(MDefinition *mir)
{
    return use(mir, LUse(LUse::REGISTER));
}

LUse
LIRGeneratorShared::use(MDefinition *mir)
{
    return use(mir, LUse(LUse::ANY));
}

LAllocation
LIRGeneratorShared::useOrConstant(MDefinition *mir)
{
    if (mir->isConstant())
        return LAllocation(mir->toConstant()->vp());
    return use(mir);
}

LAllocation
LIRGeneratorShared::useRegisterOrConstant(MDefinition *mir)
{
    if (mir->isConstant())
        return LAllocation(mir->toConstant()->vp());
    return use(mir, LUse(LUse::REGISTER));
}

LAllocation
LIRGeneratorShared::useKeepaliveOrConstant(MDefinition *mir)
{
    if (mir->isConstant())
        return LAllocation(mir->toConstant()->vp());
    return use(mir, LUse(LUse::KEEPALIVE));
}

LUse
LIRGeneratorShared::useFixed(MDefinition *mir, Register reg)
{
    return use(mir, LUse(reg));
}

LUse
LIRGeneratorShared::useFixed(MDefinition *mir, FloatRegister reg)
{
    return use(mir, LUse(reg));
}

LDefinition
LIRGeneratorShared::temp(LDefinition::Type type)
{
    uint32 vreg = getVirtualRegister();
    if (vreg >= MAX_VIRTUAL_REGISTERS) {
        gen->abort("max virtual registers");
        return LDefinition();
    }
    return LDefinition(vreg, type);
}

LDefinition
LIRGeneratorShared::tempFixed(Register reg)
{
    LDefinition t = temp(LDefinition::INTEGER);
    t.setOutput(LGeneralReg(reg));
    return t;
}

LDefinition
LIRGeneratorShared::tempFloat()
{
    return temp(LDefinition::DOUBLE);
}

template <typename T> bool
LIRGeneratorShared::annotate(T *ins)
{
    for (size_t i = 0; i < ins->numDefs(); i++) {
        if (ins->getDef(i)->policy() != LDefinition::REDEFINED) {
            ins->setId(ins->getDef(i)->virtualRegister());
            return true;
        }
    }

    ins->setId(getVirtualRegister());
    return ins->id() < MAX_VIRTUAL_REGISTERS;
}

template <typename T> bool
LIRGeneratorShared::add(T *ins, MInstruction *mir)
{
    JS_ASSERT(!ins->isPhi());
    current->add(ins);
    if (mir)
        ins->setMir(mir);
    return annotate(ins);
}

} 
} 

#endif 

