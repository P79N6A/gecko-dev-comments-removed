








































#ifndef jsion_ion_lowering_inl_h__
#define jsion_ion_lowering_inl_h__

#include "MIR.h"

namespace js {
namespace ion {

template <size_t X, size_t Y> bool
LIRGenerator::define(LInstructionHelper<1, X, Y> *lir, MInstruction *mir, const LDefinition &def)
{
    uint32 vreg = nextVirtualRegister();
    if (vreg >= MAX_VIRTUAL_REGISTERS)
        return false;

    
    
    lir->setDef(0, def);
    lir->getDef(0)->setVirtualRegister(vreg);
    mir->setId(vreg);
    mir->setInWorklist();
    return add(lir);
}

template <size_t X, size_t Y> bool
LIRGenerator::define(LInstructionHelper<1, X, Y> *lir, MInstruction *mir)
{
    LDefinition::Type type;
    switch (mir->type()) {
      case MIRType_Boolean:
      case MIRType_Int32:
        type = LDefinition::INTEGER;
        break;
      case MIRType_String:
      case MIRType_Object:
        type = LDefinition::OBJECT;
        break;
      case MIRType_Double:
        type = LDefinition::DOUBLE;
        break;
#if defined(JS_PUNBOX64)
      case MIRType_Value:
        type = LDefinition::BOX;
        break;
#endif
      default:
        JS_NOT_REACHED("unexpected type");
        return false;
    }

    return define(lir, mir, LDefinition(type));
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

} 
} 

#endif 

