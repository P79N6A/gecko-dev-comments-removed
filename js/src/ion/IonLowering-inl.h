








































#ifndef jsion_ion_lowering_inl_h__
#define jsion_ion_lowering_inl_h__

#include "MIR.h"
#include "MIRGraph.h"

namespace js {
namespace ion {

template <size_t X, size_t Y> bool
LIRGenerator::define(LInstructionHelper<1, X, Y> *lir, MDefinition *mir, const LDefinition &def)
{
    uint32 vreg = getVirtualRegister();
    if (vreg >= MAX_VIRTUAL_REGISTERS)
        return false;

    
    
    lir->setDef(0, def);
    lir->getDef(0)->setVirtualRegister(vreg);
    mir->setId(vreg);
    return add(lir);
}

template <size_t X, size_t Y> bool
LIRGenerator::define(LInstructionHelper<1, X, Y> *lir, MDefinition *mir, LDefinition::Policy policy)
{
    LDefinition::Type type = LDefinition::TypeFrom(mir->type());
    return define(lir, mir, LDefinition(type, policy));
}

template <size_t Ops, size_t Temps> bool
LIRGenerator::defineReuseInput(LInstructionHelper<1, Ops, Temps> *lir, MDefinition *mir)
{
    return define(lir, mir, LDefinition::MUST_REUSE_INPUT);
}

template <size_t Ops, size_t Temps> bool
LIRGenerator::defineBox(LInstructionHelper<BOX_PIECES, Ops, Temps> *lir, MDefinition *mir,
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

    mir->setId(vreg);
    return add(lir);
}

bool
LIRGenerator::ensureDefined(MDefinition *mir)
{
    if (mir->isEmittedAtUses()) {
        if (!mir->toInstruction()->accept(this))
            return false;
        JS_ASSERT(mir->id());
    }
    return true;
}

LUse
LIRGenerator::useRegister(MDefinition *mir)
{
    return use(mir, LUse(LUse::REGISTER));
}

LUse
LIRGenerator::use(MDefinition *mir)
{
    return use(mir, LUse(LUse::ANY));
}

LAllocation
LIRGenerator::useOrConstant(MDefinition *mir)
{
    if (mir->isConstant())
        return LAllocation(mir->toConstant()->vp());
    return use(mir);
}

LAllocation
LIRGenerator::useRegisterOrConstant(MDefinition *mir)
{
    if (mir->isConstant())
        return LAllocation(mir->toConstant()->vp());
    return use(mir, LUse(LUse::REGISTER));
}

LAllocation
LIRGenerator::useKeepaliveOrConstant(MDefinition *mir)
{
    if (mir->isConstant())
        return LAllocation(mir->toConstant()->vp());
    return use(mir, LUse(LUse::KEEPALIVE));
}

LUse
LIRGenerator::useFixed(MDefinition *mir, Register reg)
{
    return use(mir, LUse(reg));
}

LUse
LIRGenerator::useFixed(MDefinition *mir, FloatRegister reg)
{
    return use(mir, LUse(reg));
}

LDefinition
LIRGenerator::temp(LDefinition::Type type)
{
    uint32 vreg = getVirtualRegister();
    if (vreg >= MAX_VIRTUAL_REGISTERS) {
        gen->abort("max virtual registers");
        return LDefinition();
    }
    return LDefinition(vreg, type);
}

template <typename T> bool
LIRGenerator::annotate(T *ins)
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
LIRGenerator::add(T *ins)
{
    JS_ASSERT(!ins->isPhi());
    current->add(ins);
    return annotate(ins);
}

} 
} 

#endif 

