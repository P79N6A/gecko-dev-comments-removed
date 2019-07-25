








































#include "Ion.h"
#include "IonSpewer.h"
#include "MIR.h"
#include "MIRGraph.h"
#include "IonBuilder.h"
#include "jsemit.h"
#include "jsscriptinlines.h"

using namespace js;
using namespace js::ion;

MIRGenerator::MIRGenerator(JSContext *cx, TempAllocator &temp, JSScript *script, JSFunction *fun,
                           MIRGraph &graph)
  : script(script),
    cx(cx),
    pc(NULL),
    temp_(temp),
    fun_(fun),
    graph_(graph),
    error_(false)
{
    nslots_ = script->nslots + CountArgSlots(fun);
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

void
MIRGraph::addBlock(MBasicBlock *block)
{
    block->setId(blockIdGen_++);
    blocks_.pushBack(block);
#ifdef DEBUG
    numBlocks_++;
#endif
}

void
MIRGraph::unmarkBlocks() {
    for (MBasicBlockIterator i(blocks_.begin()); i != blocks_.end(); i++)
        i->unmark();
}

MBasicBlock *
MBasicBlock::New(MIRGenerator *gen, MBasicBlock *pred, jsbytecode *entryPc, Kind kind)
{
    MBasicBlock *block = new MBasicBlock(gen, entryPc, kind);
    if (!block->init())
        return NULL;

    if (!block->inherit(pred))
        return NULL;

    return block;
}

MBasicBlock *
MBasicBlock::NewPendingLoopHeader(MIRGenerator *gen, MBasicBlock *pred, jsbytecode *entryPc)
{
    return MBasicBlock::New(gen, pred, entryPc, PENDING_LOOP_HEADER);
}

MBasicBlock *
MBasicBlock::NewSplitEdge(MIRGenerator *gen, MBasicBlock *pred)
{
    return MBasicBlock::New(gen, pred, pred->pc(), SPLIT_EDGE);
}

MBasicBlock::MBasicBlock(MIRGenerator *gen, jsbytecode *pc, Kind kind)
  : gen_(gen),
    slots_(NULL),
    stackPosition_(gen->firstStackSlot()),
    lastIns_(NULL),
    pc_(pc),
    lir_(NULL),
    start_(NULL),
    successorWithPhis_(NULL),
    positionInPhiSuccessor_(0),
    kind_(kind),
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
    if (pred)
        copySlots(pred);

    
    entrySnapshot_ = new MSnapshot(this, pc());
    if (!entrySnapshot_->init(this))
        return false;

    if (pred) {
        if (!predecessors_.append(pred))
            return false;

        for (size_t i = 0; i < stackDepth(); i++)
            entrySnapshot()->initOperand(i, getSlot(i));
    }

    return true;
}

MDefinition *
MBasicBlock::getSlot(uint32 index)
{
    JS_ASSERT(index < stackPosition_);
    return slots_[index].def;
}

void
MBasicBlock::initSlot(uint32 slot, MDefinition *ins)
{
    slots_[slot].set(ins);
    entrySnapshot()->initOperand(slot, ins);
}

void
MBasicBlock::setSlot(uint32 slot, MDefinition *ins)
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

    MDefinition *def = top.def;
    if (top.isCopy()) {
        
        
        
        
        
        MInstruction *ins = MCopy::New(def);
        add(ins);
        def = ins;
    }

    setSlot(index, def);

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
MBasicBlock::push(MDefinition *ins)
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

    to.def = from.def;
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

MDefinition *
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

    return slot.def;
}

MDefinition *
MBasicBlock::peek(int32 depth)
{
    JS_ASSERT(depth < 0);
    JS_ASSERT(stackPosition_ + depth >= gen()->firstStackSlot());
    return getSlot(stackPosition_ + depth);
}

void
MBasicBlock::remove(MInstruction *ins)
{
    instructions_.remove(ins);
}

MInstructionIterator
MBasicBlock::removeAt(MInstructionIterator &iter)
{
    for (size_t i = 0; i < iter->numOperands(); i++)
        iter->replaceOperand(i, NULL);

    return instructions_.removeAt(iter);
}

MDefinitionIterator
MBasicBlock::removeDefAt(MDefinitionIterator &old)
{
    MDefinitionIterator iter(old);

    if (iter.atPhi())
        iter.phiIter_ = iter.block_->removePhiAt(iter.phiIter_);
    else
        iter.iter_ = iter.block_->removeAt(iter.iter_);

    return iter;
}

void
MBasicBlock::insertBefore(MInstruction *at, MInstruction *ins)
{
    ins->setBlock(this);
    gen()->graph().allocDefinitionId(ins);
    instructions_.insertBefore(at, ins);
}

void
MBasicBlock::insertAfter(MInstruction *at, MInstruction *ins)
{
    ins->setBlock(this);
    gen()->graph().allocDefinitionId(ins);
    instructions_.insertAfter(at, ins);
}

void
MBasicBlock::add(MInstruction *ins)
{
    JS_ASSERT(!lastIns_);
    ins->setBlock(this);
    gen()->graph().allocDefinitionId(ins);
    instructions_.pushBack(ins);
}

