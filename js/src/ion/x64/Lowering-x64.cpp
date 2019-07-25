








































#include "ion/MIR.h"
#include "Lowering-x64.h"
#include "ion/IonLowering-inl.h"

using namespace js;
using namespace js::ion;

bool
LIRGeneratorX64::visitConstant(MConstant *ins)
{
    if (!ins->inWorklist())
        return emitAtUses(ins);

    return LIRGenerator::visitConstant(ins);
}

bool
LIRGeneratorX64::visitBox(MBox *box)
{
    MInstruction *opd = box->getInput(0);

    
    if (opd->isConstant() && !box->inWorklist())
        return emitAtUses(box);

    LBox *ins = new LBox(opd->type(), useRegisterOrConstant(opd));
    return define(ins, box, LDefinition(LDefinition::BOX));
}

bool
LIRGeneratorX64::visitUnbox(MUnbox *unbox)
{
    MInstruction *box = unbox->getInput(0);

    switch (box->type()) {
      
      
      case MIRType_Boolean:
        return define(new LUnboxBoolean(useRegister(box), temp(LDefinition::INTEGER)), unbox);
      case MIRType_Int32:
        return define(new LUnboxInteger(useRegister(box), temp(LDefinition::INTEGER)), unbox);
      case MIRType_String:
        return define(new LUnboxString(useRegister(box), temp(LDefinition::INTEGER)), unbox);
      case MIRType_Object:
      {
        
        LDefinition out(LDefinition::POINTER, LDefinition::CAN_REUSE_INPUT);
        return define(new LUnboxObject(useRegister(box)), unbox, out);
      }
      case MIRType_Double:
      {
        
        return define(new LUnboxDouble(useRegister(box)), unbox);
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

