








































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

bool
BoxInputsPolicy::adjustInputs(MInstruction *ins)
{
    for (size_t i = 0; i < ins->numOperands(); i++) {
        MDefinition *in = ins->getOperand(i);
        if (in->type() == MIRType_Value)
            continue;
        MBox *box = MBox::New(in);
        ins->block()->insertBefore(ins, box);
        ins->replaceOperand(i, box);
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

    analysis->preferType(ins->getOperand(0), ins->type());
    analysis->preferType(ins->getOperand(1), ins->type());
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
        if (in->type() == MIRType_Value && specialization_ != MIRType_Any)
            replace = MUnbox::New(in, ins->type());
        else if (ins->type() == MIRType_Double)
            replace = MToDouble::New(in);
        else
            replace = MToInt32::New(in);

        ins->block()->insertBefore(ins, replace);
        ins->replaceOperand(i, replace);
    }

    return true;
}

bool
BitwisePolicy::respecialize(MInstruction *ins)
{
    return false;
}

void
BitwisePolicy::specializeInputs(MInstruction *ins, TypeAnalysis *analysis)
{
    if (specialization_ == MIRType_None)
        return;

    analysis->preferType(ins->getOperand(0), MIRType_Int32);
    analysis->preferType(ins->getOperand(1), MIRType_Int32);
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

        MInstruction *replace;
        if (in->type() == MIRType_Value && specialization_ != MIRType_Any)
            replace = MUnbox::New(in, MIRType_Int32);
        else
            replace = MTruncateToInt32::New(in);

        ins->block()->insertBefore(ins, replace);
        ins->replaceOperand(i, replace);
    }

    return true;
}