void
MBasicBlock::end(MControlInstruction *ins)
{
    JS_ASSERT(ins);
    add(ins);
    lastIns_ = ins;
}

void
MBasicBlock::addPhi(MPhi *phi)
{
    phis_.pushBack(phi);
    phi->setBlock(this);
    gen()->graph().allocDefinitionId(phi);
}

MPhiIterator
MBasicBlock::removePhiAt(MPhiIterator &at)
{
    JS_ASSERT(!phis_.empty());

    for (size_t i = 0; i < at->numOperands(); i++)
        at->replaceOperand(i, NULL);

    MPhiIterator result = phis_.removeAt(at);

    if (phis_.empty()) {
        for (MBasicBlock **pred = predecessors_.begin(); pred != predecessors_.end(); pred++)
            (*pred)->setSuccessorWithPhis(NULL, 0);
    }
    return result;
}

bool
MBasicBlock::addPredecessor(MBasicBlock *pred)
{
    JS_ASSERT(predecessors_.length() > 0);

    
    JS_ASSERT(pred->lastIns_);
    JS_ASSERT(pred->stackPosition_ == stackPosition_);

    for (uint32 i = 0; i < stackPosition_; i++) {
        MDefinition *mine = getSlot(i);
        MDefinition *other = pred->getSlot(i);

        if (mine != other) {
            MPhi *phi;

            
            
            
            if (mine->isPhi() && mine->block() == this) {
                JS_ASSERT(predecessors_.length());
                phi = mine->toPhi();
            } else {
                
                phi = MPhi::New(i);
                addPhi(phi);

                
                
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
MBasicBlock::assertUsesAreNotWithin(MUseIterator use, MUseIterator end)
{
#ifdef DEBUG
    for (; use != end; use++) {
        JS_ASSERT_IF(use->node()->isDefinition(),
                     use->node()->toDefinition()->block()->id() < id());
    }
#endif
}

static inline MDefinition *
FollowCopy(MDefinition *def)
{
    MDefinition *ret = def->isCopy() ? def->getOperand(0) : def;
    JS_ASSERT(!ret->isCopy());
    return ret;
}

bool
MBasicBlock::setBackedge(MBasicBlock *pred, MBasicBlock *successor)
{
    
    JS_ASSERT(lastIns_);
    JS_ASSERT(pred->lastIns_);
    JS_ASSERT(pred->stackPosition_ == stackPosition_);
    JS_ASSERT(entrySnapshot()->stackDepth() == stackPosition_);

    
    JS_ASSERT(kind_ == PENDING_LOOP_HEADER);

    
    
    
    
    
    
    
    for (uint32 i = 0; i < stackPosition_; i++) {
        MDefinition *entryDef = entrySnapshot()->getOperand(i);
        MDefinition *exitDef = pred->slots_[i].def;

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        


        
        
        if (FollowCopy(entryDef) == FollowCopy(exitDef))
            continue;

        
        
        MPhi *phi = MPhi::New(i);
        addPhi(phi);

        for (MUseIterator use(entryDef->usesBegin()); use != entryDef->usesEnd(); ) {
            JS_ASSERT(use->node()->getOperand(use->index()) == entryDef);

            
            
            
            if (use->node()->block()->id() < id()) {
                assertUsesAreNotWithin(use, entryDef->usesEnd());
                break;
            }

            
            
            
            use = use->node()->replaceOperand(use, phi);
        }

#ifdef DEBUG
        
        
        
        
        
        
        
        for (uint32 j = i + 1; j < stackPosition_; j++)
            JS_ASSERT(slots_[j].def != entryDef);
#endif

        if (!phi->addInput(entryDef) || !phi->addInput(exitDef))
            return false;

        setSlot(i, phi);

        
        
        
        
        
        
        
        
        
        
        
        
        if (successor && successor->getSlot(i) == entryDef) {
            successor->setSlot(i, phi);

            
            JS_ASSERT(successor->entrySnapshot()->getOperand(i) == phi);
        }
    }

    
    kind_ = LOOP_HEADER;

    return predecessors_.append(pred);
}

size_t
MBasicBlock::numSuccessors() const
{
    JS_ASSERT(lastIns());
    return lastIns()->numSuccessors();
}

MBasicBlock *
MBasicBlock::getSuccessor(size_t index) const
{
    JS_ASSERT(lastIns());
    return lastIns()->getSuccessor(index);
}

void
MBasicBlock::replaceSuccessor(size_t pos, MBasicBlock *split)
{
    JS_ASSERT(lastIns());
    lastIns()->replaceSuccessor(pos, split);

    
    JS_ASSERT(!successorWithPhis_);
}

void
MBasicBlock::replacePredecessor(MBasicBlock *old, MBasicBlock *split)
{
    for (size_t i = 0; i < numPredecessors(); i++) {
        if (getPredecessor(i) == old) {
            predecessors_[i] = split;

#ifdef DEBUG
            
            for (size_t j = i; j < numPredecessors(); j++)
                JS_ASSERT(predecessors_[j] != old);
#endif

            return;
        }
    }
    JS_NOT_REACHED("predecessor was not found");
}

