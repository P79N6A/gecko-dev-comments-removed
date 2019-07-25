








































#include "ion/MIR.h"
#include "Lowering-x64.h"
#include "ion/IonLowering-inl.h"

using namespace js;
using namespace js::ion;

bool
LIRGeneratorX64::visitConstant(MConstant *ins)
{
    if (!ins->emitAtUses())
        return emitAtUses(ins);

    return LIRGenerator::visitConstant(ins);
}

bool
LIRGeneratorX64::visitBox(MBox *box)
{
    MInstruction *opd = box->getInput(0);

    
    if (opd->isConstant() && !box->emitAtUses())
        return emitAtUses(box);

    LBox *ins = new LBox(opd->type(), useRegisterOrConstant(opd));
    return define(ins, box, LDefinition(LDefinition::BOX));
}

bool
LIRGeneratorX64::visitUnbox(MUnbox *unbox)
{
    MInstruction *box = unbox->getInput(0);

    switch (unbox->type()) {
      
      
      case MIRType_Boolean: {
        LUnboxBoolean *ins = new LUnboxBoolean(useRegister(box), temp(LDefinition::INTEGER));
        return define(ins, unbox) && assignSnapshot(ins);
      }
      case MIRType_Int32: {
        LUnboxInteger *ins = new LUnboxInteger(useRegister(box), temp(LDefinition::INTEGER));
        return define(ins, unbox) && assignSnapshot(ins);
      }
      case MIRType_String: {
        LUnboxString *ins = new LUnboxString(useRegister(box), temp(LDefinition::INTEGER));
        return define(ins, unbox) && assignSnapshot(ins);
      }
      case MIRType_Object: {
        
        LDefinition out(LDefinition::POINTER, LDefinition::CAN_REUSE_INPUT);
        LUnboxObject *ins = new LUnboxObject(useRegister(box));
        return define(ins, unbox, out) && assignSnapshot(ins);
      }
      case MIRType_Double: {
        
        LUnboxDouble *ins = new LUnboxDouble(useRegister(box));
        return define(ins, unbox) && assignSnapshot(ins);
      }
      default:
        JS_NOT_REACHED("cannot unbox a value with no payload");
    }

    return false;
}

bool
LIRGeneratorX64::visitReturn(MReturn *ret)
{
    MInstruction *opd = ret->getInput(0);
    JS_ASSERT(opd->type() == MIRType_Value);

    LReturn *ins = new LReturn;
    ins->setOperand(0, useFixed(opd, JSReturnReg));
    return add(ins);
}

bool
LIRGeneratorX64::preparePhi(MPhi *phi)
{
    uint32 vreg = nextVirtualRegister();
    if (vreg >= MAX_VIRTUAL_REGISTERS)
        return false;

    phi->setId(vreg);
    return true;
}

bool
LIRGeneratorX64::visitPhi(MPhi *phi)
{
    return lowerPhi(phi);
}

void
LIRGeneratorX64::fillSnapshot(LSnapshot *snapshot)
{
    MSnapshot *mir = snapshot->mir();
    for (size_t i = 0; i < mir->numOperands(); i++) {
        MInstruction *ins = mir->getInput(i);
        LAllocation *a = snapshot->getEntry(i);
        *a = useOrConstant(ins);
    }
}

