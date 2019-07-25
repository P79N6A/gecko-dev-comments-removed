








































#ifndef jsion_ion_lowering_x86_inl_h__
#define jsion_ion_lowering_x86_inl_h__

#include "ion/IonLIR-inl.h"
#include "ion/IonLowering-inl.h"

namespace js {
namespace ion {





static inline uint32
VirtualRegisterOfPayload(MInstruction *mir)
{
    if (mir->isBox()) {
        MInstruction *inner = mir->toBox()->getInput(0);
        if (!inner->isConstant())
            return inner->id();
    }
    return mir->id() + VREG_DATA_OFFSET;
}

LUse
LIRGeneratorX86::useType(MInstruction *mir, LUse::Policy policy)
{
    JS_ASSERT(mir->id());
    JS_ASSERT(mir->type() == MIRType_Value);

    return LUse(mir->id() + VREG_TYPE_OFFSET, policy);
}

LUse
LIRGeneratorX86::usePayload(MInstruction *mir, LUse::Policy policy)
{
    JS_ASSERT(mir->id());
    JS_ASSERT(mir->type() == MIRType_Value);

    return LUse(VirtualRegisterOfPayload(mir), policy);
}

LUse
LIRGeneratorX86::usePayloadInRegister(MInstruction *mir)
{
    return usePayload(mir, LUse::REGISTER);
}

bool
LIRGeneratorX86::fillBoxUses(LInstruction *lir, size_t n, MInstruction *mir)
{
    if (!ensureDefined(mir))
        return false;
    lir->getOperand(n)->toUse()->setVirtualRegister(mir->id() + VREG_TYPE_OFFSET);
    lir->getOperand(n + 1)->toUse()->setVirtualRegister(VirtualRegisterOfPayload(mir));
    return true;
}

} 
} 

#endif 

