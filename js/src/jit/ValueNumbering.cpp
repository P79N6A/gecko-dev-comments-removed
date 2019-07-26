





#include "jit/ValueNumbering.h"

#include "jit/IonSpewer.h"
#include "jit/MIRGenerator.h"
#include "jit/MIRGraph.h"

using namespace js;
using namespace js::jit;

ValueNumberer::ValueNumberer(MIRGenerator *mir, MIRGraph &graph, bool optimistic)
  : mir(mir),
    graph_(graph),
    values(graph.alloc()),
    pessimisticPass_(!optimistic),
    count_(0)
{ }

TempAllocator &
ValueNumberer::alloc() const
{
    return graph_.alloc();
}

uint32_t
ValueNumberer::lookupValue(MDefinition *ins)
{
    ValueMap::AddPtr p = values.lookupForAdd(ins);
    if (p) {
        
        setClass(ins, p->key());
        return p->value();
    }

    if (!values.add(p, ins, ins->id()))
        return 0;
    breakClass(ins);

    return ins->id();
}

MDefinition *
ValueNumberer::simplify(MDefinition *def, bool useValueNumbers)
{
    if (def->isEffectful())
        return def;

    MDefinition *ins = def->foldsTo(alloc(), useValueNumbers);
    if (ins == def)
        return def;

    
    if (!ins->valueNumberData())
        ins->setValueNumberData(new(alloc()) ValueNumberData);

    if (!ins->block()) {
        
        

        
        JS_ASSERT(!def->isPhi());

        def->block()->insertAfter(def->toInstruction(), ins->toInstruction());
        ins->setValueNumber(lookupValue(ins));
    }

    JS_ASSERT(ins->id() != 0);

    def->replaceAllUsesWith(ins);

    IonSpew(IonSpew_GVN, "Folding %d to be %d", def->id(), ins->id());
    return ins;
}

MControlInstruction *
ValueNumberer::simplifyControlInstruction(MControlInstruction *def)
{
    if (def->isEffectful())
        return def;

    MDefinition *repl = def->foldsTo(alloc(), false);
    if (repl == def)
        return def;

    
    if (!repl->valueNumberData())
        repl->setValueNumberData(new(alloc()) ValueNumberData);

    MBasicBlock *block = def->block();

    
    JS_ASSERT(repl->isControlInstruction());
    JS_ASSERT(!def->hasUses());

    if (def->isInWorklist())
        repl->setInWorklist();

    block->discardLastIns();
    block->end(repl->toControlInstruction());
    return repl->toControlInstruction();
}

void
ValueNumberer::markDefinition(MDefinition *def)
{
    if (isMarked(def))
        return;

    IonSpew(IonSpew_GVN, "marked %d", def->id());
    def->setInWorklist();
    count_++;
}

void
ValueNumberer::unmarkDefinition(MDefinition *def)
{
    if (pessimisticPass_)
        return;

    JS_ASSERT(count_ > 0);
    IonSpew(IonSpew_GVN, "unmarked %d", def->id());
    def->setNotInWorklist();
    count_--;
}

void
ValueNumberer::markBlock(MBasicBlock *block)
{
    for (MDefinitionIterator iter(block); iter; iter++)
        markDefinition(*iter);
    markDefinition(block->lastIns());
}

void
ValueNumberer::markConsumers(MDefinition *def)
{
    if (pessimisticPass_)
        return;

    JS_ASSERT(!def->isInWorklist());
    JS_ASSERT(!def->isControlInstruction());
    for (MUseDefIterator use(def); use; use++)
        markDefinition(use.def());
}

