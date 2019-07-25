








































#include "TypePolicy.h"
#include "MIR.h"
#include "MIRGraph.h"

using namespace js;
using namespace js::ion;

void
TypeAnalysis::preferType(MDefinition *def, MIRType type)
{
    if (def->type() != MIRType_Value)
        return;
    addPreferredType(def, type);
}

bool
BoxInputsPolicy::respecialize(MInstruction *ins)
{
    return false;
}

void
BoxInputsPolicy::specializeInputs(MInstruction *ins, TypeAnalysis *analysis)
{
}

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
BinaryArithPolicy::respecialize(MInstruction *ins)
{
    
    if (specialization_ == MIRType_None)
        return false;

    MDefinition *lhs = ins->getOperand(0);
    MDefinition *rhs = ins->getOperand(1);

    MIRType oldType = ins->type();

    
    if (CoercesToDouble(lhs->type()) || CoercesToDouble(rhs->type())) {
        if (ins->type() != MIRType_Double) {
            specialization_ = MIRType_Double;
            ins->setResultType(specialization_);
        }
    }

    return oldType != ins->type();
}

void
BinaryArithPolicy::specializeInputs(MInstruction *ins, TypeAnalysis *analysis)
{
    if (specialization_ == MIRType_None)
        return;

    analysis->preferType(ins->getOperand(0), specialization_);
    analysis->preferType(ins->getOperand(1), specialization_);
}

bool
BinaryArithPolicy::adjustInputs(MInstruction *ins)
{
    if (specialization_ == MIRType_None)
        return BoxInputsPolicy::adjustInputs(ins);

    JS_ASSERT(ins->type() == MIRType_Double || ins->type() == MIRType_Int32);

    for (size_t i = 0; i < 2; i++) {
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
ComparePolicy::respecialize(MInstruction *def)
{
    
    if (specialization_ == MIRType_None)
        return false;

    MDefinition *lhs = def->getOperand(0);
    MDefinition *rhs = def->getOperand(1);

    
    if (CoercesToDouble(lhs->type()) || CoercesToDouble(rhs->type()))
        specialization_ = MIRType_Double;

    return false;
}

void
ComparePolicy::specializeInputs(MInstruction *ins, TypeAnalysis *analyzer)
{
    if (specialization_ == MIRType_None)
        return;

    analyzer->preferType(ins->getOperand(0), specialization_);
    analyzer->preferType(ins->getOperand(1), specialization_);
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

void
BitwisePolicy::specializeInputs(MInstruction *ins, TypeAnalysis *analysis)
{
    if (specialization_ == MIRType_None)
        return;

    for (size_t i = 0; i < ins->numOperands(); i++)
        analysis->preferType(ins->getOperand(i), MIRType_Int32);
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

void
TableSwitchPolicy::specializeInputs(MInstruction *ins, TypeAnalysis *analysis)
{
    analysis->preferType(ins->getOperand(0), MIRType_Int32);
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
ObjectPolicy::specializeInputs(MInstruction *ins, TypeAnalysis *analyzer)
{
    analyzer->preferType(ins->getOperand(0), MIRType_Object);
}

bool
ObjectPolicy::adjustInputs(MInstruction *def)
{
    MDefinition *in = def->getOperand(0);
    if (in->type() == MIRType_Object || in->type() == MIRType_Slots ||
        in->type() == MIRType_Elements)
    {
        return true;
    }

    
    
    if (in->type() != MIRType_Value)
        in = boxAt(def, in);

    MUnbox *replace = MUnbox::New(in, MIRType_Object, MUnbox::Fallible);
    def->block()->insertBefore(def, replace);
    def->replaceOperand(0, replace);

    return true;
}

void
CallPolicy::specializeInputs(MInstruction *ins, TypeAnalysis *analysis)
{
    analysis->preferType(ins->getOperand(0), MIRType_Object);
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

