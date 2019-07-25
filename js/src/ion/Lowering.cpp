








































#include "IonLIR.h"
#include "Lowering.h"
#include "MIR.h"
#include "MIRGraph.h"
#include "IonSpewer.h"
#include "jsbool.h"
#include "jsnum.h"
#include "shared/Lowering-shared-inl.h"

using namespace js;
using namespace ion;

bool
LIRGenerator::visitParameter(MParameter *param)
{
    ptrdiff_t offset;
    if (param->index() == -1)
        offset = THIS_FRAME_SLOT;
    else
        offset = 1 + param->index();

    LParameter *ins = new LParameter;
    if (!defineBox(ins, param, LDefinition::PRESET))
        return false;

    offset *= sizeof(Value);
#if defined(JS_NUNBOX32)
# if defined(IS_BIG_ENDIAN)
    ins->getDef(0)->setOutput(LArgument(offset));
    ins->getDef(1)->setOutput(LArgument(offset + 4));
# else
    ins->getDef(0)->setOutput(LArgument(offset + 4));
    ins->getDef(1)->setOutput(LArgument(offset));
# endif
#elif defined(JS_PUNBOX64)
    ins->getDef(0)->setOutput(LArgument(offset));
#endif

    return true;
}

bool
LIRGenerator::visitTableSwitch(MTableSwitch *tableswitch)
{
    MDefinition *opd = tableswitch->getOperand(0);

    
    JS_ASSERT(tableswitch->numSuccessors() > 0);

    
    if (tableswitch->numSuccessors() == 1)
        return add(new LGoto(tableswitch->getDefault()));        

    
    if (opd->type() != MIRType_Int32)
        return add(new LGoto(tableswitch->getDefault()));

    
    if (opd->isConstant()) {
        MConstant *ins = opd->toConstant();

        int32 switchval = ins->value().toInt32();
        if (switchval < tableswitch->low() || switchval > tableswitch->high())
            return add(new LGoto(tableswitch->getDefault()));

        return add(new LGoto(tableswitch->getCase(switchval-tableswitch->low())));
    }

    
    return add(new LTableSwitch(useRegister(opd), temp(LDefinition::INTEGER),
                                temp(LDefinition::POINTER), tableswitch));
}

bool
LIRGenerator::visitGoto(MGoto *ins)
{
    return add(new LGoto(ins->target()));
}

bool
LIRGenerator::visitTest(MTest *test)
{
    MDefinition *opd = test->getOperand(0);
    MBasicBlock *ifTrue = test->ifTrue();
    MBasicBlock *ifFalse = test->ifFalse();

    if (opd->isConstant()) {
        MConstant *ins = opd->toConstant();
        JSBool truthy = js_ValueToBoolean(ins->value());
        MBasicBlock *target = truthy ? ifTrue : ifFalse;
        return add(new LGoto(target));
    }

    if (opd->type() == MIRType_Value) {
        LTestVAndBranch *lir = new LTestVAndBranch(ifTrue, ifFalse);
        if (!useBox(lir, 0, opd))
            return false;
        return add(lir);
    }

    
    
    if (opd->type() == MIRType_Undefined || opd->type() == MIRType_Null)
        return add(new LGoto(ifFalse));

    if (opd->type() == MIRType_Double)
        return add(new LTestDAndBranch(useRegister(opd), temp(LDefinition::DOUBLE), ifTrue, ifFalse));

    return add(new LTestIAndBranch(useRegister(opd), ifTrue, ifFalse));
}

static void
ReorderCommutative(MDefinition **lhsp, MDefinition **rhsp)
{
    MDefinition *lhs = *lhsp;
    MDefinition *rhs = *rhsp;

    
    if (lhs->isConstant()) {
        *rhsp = lhs;
        *lhsp = rhs;
    }
}

bool
LIRGenerator::lowerBitOp(JSOp op, MInstruction *ins)
{
    MDefinition *lhs = ins->getOperand(0);
    MDefinition *rhs = ins->getOperand(1);

    if (lhs->type() == MIRType_Int32 && rhs->type() == MIRType_Int32) {
        ReorderCommutative(&lhs, &rhs);
        return lowerForALU(new LBitOp(op), ins, lhs, rhs);
    }

    JS_NOT_REACHED("NYI");
    return false;
}

bool
LIRGenerator::visitBitNot(MBitNot *ins)
{
    MDefinition *input = ins->getOperand(0);

    JS_ASSERT(input->type() == MIRType_Int32);

    return lowerForALU(new LBitNot(), ins, input);
}

bool
LIRGenerator::visitBitAnd(MBitAnd *ins)
{
    return lowerBitOp(JSOP_BITAND, ins);
}

bool
LIRGenerator::visitBitOr(MBitOr *ins)
{
    return lowerBitOp(JSOP_BITOR, ins);
}

bool
LIRGenerator::visitBitXor(MBitXor *ins)
{
    return lowerBitOp(JSOP_BITXOR, ins);
}

bool
LIRGenerator::visitAdd(MAdd *ins)
{
    MDefinition *lhs = ins->getOperand(0);
    MDefinition *rhs = ins->getOperand(1);

    JS_ASSERT(lhs->type() == rhs->type());

    if (ins->specialization() == MIRType_Int32) {
        JS_ASSERT(lhs->type() == MIRType_Int32);
        ReorderCommutative(&lhs, &rhs);
        LAddI *lir = new LAddI;
        if (!assignSnapshot(lir))
            return false;
        return lowerForALU(lir, ins, lhs, rhs);
    }
    if (ins->specialization() == MIRType_Double) {
        JS_ASSERT(lhs->type() == MIRType_Double);
        return lowerForFPU(new LMathD(JSOP_ADD), ins, lhs, rhs);
    }

    JS_NOT_REACHED("NYI");
    return false;
}