bool
ValueNumberer::computeValueNumbers()
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    IonSpew(IonSpew_GVN, "Numbering instructions");

    if (!values.init())
        return false;
    
    for (ReversePostorderIterator block(graph_.rpoBegin()); block != graph_.rpoEnd(); block++) {
        if (mir->shouldCancel("Value Numbering (preparation loop"))
            return false;
        for (MDefinitionIterator iter(*block); iter; iter++)
            iter->setValueNumberData(new(alloc()) ValueNumberData);
        MControlInstruction *jump = block->lastIns();
        jump->setValueNumberData(new(alloc()) ValueNumberData);
    }

    
    
    
    if (pessimisticPass_) {
        for (ReversePostorderIterator block(graph_.rpoBegin()); block != graph_.rpoEnd(); block++) {
            for (MDefinitionIterator iter(*block); iter; iter++)
                iter->setValueNumber(iter->id());
        }
    } else {
        
        markBlock(*(graph_.begin()));
        if (graph_.osrBlock())
            markBlock(graph_.osrBlock());
    }

    while (count_ > 0) {
#ifdef DEBUG
        if (!pessimisticPass_) {
            size_t debugCount = 0;
            IonSpew(IonSpew_GVN, "The following instructions require processing:");
            for (ReversePostorderIterator block(graph_.rpoBegin()); block != graph_.rpoEnd(); block++) {
                for (MDefinitionIterator iter(*block); iter; iter++) {
                    if (iter->isInWorklist()) {
                        IonSpew(IonSpew_GVN, "\t%d", iter->id());
                        debugCount++;
                    }
                }
                if (block->lastIns()->isInWorklist()) {
                    IonSpew(IonSpew_GVN, "\t%d", block->lastIns()->id());
                    debugCount++;
                }
            }
            if (!debugCount)
                IonSpew(IonSpew_GVN, "\tNone");
            JS_ASSERT(debugCount == count_);
        }
#endif
        for (ReversePostorderIterator block(graph_.rpoBegin()); block != graph_.rpoEnd(); block++) {
            if (mir->shouldCancel("Value Numbering (main loop)"))
                return false;
            for (MDefinitionIterator iter(*block); iter; ) {

                if (!isMarked(*iter)) {
                    iter++;
                    continue;
                }

                JS_ASSERT_IF(!pessimisticPass_, count_ > 0);
                unmarkDefinition(*iter);

                MDefinition *ins = simplify(*iter, false);

                if (ins != *iter) {
                    iter = block->discardDefAt(iter);
                    continue;
                }

                
                
                
                
                if (!ins->hasDefUses() && (!ins->isMovable() || ins->isEffectful())) {
                    iter++;
                    continue;
                }

                uint32_t value = lookupValue(ins);

                if (!value)
                    return false; 

                if (ins->valueNumber() != value) {
                    IonSpew(IonSpew_GVN,
                            "Broke congruence for instruction %d (%p) with VN %d (now using %d)",
                            ins->id(), (void *) ins, ins->valueNumber(), value);
                    ins->setValueNumber(value);
                    markConsumers(ins);
                }

                iter++;
            }
            
            MControlInstruction *jump = block->lastIns();
            jump = simplifyControlInstruction(jump);

            
            if (!jump->isInWorklist())
                continue;
            unmarkDefinition(jump);
            if (jump->valueNumber() == 0) {
                jump->setValueNumber(jump->id());
                for (size_t i = 0; i < jump->numSuccessors(); i++)
                    markBlock(jump->getSuccessor(i));
            }

        }

        
        
        if (pessimisticPass_)
            break;
    }
#ifdef DEBUG
    for (ReversePostorderIterator block(graph_.rpoBegin()); block != graph_.rpoEnd(); block++) {
        for (MDefinitionIterator iter(*block); iter; iter++) {
            JS_ASSERT(!iter->isInWorklist());
            JS_ASSERT_IF(iter->valueNumber() == 0,
                         !iter->hasDefUses() && (!iter->isMovable() || iter->isEffectful()));
        }
    }
#endif
    return true;
}

MDefinition *
ValueNumberer::findDominatingDef(InstructionMap &defs, MDefinition *ins, size_t index)
{
    JS_ASSERT(ins->valueNumber() != 0);
    InstructionMap::Ptr p = defs.lookup(ins->valueNumber());
    MDefinition *dom;
    if (!p || index > p->value().validUntil) {
        DominatingValue value;
        value.def = ins;
        
        
        
        
        
        
        
        value.validUntil = index + ins->block()->numDominated();

        if(!defs.put(ins->valueNumber(), value))
            return nullptr;

        dom = ins;
    } else {
        dom = p->value().def;
    }

    return dom;
}

bool
ValueNumberer::eliminateRedundancies()
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    InstructionMap defs(alloc());

    if (!defs.init())
        return false;

    IonSpew(IonSpew_GVN, "Eliminating redundant instructions");

    
    Vector<MBasicBlock *, 1, IonAllocPolicy> worklist(alloc());

    
    size_t index = 0;

    
    
    for (MBasicBlockIterator i(graph_.begin()); i != graph_.end(); i++) {
        MBasicBlock *block = *i;
        if (block->immediateDominator() == block) {
            if (!worklist.append(block))
                return false;
        }
    }

    
    while (!worklist.empty()) {
        if (mir->shouldCancel("Value Numbering (eliminate loop)"))
            return false;
        MBasicBlock *block = worklist.popCopy();

        IonSpew(IonSpew_GVN, "Looking at block %d", block->id());

        
        if (!worklist.append(block->immediatelyDominatedBlocksBegin(),
                             block->immediatelyDominatedBlocksEnd())) {
            return false;
        }

        
        for (MDefinitionIterator iter(block); iter; ) {
            MDefinition *ins = simplify(*iter, true);

            
            if (ins != *iter) {
                iter = block->discardDefAt(iter);
                continue;
            }

            
            if (!ins->isMovable() || ins->isEffectful()) {
                iter++;
                continue;
            }

            MDefinition *dom = findDominatingDef(defs, ins, index);
            if (!dom)
                return false; 

            if (dom == ins || !dom->updateForReplacement(ins)) {
                iter++;
                continue;
            }

            IonSpew(IonSpew_GVN, "instruction %d is dominated by instruction %d (from block %d)",
                    ins->id(), dom->id(), dom->block()->id());

            ins->replaceAllUsesWith(dom);

            JS_ASSERT(!ins->hasUses());
            JS_ASSERT(ins->block() == block);
            JS_ASSERT(!ins->isEffectful());
            JS_ASSERT(ins->isMovable());

            iter = ins->block()->discardDefAt(iter);
        }
        index++;
    }

    JS_ASSERT(index == graph_.numBlocks());
    return true;
}


