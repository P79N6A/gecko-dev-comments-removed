








































#include "IonLIR.h"
#include "IonLowering.h"
#include "IonLowering-inl.h"
#include "MIR.h"
#include "MIRGraph.h"
#include "jsbool.h"

using namespace js;
using namespace ion;

bool
LIRGenerator::emitAtUses(MInstruction *mir)
{
    mir->setEmittedAtUses();
    mir->setId(0);
    return true;
}

LUse
LIRGenerator::use(MDefinition *mir, LUse policy)
{
    
#if BOX_PIECES > 1
    JS_ASSERT(mir->type() != MIRType_Value);
#endif
    if (!ensureDefined(mir))
        return policy;
    policy.setVirtualRegister(mir->id());
    return policy;
}

bool
LIRGenerator::assignSnapshot(LInstruction *ins)
{
    LSnapshot *snapshot = LSnapshot::New(gen, last_snapshot_);
    if (!snapshot)
        return false;
    fillSnapshot(snapshot);
    ins->assignSnapshot(snapshot);
    return true;
}

bool
LIRGenerator::visitConstant(MConstant *ins)
{
    const Value &v = ins->value();
    switch (ins->type()) {
      case MIRType_Boolean:
        return define(new LInteger(v.toBoolean()), ins);
      case MIRType_Int32:
        return define(new LInteger(v.toInt32()), ins);
      case MIRType_Double:
        return define(new LDouble(v.toDouble()), ins);
      case MIRType_String:
        return define(new LPointer(v.toString()), ins);
      case MIRType_Object:
        return define(new LPointer(&v.toObject()), ins);
      default:
        
        
        JS_NOT_REACHED("unexpected constant type");
        return false;
    }
    return true;
}

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
        if (!fillBoxUses(lir, 0, opd))
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
LIRGenerator::doBitOp(JSOp op, MInstruction *ins)
{
    MDefinition *lhs = ins->getOperand(0);
    MDefinition *rhs = ins->getOperand(1);

    if (lhs->type() == MIRType_Int32 && rhs->type() == MIRType_Int32) {
        ReorderCommutative(&lhs, &rhs);
        LBitOp *bitop = new LBitOp(op, useRegister(lhs), useOrConstant(rhs));
        return defineReuseInput(bitop, ins);
    }

    JS_NOT_REACHED("NYI");
    return false;
}

bool
LIRGenerator::visitBitAnd(MBitAnd *ins)
{
    return doBitOp(JSOP_BITAND, ins);
}

bool
LIRGenerator::visitBitOr(MBitOr *ins)
{
    return doBitOp(JSOP_BITOR, ins);
}

bool
LIRGenerator::visitBitXor(MBitXor *ins)
{
    return doBitOp(JSOP_BITXOR, ins);
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
        return lowerForALU(new LAddI, ins, lhs, rhs);
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
    JS_NOT_REACHED("NYI");
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

bool
LIRGenerator::visitPhi(MPhi *phi)
{
    JS_NOT_REACHED("should not call accept() on phis");
    return false;
}

bool
LIRGenerator::lowerPhi(MPhi *ins)
{
    
    JS_ASSERT(ins->id());

    LPhi *phi = LPhi::New(gen, ins);
    if (!phi)
        return false;
    for (size_t i = 0; i < ins->numOperands(); i++) {
        MDefinition *opd = ins->getOperand(i);
        JS_ASSERT(opd->type() == ins->type());

        phi->setOperand(i, LUse(opd->id(), LUse::ANY));
    }
    phi->setDef(0, LDefinition(ins->id(), LDefinition::TypeFrom(ins->type())));
    return addPhi(phi);
}

bool
LIRGenerator::visitBox(MBox *ins)
{
    JS_NOT_REACHED("Must be implemented by arch");
    return false;
}

bool
LIRGenerator::visitUnbox(MUnbox *ins)
{
    JS_NOT_REACHED("Must be implemented by arch");
    return false;
}

bool
LIRGenerator::visitReturn(MReturn *ins)
{
    JS_NOT_REACHED("Must be implemented by arch");
    return false;
}

bool
LIRGenerator::visitInstruction(MInstruction *ins)
{
    if (!gen->ensureBallast())
        return false;
    if (!ins->accept(this))
        return false;
    if (ins->snapshot())
        last_snapshot_ = ins->snapshot();
    if (gen->errored())
        return false;
#ifdef DEBUG
    ins->setInWorklistUnchecked();
#endif
    return true;
}

bool
LIRGenerator::visitBlock(MBasicBlock *block)
{
    current = LBlock::New(block);
    if (!current)
        return false;

    last_snapshot_ = block->entrySnapshot();

    for (MPhiIterator phi(block->phisBegin()); phi != block->phisEnd(); phi++) {
        if (!gen->ensureBallast())
            return false;
        if (!preparePhi(*phi))
            return false;
#ifdef DEBUG
        phi->setInWorklist();
#endif
    }

    for (MInstructionIterator iter = block->begin(); *iter != block->lastIns(); iter++) {
        if (!visitInstruction(*iter))
            return false;
    }

    
    
    if (block->successorWithPhis()) {
        MBasicBlock *successor = block->successorWithPhis();
        uint32 position = block->positionInPhiSuccessor();
        for (MPhiIterator phi(successor->phisBegin()); phi != successor->phisEnd(); phi++) {
            MDefinition *opd = phi->getOperand(position);
            if (opd->isEmittedAtUses() && !opd->id()) {
                if (!ensureDefined(opd))
                    return false;
            }
        }
    }

    
    if (!visitInstruction(block->lastIns()))
        return false;

    if (!lirGraph_.addBlock(current))
        return false;
    block->assignLir(current);
    return true;
}

bool
LIRGenerator::generate()
{
    for (ReversePostorderIterator block(graph.rpoBegin()); block != graph.rpoEnd(); block++) {
        if (!visitBlock(*block))
            return false;
    }

    
    for (ReversePostorderIterator block(graph.rpoBegin()); block != graph.rpoEnd(); block++) {
        current = block->lir();
        for (MPhiIterator phi(block->phisBegin()); phi != block->phisEnd(); phi++) {
            if (!lowerPhi(*phi))
                return false;
        }
    }

    return true;
}

