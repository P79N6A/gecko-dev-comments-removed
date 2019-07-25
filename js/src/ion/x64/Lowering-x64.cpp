








































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
    lir->setOperand(n, LUse(mir->id(), policy));
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

    switch (unbox->type()) {
      
      
      case MIRType_Boolean: {
        LUnboxBoolean *ins = new LUnboxBoolean(useRegister(box), temp(LDefinition::INTEGER));
        return define(ins, unbox) && assignSnapshot(ins);
      }
      case MIRType_Int32: {
        LUnboxInteger *ins = new LUnboxInteger(useRegister(box));
        return define(ins, unbox) && assignSnapshot(ins);
      }
      case MIRType_String: {
        LUnboxString *ins = new LUnboxString(useRegister(box), temp(LDefinition::INTEGER));
        return define(ins, unbox) && assignSnapshot(ins);
      }
      case MIRType_Object: {
        
        LDefinition out(LDefinition::POINTER);
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
    MDefinition *opd = ret->getOperand(0);
    JS_ASSERT(opd->type() == MIRType_Value);

    LReturn *ins = new LReturn;
    ins->setOperand(0, useFixed(opd, JSReturnReg));
    return add(ins);
}

bool
LIRGeneratorX64::assignSnapshot(LInstruction *ins)
{
    LSnapshot *snapshot = LSnapshot::New(gen, last_snapshot_);
    if (!snapshot)
        return false;

    MSnapshot *mir = snapshot->mir();
    for (size_t i = 0; i < mir->numOperands(); i++) {
        MDefinition *def = mir->getOperand(i);
        LAllocation *a = snapshot->getEntry(i);
        *a = useKeepaliveOrConstant(def);
#ifdef DEBUG
        if (a->isUse()) {
            for (size_t j = 0; j < ins->numDefs(); j++)
                JS_ASSERT(ins->getDef(j)->virtualRegister() != a->toUse()->virtualRegister());
        }
#endif
    }

    ins->assignSnapshot(snapshot);
    return true;
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

