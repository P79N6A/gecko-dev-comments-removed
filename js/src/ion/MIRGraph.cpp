








































#include "Ion.h"
#include "IonSpewer.h"
#include "MIR.h"
#include "MIRGraph.h"
#include "IonBuilder.h"
#include "jsemit.h"
#include "jsscriptinlines.h"

using namespace js;
using namespace js::ion;

MIRGenerator::MIRGenerator(TempAllocator &temp, JSScript *script, JSFunction *fun, MIRGraph &graph)
  : script(script),
    pc(NULL),
    temp_(temp),
    fun_(fun),
    graph_(graph),
    error_(false)
{
    nslots_ = script->nslots + (fun ? fun->nargs + 2 : 0);
}

bool
MIRGenerator::abort(const char *message, ...)
{
    va_list ap;
    va_start(ap, message);
    IonSpewVA(IonSpew_Abort, message, ap);
    va_end(ap);
    error_ = true;
    return false;
}

bool
MIRGraph::addBlock(MBasicBlock *block)
{
    block->setId(blocks_.length());
    return blocks_.append(block);
}

MBasicBlock *
MBasicBlock::New(MIRGenerator *gen, MBasicBlock *pred, jsbytecode *entryPc)
{
    MBasicBlock *block = new MBasicBlock(gen, entryPc);
    if (!block->init())
        return NULL;

    if (pred && !block->inherit(pred))
        return NULL;

    return block;
}

MBasicBlock *
MBasicBlock::NewLoopHeader(MIRGenerator *gen, MBasicBlock *pred, jsbytecode *entryPc)
{
    return MBasicBlock::New(gen, pred, entryPc);
}

MBasicBlock::MBasicBlock(MIRGenerator *gen, jsbytecode *pc)
  : gen_(gen),
    slots_(NULL),
    stackPosition_(gen->firstStackSlot()),
    lastIns_(NULL),
    pc_(pc),
    lir_(NULL),
    successorWithPhis_(NULL),
    positionInPhiSuccessor_(0),
    loopSuccessor_(NULL),
    mark_(false),
    immediateDominator_(NULL),
    numDominated_(0)
{
}

bool
MBasicBlock::init()
{
    slots_ = gen()->allocate<StackSlot>(gen()->nslots());
    if (!slots_)
        return false;
    MSnapshot *snapshot = new MSnapshot(this, pc());
    if (!snapshot->init(this))
        return false;
    add(snapshot);
    return true;
}

void
MBasicBlock::copySlots(MBasicBlock *from)
{
    stackPosition_ = from->stackPosition_;

    for (uint32 i = 0; i < stackPosition_; i++)
        slots_[i] = from->slots_[i];
}

bool
MBasicBlock::inherit(MBasicBlock *pred)
{
    copySlots(pred);
    if (!predecessors_.append(pred))
        return false;

    for (size_t i = 0; i < stackDepth(); i++)
        entrySnapshot()->initOperand(i, getSlot(i));

    return true;
}

MInstruction *
MBasicBlock::getSlot(uint32 index)
{
    JS_ASSERT(index < stackPosition_);
    return slots_[index].ins;
}

void
MBasicBlock::initSlot(uint32 slot, MInstruction *ins)
{
    slots_[slot].set(ins);
    entrySnapshot()->initOperand(slot, ins);
}

void
MBasicBlock::setSlot(uint32 slot, MInstruction *ins)
{
    StackSlot &var = slots_[slot];

    
    
    if (var.isCopied()) {
        
        
        
        uint32 lowest = var.firstCopy;
        uint32 prev = NotACopy;
        do {
            uint32 next = slots_[lowest].nextCopy;
            if (next == NotACopy)
                break;
            JS_ASSERT(next < lowest);
            prev = lowest;
            lowest = next;
        } while (true);

        
        for (uint32 copy = var.firstCopy; copy != lowest; copy = slots_[copy].nextCopy)
            slots_[copy].copyOf = lowest;

        
        slots_[lowest].copyOf = NotACopy;
        slots_[lowest].firstCopy = prev;
    }

    var.set(ins);
}


























