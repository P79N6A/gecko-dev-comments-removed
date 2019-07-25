








































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
    
    if (!ins->inWorklist() && !ins->value().isDouble())
        return emitAtUses(ins);

    return LIRGenerator::visitConstant(ins);
}

bool
LIRGeneratorX86::visitBox(MBox *box)
{
    if (!box->inWorklist())
        return emitAtUses(box);

    
    
    
    
    MInstruction *inner = box->getInput(0);

    uint32 type_vreg = nextVirtualRegister();
    uint32 data_vreg = nextVirtualRegister();
    if (data_vreg >= MAX_VIRTUAL_REGISTERS)
        return false;

    box->setId(type_vreg);

    LDefinition out_type(LDefinition::TYPE, LDefinition::PRESET);
    out_type.setVirtualRegister(type_vreg);
    out_type.setOutput(LConstantIndex(inner->type()));

    LDefinition out_payload;
    if (inner->isConstant()) {
        out_payload = LDefinition(LDefinition::PAYLOAD, LDefinition::PRESET);
        out_payload.setVirtualRegister(data_vreg);
        out_payload.setOutput(LAllocation(inner->toConstant()->vp()));
    } else {
        out_payload = LDefinition(data_vreg, LDefinition::PAYLOAD);
    }

    return add(new LBox(useOrConstant(inner), out_type, out_payload));
}

bool
LIRGeneratorX86::visitUnbox(MUnbox *unbox)
{
    
    
    
    MInstruction *inner = unbox->getInput(0);

    if (unbox->type() == MIRType_Double) {
        LBoxToDouble *lir = new LBoxToDouble;
        startUsing(inner);
        lir->setOperand(0, usePayloadInRegister(inner));
        lir->setOperand(1, useType(inner));
        lir->setTemp(0, temp(LDefinition::DOUBLE));
        stopUsing(inner);
        return define(lir, unbox, LDefinition::DEFAULT);
    }

    LUnbox *lir = new LUnbox(unbox->type());
    lir->setOperand(0, usePayloadInRegister(inner));
    lir->setOperand(1, useType(inner));
    return define(lir, unbox, LDefinition::CAN_REUSE_INPUT);
}

bool
LIRGeneratorX86::visitReturn(MReturn *ret)
{
    MInstruction *opd = ret->getInput(0);
    JS_ASSERT(opd->type() == MIRType_Value);

    LReturn *ins = new LReturn;
    ins->setOperand(0, LUse(JSReturnReg_Type));
    ins->setOperand(1, LUse(JSReturnReg_Data));
    fillBoxUses(ins, 0, opd);
    return add(ins);
}

