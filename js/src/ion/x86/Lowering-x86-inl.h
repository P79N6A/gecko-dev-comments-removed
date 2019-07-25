








































#ifndef jsion_ion_lowering_x86_inl_h__
#define jsion_ion_lowering_x86_inl_h__

#include "ion/IonLIR-inl.h"

namespace js {
namespace ion {





static inline uint32
VirtualRegisterOfPayload(MDefinition *mir)
{
    if (mir->isBox()) {
        MDefinition *inner = mir->toBox()->getOperand(0);
        if (!inner->isConstant() && inner->type() != MIRType_Double)
            return inner->virtualRegister();
    }
    return mir->virtualRegister() + VREG_DATA_OFFSET;
}

LUse
LIRGeneratorX86::useType(MDefinition *mir, LUse::Policy policy)
{
    JS_ASSERT(mir->type() == MIRType_Value);

    return LUse(mir->virtualRegister() + VREG_TYPE_OFFSET, policy);
}

LUse
LIRGeneratorX86::usePayload(MDefinition *mir, LUse::Policy policy)
{
    JS_ASSERT(mir->type() == MIRType_Value);

    return LUse(VirtualRegisterOfPayload(mir), policy);
}

LUse
LIRGeneratorX86::usePayloadInRegister(MDefinition *mir)
{
    return usePayload(mir, LUse::REGISTER);
}

bool
LIRGeneratorX86::fillBoxUses(LInstruction *lir, size_t n, MDefinition *mir)
{
    if (!ensureDefined(mir))
        return false;
    lir->getOperand(n)->toUse()->setVirtualRegister(mir->virtualRegister() + VREG_TYPE_OFFSET);
    lir->getOperand(n + 1)->toUse()->setVirtualRegister(VirtualRegisterOfPayload(mir));
    return true;
}

} 
} 

#endif 

