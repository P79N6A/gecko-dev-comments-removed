








































#include "TypePolicy.h"
#include "MIR.h"
#include "MIRGraph.h"

using namespace js;
using namespace js::ion;

MDefinition *
BoxInputsPolicy::boxAt(MInstruction *at, MDefinition *operand)
{
    if (operand->isUnbox())
        return operand->toUnbox()->input();
    MBox *box = MBox::New(operand);
    at->block()->insertBefore(at, box);
    return box;
}

bool
BoxInputsPolicy::adjustInputs(MInstruction *ins)
{
    for (size_t i = 0; i < ins->numOperands(); i++) {
        MDefinition *in = ins->getOperand(i);
        if (in->type() == MIRType_Value)
            continue;
        ins->replaceOperand(i, boxAt(ins, in));
    }
    return true;
}

bool
ArithPolicy::adjustInputs(MInstruction *ins)
{
    if (specialization_ == MIRType_None)
        return BoxInputsPolicy::adjustInputs(ins);

    JS_ASSERT(ins->type() == MIRType_Double || ins->type() == MIRType_Int32);

    for (size_t i = 0; i < ins->numOperands(); i++) {
        MDefinition *in = ins->getOperand(i);
        if (in->type() == ins->type())
            continue;

        MInstruction *replace;

        
        
        
        
        if (in->type() == MIRType_Object || in->type() == MIRType_String)
            in = boxAt(ins, in);

        if (ins->type() == MIRType_Double)
            replace = MToDouble::New(in);
        else
            replace = MToInt32::New(in);

        ins->block()->insertBefore(ins, replace);
        ins->replaceOperand(i, replace);
    }

    return true;
}

bool
BinaryStringPolicy::adjustInputs(MInstruction *ins)
{
    for (size_t i = 0; i < 2; i++) {
        MDefinition *in = ins->getOperand(i);
        if (in->type() == MIRType_String)
            continue;

        MInstruction *replace = NULL;
        if (in->type() == MIRType_Int32) {
            replace = MToString::New(in);
        } else {
            if (in->type() != MIRType_Value)
                in = boxAt(ins, in);
            replace = MUnbox::New(in, MIRType_String, MUnbox::Fallible);
        }

        ins->block()->insertBefore(ins, replace);
        ins->replaceOperand(i, replace);
    }

    return true;
}

bool
ComparePolicy::adjustInputs(MInstruction *def)
{
    if (specialization_ == MIRType_None)
        return BoxInputsPolicy::adjustInputs(def);

    for (size_t i = 0; i < 2; i++) {
        MDefinition *in = def->getOperand(i);
        if (in->type() == specialization_)
            continue;

        MInstruction *replace;

        
        if (in->type() == MIRType_Object || in->type() == MIRType_String)
            in = boxAt(def, in);

        if (specialization_ == MIRType_Double)
            replace = MToDouble::New(in);
        else
            replace = MToInt32::New(in);

        def->block()->insertBefore(def, replace);
        def->replaceOperand(i, replace);
    }

    return true;
}

bool
TestPolicy::adjustInputs(MInstruction *ins)
{
    MDefinition *op = ins->getOperand(0);
    switch (op->type()) {
      case MIRType_Value:
      case MIRType_Null:
      case MIRType_Undefined:
      case MIRType_Boolean:
      case MIRType_Int32:
      case MIRType_Double:
      case MIRType_Object:
        break;

      case MIRType_String:
      {
        MStringLength *length = MStringLength::New(op);
        ins->block()->insertBefore(ins, length);
        ins->replaceOperand(0, length);
        break;
      }

      default:
        ins->replaceOperand(0, boxAt(ins, op));
        break;
    }
    return true;
}

bool
BitwisePolicy::adjustInputs(MInstruction *ins)
{
    if (specialization_ == MIRType_None)
        return BoxInputsPolicy::adjustInputs(ins);

    
    for (size_t i = 0; i < ins->numOperands(); i++) {
        MDefinition *in = ins->getOperand(i);
        if (in->type() == ins->type())
            continue;

        
        if (in->type() == MIRType_Object || in->type() == MIRType_String)
            in = boxAt(ins, in);

        MInstruction *replace = MTruncateToInt32::New(in);
        ins->block()->insertBefore(ins, replace);
        ins->replaceOperand(i, replace);
    }

    return true;
}

bool
TableSwitchPolicy::adjustInputs(MInstruction *ins)
{
    MDefinition *in = ins->getOperand(0);
    MInstruction *replace;

    
    
    switch (in->type()) {
      case MIRType_Value:
        replace = MUnbox::New(in, MIRType_Int32, MUnbox::Fallible);
        break;
      default:
        return true;
    }
    
    ins->block()->insertBefore(ins, replace);
    ins->replaceOperand(0, replace);

    return true;
}

void
ObjectPolicy::adjustInput(MInstruction *def, size_t operand)
{
    MDefinition *in = def->getOperand(operand);
    if (in->type() == MIRType_Object || in->type() == MIRType_Slots ||
        in->type() == MIRType_Elements || in->type() == MIRType_UpvarSlots)
    {
        return;
    }

    
    
    if (in->type() != MIRType_Value)
        in = boxAt(def, in);

    MUnbox *replace = MUnbox::New(in, MIRType_Object, MUnbox::Fallible);
    def->block()->insertBefore(def, replace);
    def->replaceOperand(operand, replace);
}

bool
ObjectPolicy::adjustInputs(MInstruction *def)
{
    for (size_t i = 0; i < def->numOperands(); i++)
        adjustInput(def, i);
    return true;
}

bool
SingleObjectPolicy::adjustInputs(MInstruction *def)
{
    adjustInput(def, 0);
    return true;
}

bool
StringPolicy::staticAdjustInputs(MInstruction *def)
{
    MDefinition *in = def->getOperand(0);
    if (in->type() == MIRType_String)
        return true;

    MUnbox *replace = MUnbox::New(in, MIRType_String, MUnbox::Fallible);
    def->block()->insertBefore(def, replace);
    def->replaceOperand(0, replace);
    return true;
}

template <unsigned Op>
bool
IntPolicy<Op>::staticAdjustInputs(MInstruction *def)
{
    MDefinition *in = def->getOperand(Op);
    if (in->type() == MIRType_Int32)
        return true;

    MUnbox *replace = MUnbox::New(in, MIRType_Int32, MUnbox::Fallible);
    def->block()->insertBefore(def, replace);
    def->replaceOperand(Op, replace);
    return true;
}

bool
CallPolicy::adjustInputs(MInstruction *ins)
{
    MCall *call = ins->toCall();

    MDefinition *func = call->getFunction();
    if (func->type() == MIRType_Object)
        return true;

    
    
    if (func->type() != MIRType_Value)
        func = boxAt(call, func);

    MInstruction *unbox = MUnbox::New(func, MIRType_Object, MUnbox::Fallible);
    call->block()->insertBefore(call, unbox);
    call->replaceFunction(unbox);

    return true;
}

bool
CallSetElementPolicy::adjustInputs(MInstruction *ins)
{
    
    SingleObjectPolicy::adjustInputs(ins);

    
    for (size_t i = 1; i < ins->numOperands(); i++) {
        MDefinition *in = ins->getOperand(i);
        if (in->type() == MIRType_Value)
            continue;
        ins->replaceOperand(i, boxAt(ins, in));
    }
    return true;
}