bool
ValueNumberer::analyze()
{
    return computeValueNumbers() && eliminateRedundancies();
}


bool
ValueNumberer::clear()
{
    IonSpew(IonSpew_GVN, "Clearing value numbers");

    
    for (ReversePostorderIterator block(graph_.rpoBegin()); block != graph_.rpoEnd(); block++) {
        if (mir->shouldCancel("Value Numbering (clearing)"))
            return false;
        for (MDefinitionIterator iter(*block); iter; iter++)
            iter->clearValueNumberData();
        block->lastIns()->clearValueNumberData();
    }

    return true;
}

uint32_t
MDefinition::valueNumber() const
{
    JS_ASSERT(block_);
    if (valueNumber_ == nullptr)
        return 0;
    return valueNumber_->valueNumber();
}
void
MDefinition::setValueNumber(uint32_t vn)
{
    valueNumber_->setValueNumber(vn);
}

void
ValueNumberer::setClass(MDefinition *def, MDefinition *rep)
{
    def->valueNumberData()->setClass(def, rep);
}

MDefinition *
ValueNumberer::findSplit(MDefinition *def)
{
    for (MDefinition *vncheck = def->valueNumberData()->classNext;
         vncheck != nullptr;
         vncheck = vncheck->valueNumberData()->classNext) {
        if (!def->congruentTo(vncheck)) {
            IonSpew(IonSpew_GVN, "Proceeding with split because %d is not congruent to %d",
                    def->id(), vncheck->id());
            return vncheck;
        }
    }
    return nullptr;
}

void
ValueNumberer::breakClass(MDefinition *def)
{
    if (def->valueNumber() == def->id()) {
        IonSpew(IonSpew_GVN, "Breaking congruence with itself: %d", def->id());
        ValueNumberData *defdata = def->valueNumberData();
        JS_ASSERT(defdata->classPrev == nullptr);
        
        if (defdata->classNext == nullptr)
            return;
        
        
        MDefinition *newRep = findSplit(def);
        if (!newRep)
            return;
        markConsumers(def);
        ValueNumberData *newdata = newRep->valueNumberData();

        
        
        
        
        
        
        
        
        
        
        
        MDefinition *lastOld = newdata->classPrev;

        JS_ASSERT(lastOld); 
        JS_ASSERT(lastOld->valueNumberData()->classNext == newRep);

        
        
        lastOld->valueNumberData()->classNext = nullptr;

#ifdef DEBUG
        for (MDefinition *tmp = def; tmp != nullptr; tmp = tmp->valueNumberData()->classNext) {
            JS_ASSERT(tmp->valueNumber() == def->valueNumber());
            JS_ASSERT(tmp->congruentTo(def));
            JS_ASSERT(tmp != newRep);
        }
#endif
        
        
        
        newdata->classPrev = nullptr;
        IonSpew(IonSpew_GVN, "Choosing a new representative: %d", newRep->id());

        
        for (MDefinition *tmp = newRep; tmp != nullptr; tmp = tmp->valueNumberData()->classNext) {
            
            if (tmp->isInWorklist()) {
                IonSpew(IonSpew_GVN, "Defer  to a new congruence class: %d", tmp->id());
                continue;
            }
            IonSpew(IonSpew_GVN, "Moving to a new congruence class: %d", tmp->id());
            tmp->setValueNumber(newRep->id());
            markConsumers(tmp);
            markDefinition(tmp);
        }

        
        
        
        
        values.put(newRep, newRep->id());
    } else {
        
        
        ValueNumberData *defdata = def->valueNumberData();
        if (defdata->classPrev)
            defdata->classPrev->valueNumberData()->classNext = defdata->classNext;
        if (defdata->classNext)
            defdata->classNext->valueNumberData()->classPrev = defdata->classPrev;

        
        defdata->classPrev = nullptr;
        defdata->classNext = nullptr;
    }
}
