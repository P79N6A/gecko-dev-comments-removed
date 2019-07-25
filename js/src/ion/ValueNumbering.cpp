








































#include "Ion.h"
#include "ValueNumbering.h"
#include "IonSpewer.h"

using namespace js;
using namespace js::ion;

ValueNumberer::ValueNumberer(MIRGraph &graph, bool optimistic)
  : graph_(graph),
    pessimisticPass_(!optimistic)
{ }


uint32
ValueNumberer::lookupValue(ValueMap &values, MDefinition *ins)
{

    ValueMap::AddPtr p = values.lookupForAdd(ins);

    if (!p) {
        if (!values.add(p, ins, ins->id()))
            return 0;
    }

    return p->value;
}

bool
ValueNumberer::computeValueNumbers()
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    IonSpew(IonSpew_GVN, "Numbering instructions");

    ValueMap values;

    if (!values.init())
        return false;

    
    
    
    if (pessimisticPass_) {
        for (size_t i = 0; i < graph_.numBlocks(); i++) {
            MBasicBlock *block = graph_.getBlock(i);

            for (MDefinitionIterator iter(block); iter; iter++)
                iter->setValueNumber(iter->id());
        }
    }

    bool changed = true;
    while (changed) {
        changed = false;
        for (size_t i = 0; i < graph_.numBlocks(); i++) {
            MBasicBlock *block = graph_.getBlock(i);

            for (MDefinitionIterator iter(block); iter; iter++) {
                MDefinition *ins = *iter;

                uint32 value = lookupValue(values, ins);

                if (!value)
                    return false; 

                if (ins->valueNumber() != value) {
                    IonSpew(IonSpew_GVN, "Broke congruence for instruction %d (%p) with VN %d (now using %d)",
                            ins->id(), ins, ins->valueNumber(), value);
                    ins->setValueNumber(value);
                    changed = true;
                }
            }
        }
        values.clear();

        
        
        if (pessimisticPass_)
            break;
    }
    return true;
}

MDefinition *
ValueNumberer::findDominatingDef(InstructionMap &defs, MDefinition *ins, size_t index)
{
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

    size_t index = 0;

    Vector<MBasicBlock *, 1, IonAllocPolicy> nodes;

    MBasicBlock *start = graph_.getBlock(0);
    if (!nodes.append(start))
        return false;

    while (!nodes.empty()) {
        MBasicBlock *block = nodes.popCopy();

        IonSpew(IonSpew_GVN, "Looking at block %d", block->id());

        for (size_t i = 0; i < block->numImmediatelyDominatedBlocks(); i++) {
            if (!nodes.append(block->getImmediatelyDominatedBlock(i)))
                return false;
        }
        MInstructionIterator i = block->begin();
        while (*i != block->lastIns()) {
            MInstruction *ins = *i;
            MDefinition *dom = findDominatingDef(defs, ins, index);

            if (!dom)
                return false; 

            if (dom == ins) {
                i++;
                continue;
            }

            IonSpew(IonSpew_GVN, "instruction %d is dominated by instruction %d (from block %d)",
                    ins->id(), dom->id(), dom->block()->id());

            ins->replaceAllUsesWith(dom);

            JS_ASSERT(ins->useCount() == 0);
            JS_ASSERT(ins->block() == block);
            i = ins->block()->removeAt(i);
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
