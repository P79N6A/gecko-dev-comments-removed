








































#ifndef jsion_ion_lowering_x86_inl_h__
#define jsion_ion_lowering_x86_inl_h__

#include "ion/IonLIR-inl.h"
#include "ion/IonLowering-inl.h"

namespace js {
namespace ion {

LUse
LIRGeneratorX86::useType(MInstruction *mir)
{
    JS_ASSERT(mir->id());
    JS_ASSERT(mir->type() == MIRType_Value);

    return LUse(mir->id(), LUse::ANY);
}

LUse
LIRGeneratorX86::usePayload(MInstruction *mir, LUse::Policy policy)
{
    JS_ASSERT(mir->id());
    JS_ASSERT(mir->type() == MIRType_Value);

    return LUse(mir->id() + VREG_INCREMENT, policy);
}

LUse
LIRGeneratorX86::usePayloadInRegister(MInstruction *mir)
{
    return usePayload(mir, LUse::REGISTER);
}

void
LIRGeneratorX86::fillBoxUses(LInstruction *lir, size_t n, MInstruction *mir)
{
    startUsing(mir);
    lir->getOperand(n)->toUse()->setVirtualRegister(mir->id());
    lir->getOperand(n + 1)->toUse()->setVirtualRegister(mir->id() + VREG_INCREMENT);
    stopUsing(mir);
}

} 
} 

#endif 

