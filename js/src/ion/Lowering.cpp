








































#include "LIR.h"
#include "Lowering.h"
#include "MIR.h"
#include "MIRGraph.h"
#include "IonSpewer.h"
#include "jsanalyze.h"
#include "jsbool.h"
#include "jsnum.h"
#include "jsobjinlines.h"
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
LIRGenerator::visitGoto(MGoto *ins)
{
    return add(new LGoto(ins->target()));
}

bool
LIRGenerator::visitCheckOverRecursed(MCheckOverRecursed *ins)
{
    LCheckOverRecursed *lir = new LCheckOverRecursed(temp(LDefinition::GENERAL));

    if (!assignSafepoint(lir, ins))
        return false;
    return add(lir);
}

bool
LIRGenerator::visitNewArray(MNewArray *ins)
{
    LNewArray *lir = new LNewArray();

    if (!defineVMReturn(lir, ins))
        return false;
    if (!assignSafepoint(lir, ins))
        return false;

    return true;
}

bool
LIRGenerator::visitPrepareCall(MPrepareCall *ins)
{
    allocateArguments(ins->argc());
    return true;
}

bool
LIRGenerator::visitPassArg(MPassArg *arg)
{
    MDefinition *opd = arg->getArgument();
    JS_ASSERT(opd->type() == MIRType_Value);

    uint32 argslot = getArgumentSlot(arg->getArgnum());

    LStackArg *stack = new LStackArg(argslot);
    if (!useBox(stack, 0, opd))
        return false;

    
    
    
    
    
    arg->setVirtualRegister(opd->virtualRegister());

    return add(stack);
}

bool
LIRGenerator::visitCall(MCall *call)
{
    uint32 argc = call->argc();
    JS_ASSERT(call->getFunction()->type() == MIRType_Object);

    
    uint32 argslot = getArgumentSlotForCall();

    
    
    LCallGeneric *ins = new LCallGeneric(useRegister(call->getFunction()),
                                         argslot,
                                         temp(LDefinition::GENERAL),
                                         temp(LDefinition::GENERAL));
    if (!defineReturn(ins, call))
        return false;
    if (!assignSnapshot(ins))
        return false;
    if (!assignSafepoint(ins, call))
        return false;

    freeArguments(argc);
    return true;
}

static JSOp
ReorderComparison(JSOp op, MDefinition **lhsp, MDefinition **rhsp)
{
    MDefinition *lhs = *lhsp;
    MDefinition *rhs = *rhsp;

    if (lhs->isConstant()) {
        *rhsp = lhs;
        *lhsp = rhs;
        return js::analyze::ReverseCompareOp(op);
    }
    return op;
}

bool
LIRGenerator::visitTest(MTest *test)
{
    MDefinition *opd = test->getOperand(0);
    MBasicBlock *ifTrue = test->ifTrue();
    MBasicBlock *ifFalse = test->ifFalse();

    if (opd->type() == MIRType_Value) {
        LTestVAndBranch *lir = new LTestVAndBranch(ifTrue, ifFalse, tempFloat());
        if (!useBox(lir, LTestVAndBranch::Input, opd))
            return false;
        return add(lir);
    }

    
    
    if (opd->type() == MIRType_Undefined || opd->type() == MIRType_Null)
        return add(new LGoto(ifFalse));

    
    
    
    if (opd->isCompare()) {
        MCompare *comp = opd->toCompare();
        MDefinition *left = comp->getOperand(0);
        MDefinition *right = comp->getOperand(1);

        if (comp->specialization() == MIRType_Int32) {
            JSOp op = ReorderComparison(comp->jsop(), &left, &right);
            return add(new LCompareIAndBranch(op, useRegister(left), useOrConstant(right),
                                              ifTrue, ifFalse));
        }
        if (comp->specialization() == MIRType_Double) {
            return add(new LCompareDAndBranch(comp->jsop(), useRegister(left),
                                              useRegister(right), ifTrue, ifFalse));
        }
        
    }

    if (opd->type() == MIRType_Double)
        return add(new LTestDAndBranch(useRegister(opd), ifTrue, ifFalse));

    return add(new LTestIAndBranch(useRegister(opd), ifTrue, ifFalse));
}

