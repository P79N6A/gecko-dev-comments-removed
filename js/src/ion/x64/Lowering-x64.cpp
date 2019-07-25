








































#include "ion/MIR.h"
#include "Lowering-x64.h"
#include "Assembler-x64.h"
#include "ion/shared/Lowering-shared-inl.h"

using namespace js;
using namespace js::ion;

bool
LIRGeneratorX64::useBox(LInstruction *lir, size_t n, MDefinition *mir, LUse::Policy policy)
{
    JS_ASSERT(mir->type() == MIRType_Value);

    if (!ensureDefined(mir))
        return false;
    lir->setOperand(n, LUse(mir->virtualRegister(), policy));
    return true;
}

bool
LIRGeneratorX64::lowerConstantDouble(double d, MInstruction *mir)
{
    return define(new LDouble(d), mir);
}

bool
LIRGeneratorX64::visitConstant(MConstant *ins)
{
    if (!ins->isEmittedAtUses())
        return emitAtUses(ins);

    if (ins->type() == MIRType_Double)
        return lowerConstantDouble(ins->value().toDouble(), ins);

    return LIRGeneratorShared::visitConstant(ins);
}

bool
LIRGeneratorX64::visitBox(MBox *box)
{
    MDefinition *opd = box->getOperand(0);

    
    if (opd->isConstant() && !box->isEmittedAtUses())
        return emitAtUses(box);

    if (opd->isConstant())
        return define(new LValue(opd->toConstant()->value()), box, LDefinition(LDefinition::BOX));

    LBox *ins = new LBox(opd->type(), useRegister(opd));
    return define(ins, box, LDefinition(LDefinition::BOX));
}

bool
LIRGeneratorX64::visitUnbox(MUnbox *unbox)
{
    MDefinition *box = unbox->getOperand(0);
    LUnbox *lir = new LUnbox(useRegister(box));

    if (unbox->fallible() && !assignSnapshot(lir, unbox->bailoutKind()))
        return false;

    return define(lir, unbox);
}

bool
LIRGeneratorX64::visitReturn(MReturn *ret)
{
    MDefinition *opd = ret->getOperand(0);
    JS_ASSERT(opd->type() == MIRType_Value);

    LReturn *ins = new LReturn;
    ins->setOperand(0, useFixed(opd, JSReturnReg));
    return add(ins);
}

bool
LIRGeneratorX64::assignSnapshot(LInstruction *ins, BailoutKind kind)
{
    LSnapshot *snapshot = LSnapshot::New(gen, lastResumePoint_, kind);
    if (!snapshot)
        return false;

    for (size_t i = 0; i < lastResumePoint_->numOperands(); i++) {
        MDefinition *def = lastResumePoint_->getOperand(i);
        LAllocation *a = snapshot->getEntry(i);

        if (def->isUnused()) {
            *a = LConstantIndex::Bogus();
            continue;
        }

        *a = useKeepaliveOrConstant(def);
    }

    ins->assignSnapshot(snapshot);
    return true;
}

bool
LIRGeneratorX64::lowerForShift(LInstructionHelper<1, 2, 0> *ins, MDefinition *mir, MDefinition *lhs, MDefinition *rhs)
{
    ins->setOperand(0, useRegister(lhs));

    
    
    if (rhs->isConstant())
        ins->setOperand(1, useOrConstant(rhs));
    else
        ins->setOperand(1, useFixed(rhs, rcx));

    return defineReuseInput(ins, mir);
}

bool
LIRGeneratorX64::lowerForALU(LInstructionHelper<1, 1, 0> *ins, MDefinition *mir, MDefinition *input)
{
    ins->setOperand(0, useRegister(input));
    return defineReuseInput(ins, mir);
}

bool
LIRGeneratorX64::lowerForALU(LInstructionHelper<1, 2, 0> *ins, MDefinition *mir, MDefinition *lhs, MDefinition *rhs)
{
    ins->setOperand(0, useRegister(lhs));
    ins->setOperand(1, useOrConstant(rhs));
    return defineReuseInput(ins, mir);
}

bool
LIRGeneratorX64::lowerForFPU(LMathD *ins, MDefinition *mir, MDefinition *lhs, MDefinition *rhs)
{
    ins->setOperand(0, useRegister(lhs));
    ins->setOperand(1, useRegister(rhs));
    return defineReuseInput(ins, mir);
}

bool
LIRGeneratorX64::defineUntypedPhi(MPhi *phi, size_t lirIndex)
{
    return defineTypedPhi(phi, lirIndex);
}

void
LIRGeneratorX64::lowerUntypedPhiInput(MPhi *phi, uint32 inputPosition, LBlock *block, size_t lirIndex)
{
    lowerTypedPhiInput(phi, inputPosition, block, lirIndex);
}

bool
LIRGeneratorX64::lowerDivI(MDiv *div)
{
    LDivI *lir = new LDivI(useFixed(div->lhs(), rax), useRegister(div->rhs()), tempFixed(rdx));
    return defineReuseInput(lir, div) && assignSnapshot(lir);
}