bool
MBasicBlock::setVariable(uint32 index)
{
    JS_ASSERT(stackPosition_ > gen()->firstStackSlot());
    StackSlot &top = slots_[stackPosition_ - 1];

    MInstruction *ins = top.ins;
    if (top.isCopy()) {
        
        
        
        
        
        ins = MCopy::New(ins);
        add(ins);
    }

    setSlot(index, ins);

    if (!top.isCopy()) {
        
        
        
        
        
        
        
        
        
        
        top.copyOf = index;
        top.nextCopy = slots_[index].firstCopy;
        slots_[index].firstCopy = stackPosition_ - 1;
    }

    return true;
}

bool
MBasicBlock::setArg(uint32 arg)
{
    
    return setVariable(gen()->argSlot(arg));
}

bool
MBasicBlock::setLocal(uint32 local)
{
    
    return setVariable(gen()->localSlot(local));
}

void
MBasicBlock::push(MInstruction *ins)
{
    JS_ASSERT(stackPosition_ < gen()->nslots());
    slots_[stackPosition_].set(ins);
    stackPosition_++;
}

void
MBasicBlock::pushVariable(uint32 slot)
{
    if (slots_[slot].isCopy())
        slot = slots_[slot].copyOf;

    JS_ASSERT(stackPosition_ < gen()->nslots());
    StackSlot &to = slots_[stackPosition_];
    StackSlot &from = slots_[slot];

    to.ins = from.ins;
    to.copyOf = slot;
    to.nextCopy = from.firstCopy;
    from.firstCopy = stackPosition_;

    stackPosition_++;
}

void
MBasicBlock::pushArg(uint32 arg)
{
    
    pushVariable(gen()->argSlot(arg));
}

void
MBasicBlock::pushLocal(uint32 local)
{
    
    pushVariable(gen()->localSlot(local));
}

MInstruction *
MBasicBlock::pop()
{
    JS_ASSERT(stackPosition_ > gen()->firstStackSlot());

    StackSlot &slot = slots_[--stackPosition_];
    if (slot.isCopy()) {
        
        
        StackSlot &backing = slots_[slot.copyOf];
        JS_ASSERT(backing.isCopied());
        JS_ASSERT(backing.firstCopy == stackPosition_);

        backing.firstCopy = slot.nextCopy;
    }

    
    JS_ASSERT(!slot.isCopied());

    return slot.ins;
}

MInstruction *
MBasicBlock::peek(int32 depth)
{
    JS_ASSERT(depth < 0);
    JS_ASSERT(stackPosition_ + depth >= gen()->firstStackSlot());
    return getSlot(stackPosition_ + depth);
}

void
MBasicBlock::remove(MInstruction *ins)
{
    MInstructionIterator iter(ins);
    removeAt(iter);
}

MInstructionIterator
MBasicBlock::removeAt(MInstructionIterator &iter)
{
    return instructions_.removeAt(iter);
}

void
MBasicBlock::insertBefore(MInstruction *at, MInstruction *ins)
{
    ins->setBlock(this);
    gen()->graph().allocInstructionId(ins);
    instructions_.insertBefore(at, ins);
}

void
MBasicBlock::insertAfter(MInstruction *at, MInstruction *ins)
{
    ins->setBlock(this);
    gen()->graph().allocInstructionId(ins);
    instructions_.insertAfter(at, ins);
}

void
MBasicBlock::add(MInstruction *ins)
{
    JS_ASSERT(!lastIns_);
    ins->setBlock(this);
    gen()->graph().allocInstructionId(ins);
    instructions_.insert(ins);
}

void
MBasicBlock::end(MControlInstruction *ins)
{
    add(ins);
    lastIns_ = ins;
}

