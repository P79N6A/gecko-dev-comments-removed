








































#include "IonLIR.h"
#include "IonLowering.h"
#include "IonLowering-inl.h"
#include "MIR.h"
#include "MIRGraph.h"

using namespace js;
using namespace ion;

bool
LIRGenerator::emitAtUses(MInstruction *mir)
{
    mir->setEmitAtUses();
    mir->setId(0);
    return true;
}

LUse
LIRGenerator::use(MInstruction *mir, LUse policy)
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
        
        
        JS_NOT_REACHED("unexepcted constant type");
        return false;
    }
    return true;
}

bool
LIRGenerator::visitParameter(MParameter *param)
{
    ptrdiff_t offset;
    if (param->index() == -2)
        offset = StackFrame::offsetOfCallee(gen->fun());
    else if (param->index() == -1)
        offset = StackFrame::offsetOfThis(gen->fun());
    else
        offset = StackFrame::offsetOfFormalArg(gen->fun(), param->index());

    JS_ASSERT(offset % sizeof(Value) == 0);
    JS_ASSERT(offset % STACK_SLOT_SIZE == 0);

    LParameter *ins = new LParameter;
    if (!defineBox(ins, param, LDefinition::PRESET))
        return false;

    offset /= STACK_SLOT_SIZE;
#if defined(JS_NUNBOX32)
# if defined(IS_BIG_ENDIAN)
    ins->getDef(0)->setOutput(LArgument(offset));
    ins->getDef(1)->setOutput(LArgument(offset + 1));
# else
    ins->getDef(0)->setOutput(LArgument(offset + 1));
    ins->getDef(1)->setOutput(LArgument(offset));
# endif
#elif defined(JS_PUNBOX64)
    ins->getDef(0)->setOutput(LArgument(offset));
#endif

    return true;
}

bool
LIRGenerator::visitGoto(MGoto *ins)
{
    return add(new LGoto(ins->target()));
}

bool
LIRGenerator::visitTest(MTest *test)
{
    MInstruction *opd = test->getInput(0);
    MBasicBlock *ifTrue = test->ifTrue();
    MBasicBlock *ifFalse = test->ifFalse();

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
ReorderCommutative(MInstruction **lhsp, MInstruction **rhsp)
{
    MInstruction *lhs = *lhsp;
    MInstruction *rhs = *rhsp;

    
    if (lhs->isConstant()) {
        *rhsp = lhs;
        *lhsp = rhs;
    }
}

bool
LIRGenerator::doBitOp(JSOp op, MInstruction *ins)
{
    MInstruction *lhs = ins->getInput(0);
    MInstruction *rhs = ins->getInput(1);

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
    return doBitOp(JSOP_AND, ins);
}

bool
LIRGenerator::visitAdd(MAdd *ins)
{
    
    return true;
}

bool
LIRGenerator::visitStart(MStart *start)
{
    
    return true;
}

bool
LIRGenerator::visitCopy(MCopy *ins)
{
    JS_NOT_REACHED("unexpected copy");
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
        MInstruction *opd = ins->getInput(i);
        phi->setOperand(i, LUse(opd->id(), LUse::ANY));
    }
    phi->setDef(0, LDefinition(ins->id(), LDefinition::TypeFrom(ins->type())));
    return addPhi(phi);
}

bool
LIRGenerator::visitPhi(MPhi *ins)
{
    JS_NOT_REACHED("Not used.");
    return true;
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
LIRGenerator::visitSnapshot(MSnapshot *snapshot)
{
    JS_ASSERT(!snapshot->inWorklist());
    last_snapshot_ = snapshot;
    last_snapshot_->setInWorklist();
    return true;
}

void
LIRGenerator::rewriteDefsInSnapshots(MInstruction *ins, MInstruction *old)
{
    MUseIterator iter(old);
    while (iter.more()) {
        MInstruction *use = iter->ins();
        if (!ins->isSnapshot() || ins->inWorklist()) {
            iter.next();
            continue;
        }
        use->replaceOperand(iter, ins);
    }
}

bool
LIRGenerator::visitBlock(MBasicBlock *block)
{
    current = LBlock::New(block);
    if (!current)
        return false;

    last_snapshot_ = block->entrySnapshot();

    for (size_t i = 0; i < block->numPhis(); i++) {
        if (!gen->ensureBallast())
            return false;
        if (!preparePhi(block->getPhi(i)))
            return false;
#ifdef DEBUG
        block->getPhi(i)->setInWorklist();
#endif
    }

    for (MInstructionIterator iter = block->begin(); iter != block->end(); iter++) {
        if (!gen->ensureBallast())
            return false;
        if (iter->rewritesDef())
            rewriteDefsInSnapshots(*iter, iter->rewrittenDef());
        if (!iter->accept(this))
            return false;
        if (gen->errored())
            return false;
#ifdef DEBUG
        iter->setInWorklistUnchecked();
#endif
    }

    
    
    if (block->successorWithPhis()) {
        MBasicBlock *successor = block->successorWithPhis();
        uint32 position = block->positionInPhiSuccessor();
        for (size_t i = 0; i < successor->numPhis(); i++) {
            MPhi *phi = successor->getPhi(i);
            MInstruction *opd = phi->getInput(position);
            if (opd->emitAtUses() && !opd->id()) {
                if (!ensureDefined(opd))
                    return false;
            }
        }
    }

    block->assignLir(current);
    return true;
}

bool
LIRGenerator::generate()
{
    for (size_t i = 0; i < graph.numBlocks(); i++) {
        if (!visitBlock(graph.getBlock(i)))
            return false;
    }

    
    for (size_t i = 0; i < graph.numBlocks(); i++) {
        MBasicBlock *block = graph.getBlock(i);
        current = block->lir();
        for (size_t j = 0; j < block->numPhis(); j++) {
            if (!block->getPhi(j)->accept(this))
                return false;
        }
    }

    return true;
}

