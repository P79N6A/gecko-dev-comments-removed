








































#include "TypePolicy.h"
#include "MIR.h"
#include "MIRGraph.h"

using namespace js;
using namespace js::ion;

bool
BinaryArithPolicy::respecialize(MInstruction *ins)
{
    
    if (specialization_ == MIRType_None)
        return false;

    MDefinition *lhs = ins->getOperand(0);
    MDefinition *rhs = ins->getOperand(1);

    
    if (CoercesToDouble(lhs->type()) || CoercesToDouble(rhs->type())) {
        if (ins->type() != MIRType_Double) {
            ins->setResultType(MIRType_Int32);
            return true;
        }
    }

    return false;
}

bool
BoxInputPolicy::respecialize(MInstruction *ins)
{
    return false;
}

bool
BoxInputPolicy::adjustInputs(MInstruction *ins)
{
    for (size_t i = 0; i < ins->numOperands(); i++) {
        MDefinition *in = ins->getOperand(i);
        if (in->type() == MIRType_Value)
            return true;
        MBox *box = MBox::New(in);
        ins->block()->insertBefore(ins, box);
        ins->replaceOperand(i, box);
    }
    return true;
}

bool
BoxInputPolicy::useSpecializedInput(MInstruction *ins, size_t index, MInstruction *special)
{
    return false;
}

bool
BinaryArithPolicy::adjustInputs(MInstruction *ins)
{
    if (specialization_ == MIRType_None)
        return BoxInputPolicy::adjustInputs(ins);

    for (size_t i = 0; i < 2; i++) {
        MDefinition *in = ins->getOperand(0);
        if (in->type() == ins->type())
            continue;

        MInstruction *replace;
        if (in->type() == MIRType_Value && specialization_ != MIRType_Any)
            replace = MUnbox::New(in, ins->type());
        else
            replace = MConvertPrim::New(in, ins->type());

        ins->block()->insertBefore(ins, replace);
        ins->replaceOperand(i, replace);
    }

    return true;
}

bool
BinaryArithPolicy::useSpecializedInput(MInstruction *ins, size_t index, MInstruction *special)
{
    
    
    JS_ASSERT(ins->type() == special->type());
    return true;
}