bool
LIRGenerator::visitCompare(MCompare *comp)
{
    MDefinition *left = comp->getOperand(0);
    MDefinition *right = comp->getOperand(1);

    if (comp->specialization() != MIRType_None) {
        
        
        
        
        bool willOptimize = true;
        for (MUseIterator iter(comp->usesBegin()); iter!= comp->usesEnd(); iter++) {
            MNode *node = iter->node();
            if (node->isResumePoint() ||
                (node->isDefinition() && !node->toDefinition()->isControlInstruction()))
            {
                willOptimize = false;
                break;
            }
        }

        if (willOptimize && comp->canEmitAtUses())
            return emitAtUses(comp);

        if (comp->specialization() == MIRType_Int32) {
            JSOp op = ReorderComparison(comp->jsop(), &left, &right);
            return define(new LCompareI(op, useRegister(left), useOrConstant(right)), comp);
        }

        if (comp->specialization() == MIRType_Double)
            return define(new LCompareD(comp->jsop(), useRegister(left), useRegister(right)), comp);
    }

    
    JS_NOT_REACHED("LCompareV NYI");
    return true;
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
LIRGenerator::lowerShiftOp(JSOp op, MInstruction *ins)
{
    MDefinition *lhs = ins->getOperand(0);
    MDefinition *rhs = ins->getOperand(1);

    if (lhs->type() == MIRType_Int32 && rhs->type() == MIRType_Int32) {
        LShiftOp *lir = new LShiftOp(op);
        if (op == JSOP_URSH) {
            MUrsh *ursh = ins->toUrsh();
            if (ursh->fallible() && !assignSnapshot(lir))
                return false;
        }
        return lowerForShift(lir, ins, lhs, rhs);
    }
    JS_NOT_REACHED("NYI");
    return false;
}

bool
LIRGenerator::visitLsh(MLsh *ins)
{
    return lowerShiftOp(JSOP_LSH, ins);
}

bool
LIRGenerator::visitRsh(MRsh *ins)
{
    return lowerShiftOp(JSOP_RSH, ins);
}

bool
LIRGenerator::visitUrsh(MUrsh *ins)
{
    return lowerShiftOp(JSOP_URSH, ins);
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
LIRGenerator::visitSub(MSub *ins)
{
    MDefinition *lhs = ins->lhs();
    MDefinition *rhs = ins->rhs();

    JS_ASSERT(lhs->type() == rhs->type());

    if (ins->specialization() == MIRType_Int32) {
        JS_ASSERT(lhs->type() == MIRType_Int32);
        LSubI *lir = new LSubI;
        if (!assignSnapshot(lir))
            return false;
        return lowerForALU(lir, ins, lhs, rhs);
    }
    if (ins->specialization() == MIRType_Double) {
        JS_ASSERT(lhs->type() == MIRType_Double);
        return lowerForFPU(new LMathD(JSOP_SUB), ins, lhs, rhs);
    }

    JS_NOT_REACHED("NYI");
    return false;
}

bool
LIRGenerator::visitMul(MMul *ins)
{
    MDefinition *lhs = ins->lhs();
    MDefinition *rhs = ins->rhs();
    JS_ASSERT(lhs->type() == rhs->type());

    if (ins->specialization() == MIRType_Int32) {
        JS_ASSERT(lhs->type() == MIRType_Int32);
        ReorderCommutative(&lhs, &rhs);
        LMulI *lir = new LMulI;
        if (ins->fallible() && !assignSnapshot(lir))
            return false;
        return lowerForALU(lir, ins, lhs, rhs);
    }
    if (ins->specialization() == MIRType_Double) {
        JS_ASSERT(lhs->type() == MIRType_Double);
        return lowerForFPU(new LMathD(JSOP_MUL), ins, lhs, rhs);
    }

    JS_NOT_REACHED("NYI");
    return false;
}

bool
LIRGenerator::visitDiv(MDiv *ins)
{
    MDefinition *lhs = ins->lhs();
    MDefinition *rhs = ins->rhs();
    JS_ASSERT(lhs->type() == rhs->type());

    if (ins->specialization() == MIRType_Int32) {
        JS_ASSERT(lhs->type() == MIRType_Int32);
        return lowerDivI(ins);
    }
    if (ins->specialization() == MIRType_Double) {
        JS_ASSERT(lhs->type() == MIRType_Double);
        return lowerForFPU(new LMathD(JSOP_DIV), ins, lhs, rhs);
    }

    JS_NOT_REACHED("NYI");
    return false;
}

bool
LIRGenerator::visitStart(MStart *start)
{
    
    LStart *lir = new LStart;
    if (!assignSnapshot(lir))
        return false;

    if (start->startType() == MStart::StartType_Default)
        lirGraph_.setEntrySnapshot(lir->snapshot());
    return add(lir);
}

bool
LIRGenerator::visitOsrEntry(MOsrEntry *entry)
{
    LOsrEntry *lir = new LOsrEntry;
    return defineFixed(lir, entry, LAllocation(AnyRegister(OsrFrameReg)));
}

bool
LIRGenerator::visitOsrValue(MOsrValue *value)
{
    LOsrValue *lir = new LOsrValue(useRegister(value->entry()));
    return defineBox(lir, value);
}

bool
LIRGenerator::visitToDouble(MToDouble *convert)
{
    MDefinition *opd = convert->input();

    switch (opd->type()) {
      case MIRType_Value:
      {
        LValueToDouble *lir = new LValueToDouble();
        if (!useBox(lir, LValueToDouble::Input, opd))
            return false;
        return define(lir, convert) && assignSnapshot(lir);
      }

      case MIRType_Null:
        return lowerConstantDouble(0, convert);

      case MIRType_Undefined:
        return lowerConstantDouble(js_NaN, convert);

      case MIRType_Int32:
      case MIRType_Boolean:
      {
        LInt32ToDouble *lir = new LInt32ToDouble(useRegister(opd));
        return define(lir, convert);
      }

      case MIRType_Double:
        return redefine(convert, opd);

      default:
        
        
        JS_NOT_REACHED("unexpected type");
    }
    return false;
}

bool
LIRGenerator::visitToInt32(MToInt32 *convert)
{
    MDefinition *opd = convert->input();

    switch (opd->type()) {
      case MIRType_Value:
      {
        LValueToInt32 *lir = new LValueToInt32(tempFloat(), LValueToInt32::NORMAL);
        if (!useBox(lir, LValueToInt32::Input, opd))
            return false;
        return define(lir, convert) && assignSnapshot(lir);
      }

      case MIRType_Null:
        return define(new LInteger(0), convert);

      case MIRType_Int32:
      case MIRType_Boolean:
        return redefine(convert, opd);

      case MIRType_Double:
      {
        LDoubleToInt32 *lir = new LDoubleToInt32(use(opd));
        return define(lir, convert) && assignSnapshot(lir);
      }

      default:
        
        
        
        JS_NOT_REACHED("unexpected type");
    }

    return false;
}

bool
LIRGenerator::visitTruncateToInt32(MTruncateToInt32 *truncate)
{
    MDefinition *opd = truncate->input();

    switch (opd->type()) {
      case MIRType_Value:
      {
        LValueToInt32 *lir = new LValueToInt32(tempFloat(), LValueToInt32::TRUNCATE);
        if (!useBox(lir, LValueToInt32::Input, opd))
            return false;
        return define(lir, truncate) && assignSnapshot(lir);
      }

      case MIRType_Null:
      case MIRType_Undefined:
        return define(new LInteger(0), truncate);

      case MIRType_Int32:
      case MIRType_Boolean:
        return redefine(truncate, opd);

      case MIRType_Double:
      {
        LTruncateDToInt32 *lir = new LTruncateDToInt32(useRegister(opd));
        return define(lir, truncate) && assignSnapshot(lir);
      }

      default:
        
        
        JS_NOT_REACHED("unexpected type");
    }

    return false;
}

bool
LIRGenerator::visitCopy(MCopy *ins)
{
    JS_NOT_REACHED("unexpected copy");
    return false;
}

bool
LIRGenerator::visitImplicitThis(MImplicitThis *ins)
{
    JS_ASSERT(ins->callee()->type() == MIRType_Object);

    LImplicitThis *lir = new LImplicitThis(useRegister(ins->callee()));
    return assignSnapshot(lir) && defineBox(lir, ins);
}

bool
LIRGenerator::visitSlots(MSlots *ins)
{
    return define(new LSlots(useRegister(ins->object())), ins);
}

bool
LIRGenerator::visitElements(MElements *ins)
{
    return define(new LElements(useRegister(ins->object())), ins);
}

bool
LIRGenerator::visitLoadSlot(MLoadSlot *ins)
{
    switch (ins->type()) {
      case MIRType_Value:
        return defineBox(new LLoadSlotV(useRegister(ins->slots())), ins);

      case MIRType_Undefined:
      case MIRType_Null:
        JS_NOT_REACHED("typed load must have a payload");
        return false;

      default:
        return define(new LLoadSlotT(useRegister(ins->slots())), ins);
    }

    return true;
}

bool
LIRGenerator::emitWriteBarrier(MInstruction *ins, MDefinition *input)
{
#ifdef JSGC_INCREMENTAL
    JS_ASSERT(GetIonContext()->cx->compartment->needsBarrier());

    switch (input->type()) {
      
      case MIRType_Value: {
        LInstruction *barrier = new LWriteBarrierV;
        if (!useBox(barrier, LWriteBarrierV::Input, input))
            return false;
        add(barrier, ins);
        break;
      }

      
      case MIRType_String:
      case MIRType_Object:
        add(new LWriteBarrierT(useRegisterOrConstant(input)), ins);
        break;

      
      case MIRType_Undefined:
      case MIRType_Null:
      case MIRType_Boolean:
      case MIRType_Int32:
      case MIRType_Double:
        break;

      
      default:
        JS_NOT_REACHED("Unexpected MIRType.");
    }
#endif
    return true;
}

bool
LIRGenerator::visitStoreSlot(MStoreSlot *ins)
{
    LInstruction *lir;

#ifdef JSGC_INCREMENTAL
    if (ins->needsBarrier() && !emitWriteBarrier(ins, ins->value()))
        return false;
#endif

    switch (ins->value()->type()) {
      case MIRType_Value:
        lir = new LStoreSlotV(useRegister(ins->slots()));
        if (!useBox(lir, LStoreSlotV::Value, ins->value()))
            return false;
        return add(lir, ins);

      case MIRType_Double:
        return add(new LStoreSlotT(useRegister(ins->slots()), useRegister(ins->value())), ins);

      default:
        return add(new LStoreSlotT(useRegister(ins->slots()), useRegisterOrConstant(ins->value())),
                   ins);
    }

    return true;
}

bool
LIRGenerator::visitTypeBarrier(MTypeBarrier *ins)
{
    
    
    LTypeBarrier *barrier = new LTypeBarrier(temp(LDefinition::GENERAL));
    if (!useBox(barrier, LTypeBarrier::Input, ins->input()))
        return false;
    if (!assignSnapshot(barrier, ins->bailoutKind()))
        return false;
    return defineAs(barrier, ins, ins->input()) && add(barrier);
}

bool
LIRGenerator::visitArrayLength(MArrayLength *ins)
{
    JS_ASSERT(ins->elements()->type() == MIRType_Elements);
    return define(new LArrayLength(useRegister(ins->elements())), ins);
}

bool
LIRGenerator::visitInitializedLength(MInitializedLength *ins)
{
    JS_ASSERT(ins->elements()->type() == MIRType_Elements);
    return define(new LInitializedLength(useRegister(ins->elements())), ins);
}

bool
LIRGenerator::visitBoundsCheck(MBoundsCheck *ins)
{
    LBoundsCheck *check = new LBoundsCheck(useRegisterOrConstant(ins->index()),
                                           useRegister(ins->length()));
    return assignSnapshot(check) && add(check, ins);
}

bool
LIRGenerator::visitLoadElement(MLoadElement *ins)
{
    JS_ASSERT(ins->elements()->type() == MIRType_Elements);
    JS_ASSERT(ins->index()->type() == MIRType_Int32);

    switch (ins->type()) {
      case MIRType_Value:
      {
        LLoadElementV *lir = new LLoadElementV(useRegister(ins->elements()),
                                               useRegisterOrConstant(ins->index()));
        if (ins->fallible() && !assignSnapshot(lir))
            return false;
        return defineBox(lir, ins);
      }
      case MIRType_Undefined:
      case MIRType_Null:
        JS_NOT_REACHED("typed load must have a payload");
        return false;

      default:
        JS_ASSERT(!ins->fallible());
        return define(new LLoadElementT(useRegister(ins->elements()),
                                        useRegisterOrConstant(ins->index())), ins);
    }
}

bool
LIRGenerator::visitStoreElement(MStoreElement *ins)
{
    JS_ASSERT(ins->elements()->type() == MIRType_Elements);
    JS_ASSERT(ins->index()->type() == MIRType_Int32);

#ifdef JSGC_INCREMENTAL
    if (ins->needsBarrier() && !emitWriteBarrier(ins, ins->value()))
        return false;
#endif

    switch (ins->value()->type()) {
      case MIRType_Value:
      {
        LInstruction *lir = new LStoreElementV(useRegister(ins->elements()),
                                               useRegisterOrConstant(ins->index()));
        if (!useBox(lir, LStoreElementV::Value, ins->value()))
            return false;
        return add(lir, ins);
      }
      case MIRType_Double:
        return add(new LStoreElementT(useRegister(ins->elements()),
                                      useRegisterOrConstant(ins->index()),
                                      useRegister(ins->value())), ins);

      default:
        return add(new LStoreElementT(useRegister(ins->elements()),
                                      useRegisterOrConstant(ins->index()),
                                      useRegisterOrConstant(ins->value())), ins);
    }
}

bool
LIRGenerator::visitGetPropertyCache(MGetPropertyCache *ins)
{
    JS_ASSERT(ins->object()->type() == MIRType_Object);

    if (ins->type() == MIRType_Value) {
        LGetPropertyCacheV *lir = new LGetPropertyCacheV(useRegister(ins->object()));
        if (!defineBox(lir, ins))
            return false;
        return assignSafepoint(lir, ins);
    }

    LGetPropertyCacheT *lir = new LGetPropertyCacheT(useRegister(ins->object()));
    if (!define(lir, ins))
        return false;
    return assignSafepoint(lir, ins);
}

bool
LIRGenerator::visitGuardClass(MGuardClass *ins)
{
    LDefinition t = temp(LDefinition::GENERAL);
    LGuardClass *guard = new LGuardClass(useRegister(ins->obj()), t);
    return assignSnapshot(guard) && add(guard, ins);
}

bool
LIRGenerator::visitLoadProperty(MLoadProperty *ins)
{
    LLoadPropertyGeneric *lir = new LLoadPropertyGeneric();
    lir->setOperand(0, useRegister(ins->getOperand(0)));

    if (!defineVMReturn(lir, ins))
        return false;
    if (!assignSafepoint(lir, ins))
        return false;

    return true;
}

bool
LIRGenerator::visitStringLength(MStringLength *ins)
{
    JS_ASSERT(ins->string()->type() == MIRType_String);
    return define(new LStringLength(useRegister(ins->string())), ins);
}

static void
SpewResumePoint(MBasicBlock *block, MInstruction *ins, MResumePoint *resumePoint)
{
    fprintf(IonSpewFile, "Current resume point %p details:\n", (void *)resumePoint);
    fprintf(IonSpewFile, "    frame count: %u\n", resumePoint->frameCount());

    if (ins) {
        fprintf(IonSpewFile, "    taken after: ");
        ins->printName(IonSpewFile);
    } else {
        fprintf(IonSpewFile, "    taken at block %d entry", block->id());
    }
    fprintf(IonSpewFile, "\n");

    fprintf(IonSpewFile, "    pc: %p\n", resumePoint->pc());

    for (size_t i = 0; i < resumePoint->numOperands(); i++) {
        MDefinition *in = resumePoint->getOperand(i);
        fprintf(IonSpewFile, "    slot%u: ", (unsigned)i);
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

    if (ins->resumePoint())
        updateResumeState(ins);

    if (gen->errored())
        return false;
#ifdef DEBUG
    ins->setInWorklistUnchecked();
#endif

    
    if (!postSnapshot_)
        return true;

    
    
    
    LSnapshot *post = postSnapshot_;
    postSnapshot_ = NULL;

    return add(new LCaptureAllocations(post));
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

void
LIRGenerator::updateResumeState(MInstruction *ins)
{
    lastResumePoint_ = ins->resumePoint();
    if (IonSpewEnabled(IonSpew_Snapshots))
        SpewResumePoint(NULL, ins, lastResumePoint_);
}

void
LIRGenerator::updateResumeState(MBasicBlock *block)
{
    lastResumePoint_ = block->entryResumePoint();
    if (IonSpewEnabled(IonSpew_Snapshots))
        SpewResumePoint(block, NULL, lastResumePoint_);
}

void
LIRGenerator::allocateArguments(uint32 argc)
{
    argslots_ += argc;
    if (argslots_ > maxargslots_)
        maxargslots_ = argslots_;
}

uint32
LIRGenerator::getArgumentSlot(uint32 argnum)
{
    
    JS_ASSERT(argnum < argslots_);
    return argslots_ - argnum ;
}

void
LIRGenerator::freeArguments(uint32 argc)
{
    JS_ASSERT(argc <= argslots_);
    argslots_ -= argc;
}

bool
LIRGenerator::visitBlock(MBasicBlock *block)
{
    current = block->lir();
    updateResumeState(block);

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

    if (graph.osrBlock())
        lirGraph_.setOsrBlock(graph.osrBlock()->lir());

    lirGraph_.setArgumentSlotCount(maxargslots_);

    return true;
}

