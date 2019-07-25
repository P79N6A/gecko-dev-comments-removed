








































#include "Ion.h"
#include "ValueNumbering.h"
#include "IonSpewer.h"

using namespace js;
using namespace js::ion;

ValueNumberer::ValueNumberer(MIRGraph &graph)
  : graph_(graph)
{ }


uint32
ValueNumberer::lookupValue(ValueMap &values, MInstruction *ins)
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

    bool changed = true;

    while (changed) {
        changed = false;
        for (size_t i = 0; i < graph_.numBlocks(); i++) {
            MBasicBlock *block = graph_.getBlock(i);

            MDefinitionIterator iter(block);

            while (iter.more()) {
                MInstruction *ins = *iter;

                uint32 value = lookupValue(values, ins);

                if (!value)
                    return false; 

                if (ins->valueNumber() != value) {
                    ins->setValueNumber(value);
                    changed = true;
                }

                iter.next();
            }
        }
        values.clear();
    }
    return true;
}

MInstruction *
ValueNumberer::findDominatingInstruction(InstructionMap &defs, MInstruction *ins, size_t index)
{
    InstructionMap::Ptr p = defs.lookup(ins->valueNumber());
    MInstruction *dom;
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
        while (i != block->lastIns()) {
            MInstruction *ins = *i;
            MInstruction *dom = findDominatingInstruction(defs, ins, index);

            if (!dom)
                return false; 

            if (dom == ins) {
                i++;
                continue;
            }

            IonSpew(IonSpew_GVN, "instruction %d is dominated by instruction %d (from block %d)",
                    ins->id(), dom->id(), dom->block()->id());

            MUseIterator uses(ins);
            while (uses.more())
                uses->ins()->replaceOperand(uses, dom);

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