bool
MBasicBlock::addPhi(MPhi *phi)
{
    if (!phis_.append(phi))
        return false;
    phi->setBlock(this);
    gen()->graph().allocInstructionId(phi);
    return true;
}

bool
MBasicBlock::addPredecessor(MBasicBlock *pred)
{
    if (!hasHeader())
        return inherit(pred);

    
    JS_ASSERT(pred->lastIns_);
    JS_ASSERT(pred->stackPosition_ == stackPosition_);

    for (uint32 i = 0; i < stackPosition_; i++) {
        MInstruction *mine = getSlot(i);
        MInstruction *other = pred->getSlot(i);

        if (mine != other) {
            MPhi *phi;

            
            
            
            if (mine->isPhi() && mine->block() == this) {
                JS_ASSERT(predecessors_.length());
                phi = mine->toPhi();
            } else {
                
                phi = MPhi::New(i);
                if (!addPhi(phi))
                    return false;

                
                
                for (size_t j = 0; j < predecessors_.length(); j++) {
                    JS_ASSERT(predecessors_[j]->getSlot(i) == mine);
                    if (!phi->addInput(mine))
                        return false;
                }

                setSlot(i, phi);
                entrySnapshot()->replaceOperand(i, phi);
            }

            if (!phi->addInput(other))
                return false;
        }
    }

    return predecessors_.append(pred);
}

bool
MBasicBlock::addImmediatelyDominatedBlock(MBasicBlock *child)
{
    return immediatelyDominated_.append(child);
}

void
MBasicBlock::assertUsesAreNotWithin(MUse *use)
{
#ifdef DEBUG
    for (; use; use = use->next())
        JS_ASSERT(use->ins()->block()->id() < id());
#endif
}

bool
MBasicBlock::setBackedge(MBasicBlock *pred, MBasicBlock *successor)
{
    
    JS_ASSERT(lastIns_);
    JS_ASSERT(pred->lastIns_);
    JS_ASSERT(pred->stackPosition_ == stackPosition_);
    JS_ASSERT(entrySnapshot()->stackDepth() == stackPosition_);

    
    
    
    
    
    
    
    for (uint32 i = 0; i < stackPosition_; i++) {
        MInstruction *entryDef = entrySnapshot()->getInput(i);
        MInstruction *exitDef = pred->slots_[i].ins;

        
        
        if (entryDef == exitDef)
            continue;

        
        
        MPhi *phi = MPhi::New(i);
        if (!addPhi(phi))
            return false;

        MUse *use = entryDef->uses();
        MUse *prev = NULL;
        while (use) {
            JS_ASSERT(use->ins()->getOperand(use->index())->ins() == entryDef);

            
            
            
            if (use->ins()->block()->id() < id()) {
                assertUsesAreNotWithin(use);
                break;
            }

            
            
            
            MUse *next = use->next();
            use->ins()->replaceOperand(prev, use, phi); 
            use = next;
        }

#ifdef DEBUG
        
        
        
        
        
        
        
        for (uint32 j = i + 1; j < stackPosition_; j++)
            JS_ASSERT(slots_[j].ins != entryDef);
#endif

        if (!phi->addInput(entryDef) || !phi->addInput(exitDef))
            return false;

        setSlot(i, phi);

        
        
        
        
        
        
        
        
        
        
        
        
        if (successor && successor->getSlot(i) == entryDef) {
            successor->setSlot(i, phi);

            
            JS_ASSERT(successor->entrySnapshot()->getInput(i) == phi);
        }
    }

    
    loopSuccessor_ = successor;

    return predecessors_.append(pred);
}

size_t
MBasicBlock::numSuccessors() const
{
    return lastIns()->numSuccessors();
}

MBasicBlock *
MBasicBlock::getSuccessor(size_t index) const
{
    return lastIns()->getSuccessor(index);
}

