








































#ifndef jsion_ion_lowering_inl_h__
#define jsion_ion_lowering_inl_h__

#include "MIR.h"
#include "MIRGraph.h"

namespace js {
namespace ion {

template <size_t X, size_t Y> bool
LIRGenerator::define(LInstructionHelper<1, X, Y> *lir, MInstruction *mir, const LDefinition &def)
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
LIRGenerator::define(LInstructionHelper<1, X, Y> *lir, MInstruction *mir, LDefinition::Policy policy)
{
    LDefinition::Type type = LDefinition::TypeFrom(mir->type());
    return define(lir, mir, LDefinition(type, policy));
}

template <size_t Ops, size_t Temps> bool
LIRGenerator::defineReuseInput(LInstructionHelper<1, Ops, Temps> *lir, MInstruction *mir)
{
    return define(lir, mir, LDefinition::MUST_REUSE_INPUT);
}

template <size_t Ops, size_t Temps> bool
LIRGenerator::defineBox(LInstructionHelper<BOX_PIECES, Ops, Temps> *lir, MInstruction *mir,
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
LIRGenerator::ensureDefined(MInstruction *mir)
{
    if (mir->emitAtUses()) {
        if (!mir->accept(this))
            return false;
        JS_ASSERT(mir->id());
    }
    return true;
}

LUse
LIRGenerator::useRegister(MInstruction *mir)
{
    return use(mir, LUse(LUse::REGISTER));
}

LUse
LIRGenerator::use(MInstruction *mir)
{
    return use(mir, LUse(LUse::ANY));
}

LAllocation
LIRGenerator::useOrConstant(MInstruction *mir)
{
    if (mir->isConstant())
        return LAllocation(mir->toConstant()->vp());
    return use(mir);
}

LAllocation
LIRGenerator::useRegisterOrConstant(MInstruction *mir)
{
    if (mir->isConstant())
        return LAllocation(mir->toConstant()->vp());
    return use(mir, LUse(LUse::REGISTER));
}

LUse
LIRGenerator::useFixed(MInstruction *mir, Register reg)
{
    return use(mir, LUse(reg));
}

LUse
LIRGenerator::useFixed(MInstruction *mir, FloatRegister reg)
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
    if (ins->numDefs()) {
        ins->setId(ins->getDef(0)->virtualRegister());
    } else {
        ins->setId(getVirtualRegister());
        if (ins->id() >= MAX_VIRTUAL_REGISTERS)
            return false;
    }
    return true;
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

