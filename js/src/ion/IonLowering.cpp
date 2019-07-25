








































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
    mir->setInWorklist();
    mir->setId(0);
    return true;
}

template <size_t X, size_t Y> bool
LIRGenerator::defineBox(LInstructionHelper<BOX_PIECES, X, Y> *lir, MInstruction *mir,
                        LDefinition::Policy policy)
{
    uint32 vreg = nextVirtualRegister();
    if (vreg >= MAX_VIRTUAL_REGISTERS)
        return false;

#if defined(JS_NUNBOX32)
    lir->setDef(0, LDefinition(vreg, LDefinition::TYPE, policy));
    lir->setDef(1, LDefinition(vreg + 1, LDefinition::PAYLOAD, policy));
    if (nextVirtualRegister() >= MAX_VIRTUAL_REGISTERS)
        return false;
#elif defined(JS_PUNBOX64)
    lir->setDef(0, LDefinition(vreg, LDefinition::BOX, policy));
#endif

    mir->setId(vreg);
    mir->setInWorklist();
    return add(lir);
}

LUse
LIRGenerator::use(MInstruction *mir, LUse policy)
{
    JS_ASSERT(mir->inWorklist());

    bool emitAtUse = false;
    if (!mir->id()) {
        
        if (!mir->accept(this))
            gen->error();
        JS_ASSERT(mir->id());
        emitAtUse = true;
    }

    policy.setVirtualRegister(mir->id());

    
    if (emitAtUse)
        mir->setId(0);

    return policy;
}

bool
LIRGenerator::visitConstant(MConstant *ins)
{
    const Value &v = ins->value();
    switch (ins->type()) {
      case MIRType_Boolean:
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
    return true;
}

bool
LIRGenerator::visitTest(MTest *ins)
{
    return true;
}

bool
LIRGenerator::visitPhi(MPhi *ins)
{
    return true;
}

bool
LIRGenerator::visitBitAnd(MBitAnd *ins)
{
    return true;
}

bool
LIRGenerator::visitAdd(MAdd *ins)
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
LIRGenerator::visitBox(MBox *ins)
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
LIRGenerator::visitUnbox(MUnbox *ins)
{
    return true;
}

bool
LIRGenerator::visitSnapshot(MSnapshot *snapshot)
{
    
    return true;
}

bool
LIRGenerator::visitBlock(MBasicBlock *block)
{
    current = LBlock::New(block);
    if (!current)
        return false;

    for (size_t i = 0; i < block->numPhis(); i++) {
        if (!gen->ensureBallast())
            return false;
        if (!block->getPhi(i)->accept(this))
            return false;
    }

    for (MInstructionIterator iter = block->begin(); iter != block->end(); iter++) {
        if (!gen->ensureBallast())
            return false;
        if (!iter->accept(this))
            return false;
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
    return true;
}

