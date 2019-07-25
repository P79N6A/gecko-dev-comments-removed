








































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
    return defineBox(new LBox(useOrConstant(inner)), box);
}

bool
LIRGeneratorX86::visitUnbox(MUnbox *unbox)
{
    
    
    
    MInstruction *inner = unbox->getInput(0);

    if (unbox->type() == MIRType_Double) {
        LBoxToDouble *lir = new LBoxToDouble;
        if (!ensureDefined(inner))
            return false;
        lir->setOperand(0, usePayloadInRegister(inner));
        lir->setOperand(1, useType(inner));
        lir->setTemp(0, temp(LDefinition::DOUBLE));
        return define(lir, unbox, LDefinition::DEFAULT);
    }

    LUnbox *lir = new LUnbox(unbox->type());
    lir->setOperand(0, usePayloadInRegister(inner));
    lir->setOperand(1, useType(inner));
    if (!define(lir, unbox, LDefinition::CAN_REUSE_INPUT))
        return false;
    return assignSnapshot(lir);
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
    uint32 first_vreg = nextVirtualRegister();
    if (first_vreg >= MAX_VIRTUAL_REGISTERS)
        return false;

    phi->setId(first_vreg);

    if (phi->type() == MIRType_Value) {
        uint32 payload_vreg = nextVirtualRegister();
        if (payload_vreg >= MAX_VIRTUAL_REGISTERS)
            return false;
        JS_ASSERT(first_vreg + VREG_INCREMENT == payload_vreg);
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

        type->setOperand(i, LUse(opd->id(), LUse::ANY));
        payload->setOperand(i, LUse(opd->id() + VREG_INCREMENT, LUse::ANY));
    }

    type->setDef(0, LDefinition(ins->id(), LDefinition::TYPE));
    payload->setDef(0, LDefinition(ins->id() + VREG_INCREMENT, LDefinition::PAYLOAD));
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
            *payload = use(ins, LUse::ANY);
        } else {
            *type = useType(ins);
            *payload = usePayload(ins, LUse::ANY);
        }
    }
}

