








































#include "Ion.h"
#include "IonSpewer.h"
#include "CompileInfo.h"
#include "ValueNumbering.h"

using namespace js;
using namespace js::ion;

ValueNumberer::ValueNumberer(MIRGraph &graph, bool optimistic)
  : graph_(graph),
    pessimisticPass_(!optimistic),
    count_(0)
{ }

uint32
ValueNumberer::lookupValue(MDefinition *ins)
{

    ValueMap::AddPtr p = values.lookupForAdd(ins);

    if (!p) {
        if (!values.add(p, ins, ins->id()))
            return 0;
    }

    return p->value;
}

MDefinition *
ValueNumberer::simplify(MDefinition *def, bool useValueNumbers)
{
    if (def->isEffectful())
        return def;

    MDefinition *ins = def->foldsTo(useValueNumbers);

    if (ins == def || !ins->updateForReplacement(def))
        return def;

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

                uint32 value = lookupValue(ins);

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
            JS_ASSERT(iter->valueNumber() != 0);
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
    if (!p || index > p->value.validUntil) {
        DominatingValue value;
        value.def = ins;
        
        
        
        
        
        
        
        value.validUntil = index + ins->block()->numDominated();

        if(!defs.put(ins->valueNumber(), value))
            return NULL;

        dom = ins;
    } else {
        dom = p->value.def;
    }

    return dom;
}

bool
ValueNumberer::eliminateRedundancies()
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    InstructionMap defs;

    if (!defs.init())
        return false;

    IonSpew(IonSpew_GVN, "Eliminating redundant instructions");

    
    Vector<MBasicBlock *, 1, IonAllocPolicy> worklist;

    
    size_t index = 0;

    
    
    for (MBasicBlockIterator i(graph_.begin()); i != graph_.end(); i++) {
        MBasicBlock *block = *i;
        if (block->immediateDominator() == block) {
            if (!worklist.append(block))
                return false;
        }
    }

    
    while (!worklist.empty()) {
        MBasicBlock *block = worklist.popCopy();

        IonSpew(IonSpew_GVN, "Looking at block %d", block->id());

        
        for (size_t i = 0; i < block->numImmediatelyDominatedBlocks(); i++) {
            if (!worklist.append(block->getImmediatelyDominatedBlock(i)))
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

