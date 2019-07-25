








































#include "ion/MIR.h"
#include "ion/IonLowering.h"
#include "ion/IonLowering-inl.h"
#include "Lowering-x86.h"
#include "Lowering-x86-inl.h"

using namespace js;
using namespace js::ion;

bool
LIRGeneratorX86::visitConstant(MConstant *ins)
{
    
    if (!ins->emitAtUses() && !ins->value().isDouble())
        return emitAtUses(ins);

    return LIRGenerator::visitConstant(ins);
}

bool
LIRGeneratorX86::visitBox(MBox *box)
{
    if (!box->emitAtUses())
        return emitAtUses(box);

    MInstruction *inner = box->getInput(0);
    LBox *lir = new LBox(useOrConstant(inner));

    
    if (inner->isConstant())
        return defineBox(lir, box);

    
    
    uint32 vreg = getVirtualRegister();
    if (vreg >= MAX_VIRTUAL_REGISTERS)
        return false;

    lir->setDef(0, LDefinition(vreg, LDefinition::TYPE));
    lir->setDef(1, LDefinition(inner->id(), LDefinition::PAYLOAD, LDefinition::REDEFINED));
    box->setId(vreg);
    return add(lir);
}

bool
LIRGeneratorX86::visitUnbox(MUnbox *unbox)
{
    
    
    
    MInstruction *inner = unbox->getInput(0);

    if (unbox->type() == MIRType_Double) {
        LBoxToDouble *lir = new LBoxToDouble;
        if (!ensureDefined(inner))
            return false;
        lir->setOperand(0, useType(inner, LUse::ANY));
        lir->setOperand(1, usePayloadInRegister(inner));
        lir->setTemp(0, temp(LDefinition::DOUBLE));
        return define(lir, unbox, LDefinition::DEFAULT);
    }

    LUnbox *lir = new LUnbox(unbox->type());
    lir->setOperand(0, useType(inner, LUse::ANY));
    lir->setOperand(1, usePayloadInRegister(inner));

    
    LDefinition::Type type = LDefinition::TypeFrom(unbox->type());
    lir->setDef(0, LDefinition(VirtualRegisterOfPayload(inner), type, LDefinition::REDEFINED));
    unbox->setId(VirtualRegisterOfPayload(inner));

    return assignSnapshot(lir) && add(lir);
}

bool
LIRGeneratorX86::visitReturn(MReturn *ret)
{
    MInstruction *opd = ret->getInput(0);
    JS_ASSERT(opd->type() == MIRType_Value);

    LReturn *ins = new LReturn;
    ins->setOperand(0, LUse(JSReturnReg_Type));
    ins->setOperand(1, LUse(JSReturnReg_Data));
    return fillBoxUses(ins, 0, opd) && add(ins);
}

bool
LIRGeneratorX86::preparePhi(MPhi *phi)
{
    uint32 first_vreg = getVirtualRegister();
    if (first_vreg >= MAX_VIRTUAL_REGISTERS)
        return false;

    phi->setId(first_vreg);

    if (phi->type() == MIRType_Value) {
        uint32 payload_vreg = getVirtualRegister();
        if (payload_vreg >= MAX_VIRTUAL_REGISTERS)
            return false;
        JS_ASSERT(first_vreg + VREG_DATA_OFFSET == payload_vreg);
    }

    return true;
}

bool
LIRGeneratorX86::visitPhi(MPhi *ins)
{
    JS_ASSERT(ins->inWorklist() && ins->id());

    
    if (ins->type() != MIRType_Value)
        return lowerPhi(ins);

    
    
    
    
    LPhi *type = LPhi::New(gen, ins);
    LPhi *payload = LPhi::New(gen, ins);
    if (!type || !payload)
        return false;

    for (size_t i = 0; i < ins->numOperands(); i++) {
        MInstruction *opd = ins->getInput(i);
        JS_ASSERT(opd->type() == MIRType_Value);
        JS_ASSERT(opd->id());
        JS_ASSERT(opd->inWorklist());

        type->setOperand(i, LUse(opd->id() + VREG_TYPE_OFFSET, LUse::ANY));
        payload->setOperand(i, LUse(VirtualRegisterOfPayload(opd), LUse::ANY));
    }

    type->setDef(0, LDefinition(ins->id() + VREG_TYPE_OFFSET, LDefinition::TYPE));
    payload->setDef(0, LDefinition(ins->id() + VREG_DATA_OFFSET, LDefinition::PAYLOAD));
    return addPhi(type) && addPhi(payload);
}

void
LIRGeneratorX86::fillSnapshot(LSnapshot *snapshot)
{
    MSnapshot *mir = snapshot->mir();
    for (size_t i = 0; i < mir->numOperands(); i++) {
        MInstruction *ins = mir->getInput(i);
        LAllocation *type = snapshot->getEntry(i * 2);
        LAllocation *payload = snapshot->getEntry(i * 2 + 1);

        
        
        
        
        
        if (ins->isConstant()) {
            *type = LConstantIndex(0);
            *payload = LConstantIndex(0);
        } else if (ins->type() != MIRType_Value) {
            *type = LConstantIndex(0);
            *payload = use(ins, LUse::KEEPALIVE);
        } else {
            *type = useType(ins, LUse::KEEPALIVE);
            *payload = usePayload(ins, LUse::KEEPALIVE);
        }
    }
}

bool
LIRGeneratorX86::lowerForALU(LMathI *ins, MInstruction *mir, MInstruction *lhs, MInstruction *rhs)
{
    ins->setOperand(0, useRegister(lhs));
    ins->setOperand(1, useOrConstant(rhs));
    return defineReuseInput(ins, mir);
}