bool
LIRGenerator::visitStart(MStart *start)
{
    
    return true;
}

bool
LIRGenerator::visitToDouble(MToDouble *convert)
{
    JS_NOT_REACHED("NYI");
    return false;
}

bool
LIRGenerator::visitToInt32(MToInt32 *convert)
{
    MDefinition *opd = convert->input();

    switch (opd->type()) {
      case MIRType_Value:
      {
        LValueToInt32 *lir = new LValueToInt32(tempFloat());
        if (!useBox(lir, LValueToInt32::Input, opd))
            return false;
        return define(lir, convert) && assignSnapshot(lir);
      }

      case MIRType_Null:
        return define(new LInteger(0), convert);

      case MIRType_Int32:
      case MIRType_Boolean:
        return redefine(convert, opd);

      default:
        
        
        
        
        JS_NOT_REACHED("unexpected type");
    }

    return false;
}

bool
LIRGenerator::visitTruncateToInt32(MTruncateToInt32 *truncate)
{
    JS_NOT_REACHED("NYI");
    return false;
}

bool
LIRGenerator::visitCopy(MCopy *ins)
{
    JS_NOT_REACHED("unexpected copy");
    return false;
}

static void
SpewSnapshot(MInstruction *ins, MSnapshot *snapshot)
{
    fprintf(IonSpewFile, "Current snapshot %p details:\n", (void *)snapshot);

    fprintf(IonSpewFile, "    taken after:");
    ins->printName(IonSpewFile);
    fprintf(IonSpewFile, "\n");
    fprintf(IonSpewFile, "    pc: %p\n", snapshot->pc());

    for (size_t i = 0; i < snapshot->numOperands(); i++) {
        MDefinition *in = snapshot->getOperand(i);
        fprintf(IonSpewFile, "    slot%d: ", i);
        in->printName(IonSpewFile);
        fprintf(IonSpewFile, "\n");
    }
}

bool
LIRGenerator::visitInstruction(MInstruction *ins)
{
    if (!gen->ensureBallast())
        return false;
    if (!ins->accept(this))
        return false;

    if (ins->snapshot()) {
        last_snapshot_ = ins->snapshot();

        if (IonSpewEnabled(IonSpew_Snapshots))
            SpewSnapshot(ins, last_snapshot_);
    }

    if (gen->errored())
        return false;
#ifdef DEBUG
    ins->setInWorklistUnchecked();
#endif
    return true;
}

bool
LIRGenerator::definePhis()
{
    size_t lirIndex = 0;
    MBasicBlock *block = current->mir();
    for (MPhiIterator phi(block->phisBegin()); phi != block->phisEnd(); phi++) {
        if (phi->type() == MIRType_Value) {
            if (!defineUntypedPhi(*phi, lirIndex))
                return false;
            lirIndex += BOX_PIECES;
        } else {
            if (!defineTypedPhi(*phi, lirIndex))
                return false;
            lirIndex += 1;
        }
    }
    return true;
}

bool
LIRGenerator::visitBlock(MBasicBlock *block)
{
    current = block->lir();
    last_snapshot_ = block->entrySnapshot();

    if (!definePhis())
        return false;

    for (MInstructionIterator iter = block->begin(); *iter != block->lastIns(); iter++) {
        if (!visitInstruction(*iter))
            return false;
    }

    if (block->successorWithPhis()) {
        
        
        MBasicBlock *successor = block->successorWithPhis();
        uint32 position = block->positionInPhiSuccessor();
        size_t lirIndex = 0;
        for (MPhiIterator phi(successor->phisBegin()); phi != successor->phisEnd(); phi++) {
            MDefinition *opd = phi->getOperand(position);
            if (!ensureDefined(opd))
                return false;

            JS_ASSERT(opd->type() == phi->type());

            if (phi->type() == MIRType_Value) {
                lowerUntypedPhiInput(*phi, position, successor->lir(), lirIndex);
                lirIndex += BOX_PIECES;
            } else {
                lowerTypedPhiInput(*phi, position, successor->lir(), lirIndex);
                lirIndex += 1;
            }
        }
    }

    
    if (!visitInstruction(block->lastIns()))
        return false;

    return true;
}

bool
LIRGenerator::precreatePhi(LBlock *block, MPhi *phi)
{
    LPhi *lir = LPhi::New(gen, phi);
    if (!lir)
        return false;
    if (!block->addPhi(lir))
        return false;
    return true;
}

bool
LIRGenerator::generate()
{
    
    for (ReversePostorderIterator block(graph.rpoBegin()); block != graph.rpoEnd(); block++) {
        current = LBlock::New(*block);
        if (!current)
            return false;
        if (!lirGraph_.addBlock(current))
            return false;
        block->assignLir(current);

        
        
        
        for (MPhiIterator phi(block->phisBegin()); phi != block->phisEnd(); phi++) {
            int numPhis = (phi->type() == MIRType_Value) ? BOX_PIECES : 1;
            for (int i = 0; i < numPhis; i++) {
                if (!precreatePhi(block->lir(), *phi))
                    return false;
            }
        }
    }

    for (ReversePostorderIterator block(graph.rpoBegin()); block != graph.rpoEnd(); block++) {
        if (!visitBlock(*block))
            return false;
    }

    return true;
}

